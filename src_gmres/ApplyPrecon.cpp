#include "common_functions.H"
#include "common_functions_F.H"
#include "common_namespace.H"

#include "gmres_functions.H"
#include "gmres_functions_F.H"
#include "gmres_namespace.H"

using namespace common;
using namespace gmres;

void ApplyPrecon(const std::array<MultiFab, AMREX_SPACEDIM> & b_u, const MultiFab & b_p,
                 std::array<MultiFab, AMREX_SPACEDIM> & x_u, MultiFab & x_p,
                 const std::array<MultiFab, AMREX_SPACEDIM> & alpha_fc,
                 const MultiFab & beta, const std::array<MultiFab, NUM_EDGE> & beta_ed,
                 const MultiFab & gamma,
                 const Real & theta_alpha,
                 const Geometry & geom)
{

    BL_PROFILE_VAR("ApplyPrecon()",ApplyPrecon);

    BoxArray ba = b_p.boxArray();
    DistributionMapping dmap = b_p.DistributionMap();

    Real         mean_val_pres;
    Vector<Real> mean_val_umac(AMREX_SPACEDIM);


    MultiFab phi     (ba,dmap,1,1);
    MultiFab mac_rhs (ba,dmap,1,0);
    MultiFab zero_fab(ba,dmap,1,0);
    MultiFab x_p_tmp (ba,dmap,1,1);

    // set zero_fab_fc to 0
    zero_fab.setVal(0.);

    // build alphainv_fc, one_fab_fc, zero_fab_fc, and b_u_tmp
    std::array< MultiFab, AMREX_SPACEDIM > alphainv_fc;
    std::array< MultiFab, AMREX_SPACEDIM > one_fab_fc;
    std::array< MultiFab, AMREX_SPACEDIM > zero_fab_fc;
    std::array< MultiFab, AMREX_SPACEDIM > b_u_tmp;

    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        alphainv_fc[d].define(convert(ba, nodal_flag_dir[d]), dmap, 1, 0);
        one_fab_fc[d].define(convert(ba, nodal_flag_dir[d]), dmap, 1, 0);
        zero_fab_fc[d].define(convert(ba, nodal_flag_dir[d]), dmap, 1, 0);
        b_u_tmp[d].define(convert(ba, nodal_flag_dir[d]), dmap, 1, 0);
    }

    // set alphainv_fc to 1/alpha_fc
    // set one_fab_fc to 1
    // set zero_fab_fc to 0
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        alphainv_fc[d].setVal(1.);
        alphainv_fc[d].divide(alpha_fc[d],0,1,0);
        one_fab_fc[d].setVal(1.);
        zero_fab_fc[d].setVal(0.);
    }

    // set the initial guess for Phi in the Poisson solve to 0
    // set x_u = 0 as initial guess
    phi.setVal(0.);
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        x_u[d].setVal(0.);
    }

    // 1 = projection preconditioner
    // 2 = lower triangular preconditioner
    // 3 = upper triangular preconditioner
    // 4 = block diagonal preconditioner
    // 5 = Uzawa-type approximation (see paper)
    // 6 = upper triangular + viscosity-based BFBt Schur complement (from Georg Stadler)

    // projection preconditioner
    if (abs(precon_type) == 1) {

        ////////////////////
        // STEP 1: Solve for an intermediate state, x_u^star, using an implicit viscous solve
        //         x_u^star = A^{-1} b_u
        ////////////////////

        // x_u^star = A^{-1} b_u
        StagMGSolver(alpha_fc,beta,beta_ed,gamma,x_u,b_u,theta_alpha,geom);

        ////////////////////
        // STEP 2: Construct RHS for pressure Poisson problem
        ////////////////////

        // set mac_rhs = D(x_u^star)
        ComputeDiv(mac_rhs,x_u,0,0,1,geom,0);

        // add b_p to mac_rhs
        MultiFab::Add(mac_rhs,b_p,0,0,1,0);

        ////////////////////
        // STEP 3: Compute x_u
        ////////////////////

        // use multigrid to solve for Phi
        // x_u^star is only passed in to get a norm for absolute residual criteria
        MacProj(alphainv_fc,mac_rhs,phi,geom);

        // x_u = x_u^star - (alpha I)^-1 grad Phi
        SubtractWeightedGradP(x_u,alphainv_fc,phi,geom);

        ////////////////////
        // STEP 4: Compute x_p by applying the Schur complement approximation
        ////////////////////

        if (visc_schur_approx == 0) {
            // if precon_type = +1, or theta_alpha=0 then x_p = theta_alpha*Phi - c*beta*(mac_rhs)
            // if precon_type = -1                   then x_p = theta_alpha*Phi - c*beta*L_alpha Phi

            if (precon_type == 1 || theta_alpha == 0) {
                // first set x_p = -mac_rhs
                MultiFab::Copy(x_p,mac_rhs,0,0,1,0);
                x_p.mult(-1.,0,1,0);
            }
            else {
                // first set x_p = -L_alpha Phi
                CCApplyOp(phi,x_p,zero_fab,alphainv_fc,geom);
            }

            if ( abs(visc_type) == 1 || abs(visc_type) == 2) {
                // multiply x_p by beta; x_p = -beta L_alpha Phi
                MultiFab::Multiply(x_p,beta,0,0,1,0);

                if (abs(visc_type) == 2) {
                    // multiply by c=2; x_p = -2*beta L_alpha Phi
                    x_p.mult(2.,0,1,0);
                }
            }
            else if (abs(visc_type) == 3) {

                // multiply x_p by gamma, use mac_rhs a temparary to save x_p
                MultiFab::Copy(mac_rhs,x_p,0,0,1,0);
                MultiFab::Multiply(mac_rhs,gamma,0,0,1,0);
                // multiply x_p by beta; x_p = -beta L_alpha Phi
                MultiFab::Multiply(x_p,beta,0,0,1,0);
                // multiply by c=4/3; x_p = -(4/3) beta L_alpha Phi
                x_p.mult(4./3.,0,1,0);
                // x_p = -(4/3) beta L_alpha Phi - gamma L_alpha Phi
                MultiFab::Add(x_p,mac_rhs,0,0,1,0);
            }

            // multiply Phi by theta_alpha
            phi.mult(theta_alpha,0,1,0);

            // add theta_alpha*Phi to x_p
            MultiFab::Add(x_p,phi,0,0,1,0);
        }
        else {
            Abort("StagApplyOp: visc_schur_approx != 0 not supported");
        }
    }
    else {
        Abort("StagApplyOp: unsupposed precon_type");
    }

    ////////////////////
    // STEP 5: Handle null-space issues in MG solvers
    ////////////////////

    // subtract off mean value: Single level only! No need for ghost cells
    SumStag(geom,x_u,0,mean_val_umac,true);
    SumCC(x_p,0,mean_val_pres,true);

    // The pressure Poisson problem is always singular:
    x_p.plus(-mean_val_pres,0,1,0);

    // The velocity problem is also singular under these cases
    if (theta_alpha == 0.) {

        bool no_wall_is_no_slip = true;

        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            if (bc_vel_lo[d] == 2 || bc_vel_hi[d] == 2) {
                no_wall_is_no_slip = false;
            }
        }


        if (no_wall_is_no_slip) {
            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                if (geom.isPeriodic(d)) {
                    x_u[d].plus(-mean_val_umac[d],0,1,0);
                }
            }
        }
    }
}

#include "electrostatic.H"
#include "common_namespace.H"
#include "common_functions.H"
#include <AMReX_MLMG.H>

using namespace amrex;
using namespace common;

void esSolve(MultiFab& potential, MultiFab& charge,
             std::array< MultiFab, AMREX_SPACEDIM >& efield,
             const std::array< MultiFab, AMREX_SPACEDIM >& external, const Geometry geom)
{
    AMREX_D_TERM(efield[0].setVal(0);,
                 efield[1].setVal(0);,
                 efield[2].setVal(0););

    if(es_tog==1 || es_tog==3)
    {

        LinOpBCType lo_linop_bc[3];
        LinOpBCType hi_linop_bc[3];

        for (int i=0; i<AMREX_SPACEDIM; ++i) {
            if (bc_es_lo[i] == -1 && bc_es_hi[i] == -1) {
                lo_linop_bc[i] = LinOpBCType::Periodic;
                hi_linop_bc[i] = LinOpBCType::Periodic;
            }
            if(bc_es_lo[i] == 2)
            {
                lo_linop_bc[i] = LinOpBCType::Neumann;
            }
            if(bc_es_hi[i] == 2)
            {
                hi_linop_bc[i] = LinOpBCType::Neumann;
            }
            if(bc_es_lo[i] == 1)
            {                 
                lo_linop_bc[i] = LinOpBCType::Dirichlet;
            }
            if(bc_es_hi[i] == 1)
            {
                hi_linop_bc[i] = LinOpBCType::Dirichlet;
            }
        }

          //MOVED TO OCCUR BEFOR SUMBOUNDARY!
//        MultiFABPhysBCCharge(charge, geom); //Adjust spread charge distribtion near boundaries from 

        const BoxArray& ba = charge.boxArray();
        const DistributionMapping& dmap = charge.DistributionMap();

        //create solver opject
        MLPoisson linop({geom}, {ba}, {dmap});
 
        //set BCs
        linop.setDomainBC({AMREX_D_DECL(lo_linop_bc[0],
                                        lo_linop_bc[1],
                                        lo_linop_bc[2])},
                          {AMREX_D_DECL(hi_linop_bc[0],
                                        hi_linop_bc[1],
                                        hi_linop_bc[2])});

        linop.setLevelBC(0, nullptr);

        //Multi Level Multi Grid
        MLMG mlmg(linop);

        //Solver parameters
        mlmg.setMaxIter(poisson_max_iter);
        mlmg.setVerbose(poisson_verbose);
        mlmg.setBottomVerbose(poisson_bottom_verbose);

        //Do solve
        mlmg.solve({&potential}, {&charge}, poisson_rel_tol, 0.0);

            
        potential.FillBoundary(geom.periodicity());
        MultiFABPotentialBC(potential, geom); //set ghost cell values so electric field is calculated properly

        //Find e field, gradient from cell centers to faces
        ComputeCentredGrad(potential, efield, geom);
    }

    //Add external field on top, then fill boundaries, then setup BCs for peskin interpolation
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        MultiFab::Add(efield[d], external[d], 0, 0, 1, efield[d].nGrow());
        efield[d].FillBoundary(geom.periodicity());
        //MultiFABElectricBC(efield[d], d, geom);
    }

}



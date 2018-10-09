
#include "hydro_functions.H"
#include "hydro_functions_F.H"

#include "common_functions.H"
#include "common_functions_F.H"

#include "common_namespace.H"

#include "gmres_functions.H"
#include "gmres_functions_F.H"

#include "gmres_namespace.H"

#include <AMReX_ParallelDescriptor.H>
#include <AMReX_MultiFabUtil.H>

using namespace amrex;
using namespace common;
using namespace gmres;

// argv contains the name of the inputs file entered at the command line
void advance(  std::array< MultiFab, AMREX_SPACEDIM >& umac, 
	       std::array< MultiFab, AMREX_SPACEDIM >& umacNew, 
	       MultiFab& pres, MultiFab& tracer, 
	       const std::array< MultiFab, AMREX_SPACEDIM >& mfluxdiv_predict,
	       const std::array< MultiFab, AMREX_SPACEDIM >& mfluxdiv_correct,
	       const std::array< MultiFab, AMREX_SPACEDIM >& alpha_fc,
	       const MultiFab& beta, const MultiFab& gamma,
	       const std::array< MultiFab, NUM_EDGE >& beta_ed, 
	       const Geometry geom, const Real& dt)
{
  
  BL_PROFILE_VAR("advance()",advance);

  const Real* dx = geom.CellSize();
  const Real dtinv = 1.0/dt;
  Real theta_alpha = 1.;
  Real norm_pre_rhs;

  const BoxArray& ba = beta.boxArray();
  const DistributionMapping& dmap = beta.DistributionMap();

   // rhs_p GMRES solve
   MultiFab gmres_rhs_p(ba, dmap, 1, 0);
   gmres_rhs_p.setVal(0.);

  // rhs_u GMRES solve
  std::array< MultiFab, AMREX_SPACEDIM > gmres_rhs_u;
  AMREX_D_TERM(gmres_rhs_u[0].define(convert(ba,nodal_flag_x), dmap, 1, 1);,
	       gmres_rhs_u[1].define(convert(ba,nodal_flag_y), dmap, 1, 1);,
	       gmres_rhs_u[2].define(convert(ba,nodal_flag_z), dmap, 1, 1););

  std::array< MultiFab, AMREX_SPACEDIM > Lumac;
  AMREX_D_TERM(Lumac[0].define(convert(ba,nodal_flag_x), dmap, 1, 1);,
	       Lumac[1].define(convert(ba,nodal_flag_y), dmap, 1, 1);,
	       Lumac[2].define(convert(ba,nodal_flag_z), dmap, 1, 1););

  // advective terms
  std::array< MultiFab, AMREX_SPACEDIM > advFluxdiv;
  AMREX_D_TERM(advFluxdiv[0].define(convert(ba,nodal_flag_x), dmap, 1, 1);,
	       advFluxdiv[1].define(convert(ba,nodal_flag_y), dmap, 1, 1);,
	       advFluxdiv[2].define(convert(ba,nodal_flag_z), dmap, 1, 1););

  std::array< MultiFab, AMREX_SPACEDIM > advFluxdivPred;
  AMREX_D_TERM(advFluxdivPred[0].define(convert(ba,nodal_flag_x), dmap, 1, 1);,
	       advFluxdivPred[1].define(convert(ba,nodal_flag_y), dmap, 1, 1);,
	       advFluxdivPred[2].define(convert(ba,nodal_flag_z), dmap, 1, 1););

  // staggered momentum
  std::array< MultiFab, AMREX_SPACEDIM > uMom;
  AMREX_D_TERM(uMom[0].define(convert(ba,nodal_flag_x), dmap, 1, 1);,
	       uMom[1].define(convert(ba,nodal_flag_y), dmap, 1, 1);,
	       uMom[2].define(convert(ba,nodal_flag_z), dmap, 1, 1););

  MultiFab tracerPred(ba,dmap,1,1);
  MultiFab advFluxdivS(ba,dmap,1,1);

  ///////////////////////////////////////////
  // Scaled alpha, beta, gamma:
  ///////////////////////////////////////////

  // alpha_fc_0 arrays
  std::array< MultiFab, AMREX_SPACEDIM > alpha_fc_0;
  AMREX_D_TERM(alpha_fc_0[0].define(convert(ba,nodal_flag_x), dmap, 1, 1);,
	       alpha_fc_0[1].define(convert(ba,nodal_flag_y), dmap, 1, 1);,
	       alpha_fc_0[2].define(convert(ba,nodal_flag_z), dmap, 1, 1););
  AMREX_D_TERM(alpha_fc_0[0].setVal(0.);,
	       alpha_fc_0[1].setVal(0.);,
	       alpha_fc_0[2].setVal(0.););

  // Scaled by 1/2:
  // beta_hlf cell centered
  MultiFab beta_hlf(ba, dmap, 1, 1);
  MultiFab::Copy(beta_hlf, beta, 0, 0, 1, 1);
  beta_hlf.mult(0.5, 1);

  // beta_hlf on nodes in 2d
  // beta_hlf on edges in 3d
  std::array< MultiFab, NUM_EDGE > beta_ed_hlf;
#if (AMREX_SPACEDIM == 2)
  beta_ed_hlf[0].define(convert(ba,nodal_flag), dmap, 1, 1);
  MultiFab::Copy(beta_ed_hlf[0], beta_ed[0], 0, 0, 1, 1);
  beta_ed_hlf[0].mult(0.5, 1);
#elif (AMREX_SPACEDIM == 3)
  beta_ed_hlf[0].define(convert(ba,nodal_flag_xy), dmap, 1, 1);
  beta_ed_hlf[1].define(convert(ba,nodal_flag_xz), dmap, 1, 1);
  beta_ed_hlf[2].define(convert(ba,nodal_flag_yz), dmap, 1, 1);
  for(int d=0; d<AMREX_SPACEDIM; d++) {
    MultiFab::Copy(beta_ed_hlf[d], beta_ed[d], 0, 0, 1, 1);
    beta_ed_hlf[d].mult(0.5, 1);
  }
#endif

  // cell-centered gamma_hlf
  MultiFab gamma_hlf(ba, dmap, 1, 1);
  MultiFab::Copy(gamma_hlf, gamma, 0, 0, 1, 1);
  gamma_hlf.mult(-0.5, 1);

  // Scaled by -1/2:
  // beta_neghlf cell centered
  MultiFab beta_neghlf(ba, dmap, 1, 1);
  MultiFab::Copy(beta_neghlf, beta, 0, 0, 1, 1);
  beta_neghlf.mult(-0.5, 1);

  // beta_neghlf on nodes in 2d
  // beta_neghlf on edges in 3d
  std::array< MultiFab, NUM_EDGE > beta_ed_neghlf;
#if (AMREX_SPACEDIM == 2)
  beta_ed_neghlf[0].define(convert(ba,nodal_flag), dmap, 1, 1);
  MultiFab::Copy(beta_ed_neghlf[0], beta_ed[0], 0, 0, 1, 1);
  beta_ed_neghlf[0].mult(-0.5, 1);
#elif (AMREX_SPACEDIM == 3)
  beta_ed_neghlf[0].define(convert(ba,nodal_flag_xy), dmap, 1, 1);
  beta_ed_neghlf[1].define(convert(ba,nodal_flag_xz), dmap, 1, 1);
  beta_ed_neghlf[2].define(convert(ba,nodal_flag_yz), dmap, 1, 1);
  for(int d=0; d<AMREX_SPACEDIM; d++) {
    MultiFab::Copy(beta_ed_neghlf[d], beta_ed[d], 0, 0, 1, 1);
    beta_ed_neghlf[d].mult(-0.5, 1);
  }
#endif

  // cell-centered gamma
  MultiFab gamma_neghlf(ba, dmap, 1, 1);
  MultiFab::Copy(gamma_neghlf, gamma, 0, 0, 1, 1);
  gamma_neghlf.mult(-0.5, 1);
  ///////////////////////////////////////////

  AMREX_D_TERM(umac[0].FillBoundary(geom.periodicity());,
	       umac[1].FillBoundary(geom.periodicity());,
	       umac[2].FillBoundary(geom.periodicity()););
	
  //////////////////////////
  // Advance tracer
  //////////////////////////

  // Compute tracer:
  tracer.FillBoundary(geom.periodicity());
  MkAdvSFluxdiv(umac,tracer,advFluxdivS,dx,geom,0);
  advFluxdivS.mult(dt, 1);

  // compute predictor
  MultiFab::Copy(tracerPred, tracer, 0, 0, 1, 0);
  MultiFab::Add(tracerPred, advFluxdivS, 0, 0, 1, 0);
  tracerPred.FillBoundary(geom.periodicity());
  MkAdvSFluxdiv(umac,tracerPred,advFluxdivS,dx,geom,0);
  advFluxdivS.mult(dt, 1);

  // advance in time
  MultiFab::Add(tracer, tracerPred, 0, 0, 1, 0);
  MultiFab::Add(tracer, advFluxdivS, 0, 0, 1, 0);
  tracer.mult(0.5, 1);

  // amrex::Print() << "tracer L0 norm = " << tracer.norm0() << "\n";
  //////////////////////////

  //////////////////////////////////////////////////
  // ADVANCE velocity field
  //////////////////////////////////////////////////

  // PREDICTOR STEP (heun's method: part 1)
  // compute advective term
  AMREX_D_TERM(MultiFab::Copy(uMom[0], umac[0], 0, 0, 1, 0);,
	       MultiFab::Copy(uMom[1], umac[1], 0, 0, 1, 0);,
	       MultiFab::Copy(uMom[2], umac[2], 0, 0, 1, 0););

  // let rho = 1
  for (int d=0; d<AMREX_SPACEDIM; d++) {
    uMom[d].mult(1.0, 1);
  }

  AMREX_D_TERM(uMom[0].FillBoundary(geom.periodicity());,
	       uMom[1].FillBoundary(geom.periodicity());,
	       uMom[2].FillBoundary(geom.periodicity()););

  MkAdvMFluxdiv(umac,uMom,advFluxdiv,dx,0);

  // crank-nicolson terms
  StagApplyOp(beta_neghlf,gamma_neghlf,beta_ed_neghlf,umac,Lumac,alpha_fc_0,dx,theta_alpha);

  AMREX_D_TERM(MultiFab::Copy(gmres_rhs_u[0], umac[0], 0, 0, 1, 0);,
	       MultiFab::Copy(gmres_rhs_u[1], umac[1], 0, 0, 1, 0);,
	       MultiFab::Copy(gmres_rhs_u[2], umac[2], 0, 0, 1, 0););
  for (int d=0; d<AMREX_SPACEDIM; d++) {
    gmres_rhs_u[d].mult(dtinv, 1);
  }
  AMREX_D_TERM(MultiFab::Add(gmres_rhs_u[0], mfluxdiv_predict[0], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[1], mfluxdiv_predict[1], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[2], mfluxdiv_predict[2], 0, 0, 1, 0););
  AMREX_D_TERM(MultiFab::Add(gmres_rhs_u[0], Lumac[0], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[1], Lumac[1], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[2], Lumac[2], 0, 0, 1, 0););
  AMREX_D_TERM(MultiFab::Add(gmres_rhs_u[0], advFluxdiv[0], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[1], advFluxdiv[1], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[2], advFluxdiv[2], 0, 0, 1, 0););

  AMREX_D_TERM(gmres_rhs_u[0].FillBoundary(geom.periodicity());,
	       gmres_rhs_u[1].FillBoundary(geom.periodicity());,
	       gmres_rhs_u[2].FillBoundary(geom.periodicity()););

  // initial guess for new solution
  AMREX_D_TERM(MultiFab::Copy(umacNew[0], umac[0], 0, 0, 1, 0);,
	       MultiFab::Copy(umacNew[1], umac[1], 0, 0, 1, 0);,
	       MultiFab::Copy(umacNew[2], umac[2], 0, 0, 1, 0););
  pres.setVal(0.);  // initial guess

  // call GMRES to compute predictor
  GMRES(gmres_rhs_u,gmres_rhs_p,umacNew,pres,alpha_fc,beta_hlf,beta_ed_hlf,gamma_hlf,theta_alpha,geom,norm_pre_rhs);

  // Compute predictor advective term
  AMREX_D_TERM(umacNew[0].FillBoundary(geom.periodicity());,
	       umacNew[1].FillBoundary(geom.periodicity());,
	       umacNew[2].FillBoundary(geom.periodicity()););

  AMREX_D_TERM(MultiFab::Copy(uMom[0], umacNew[0], 0, 0, 1, 0);,
	       MultiFab::Copy(uMom[1], umacNew[1], 0, 0, 1, 0);,
	       MultiFab::Copy(uMom[2], umacNew[2], 0, 0, 1, 0););

  // let rho = 1
  for (int d=0; d<AMREX_SPACEDIM; d++) {
    uMom[d].mult(1.0, 1);
  }

  AMREX_D_TERM(uMom[0].FillBoundary(geom.periodicity());,
	       uMom[1].FillBoundary(geom.periodicity());,
	       uMom[2].FillBoundary(geom.periodicity()););

  MkAdvMFluxdiv(umacNew,uMom,advFluxdivPred,dx,0);

  // ADVANCE STEP (crank-nicolson + heun's method)

  // Compute gmres_rhs

  // trapezoidal advective terms
  for (int d=0; d<AMREX_SPACEDIM; d++) {
    advFluxdiv[d].mult(0.5, 1);
    advFluxdivPred[d].mult(0.5, 1);
  }

  // crank-nicolson terms
  StagApplyOp(beta_neghlf,gamma_neghlf,beta_ed_neghlf,umac,Lumac,alpha_fc_0,dx,theta_alpha);

  AMREX_D_TERM(MultiFab::Copy(gmres_rhs_u[0], umac[0], 0, 0, 1, 0);,
	       MultiFab::Copy(gmres_rhs_u[1], umac[1], 0, 0, 1, 0);,
	       MultiFab::Copy(gmres_rhs_u[2], umac[2], 0, 0, 1, 0););
  for (int d=0; d<AMREX_SPACEDIM; d++) {
    gmres_rhs_u[d].mult(dtinv, 1);
  }
  AMREX_D_TERM(MultiFab::Add(gmres_rhs_u[0], mfluxdiv_correct[0], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[1], mfluxdiv_correct[1], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[2], mfluxdiv_correct[2], 0, 0, 1, 0););
  AMREX_D_TERM(MultiFab::Add(gmres_rhs_u[0], Lumac[0], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[1], Lumac[1], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[2], Lumac[2], 0, 0, 1, 0););
  AMREX_D_TERM(MultiFab::Add(gmres_rhs_u[0], advFluxdiv[0], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[1], advFluxdiv[1], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[2], advFluxdiv[2], 0, 0, 1, 0););
  AMREX_D_TERM(MultiFab::Add(gmres_rhs_u[0], advFluxdivPred[0], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[1], advFluxdivPred[1], 0, 0, 1, 0);,
	       MultiFab::Add(gmres_rhs_u[2], advFluxdivPred[2], 0, 0, 1, 0););

  AMREX_D_TERM(gmres_rhs_u[0].FillBoundary(geom.periodicity());,
	       gmres_rhs_u[1].FillBoundary(geom.periodicity());,
	       gmres_rhs_u[2].FillBoundary(geom.periodicity()););

  // initial guess for new solution
  AMREX_D_TERM(MultiFab::Copy(umacNew[0], umac[0], 0, 0, 1, 0);,
	       MultiFab::Copy(umacNew[1], umac[1], 0, 0, 1, 0);,
	       MultiFab::Copy(umacNew[2], umac[2], 0, 0, 1, 0););
  pres.setVal(0.);  // initial guess

  // call GMRES here
  GMRES(gmres_rhs_u,gmres_rhs_p,umacNew,pres,alpha_fc,beta_hlf,beta_ed_hlf,gamma_hlf,theta_alpha,geom,norm_pre_rhs);

  AMREX_D_TERM(MultiFab::Copy(umac[0], umacNew[0], 0, 0, 1, 0);,
	       MultiFab::Copy(umac[1], umacNew[1], 0, 0, 1, 0);,
	       MultiFab::Copy(umac[2], umacNew[2], 0, 0, 1, 0););
  //////////////////////////////////////////////////

}

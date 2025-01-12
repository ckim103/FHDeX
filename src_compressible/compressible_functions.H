#ifndef _compressible_functions_H_
#define _compressible_functions_H_

#define MAX_SPECIES 10
#define LOHI 2

#include <AMReX.H>
#include <AMReX_MultiFab.H>
#include <AMReX_ArrayLim.H>

using namespace amrex;

// copy contents of compressible_params_module to C++ compressible namespace
void InitializeCompressibleNamespace();

void calculateFlux(const MultiFab& cons, const MultiFab& prim,
                   const MultiFab& eta, const MultiFab& zeta, const MultiFab& kappa,
		   const MultiFab& chi, const MultiFab& D,
                   std::array<MultiFab, AMREX_SPACEDIM>& flux,
                   std::array<MultiFab, AMREX_SPACEDIM>& stochFlux,
                   std::array<MultiFab, AMREX_SPACEDIM>& cornx,
                   std::array<MultiFab, AMREX_SPACEDIM>& corny,
                   std::array<MultiFab, AMREX_SPACEDIM>& cornz,
                   MultiFab& visccorn,
                   MultiFab& rancorn,
                   const amrex::Geometry geom,
		   const amrex::Vector< amrex::Real >& stoch_weights,
	           const amrex::Real* dx, const amrex::Real dt);

void calculateTransportCoeffs(const MultiFab& prim, 
			      MultiFab& eta, MultiFab& zeta, MultiFab& kappa,
			      MultiFab& chi, MultiFab& D);

void RK3step(MultiFab& cu, MultiFab& cup, MultiFab& cup2, MultiFab& cup3,
             MultiFab& prim, MultiFab& source,
             MultiFab& eta, MultiFab& zeta, MultiFab& kappa,
             MultiFab& chi, MultiFab& D,
             std::array<MultiFab, AMREX_SPACEDIM>& flux,
             std::array<MultiFab, AMREX_SPACEDIM>& stochFlux, 
             std::array<MultiFab, AMREX_SPACEDIM>& cornx,
             std::array<MultiFab, AMREX_SPACEDIM>& corny,
             std::array<MultiFab, AMREX_SPACEDIM>& cornz,
             MultiFab& visccorn, MultiFab& rancorn,
             const amrex::Geometry geom, const amrex::Real* dx, const amrex::Real dt);

void conservedToPrimitive(MultiFab& prim, const MultiFab& cons);

void primitiveToConserved(const MultiFab& prim, MultiFab& cons);

void setBC(MultiFab& prim, MultiFab& cons);

void evaluateStats(const MultiFab& cons, MultiFab& consMean, MultiFab& consVar,
                   const MultiFab& prim, MultiFab& primMean, MultiFab& primVar,
                   const int steps, const amrex::Real* dx);

void doMembrane(MultiFab& cons, MultiFab& prim, std::array<MultiFab, AMREX_SPACEDIM>& flux,
                const amrex::Geometry geom, const amrex::Real* dx, const amrex::Real dt);

void WritePlotFile(int step,
                   const amrex::Real time,
                   const amrex::Geometry geom,
	           const amrex::MultiFab& cu,
	           const amrex::MultiFab& cuMeans,
	           const amrex::MultiFab& cuVars,
	           const amrex::MultiFab& prim,
	           const amrex::MultiFab& primMeans,
	           const amrex::MultiFab& primVars,
                   const amrex::MultiFab& eta, 
		   const amrex::MultiFab& kappa);

#endif

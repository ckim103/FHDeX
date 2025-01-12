#ifndef _hydro_test_functions_H_
#define _hyrdo_test_functions_H_

#include "common_functions.H"
#include "gmres_functions.H"

///////////////////////////
// in advance.cpp
void advance(std::array< amrex::MultiFab, AMREX_SPACEDIM >& umac,
	     std::array< amrex::MultiFab, AMREX_SPACEDIM >& umacNew,
	     amrex::MultiFab& pres, amrex::MultiFab& tracer,
	     const std::array< amrex::MultiFab, AMREX_SPACEDIM >& mfluxdiv_stoch,
	     std::array< amrex::MultiFab, AMREX_SPACEDIM >& alpha_fc,
	     const amrex::MultiFab& beta, const amrex::MultiFab& gamma,
	     const std::array< amrex::MultiFab, NUM_EDGE >& beta_ed,
	     const amrex::Geometry geom, const amrex::Real& dt);

///////////////////////////

///////////////////////////
// in WritePlotFile.cpp

void WritePlotFile(const int step,
                   const amrex::Real time,
                   const amrex::Geometry geom,
                   std::array< MultiFab, AMREX_SPACEDIM >& umac,
		   const MultiFab& tracer,
                   const MultiFab& pres);

///////////////////////////

///////////////////////////
// in Checkpoint.cpp

void WriteCheckPoint(const int step,
                     const amrex::Real time,
                     std::array< MultiFab, AMREX_SPACEDIM >& umac,
                     const MultiFab& tracer);

void ReadCheckPoint(int& step,
                    amrex::Real& time,
                    std::array< MultiFab, AMREX_SPACEDIM >& umac,
                    MultiFab& tracer);

///////////////////////////

#endif

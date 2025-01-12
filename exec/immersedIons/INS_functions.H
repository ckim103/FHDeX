#ifndef _INS_functions_H_
#define _INS_functions_H_

#include "common_functions.H"
#include "gmres_functions.H"
#include "FhdParticleContainer.H"

///////////////////////////
// in WritePlotFile.cpp

void WritePlotFile(int step,
                   const amrex::Real time,
                   const amrex::Geometry geom,
                   const amrex::Geometry cgeom,
                   const amrex::Geometry egeom,
                   const MultiFab& particleInstant,
                   const MultiFab& particleMeans,
                   const MultiFab& particleVars,
                   FhdParticleContainer& particles,
                   const MultiFab& charge,
                   const MultiFab& potential,
                   const std::array< MultiFab, AMREX_SPACEDIM >& efield,
                   const MultiFab& mobility);

///////////////////////////

///////////////////////////
// in WritePlotFileHydro.cpp

void WritePlotFileHydro(int step,
                        const amrex::Real time,
                        const amrex::Geometry geom,
                        std::array< MultiFab, AMREX_SPACEDIM >& umac,
                        const MultiFab& pres,
                        std::array< MultiFab, AMREX_SPACEDIM >& umacM,
                        std::array< MultiFab, AMREX_SPACEDIM >& umacV);

///////////////////////////

///////////////////////////
// in Checkpoint.cpp

void WriteCheckPoint(int step,
                     amrex::Real time,
                     int statsCount,
                     const std::array< MultiFab, AMREX_SPACEDIM >& umac,
                     const std::array< MultiFab, AMREX_SPACEDIM >& umacM,
                     const std::array< MultiFab, AMREX_SPACEDIM >& umacV,
                     const MultiFab& pres,
                     const FhdParticleContainer& particles,
                     const MultiFab& particleMeans,
                     const MultiFab& particleVars,
                     const MultiFab& potential);

void ReadCheckPoint(int& step,
                    amrex::Real& time,
                    int& statsCount,
                    std::array< MultiFab, AMREX_SPACEDIM >& umac,
                    std::array< MultiFab, AMREX_SPACEDIM >& umacM,
                    std::array< MultiFab, AMREX_SPACEDIM >& umacV,
                    MultiFab& pres,
                    MultiFab& particleMeans,
                    MultiFab& particleVars,
                    MultiFab& potential);

void ReadCheckPointParticles(FhdParticleContainer& particles);

///////////////////////////

#endif

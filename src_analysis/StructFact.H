#ifndef _StructFact_H
#define _StructFact_H_

#include <AMReX.H>
#include <AMReX_MultiFab.H>
#include <AMReX_Vector.H>
#include <AMReX_VisMF.H>

// These are for SWFFT
#include <Distribution.H>
#include <AlignedAllocator.h>
#include <Dfft.H>

#include <string>

#include <common_namespace.H>

#define ALIGN 16

using namespace amrex;
using namespace common;

class StructFact {

    int NVAR = 1;        // Number of variable
    int NVARU = NVAR;    // Number of unique variables
    int NCOV = 1;        // Number of covariances (lower diagonal of matrix - by default)

    int verbosity = 0;

    // Total number of states to average over, updated by FortStructure()
    int nsamples = 0;

    // Vector containing running sums of real and imaginary components 
    // of inner products (covariances) of DFTs
    MultiFab cov_real;
    MultiFab cov_imag;

    // Vector of MultiFabs containing final magnitude of covariances
    MultiFab cov_mag;

    // Vector containing covariance scaling
    Vector< Real > scaling;

    // Vector containing names of covariances
    amrex::Vector< std::string > cov_names;

    // 2 vectors containing structure factor pairs
    amrex::Vector< int > s_pairA;
    amrex::Vector< int > s_pairB;

    // Define vector of unique selected variables
    amrex::Vector< int > var_u;

public:

    StructFact();
    
    StructFact(const amrex::BoxArray, const amrex::DistributionMapping, 
               const amrex::Vector< std::string >&,
               const amrex::Vector< amrex::Real >&,
               const amrex::Vector< int >&, const amrex::Vector< int >&, 
               const int verbosity=0);

    StructFact(const amrex::BoxArray, const amrex::DistributionMapping, 
               const amrex::Vector< std::string >&,
               const amrex::Vector< amrex::Real >&,
               const int verbosity=0);

    void FortStructure(const amrex::MultiFab&, const amrex::Geometry);
    
    void Finalize(amrex::MultiFab&, amrex::MultiFab&, 
                  const int zero_avg=1);
    
    void WritePlotFile(const int, const amrex::Real, const amrex::Geometry, 
                       std::string, const int zero_avg=1);
    
    void StructOut(amrex::MultiFab&);
    
    void ComputeFFT(const amrex::MultiFab&, amrex::MultiFab&,
                    amrex::MultiFab&, const amrex::Geometry);
    
    void ShiftFFT(amrex::MultiFab&, const int zero_avg=1);
};

#endif


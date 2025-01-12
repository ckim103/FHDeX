#ifndef _FhdParticleContainer_H_
#define _FhdParticleContainer_H_

#include <AMReX.H>
#include <AMReX_Vector.H>
#include <AMReX_RealVect.H>
#include <AMReX_AmrParGDB.H>
#include <AMReX_Particles.H>
#include <AMReX_Periodicity.H>
#include <AMReX_NeighborParticles.H>
#include <IBParticleInfo.H>
#include <common_namespace.H>
#include <IBMarkerContainerBase.H>
#include <species.H>
#include "rng_functions_F.H"
#include "surfaces.H"
#include "particle_functions_F.H"

using namespace amrex;
using namespace std;

// IBM => Immmersed Boundary Marker
struct FHD_realData {
    //Analogous to particle realData (p.m_data)
    enum {
        radius = 0,
        velx,
        vely,
        velz,
        forcex,
        forcey,
        forcez,
        pred_posx,
        pred_posy,
        pred_posz,
        pred_velx,
        pred_vely,
        pred_velz,
        pred_forcex,
        pred_forcey,
        pred_forcez,
        vx,
        vy,
        vz,
        fx,
        fy,
        fz,
        ux,
        uy,
        uz,
        mass,
        R,
        q,
        accelFactor,
        dragFactor,
        ox,
        oy,
        oz,
        ax,
        ay,
        az,
        travelTime,
        diffAv,
        stepCount,
        multi,
        dryDiff,
        wetDiff,
        totalDiff,
        sigma,
        eepsilon,
        potential,
	p3m_radius,
        count    // Awesome little trick! (only works if first field is 0)
    };

    static Vector<std::string> names() {
        return Vector<std::string> {
            "radius",
            "velx",
            "vely",
            "velz",
            "forcex",
            "forcey",
            "forcez",
            "pred_posx",
            "pred_posy",
            "pred_posz",
            "pred_velx",
            "pred_vely",
            "pred_velz",
            "pred_forcex",
            "pred_forcey",
            "pred_forcez"
        };
    };
};



struct FHD_intData {
    //Analogous to particle intData (p.m_data)
    enum {
        sorted,
        i,
        j,
        k,
        species,
        count
    };

    static Vector<std::string> names() {
        return Vector<std::string> {
            "id_0",
            "cpu_0",
            "id_1",
            "cpu_1",
        };
    };
};

struct FHD_realDataLW {
    //Analogous to particle realData (p.m_data)
    enum {
        velx,
        vely,
        velz,
        forcex,
        forcey,
        forcez,
        posx,
        posy,
        posz,
        count    // Awesome little trick! (only works if first field is 0)
    };
};


class FhdParIter
    : public IBMarIterBase<FHD_realData::count, FHD_intData::count>
{

public:
    using IBMarIterBase<FHD_realData::count, FHD_intData::count>::IBMarIterBase;

};



class FhdParticleContainer
    : public IBMarkerContainerBase<FHD_realData, FHD_intData>
{

public:

    using IBMarkerContainerBase<FHD_realData, FHD_intData>
        ::IBMarkerContainerBase;

    using MyConstIBMarIter = IBMarkerContainerBase<FHD_realData, FHD_intData>
        ::MyConstIBMarIter;

    FhdParticleContainer(AmrCore * amr_core, int n_nbhd);
    FhdParticleContainer(const Geometry & geom, const DistributionMapping & dmap,
                      const BoxArray & ba, int n_nbhd);

    virtual ~FhdParticleContainer() {};

    // (ID, initial CPU) tuple: unique to each particle
    using MarkerIndex = typename IBMarkerContainerBase<FHD_realData, FHD_intData>
        ::PairIndex;

    void InitParticles(species* particleInfo, const Real* dxp);

    void PostRestart();

    void WriteParticlesAscii(string asciiName);

    void DoRFD(const Real dt, const Real* dxFluid, const Real* dxE, const Geometry geomF,
               const std::array<MultiFab, AMREX_SPACEDIM>& umac, const std::array<MultiFab, AMREX_SPACEDIM>& efield,
               const std::array<MultiFab, AMREX_SPACEDIM>& RealFaceCoords,
               const MultiFab& cellCenters,
               std::array<MultiFab, AMREX_SPACEDIM>& source,
               std::array<MultiFab, AMREX_SPACEDIM>& sourceTemp,
               const surface* surfaceList, const int surfaceCount, int sw);
    
    void PrintParticles();
    void SetPosition(int rank, int id, Real x, Real y, Real z);
    void SetVel(int rank, int id, Real x, Real y, Real z);

    void MeanSqrCalc(int lev, int reset);

    void BuildCorrectionTable(const Real* dx, int setMeasureFinal);

    void DoRFDbase(const Real dt, const Real* dxFluid, const Real* dxE, const Geometry geomF,
                   const std::array<MultiFab, AMREX_SPACEDIM>& umac, const std::array<MultiFab, AMREX_SPACEDIM>& efield,
                   const std::array<MultiFab, AMREX_SPACEDIM>& RealFaceCoords,
                   const MultiFab& cellCenters,
                   std::array<MultiFab, AMREX_SPACEDIM>& source,
                   std::array<MultiFab, AMREX_SPACEDIM>& sourceTemp,
                   const surface* surfaceList, const int surfaceCount, int sw);

    void computeForcesNL(const MultiFab& charge, const MultiFab& coords, const Real* dx);

    void MoveParticlesDSMC(const Real dt, const surface* surfaceList, const int surfaceCount,Real time, int* flux);

    void MoveIons(const Real dt, const Real* dxFluid, const Real* dxE, const Geometry geomF,
                  const std::array<MultiFab, AMREX_SPACEDIM>& umac, const std::array<MultiFab, AMREX_SPACEDIM>& efield,
                  const std::array<MultiFab, AMREX_SPACEDIM>& RealFaceCoords,
                  std::array<MultiFab, AMREX_SPACEDIM>& source,
                  std::array<MultiFab, AMREX_SPACEDIM>& sourceTemp,
                  const MultiFab& mobility,
                  const surface* surfaceList, const int surfaceCount, int sw);


    void SpreadIons(const Real dt, const Real* dxFluid, const Real* dxE, const Geometry geomF,
                    const std::array<MultiFab, AMREX_SPACEDIM>& umac, const std::array<MultiFab, AMREX_SPACEDIM>& efield,
                    const MultiFab& charge,
                    const std::array<MultiFab, AMREX_SPACEDIM>& RealFaceCoords,
                    const MultiFab& cellCenters,
                    std::array<MultiFab, AMREX_SPACEDIM>& source,
                    std::array<MultiFab, AMREX_SPACEDIM>& sourceTemp,
                    const surface* surfaceList, const int surfaceCount, int sw);
    
    void SyncMembrane(double* spec3xPos, double* spec3yPos, double* spec3zPos, double* spec3xForce, double* spec3yForce, double* spec3zForce, int length, int step, const species* particleInfo);

    void RadialDistribution(long totalParticles, const int step, const species* particleInfo);
    void CartesianDistribution(long totalParticles, const int step, const species* particleInfo);

    void collectFields(const Real dt, const Real* dxPotential, const MultiFab& RealCenterCoords, const Geometry geomF, MultiFab& charge, MultiFab& chargeTemp, MultiFab& mass, MultiFab& massTemp);

    void InitCollisionCells(MultiFab& collisionPairs,
                            MultiFab& collisionFactor, 
                            MultiFab& cellVols, const species particleInfo, const Real delt);

    void CollideParticles(MultiFab& collisionPairs,
                          MultiFab& collisionFactor, 
                          MultiFab& cellVols, const species particleInfo, const Real delt);


    void InitializeFields(MultiFab& particleInstant,
                          MultiFab& cellVols, const species particleInfo);

    void EvaluateStats(MultiFab& particleInstant,
                       MultiFab& particleMeans,
                       MultiFab& particleVars,
                       MultiFab& cellVols, species particleInfo, const Real delt, int steps);

    /****************************************************************************
     *                                                                          *
     * Access marker (amrex particle) data                                      *
     *                                                                          *
     ***************************************************************************/


    const IntVector & get_neighbor_list(int lev, const PairIndex & index) const {
        return neighbor_list[lev].at(index);
    };

    ParticleType * get_neighbors(int lev, const PairIndex & index) const {
        return (ParticleType *) neighbors[lev].at(index).dataPtr();
    };

    int numWrongCell();


    void ReBin();

    static bool use_neighbor_list;
    static bool sort_neighbor_list;

    int totalBins;
    Real binSize;

    Real *meanRadialDistribution   ;
    Real *meanRadialDistribution_pp;
    Real *meanRadialDistribution_pm;
    Real *meanRadialDistribution_mm;
    
    Real *binVolRadial;
    int radialStatsCount;

    Real *meanXDistribution   ;
    Real *meanXDistribution_pp;
    Real *meanXDistribution_pm;
    Real *meanXDistribution_mm;
    
    Real *meanYDistribution   ;
    Real *meanYDistribution_pp;
    Real *meanYDistribution_pm;
    Real *meanYDistribution_mm;
    
    Real *meanZDistribution   ;
    Real *meanZDistribution_pp;
    Real *meanZDistribution_pm;
    Real *meanZDistribution_mm;
    
    Real binVolCartesian;
    int cartesianStatsCount;   

    int  threepmBins = 50;
    int  threepmCurrentBin = 1;
    int  threepmCurrentSample = 1;
    int  threepmSamples = 40;
    Real threepmVals[50];
    Real threepmMax[50];
    Real threepmMin[50];
    Real threepmPoints[50]; 
    Real threepmRange = 5; 

protected:

    void UpdateFortranStructures();

    void UpdateCellVectors();

    // used to store vectors of particle indices on a cell-by-cell basis
    std::map<int, amrex::BaseFab<std::vector<int> > > m_cell_vectors;

    // primitive data for passing the above into Fortran
    std::map<int, amrex::BaseFab<int> > m_vector_size;
    std::map<int, amrex::BaseFab<int*> > m_vector_ptrs;

    // used to keep track of whether we need to rebuild the above or not
    bool m_vectors_initialized = false;
    amrex::BoxArray::RefID m_BARef;
    amrex::DistributionMapping::RefID m_DMRef;

private:

    void correctCellVectors(int old_index, int new_index, int grid, const ParticleType& p);

    int n_list;

    struct {
        bool operator()(const ParticleType & p1, const ParticleType & p2) const {
            return true;
        }
    } CheckPair;
};


#endif

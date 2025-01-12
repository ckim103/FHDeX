#ifndef _IBMarkerContainerBaseI_H_
#define _IBMarkerContainerBaseI_H_



#include <AMReX.H>
#include <AMReX_AmrParGDB.H>
#include <AMReX_ParmParse.H>
#include <AMReX_Particles.H>
#include <AMReX_NeighborParticles.H>

#include <AMReX_VisMF.H>  // amrex::VisMF::Write(MultiFab)

#include <common_functions.H>
#include <common_namespace.H>

#include <IBMarkerContainer.H>
#include <ib_functions_F.H>
#include "rng_functions_F.H"

#include <iostream>

using namespace common;
using namespace amrex;



// Needs to be specified once a concrete class is created
// bool IBMarkerContainer::use_neighbor_list  {true};
// bool IBMarkerContainer::sort_neighbor_list {false};



template <typename StructReal, typename StructInt>
IBMarkerContainerBase<StructReal, StructInt>::IBMarkerContainerBase(
            const Geometry & geom,
            const DistributionMapping & dmap,
            const BoxArray & ba,
            int n_nbhd
        ) : NeighborParticleContainer<StructReal::count, StructInt::count>(
            geom, dmap, ba, n_nbhd
        ),
    nghost(n_nbhd)
{
    InitInternals(n_nbhd);
}



template <typename StructReal, typename StructInt>
IBMarkerContainerBase<StructReal, StructInt>::IBMarkerContainerBase(
            AmrCore * amr_core, int n_nbhd
        ) : NeighborParticleContainer<StructReal::count, StructReal::count>(
            amr_core->GetParGDB(), n_nbhd
        ),
    m_amr_core(amr_core),
    nghost(n_nbhd)
{
    InitInternals(n_nbhd);
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::MoveMarkers(int lev, Real dt) {

    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());

        AoS & particles = this->GetParticles(lev).at(index).GetArrayOfStructs();
        long np = particles.size();

        for (int i = 0; i < np; ++ i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d)
                part.pos(d) += dt * part.rdata(StructReal::velx + d);
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::RFD(
            int lev, const Real * dx,
                  std::array<MultiFab, AMREX_SPACEDIM> & f_out,
            const std::array<MultiFab, AMREX_SPACEDIM> & coords
        ) {

    Real delta = 1e-6*dx[0];
    Real deltaover2 = delta/2.0;
    Real force = k_B*T_init[0]/delta;

    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());

        AoS & particles = this->GetParticles(lev).at(index).GetArrayOfStructs();
        long np = particles.size();

        for (int i=0; i<np; ++i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                // using pred_pos to temporarily store the random direction -
                // hopefully this doesn't cause problems elsewhere
                part.rdata(StructReal::pred_posx + d) =  get_particle_normal_func();

                // move particle to q^n + (dt/2) * Wtilde
                part.pos(d) += deltaover2 * part.rdata(StructReal::pred_posx + d);
                // scale by k_B*T/delta
                // NOTE: this assumed that the pred_pos* fields are
                // displacement vectors wrt pos(*)

                part.rdata(StructReal::forcex + d) =
                    force * part.rdata(StructReal::pred_posx + d);

            }

        }
    }

    // spread this term into the velocity forcing
    SpreadMarkers(lev, f_out, coords, dx, 1);

    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());

        AoS & particles = this->GetParticles(lev).at(index).GetArrayOfStructs();
        long np = particles.size();

        for (int i=0; i<np; ++i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                // move particle to q^n - (dt/2) * Wtilde (by subtracting dt *
                // Wtilde)
                part.pos(d) -= delta * part.rdata(StructReal::pred_posx + d);
                // scale by -k_B*T/delta
                // NOTE: this assumed that the pred_pos* fields are
                // displacement vectors wrt pos(*)

                part.rdata(StructReal::forcex + d) =
                    -force * part.rdata(StructReal::pred_posx + d);

            }
        }
    }

    // spread this term into the velocity forcing
    SpreadMarkers(lev, f_out, coords, dx, 1);

    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());

        AoS & particles = this->GetParticles(lev).at(index).GetArrayOfStructs();
        long np = particles.size();

        for (int i=0; i<np; ++i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                // move particle back to original position q^n (by adding dt/2
                // * Wtilde)
                part.pos(d) += deltaover2 * part.rdata(StructReal::pred_posx + d);
                // scale by 0
                part.rdata(StructReal::forcex + d) = 0;
            }
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::MovePredictor(int lev, Real dt) {

    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());

        AoS & particles = this->GetParticles(lev).at(index).GetArrayOfStructs();
        long np = particles.size();

        for (int i = 0; i < np; ++ i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d)
                part.rdata(StructReal::pred_posx + d) +=
                    dt * part.rdata(StructReal::pred_velx + d);
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::PullDown(
            int lev, Real * list, int element, int totalParticles
        ) {

    Real temp;

    for (int i = 0; i < totalParticles; ++i) {
        //std::cout << i << " of " << totalParticles << std::endl;
        list[i] = 0;
    }


    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());
        AoS & particles = this->GetParticles(lev).at(index).GetArrayOfStructs();
        long np = particles.size();

        int id;
        if (element >= 0) {

            for (int i = 0; i < np; ++i) {
                ParticleType & part = particles[i];
                id = part.id();
                //std::cout << id << " of " << totalParticles << std::endl;
                list[id-1] = particles[i].rdata(StructReal::radius + element);
            }

        } else {

            for (int i = 0; i < np; ++i) {
                ParticleType & part = particles[i];
                id = part.id();
                list[id-1] = part.pos((-element)-1);
            }
        }
    }

    //I'm sure there is an array version of this but this will do for now.
    // There is: void amrex::ParallelDescriptor::ReduceRealSum (Vector< std::reference_wrapper< Real > > &&)
    // the commented code below should do the trick (but it has not been tested)
    for (int i=0; i<totalParticles; ++i) {
        temp = list[i];
        ParallelDescriptor::ReduceRealSum(temp);
        list[i] = temp;
    }
    // Vectorized alternative (warning: untested!):
    // // Create a vector wrapping the `list` array:
    // Vector<std::reference_wrapper<Real>> tmp(totalParticles);
    // for (int i=0; i<totalParticles; ++i) tmp[i] = & list[i];

    // // Vectorized parallel reduce:
    // ParallelDescriptor::ReduceRealSum(std::move(tmp)); // Ask Weiqun why tmp needs to be &&?
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::PushUp(
            int lev, Real * list, int element, int totalParticles
        ) {


    Real temp;

    //I'm sure there is an array version of this but this will do for now.
    // There is: void amrex::ParallelDescriptor::ReduceRealSum (Vector< std::reference_wrapper< Real > > &&)
    // the commented code below should do the trick (but it has not been tested)
    for(int i=0; i<totalParticles; ++i) {
        temp = list[i];
        ParallelDescriptor::ReduceRealSum(temp);
        list[i] = temp;
    }
    // Vectorized alternative (warning: untested!):
    // // Create a vector wrapping the `list` array:
    // Vector<std::reference_wrapper<Real>> tmp(totalParticles);
    // for (int i=0; i<totalParticles; ++i) tmp[i] = & list[i];

    // // Vectorized parallel reduce:
    // ParallelDescriptor::ReduceRealSum(std::move(tmp)); // Ask Weiqun why tmp needs to be &&?


    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());
        AoS & particles = this->GetParticles(lev).at(index).GetArrayOfStructs();
        long np = particles.size();

        int id;
        if(element >= 0) {

            for (int i = 0; i < np; ++i) {
                ParticleType & part = particles[i];
                id = part.id();

                particles[i].rdata(StructReal::radius + element) = list[id-1];
            }

        } else {

            for (int i = 0; i < np; ++i) {
                ParticleType & part = particles[i];
                id = part.id();

                particles[i].pos((-element)-1) = list[id-1];
            }
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::ResetMarkers(int lev) {

    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());
        auto & particle_data = this->GetParticles(lev).at(index);
        long np = particle_data.size();

        AoS & particles = particle_data.GetArrayOfStructs();
        for (int i = 0; i < np; ++i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                part.rdata(StructReal::velx + d)   = 0.;
                part.rdata(StructReal::forcex + d) = 0.;
            }
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::ResetPredictor(int lev) {

    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        TileIndex index(pti.index(), pti.LocalTileIndex());
        auto & particle_data = this->GetParticles(lev).at(index);
        long np = particle_data.size();

        AoS & particles = particle_data.GetArrayOfStructs();
        for (int i = 0; i < np; ++i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                part.rdata(StructReal::pred_posx + d)   = 0.;
                part.rdata(StructReal::pred_velx + d)   = 0.;
                part.rdata(StructReal::pred_forcex + d) = 0.;
            }
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::SpreadMarkers(
            const Vector<RealVect> & f_in,
            const Vector<RealVect> & f_pos,
                  std::array<MultiFab, AMREX_SPACEDIM> & f_out,
                  std::array<MultiFab, AMREX_SPACEDIM> & f_weights,
            const std::array<MultiFab, AMREX_SPACEDIM> & coords,
            const Real * dx, const int ghost
        ) {

    for (MFIter mfi(f_out[0]); mfi.isValid(); ++mfi) {

        // Convert stagered boxes into cell centered
        Box tile_box = enclosedCells(mfi.tilebox());

        std::array<      FArrayBox *, AMREX_SPACEDIM> f_out_fab;
        std::array<      FArrayBox *, AMREX_SPACEDIM> f_weights_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> coords_fab;
        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            f_out_fab[d]     = & f_out[d][mfi];
            f_weights_fab[d] = & f_weights[d][mfi];
            coords_fab[d]    = & coords[d][mfi];
        }

        SpreadMarkers(f_in, f_pos, tile_box,
                      f_out_fab, f_weights_fab, coords_fab, dx, ghost);
    }
}

//template <typename StructReal, typename StructInt>
//void IBMarkerContainerBase<StructReal, StructInt>::SpreadMarkersCC(
//            const Vector<RealVect> & f_in,
//            const Vector<RealVect> & f_pos,
//                  std::array<MultiFab, AMREX_SPACEDIM> & f_out,
//                  std::array<MultiFab, AMREX_SPACEDIM> & f_weights,
//            const std::array<MultiFab, AMREX_SPACEDIM> & coords,
//            const Real * dx, const int ghost
//        ) {

//    for (MFIter mfi(f_out[0]); mfi.isValid(); ++mfi) {

//        // Convert stagered boxes into cell centered
//        Box tile_box = enclosedCells(mfi.tilebox());

//        std::array<      FArrayBox *, AMREX_SPACEDIM> f_out_fab;
//        std::array<      FArrayBox *, AMREX_SPACEDIM> f_weights_fab;
//        std::array<const FArrayBox *, AMREX_SPACEDIM> coords_fab;
//        for (int d=0; d<AMREX_SPACEDIM; ++d) {
//            f_out_fab[d]     = & f_out[d][mfi];
//            f_weights_fab[d] = & f_weights[d][mfi];
//            coords_fab[d]    = & coords[d][mfi];
//        }

//        SpreadMarkers(f_in, f_pos, tile_box,
//                      f_out_fab, f_weights_fab, coords_fab, dx, ghost);
//    }
//}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::SpreadMarkers(
            const Vector<RealVect> & f_in,
            const Vector<RealVect> & f_pos,
            const Box & tile_box,
                  std::array<      FArrayBox *, AMREX_SPACEDIM> & f_out,
                  std::array<      FArrayBox *, AMREX_SPACEDIM> & f_weights,
            const std::array<const FArrayBox *, AMREX_SPACEDIM> & coords,
            const Real* dx, int ghost
        ) {

    const int n_marker = f_pos.size();

    // Convert stagered boxes into cell centered
    Box bx = enclosedCells(f_out[0]->box());

    spread_markers(BL_TO_FORTRAN_BOX(bx),
                   BL_TO_FORTRAN_BOX(tile_box),
                   BL_TO_FORTRAN_ANYD(* f_out[0]),
#if (AMREX_SPACEDIM > 1)
                   BL_TO_FORTRAN_ANYD(* f_out[1]),
#endif
#if (AMREX_SPACEDIM > 2)
                   BL_TO_FORTRAN_ANYD(* f_out[2]),
#endif
                   BL_TO_FORTRAN_ANYD(* f_weights[0]),
#if (AMREX_SPACEDIM > 1)
                   BL_TO_FORTRAN_ANYD(* f_weights[1]),
#endif
#if (AMREX_SPACEDIM > 2)
                   BL_TO_FORTRAN_ANYD(* f_weights[2]),
#endif
                   BL_TO_FORTRAN_ANYD(* coords[0]),
#if (AMREX_SPACEDIM > 1)
                   BL_TO_FORTRAN_ANYD(* coords[1]),
#endif
#if (AMREX_SPACEDIM > 2)
                   BL_TO_FORTRAN_ANYD(* coords[2]),
#endif
                   f_pos.dataPtr(),
                   f_in.dataPtr(),
                   & n_marker,
                   dx, & ghost);
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::SpreadMarkers(
            int lev,
                  std::array<MultiFab, AMREX_SPACEDIM> & f_out,
            const std::array<MultiFab, AMREX_SPACEDIM> & coords,
            const Real * dx, int ghost
        ) const {


    //___________________________________________________________________________
    // We don't need these spreading weights => create a dummy MF
    std::array<MultiFab, AMREX_SPACEDIM> f_weights;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        f_weights[d].define(f_out[d].boxArray(), f_out[d].DistributionMap(),
                            1, f_out[d].nGrow());
        f_weights[d].setVal(0.);
    }


    // for (MFIter pti = MakeMFIter(lev, true); pti.isValid(); ++pti) {
    for (MyConstIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        // Marker (non-neighbor particle) data for current tile
        TileIndex index(pti.index(), pti.LocalTileIndex());
        const auto & particle_data = this->GetParticles(lev).at(index);


        //_______________________________________________________________________
        // Fill vector of marker positions and forces (for current level)
        long np = particle_data.size();

        Vector<RealVect> marker_positions(np), marker_forces(np);

        const AoS & particles = particle_data.GetArrayOfStructs();
        for (int i = 0; i < np; ++i) {
            const ParticleType & part = particles[i];

            RealVect ppos, pfor;
            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                ppos[d] = part.pos(d);
                pfor[d] = part.rdata(StructReal::forcex + d);
            }

            marker_positions[i] = ppos;
            marker_forces[i]    = pfor;
        }

        //_______________________________________________________________________
        // Spread the non-neighbor particles (markers)

        // Convert stagered boxes into cell centered
        Box tile_box = enclosedCells(f_out[0][pti].box());

        std::array<      FArrayBox *, AMREX_SPACEDIM> f_out_fab;
        std::array<      FArrayBox *, AMREX_SPACEDIM> f_weights_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> coords_fab;
        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            f_out_fab[d]     = & f_out[d][pti];
            f_weights_fab[d] = & f_weights[d][pti];
            coords_fab[d]    = & coords[d][pti];
        }

        SpreadMarkers(marker_forces, marker_positions, tile_box,
                      f_out_fab, f_weights_fab, coords_fab, dx, ghost);
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::SpreadMarkers(
            int lev,
            std::array<MultiFab, AMREX_SPACEDIM> & f_out
        ) const {

    //___________________________________________________________________________
    // Geometry data
    const Geometry & geom = this->Geom(0);
    const Real     *   dx = geom.CellSize();

    SpreadMarkers(lev, f_out, face_coords[lev], dx, 0);
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::SpreadPredictor(
            int lev,
            std::array<MultiFab, AMREX_SPACEDIM> & f_out
        ) const {


    //___________________________________________________________________________
    // Geometry data
    const Geometry & geom = this->Geom(0);
    const Real     *   dx = geom.CellSize();

    //___________________________________________________________________________
    // We don't need these spreading weights => create a dummy MF
    std::array<MultiFab, AMREX_SPACEDIM> f_weights;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        f_weights[d].define(f_out[d].boxArray(), f_out[d].DistributionMap(),
                            1, f_out[d].nGrow());
        f_weights[d].setVal(0.);
    }


    // for (MFIter pti = MakeMFIter(lev, true); pti.isValid(); ++pti) {
    for (MyConstIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        // Marker (non-neighbor particle) data for current tile
        TileIndex index(pti.index(), pti.LocalTileIndex());
        const auto & particle_data = this->GetParticles(lev).at(index);

        //_______________________________________________________________________
        // Fill vector of marker positions and forces (for current level)
        long np = particle_data.size();

        Vector<RealVect> marker_positions(np), marker_forces(np);

        const AoS & particles = particle_data.GetArrayOfStructs();
        for (int i = 0; i < np; ++i) {
            const ParticleType & part = particles[i];

            RealVect ppos, pfor;

            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                ppos[d] = part.pos(d) + part.rdata(StructReal::pred_posx + d);
                pfor[d] = part.rdata(StructReal::pred_forcex + d);
            }

            marker_positions[i] = ppos;
            marker_forces[i]    = pfor;
        }

        //_______________________________________________________________________
        // Spread the non-neighbor particles (markers)

        // Convert stagered boxes into cell centered
        Box tile_box = enclosedCells(f_out[0][pti].box());

        std::array<      FArrayBox *, AMREX_SPACEDIM> f_out_fab;
        std::array<      FArrayBox *, AMREX_SPACEDIM> f_weights_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> coords_fab;
        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            f_out_fab[d]     = & f_out[d][pti];
            f_weights_fab[d] = & f_weights[d][pti];
            coords_fab[d]    = & face_coords[lev][d][pti];
        }

        SpreadMarkers(marker_forces, marker_positions, tile_box,
                      f_out_fab, f_weights_fab, coords_fab, dx, 0);
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::InterpolateMarkers(
                  Vector<RealVect> & f_out,
            const Vector<RealVect> & f_pos,
            const std::array<MultiFab, AMREX_SPACEDIM> & f_in,
            const std::array<MultiFab, AMREX_SPACEDIM> & f_weights,
            const std::array<MultiFab, AMREX_SPACEDIM> & coords,
            const Real * dx, const int ghost
        ) {



    for (MFIter mfi(f_in[0]); mfi.isValid(); ++mfi) {

        // Convert stagered boxes into cell centered
        Box tile_box = enclosedCells(mfi.tilebox());

        std::array<const FArrayBox *, AMREX_SPACEDIM> f_in_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> f_weights_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> coords_fab;
        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            f_in_fab[d]      = & f_in[d][mfi];
            f_weights_fab[d] = & f_weights[d][mfi];
            coords_fab[d]    = & coords[d][mfi];
        }

        InterpolateMarkers(f_out, f_pos, tile_box,
                           f_in_fab, f_weights_fab, coords_fab, dx, ghost);
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::InterpolateMarkers(
                  Vector<RealVect> & f_out,
            const Vector<RealVect> & f_pos,
            const Box & tile_box,
            const std::array<const FArrayBox *, AMREX_SPACEDIM> & f_in,
            const std::array<const FArrayBox *, AMREX_SPACEDIM> & f_weights,
            const std::array<const FArrayBox *, AMREX_SPACEDIM> & coords,
            const Real * dx, const int ghost
        ){

    const int n_marker = f_pos.size();

    // Convert stagered boxes into cell centered
    Box bx = enclosedCells(f_in[0]->box());

    interpolate_markers(BL_TO_FORTRAN_BOX(bx),
                        BL_TO_FORTRAN_BOX(tile_box),
                        BL_TO_FORTRAN_ANYD(* f_in[0]),
#if (AMREX_SPACEDIM > 1)
                        BL_TO_FORTRAN_ANYD(* f_in[1]),
#endif
#if (AMREX_SPACEDIM > 2)
                        BL_TO_FORTRAN_ANYD(* f_in[2]),
#endif
                        BL_TO_FORTRAN_ANYD(* f_weights[0]),
#if (AMREX_SPACEDIM > 1)
                        BL_TO_FORTRAN_ANYD(* f_weights[1]),
#endif
#if (AMREX_SPACEDIM > 2)
                        BL_TO_FORTRAN_ANYD(* f_weights[2]),
#endif
                        BL_TO_FORTRAN_ANYD(* coords[0]),
#if (AMREX_SPACEDIM > 1)
                        BL_TO_FORTRAN_ANYD(* coords[1]),
#endif
#if (AMREX_SPACEDIM > 2)
                        BL_TO_FORTRAN_ANYD(* coords[2]),
#endif
                        f_pos.dataPtr(),
                        f_out.dataPtr(),
                        & n_marker,
                        dx );

}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::InterpolateMarkers(
            int lev,
            const std::array<MultiFab, AMREX_SPACEDIM> & f_in
        ) {


    //___________________________________________________________________________
    // Geometry data
    const Geometry & geom = this->Geom(0);
    const Real     *   dx = geom.CellSize();

    //___________________________________________________________________________
    // We don't need these spreading weights => create a dummy MF
    std::array<MultiFab, AMREX_SPACEDIM> f_weights;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        f_weights[d].define(f_in[d].boxArray(), f_in[d].DistributionMap(),
                            1, f_in[d].nGrow());
        f_weights[d].setVal(-1.);
    }


    // for (MFIter pti = MakeMFIter(lev, true); pti.isValid(); ++pti) {
    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        // Marker (non-neighbor particle) data for current tile
        TileIndex index(pti.index(), pti.LocalTileIndex());
        auto & particle_data = this->GetParticles(lev).at(index);

        //_______________________________________________________________________
        // Fill vector of marker positions and forces (for current level)
        long np = particle_data.size();

        Vector<RealVect> marker_positions(np), marker_forces(np);

        AoS & particles = particle_data.GetArrayOfStructs();
        for (int i = 0; i < np; ++i) {
            const ParticleType & part = particles[i];

            RealVect ppos, pfor;
            for (int d=0; d<AMREX_SPACEDIM; ++d)
                ppos[d] = part.pos(d);

            // pfor should be (0, .., 0)

            marker_positions[i] = ppos;
            marker_forces[i]    = pfor;
        }


        //_______________________________________________________________________
        // Interpolate the non-neighbor particles (markers)

        // Convert stagered boxes into cell centered
        Box tile_box = enclosedCells(f_in[0][pti].box());

        std::array<const FArrayBox *, AMREX_SPACEDIM> f_in_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> f_weights_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> coords_fab;
        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            f_in_fab[d]      = & f_in[d][pti];
            f_weights_fab[d] = & f_weights[d][pti];
            coords_fab[d]    = & face_coords[lev][d][pti];
        }

        InterpolateMarkers(marker_forces, marker_positions, tile_box,
                           f_in_fab, f_weights_fab, coords_fab, dx, 0);

        // Add interpolated markers back to the particles (markers)
        for (int i = 0; i < np; ++i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d)
                part.rdata(StructReal::velx + d) += marker_forces[i][d];
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::InterpolatePredictor(
            int lev,
            const std::array<MultiFab, AMREX_SPACEDIM> & f_in
        ) {


    //___________________________________________________________________________
    // Geometry data
    const Geometry & geom = this->Geom(0);
    const Real     *   dx = geom.CellSize();

    //___________________________________________________________________________
    // We don't need these spreading weights => create a dummy MF
    std::array<MultiFab, AMREX_SPACEDIM> f_weights;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        f_weights[d].define(f_in[d].boxArray(), f_in[d].DistributionMap(),
                            1, f_in[d].nGrow());
        f_weights[d].setVal(-1.);
    }


    // for (MFIter pti = MakeMFIter(lev, false); pti.isValid(); ++pti) {
    for (MyIBMarIter pti(* this, lev); pti.isValid(); ++pti) {

        // Marker (non-neighbor particle) data for current tile
        TileIndex index(pti.index(), pti.LocalTileIndex());
        auto & particle_data = this->GetParticles(lev).at(index);

        //_______________________________________________________________________
        // Fill vector of marker positions and forces (for current level)
        long np = particle_data.size();

        Vector<RealVect> marker_positions(np), marker_forces(np);

        AoS & particles = particle_data.GetArrayOfStructs();
        for (int i = 0; i < np; ++i) {
            const ParticleType & part = particles[i];

            RealVect ppos, pfor;

            for (int d=0; d<AMREX_SPACEDIM; ++d)
                ppos[d] = part.pos(d) + part.rdata(StructReal::pred_posx + d);

            // pfor should be (0, .., 0)

            marker_positions[i] = ppos;
            marker_forces[i]    = pfor;
        }


        //_______________________________________________________________________
        // Interpolate the non-neighbor particles (markers)

        // Convert stagered boxes into cell centered
        Box tile_box = enclosedCells(f_in[0][pti].box());

        std::array<const FArrayBox *, AMREX_SPACEDIM> f_in_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> f_weights_fab;
        std::array<const FArrayBox *, AMREX_SPACEDIM> coords_fab;
        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            f_in_fab[d]      = & f_in[d][pti];
            f_weights_fab[d] = & f_weights[d][pti];
            coords_fab[d]    = & face_coords[lev][d][pti];
        }

        InterpolateMarkers(marker_forces, marker_positions, tile_box,
                           f_in_fab, f_weights_fab, coords_fab, dx, 0);

        // Add interpolated markers back to the particles (markers)
        for (int i=0; i<np; ++i) {
            ParticleType & part = particles[i];

            for (int d=0; d<AMREX_SPACEDIM; ++d)
                part.rdata(StructReal::pred_velx + d) += marker_forces[i][d];
        }
    }
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::PrintMarkerData(int lev) const {

    // Inverse cell-size vector => max is used for determining IBParticle
    // radius in units of cell size
    Vector<Real> inv_dx = {AMREX_D_DECL(this->Geom(lev).InvCellSize(0),
                                        this->Geom(lev).InvCellSize(1),
                                        this->Geom(lev).InvCellSize(2)   )};

    // Find max inv_dx (in case we have an anisotropic grid)
    Real mx_inv_dx = * std::max_element(inv_dx.begin(), inv_dx.end());


    amrex::AllPrintToFile("ib_marker_data") << "Particles on each box:" << std::endl;


    long local_count = 0;

    // ParIter skips tiles without particles => Iterate over MultiFab instead
    // of ParticleIter. Note also that AmrexParticleContainer uses strange
    // tiling => don't turn off tiling (particles are stored in tile)
    for(MFIter pti = this->MakeMFIter(lev, true); pti.isValid(); ++pti) {
        // MuliFabs are indexed using a pair: (BoxArray index, tile index):
        TileIndex index(pti.index(), pti.LocalTileIndex());

        // Neighbours are stored as raw data (see below)
        int ng = this->neighbors[lev].at(index).size();

        auto & particle_data = this->GetParticles(lev).at(index);
        long np = particle_data.size();

        local_count += np;

        // Print current box info
        AllPrintToFile("ib_marker_data") << "Box:"         << pti.index()
                                         << " "            << pti.tilebox()
                                         << ", count: "    << np
                                         << ", nb count: " << ng
                                         << std::endl;

        // Print IBMarker
        AllPrintToFile("ib_marker_data") << " * IBMarkers:" << std::endl;

        const AoS & particles = particle_data.GetArrayOfStructs();
        for(int i = 0; i < np; i++){
            const ParticleType & part = particles[i];

            AllPrintToFile("ib_marker_data") << "   +--> " << part << std::endl;
        }

        // Print neighbour IBParticles
        AllPrintToFile("ib_marker_data") << " * Grown IBParticles:" << std::endl;

        // TODO: HAXOR!!! This should be fixed ASAP: if I understand this
        // correctly, the neighbor data contains the particle data as a binary
        // array (char). By casting to ParticleType, what we're doing is
        // interpreting the data in neighbours[index] as valid particle data.
        // Also we stride the neighbors[index] array in units of
        // sizeof(ParticleData). All of this is a little too dangerous for my
        // taste: never hide what you're doing from your compiler!!!
        const ParticleType * nbhd_data = (ParticleType *) this->neighbors[lev].at(index).dataPtr();
        for(int i = 0; i < ng; i++){
            const ParticleType & part = nbhd_data[i];

            AllPrintToFile("ib_marker_data") << "   +--> " << part << std::endl;
        }
    }

    AllPrintToFile("ib_marker_data") << "Total for this process: "
                                       << local_count << std::endl << std::endl;
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::InitInternals(int ngrow) {

    ReadStaticParameters();

    this->SetVerbose(0);

    // Turn off certain components for ghost particle communication
    // Field numbers: {0, 1, 2} => {x, y, z} particle coordinates
    //      => 3 corresponds to the start of IBM_realData
    for (int i=3; i < StructReal::count + 3; ++i)
        this->setRealCommComp(i,  true);

    // Field numbers: {0, 1} => {ID, CPU}
    //      => 2 corresponds to the start of IBM_intData
    // We _do_ want the the neighbour particles to have ID and cpu init data.
    for (int i = 2; i < StructInt::count + 2; ++i)
        this->setIntCommComp(i, true);


    //for (int d=0; d<AMREX_SPACEDIM; ++d) {
    //    this->setRealCommComp(StructReal::pred_forcex + d, false);
    //    this->setRealCommComp(StructReal::forcex + d, false);
    //}

    this->setEnableInverse(true);


    /****************************************************************************
     *                                                                          *
     * Fill auxiallry data used by interpolsation                               *
     *   -> face_coords: the face-centered coordinates used by the fluid grids  *
     *                                                                          *
     ***************************************************************************/

    // TODO: this is only assuming 1 fluid level (level 0)
    int lev = 0;

    face_coords.resize(lev + 1);
    const BoxArray & ba            = this->ParticleBoxArray(lev);
    const DistributionMapping & dm = this->ParticleDistributionMap(lev);

    // Determine the minimal ngrow (which depends on the kernel order): This
    // needs to be AT LEAST 1 larger than the `gs` value used in
    // [spread,interpolate]_kernel
    int ngc_min_fluid = common::pkernel_fluid == 3 ? 3 : 5;
    int ngc_min_es    = common::pkernel_es == 3 ? 3 : 5;
    // Let's be on the safe side and use the larger out of the
    // fluid/electro-static kernels
    int ngrow_min    = std::max(ngc_min_es, ngc_min_fluid);
    int ngrow_coords = std::max(ngrow, ngrow_min);

    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        const BoxArray ba_fc = convert(ba, nodal_flag_dir[d]);
        face_coords[lev][d].define(ba_fc, dm, AMREX_SPACEDIM, ngrow_coords);
    }

    const Geometry & geom = this->Geom(lev);
    FindFaceCoords(face_coords[lev], geom);
}



template <typename StructReal, typename StructInt>
void IBMarkerContainerBase<StructReal, StructInt>::ReadStaticParameters() {
    static bool initialized = false;

    if (!initialized) {
        ParmParse pp("particles");

        // AMReX default is false => enable by default
        this->do_tiling = true;
        // Allow user to overwrite
        pp.query("do_tiling",  this->do_tiling);

        // If tiling is enabled, make sure that the tile size is at least the
        // number of ghost cells (otherwise strange things happen)
        if (this->do_tiling)
            this->tile_size = IntVect{AMREX_D_DECL(max_grid_size[0],
                                                   max_grid_size[1],
                                                   max_grid_size[2])};
            //this->tile_size = IntVect{AMREX_D_DECL(nghost, nghost, nghost)};

        // User can overwrite
        Vector<int> ts(BL_SPACEDIM);
        if (pp.queryarr("tile_size", ts))
            this->tile_size = IntVect(ts);

        // pp.query("use_neighbor_list", use_neighbor_list);
        // pp.query("sort_neighbor_list", sort_neighbor_list);

        initialized = true;
    }
}


#endif

#include <main_driver.H>

#include <immbdy_namespace.H>

#include <IBMarkerMD.H>

using namespace amrex;

using namespace immbdy;
using namespace immbdy_md;
using namespace ib_flagellum;



void constrain_ibm_marker(IBMarkerContainer & ib_mc, int ib_lev, int component) {

    BL_PROFILE_VAR("constrain_ibm_marker", TIMER_CONSTRAIN_IBM_MARKER);

    for (IBMarIter pti(ib_mc, ib_lev); pti.isValid(); ++pti) {

        // Get marker data (local to current thread)
        TileIndex index(pti.index(), pti.LocalTileIndex());
        AoS & markers = ib_mc.GetParticles(ib_lev).at(index).GetArrayOfStructs();

        long np = markers.size();

        for (int i = 0; i < np; ++i) {

            ParticleType & mark = markers[i];
            // Zero component only
            mark.rdata(component) = 0.;
        }
    }

    BL_PROFILE_VAR_STOP(TIMER_CONSTRAIN_IBM_MARKER);
}



void anchor_first_marker(IBMarkerContainer & ib_mc, int ib_lev, int component) {

    BL_PROFILE_VAR("anchor_first_marker", TIMER_ANCHOR_FIRST_MARKER);

    for (IBMarIter pti(ib_mc, ib_lev); pti.isValid(); ++pti) {

        // Get marker data (local to current thread)
        TileIndex index(pti.index(), pti.LocalTileIndex());
        AoS & markers = ib_mc.GetParticles(ib_lev).at(index).GetArrayOfStructs();

        long np = markers.size();

        for (int i = 0; i < np; ++i) {

            ParticleType & mark = markers[i];

            if((mark.idata(IBMInt::id_1) == 0)||(mark.idata(IBMInt::id_1) == 1))
                for (int d=0; d<AMREX_SPACEDIM; ++d) mark.rdata(component + d) = 0.;
        }
    }

    BL_PROFILE_VAR_STOP(TIMER_ANCHOR_FIRST_MARKER);
}



Real theta(Real amp_ramp, Real time, int i_ib, int index_marker) {

    if(immbdy::contains_fourier) {

        // First two nodes reserved as "anchor"
        index_marker = std::max(0, index_marker-2);

        int N                 = chlamy_flagellum::N[i_ib][index_marker];
        int coef_len          = ib_flagellum::fourier_coef_len;
        Vector<Real> & a_coef = chlamy_flagellum::a_coef[i_ib][index_marker];
        Vector<Real> & b_coef = chlamy_flagellum::b_coef[i_ib][index_marker];

        Real frequency = ib_flagellum::frequency[i_ib];

        Real dt = 1./N;
        Real th = 0;

        Real k_fact = 2*M_PI/N;
        Real t_unit = frequency*time/dt;
        for (int i=0; i < coef_len; ++i) {
            Real k = k_fact * i;
            th += a_coef[i]*cos(k*t_unit);
            th -= b_coef[i]*sin(k*t_unit);
        }

        return amp_ramp*th/N;

    } else {

        int  N          = ib_flagellum::n_marker[i_ib];
        Real L          = ib_flagellum::length[i_ib];
        Real wavelength = ib_flagellum::wavelength[i_ib];
        Real frequency  = ib_flagellum::frequency[i_ib];
        Real amplitude  = ib_flagellum::amplitude[i_ib];
        Real l_link     = L/(N-1);

        Real theta = l_link*amp_ramp*amplitude*sin(2*M_PI*frequency*time
                     + 2*M_PI/wavelength*index_marker*l_link);

        return theta;
    }
}



void update_ibm_marker(const RealVect & driv_u, Real driv_amp, Real time,
                       IBMarkerContainer & ib_mc, int ib_lev,
                       int component, bool pred_pos) {

    BL_PROFILE_VAR("update_ibm_marker", UpdateForces);

    for (IBMarIter pti(ib_mc, ib_lev); pti.isValid(); ++pti) {

        // Get marker data (local to current thread)
        TileIndex index(pti.index(), pti.LocalTileIndex());
        AoS & markers = ib_mc.GetParticles(ib_lev).at(index).GetArrayOfStructs();
        long np = markers.size();

        for (MarkerListIndex m_index(0, 0); m_index.first<np; ++m_index.first) {

            ParticleType & mark = markers[m_index.first];

            int i_ib    = mark.idata(IBMInt::cpu_1);
            int N       = ib_flagellum::n_marker[i_ib];
            Real L      = ib_flagellum::length[i_ib];
            Real l_link = L/(N-1);

            Real k_spr  = ib_flagellum::k_spring[i_ib];
            Real k_driv = ib_flagellum::k_driving[i_ib];


            // Get previous and next markers connected to current marker (if they exist)
            ParticleType * next_marker = NULL;
            ParticleType * prev_marker = NULL;

            int status = ib_mc.ConnectedMarkers(ib_lev, index, m_index,
                                                prev_marker, next_marker);

            if (status == -1) Abort("status -1 particle detected in predictor!!! flee for your life!");

            // position vectors
            RealVect prev_pos, pos, next_pos;
            if (status == 0) {
                for(int d=0; d<AMREX_SPACEDIM; ++d) {
                    prev_pos[d] = prev_marker->pos(d);
                    pos[d]      =         mark.pos(d);
                    next_pos[d] = next_marker->pos(d);

                    if (pred_pos) {
                        prev_pos[d] += prev_marker->rdata(IBMReal::pred_posx + d);
                        pos[d]      += mark.rdata(IBMReal::pred_posx + d);
                        next_pos[d] += next_marker->rdata(IBMReal::pred_posx + d);
                    }
                }
            }

            // update spring forces
            if (status == 0) { // has next (p) and prev (m)

                RealVect r_p = next_pos - pos, r_m = pos - prev_pos;

                Real lp_m = r_m.vectorLength(),         lp_p = r_p.vectorLength();
                Real fm_0 = k_spr * (lp_m-l_link)/lp_m, fp_0 = k_spr * (lp_p-l_link)/lp_p;

                for (int d=0; d<AMREX_SPACEDIM; ++d) {
                    prev_marker->rdata(component + d) += fm_0 * r_m[d];
                    mark.rdata(component + d)         -= fm_0 * r_m[d];

                    mark.rdata(component + d)         += fp_0 * r_p[d];
                    next_marker->rdata(component + d) -= fp_0 * r_p[d];
                }
            }

            // update bending forces for curent, minus/prev, and next/plus
            if (status == 0) { // has next (p) and prev (m)

                // position vectors
                const RealVect & r = pos, & r_m = prev_pos, & r_p = next_pos;

                // Set bending forces to zero
                RealVect f_p = RealVect{AMREX_D_DECL(0., 0., 0.)};
                RealVect f   = RealVect{AMREX_D_DECL(0., 0., 0.)};
                RealVect f_m = RealVect{AMREX_D_DECL(0., 0., 0.)};

                // calling the active bending force calculation
                // This a simple since wave imposed

                Real th = theta(driv_amp, time, i_ib, mark.idata(IBMInt::id_1));
                driving_f(f, f_p, f_m, r, r_p, r_m, driv_u, th, k_driv);

                // updating the force on the minus, current, and plus particles.
                for (int d=0; d<AMREX_SPACEDIM; ++d) {
                    prev_marker->rdata(component + d) += f_m[d];
                    mark.rdata(component + d)         +=   f[d];
                    next_marker->rdata(component + d) += f_p[d];
                }
            }
        }
    }
    BL_PROFILE_VAR_STOP(UpdateForces);
};

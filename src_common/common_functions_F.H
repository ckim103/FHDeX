#ifndef _common_params_F_H_
#define _common_params_F_H_

#ifdef __cplusplus
extern "C"
{
#endif

    // these are interfaces to fortran subroutines

///////////////////////////
// in common_namelist.F90

    // read in fortran namelist into common_params_module
    void read_common_namelist (const char* inputs_file, const int length);

    // copy contents of common_params_module to C++ common namespace
    void initialize_common_namespace(amrex::Real* prob_lo, amrex::Real* prob_hi, int* n_cells,
                                     int* max_grid_size, int* max_particle_tile_size,amrex::Real* cell_depth, int* ngc,
                                     int* nvars, int* nprimvars,
                                     int* membrane_cell, int* cross_cell, amrex::Real* transmission,
                                     amrex::Real* qval, int* pkernel_fluid, int* pkernel_es,
                                     amrex::Real* fixed_dt,
                                     amrex::Real* cfl, int* max_step, int* plot_int, int* plot_stag,
                                     const char* plot_base_name,
                                     const int plot_base_name_len,
                                     int* chk_int,
                                     const char* chk_base_name, const int chk_base_name_len,
                                     int* prob_type, int* restart, int* particle_restart, int* print_int,
                                     int* project_eos_int, amrex::Real* grav, int* nspecies,
                                     amrex::Real* molmass,amrex::Real* diameter,
                                     int* dof,amrex::Real* hcv,amrex::Real* hcp, amrex::Real* rhobar,
                                     amrex::Real* rho0, amrex::Real* variance_coef_mom,
                                     amrex::Real* variance_coef_mass, amrex::Real* k_B,
                                     amrex::Real* Runiv, amrex::Real* T_init,
                                     int* algorithm_type,
                                     int* advection_type,
                                     int* barodiffusion_type, int* use_bl_rng, int* seed,
                                     int* seed_momentum, int* seed_diffusion,
                                     int* seed_reaction,
                                     int* seed_init_mass, int* seed_init_momentum,
                                     amrex::Real* visc_coef, int* visc_type,
                                     int* filtering_width, int* stoch_stress_form,
                                     amrex::Real* u_init, amrex::Real* perturb_width,
                                     amrex::Real* smoothing_width,
                                     amrex::Real* initial_variance_mom,
                                     amrex::Real* initial_variance_mass,
				     amrex::Real* domega,
                                     int* bc_vel_lo, int* bc_vel_hi,
                                     int* bc_es_lo, int* bc_es_hi,
                                     int* bc_mass_lo, int* bc_mass_hi,
                                     int* bc_therm_lo, int* bc_therm_hi,
                                     amrex::Real * p_lo, amrex::Real * p_hi,
                                     amrex::Real* t_lo, amrex::Real* t_hi,
                                     amrex::Real* wallspeed_lo, amrex::Real* wallspeed_hi,
                                     amrex::Real* potential_lo, amrex::Real* potential_hi,
                                     int* struct_fact_int, int* radialdist_int,
                                     int* cartdist_int, int* n_steps_skip,
				     int* project_dir, int* max_grid_projection,
                                     int* histogram_unit, amrex::Real* density_weights,
                                     int* shift_cc_to_boundary,
                                     int* particle_placement, int* particle_count, int* p_move_tog, int* p_force_tog, int* p_int_tog,
                                     amrex::Real* particle_neff, amrex::Real* particle_n0,
                                     amrex::Real* mass, amrex::Real* nfrac, amrex::Real* permittivity,
                                     amrex::Real* cut_off, amrex::Real* rmin,
                                     amrex::Real* eepsilon, amrex::Real* sigma, int* poisson_verbose,
                                     int* poisson_bottom_verbose,
                                     int* poisson_max_iter, amrex::Real* poisson_rel_tol,
                                     amrex::Real* particle_grid_refine, amrex::Real* es_grid_refine,
                                     amrex::Real* diff, int* all_dry,
                                     int* fluid_tog, int* es_tog, int* drag_tog,
                                     int* move_tog, int* rfd_tog, int* dry_move_tog, int* sr_tog,
                                     int* graphene_tog, int* crange, int* thermostat_tog, int* zero_net_force, int* images,
                                     amrex::Real* eamp, amrex::Real* efreq, amrex::Real* ephase,
                                     int* plot_ascii, int* solve_chem, amrex::Real* diffcoeff,
                                     amrex::Real* scaling_factor, amrex::Real* source_strength,
                                     int* regrid_int, int* do_reflux, int* particle_motion);

    void set_max_step(int* max_step);
    void set_domega(amrex::Real* domega);
    
///////////////////////////

///////////////////////////
// in init_funcs.F90

    void init_rho_2d(const int* lo, const int* hi,
                     amrex_real* data, const int* dlo, const int* dhi,
                     const amrex_real* dx, const amrex_real* prob_lo, const amrex_real* prob_hi);


    void init_vel_2d(const int* lo, const int* hi,
                     amrex_real* data, const int* dlo, const int* dhi,
                     const amrex_real* dx, const amrex_real* prob_lo, const amrex_real* prob_hi, const int* di);

    void init_vel(const int* lo, const int* hi,
                  amrex_real* data, const int* dlo, const int* dhi,
                  const amrex_real* dx, const amrex_real* prob_lo, const amrex_real* prob_hi, const int* di, const amrex::Real* real_lo, const amrex::Real* real_hi);

///////////////////////////

///////////////////////////
// in debug.F90

    void print_mf(const int* lo, const int* hi,
                  const amrex::Real* mf,
                  const int* m_lo, const int* m_hi, const int* nc_m);

///////////////////////////

///////////////////////////
// in time_step.F90

    void euler_step_stag(const int* lo, const int* hi,
                    amrex_real* old, const int* oldlo, const int* oldhi,
                    amrex_real* stagop, const int* stagoplo, const int* stagophi,
                    amrex_real* newdata, const int* newdatahi, const int* newdatalo,
                    const int* offset, const amrex_real* dt);

///////////////////////////
// in find_coords.F90

    void find_face_coords(const amrex_real* real_lo, const amrex_real* real_hi,
                    amrex_real* xface, const int* xfacelo, const int* xfacehi,
                    amrex_real* yface, const int* yfacelo, const int* yfacehi,
#if (AMREX_SPACEDIM == 3)
                    amrex_real* zface, const int* zfacelo, const int* zfacehi,
#endif
                    const amrex_real* dx);


    void find_center_coords(const amrex_real* real_lo, const amrex_real* real_hi,
                    amrex_real* centers, const int* lo, const int* hi,
                    const amrex_real* dx);
///////////////////////////




///////////////////////////
// in find_nodal_values.F90

    void find_nodal_values(const int* lo, const int* hi,
                    const amrex_real* fabIn, const int* inlo, const int* inhi,
                    amrex_real* fabOut, const int* outlo, const int* outhi,
                    const int* xCheck, const int* yCheck, const int* zCheck);
///////////////////////////

///////////////////////////
// in compute_basic_stats.F90

    void compute_means(const amrex_real* inst, const int* inlo, const int* inhi, const int* insize,
		                amrex_real* meanfab, const int* mlo, const int* mhi, const int* msize,
			            const int* incomp, const int* mcomp, const int* steps);

    void compute_vars(const amrex_real* inst, const int* inlo, const int* inhi, const int* insize,
		                const amrex_real* meanfab, const int* mlo, const int* mhi, const int* msize,
		                amrex_real* varfab, const int* vlo, const int* vhi, const int* vsize,
			            const int* incomp, const int* mcomp, const int* vcomp, const int* steps);

    void sum_fab(const int* lo, const int* hi, const amrex_real* in, const int* inlo, const int* inhi, const int* insize,
		                const int* gs, const amrex_real* total, const int* incomp);

    void x_mean_fab(const int* lo, const int* hi, const amrex_real* infab, const int* inlo, const int* inhi, const int* insize,
                                amrex_real* outfab, const int* outlo, const int* outhi, const int* outsize,
		                const int* gs);
#if (AMREX_SPACEDIM == 3)
    void max_speed(const int* lo, const int* hi, const amrex_real* ux, const int* uxlo, const int* uxhi,
                                const amrex_real* uy, const int* uylo, const int* uyhi,
                                const amrex_real* uz, const int* uzlo, const int* uzhi,
                                amrex_real* maxspeed);
#endif
#if (AMREX_SPACEDIM == 2)
    void max_speed(const int* lo, const int* hi, const amrex_real* ux, const int* uxlo, const int* uxhi,
                                const amrex_real* uy, const int* uylo, const int* uyhi,
                                amrex_real* maxspeed);
#endif
///////////////////////////

///////////////////////////
// in compute_averages.F90

  void compute_vert_average(const int* lo, const int* hi,
			    const amrex_real* inputfab, const int* inlo, const int* inhi, const int* insize,
			    amrex_real* meanfab, const int* mlo, const int* mhi, const int* msize, 
			    const int* dir, const int* incomp, const int* mcomp, const int* ncomp);

///////////////////////////

///////////////////////////
// in multifab_physbc_mod.F90

    void fab_physbc(const int * lo,     const int * hi,
                    const int * dom_lo, const int * dom_hi,
                    amrex::Real * pressure, const int * p_lo, const int * p_hi,
                    const int * p_ncomp, int ngc, const int * dim_fill_ghost);

    void fab_electricbc(const int * lo,     const int * hi,
                    const int * dom_lo, const int * dom_hi,
                    amrex::Real * efield, const int * p_lo, const int * p_hi,
                    const int * p_ncomp, int ngc, const int * dim_fill_ghost);

    void fab_potentialbc(const int * lo,     const int * hi,
                    const int * dom_lo, const int * dom_hi,
                    amrex::Real * potential, const int * p_lo, const int * p_hi,
                    const int * p_ncomp, int ngc, const int * dim_fill_ghost);

   void fab_chargebc(const int * lo,     const int * hi,
                    const int * dom_lo, const int * dom_hi,
                    amrex::Real * charge, const int * p_lo, const int * p_hi,
                    const int * p_ncomp, int ngc, const int * dim_fill_ghost);

    void fab_physbc_domainvel(const int * lo,     const int * hi,
                              const int * dom_lo, const int * dom_hi,
                              amrex::Real * vel, const int * v_lo, const int * v_hi,
                              const int * v_ncomp, int ngc, const int * dim_fill_ghost, int* dim);

    void fab_physbc_macvel(const int * lo,     const int * hi,
                           const int * dom_lo, const int * dom_hi,
                           amrex::Real * vel, const int * v_lo, const int * v_hi,
                           const int * v_ncopm, int ngc, const int * dim_fill_ghost, int* dim);

    void fab_physbc_domainstress(const int * lo,     const int * hi,
                              const int * dom_lo, const int * dom_hi,
                              amrex::Real * stress, const int * v_lo, const int * v_hi,
                              const int * v_ncomp, int ngc, const int * dim_fill_ghost, int* dim);

    void fab_physbc_macstress(const int * lo,     const int * hi,
                           const int * dom_lo, const int * dom_hi,
                           amrex::Real * stress, const int * v_lo, const int * v_hi,
                           const int * v_ncopm, int ngc, const int * dim_fill_ghost, int* dim);


///////////////////////////
// in sqrt_mf.F90
    
  void sqrt_mf(const int* lo, const int* hi,
	       const amrex_real* mf, const int* mflo, const int* mfhi, const int* ncomp);

#ifdef __cplusplus
}
#endif

#endif

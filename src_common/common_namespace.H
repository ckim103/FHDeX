namespace common {

    // misc global variables
    extern amrex::IntVect nodal_flag;
    extern amrex::Vector<amrex::IntVect> nodal_flag_dir;
    extern amrex::Vector<amrex::IntVect> nodal_flag_edge;
    extern amrex::IntVect nodal_flag_x;
    extern amrex::IntVect nodal_flag_y;
    extern amrex::IntVect nodal_flag_z;
    extern amrex::IntVect nodal_flag_xy;
    extern amrex::IntVect nodal_flag_xz;
    extern amrex::IntVect nodal_flag_yz;

    // Below are items that are read in from the common namelist

    // Problem specification
    extern amrex::Vector<amrex::Real> prob_lo;              // physical lo coordinate
    extern amrex::Vector<amrex::Real> prob_hi;              // physical hi coordinate
    extern amrex::Vector<int>         n_cells;              // number of cells in domain
    extern amrex::Vector<int>         max_grid_size;        // max number of cells in a box
    extern amrex::Vector<int>         max_particle_tile_size;        // max number of cells in a box
    extern amrex::Real                cell_depth;

    extern amrex::IntVect             ngc;           //number of chost cells
    extern int                        nvars;         //number of conserved variables
    extern int                        nprimvars;     //number of primitive variables

    extern int                        membrane_cell; //cell to locate membrane
    extern int                        cross_cell;    //cell to compute spatial correlation
    extern amrex::Real                transmission;

    extern amrex::Vector<amrex::Real> qval;
    extern int                        pkernel_fluid;
    extern int                        pkernel_es;

    // Time-step control
    extern amrex::Real                fixed_dt;
    extern amrex::Real                cfl;


    // Controls for number of steps between actions
    extern int                        max_step;
    extern int                        plot_int;
    extern int                        plot_stag;
    extern std::string                plot_base_name;
    extern int                        chk_int;
    extern std::string                chk_base_name;
    extern int                        prob_type;
    extern int                        restart;
    extern int                        particle_restart;
    extern int                        print_int;
    extern int                        project_eos_int;

    // Physical parameters
    extern amrex::Vector<amrex::Real> grav;
    extern int                        nspecies;
    extern amrex::Vector<amrex::Real> molmass;
    extern amrex::Vector<amrex::Real> rhobar;
    extern amrex::Real                rho0;
    extern amrex::Vector<amrex::Real> diameter;
    extern amrex::Vector<int>         dof;
    extern amrex::Vector<amrex::Real> hcv;
    extern amrex::Vector<amrex::Real> hcp;

    // stochastic forcing amplitudes (1 for physical values, 0 to run them off)
    extern amrex::Real                variance_coef_mom;
    extern amrex::Real                variance_coef_mass;
    extern amrex::Real                k_B;
    extern amrex::Real                Runiv;
    extern amrex::Vector<Real>        T_init;

    // Algorithm control / selection
    extern int                        algorithm_type;
    extern int                        advection_type;
    extern int                        barodiffusion_type;
    extern int                        use_bl_rng;

    // random number seed
    // 0        = unpredictable seed based on clock
    // positive = fixed seed
    extern int                        seed;

    // as assortment of other seeds in case one needs different engines
    // implementation is problem-dependent
    // 0        = unpredictable seed based on clock
    // positive = fixed seed
    extern int                        seed_momentum;
    extern int                        seed_diffusion;
    extern int                        seed_reaction;
    extern int                        seed_init_mass;
    extern int                        seed_init_momentum;

    // Viscous friction L phi operator
    // if abs(visc_type) = 1, L = div beta grad
    // if abs(visc_type) = 2, L = div [ beta (grad + grad^T) ]
    // if abs(visc_type) = 3, L = div [ beta (grad + grad^T) + I (gamma - (2/3)*beta) div ]
    // positive = assume constant coefficients
    // negative = assume spatially-varying coefficients
    extern amrex::Real                visc_coef;
    extern int                        visc_type;

    // Stochastic momentum flux controls:
    extern int                        filtering_width;
    extern int                        stoch_stress_form;

    // Initial conditions
    extern amrex::Vector<amrex::Real> u_init;
    extern amrex::Real                perturb_width;
    extern amrex::Real                smoothing_width;
    extern amrex::Real                initial_variance_mom;
    extern amrex::Real                initial_variance_mass;
    extern amrex::Real                domega;

    // Boundary conditions
    // ----------------------
    // BC specifications:
    // -1 = periodic
    //  1 = slip
    //  2 = no-slip
    extern amrex::Vector<int>          bc_vel_lo;
    extern amrex::Vector<int>          bc_vel_hi;

    // BC specifications:
    // -1 = periodic
    //  1 = Dirichlet
    //  2 = Neumann
    extern amrex::Vector<int>          bc_es_lo;
    extern amrex::Vector<int>          bc_es_hi;

    // BC specifications:
    // -1 = periodic
    //  1 = wall
    //  2 = reservoir (Dirichlet values must be suppled by other means)
    extern amrex::Vector<int>          bc_mass_lo;
    extern amrex::Vector<int>          bc_mass_hi;

    // BC specifications:
    // -1 = periodic
    //  1 = adiabatic
    //  2 = isothermal
    extern amrex::Vector<int>          bc_therm_lo;
    extern amrex::Vector<int>          bc_therm_hi;

    // Pressure drop are periodic inflow/outflow walls (bc_[hi,lo]=-2).
    extern amrex::Vector<amrex::Real>  p_lo;
    extern amrex::Vector<amrex::Real>  p_hi;

    extern amrex::Vector<amrex::Real>  t_lo;
    extern amrex::Vector<amrex::Real>  t_hi;

    // Each no-slip wall may be moving with a specified tangential
    // velocity along the tangential directions
    // In 2D:
    // wallspeed_lo/hi_x - yvel
    // wallspeed_lo/hi_y - xvel
    // In 3D:
    // wallspeed_lo/hi_x - yvel,zvel
    // wallspeed_lo/hi_y - xvel,zvel
    // wallspeed_lo/hi_z - xvel,yvel
    extern amrex::Vector<amrex::Real> wallspeed_lo;
    extern amrex::Vector<amrex::Real> wallspeed_hi;

    extern amrex::Vector<amrex::Real> potential_lo;
    extern amrex::Vector<amrex::Real> potential_hi;

    // structure factor and radial/cartesian pair correlation function analysis
    extern int                        struct_fact_int;
    extern int                        radialdist_int;
    extern int                        cartdist_int;
    extern int                        n_steps_skip;

    // projection 
    extern int                        project_dir;
    extern amrex::Vector<int>         max_grid_projection;

  /*
    extern int                        stats_int;
    extern int                        n_steps_save_stats;
    extern bool                       analyze_conserved;
    extern bool                       center_snapshots;
  */

    // These are mostly used for reaction-diffusion:
    extern int                        histogram_unit;
    extern amrex::Vector<amrex::Real> density_weights;
    extern amrex::Vector<int>         shift_cc_to_boundary;

    extern int                        particle_placement;
    extern amrex::Vector<int>         particle_count;
    extern amrex::Vector<int>         p_move_tog;
    extern amrex::Vector<int>         p_force_tog;
    extern amrex::Vector<int>         p_int_tog;
    extern amrex::Real                particle_neff;
    extern amrex::Vector<amrex::Real> particle_n0;
    extern amrex::Vector<amrex::Real> mass;
    extern amrex::Vector<amrex::Real> nfrac;

    extern amrex::Real                permittivity;
    extern amrex::Real                cut_off;
    extern amrex::Real                rmin;
    extern amrex::Vector<amrex::Real> eepsilon;
    extern amrex::Vector<amrex::Real> sigma;

    extern int                        poisson_verbose;
    extern int                        poisson_bottom_verbose;
    extern int                        poisson_max_iter;
    extern amrex::Real                poisson_rel_tol;

    extern amrex::Real                particle_grid_refine;
    extern amrex::Real                es_grid_refine;

    // total diffusion coefficient for immersed particle
    extern amrex::Vector<amrex::Real> diff;
    extern int                        all_dry;

    extern int                        fluid_tog;
    extern int                        es_tog;
    extern int                        drag_tog;
    extern int                        move_tog;
    extern int                        rfd_tog;
    extern int                        dry_move_tog;
    extern int                        sr_tog;
    extern int                        graphene_tog;
    extern int                        crange;
    extern int                        thermostat_tog;
    extern int                        zero_net_force;

    extern int                        images;
    extern amrex::Vector<amrex::Real> eamp;
    extern amrex::Vector<amrex::Real> efreq;
    extern amrex::Vector<amrex::Real> ephase;

    extern int                        plot_ascii;
    extern int                        particle_motion;
    
    // parameters neccesary for chemical reactions
    extern int                        solve_chem;    
    extern amrex::Real                diffcoeff;
    extern amrex::Real                scaling_factor;
    extern amrex::Real                source_strength;
    extern int                        regrid_int;    
    extern int                        do_reflux;    

}

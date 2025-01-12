#include "INS_functions.H"

#include "common_functions_F.H"
#include "common_namespace_declarations.H"

#include "gmres_functions_F.H"
#include "gmres_namespace.H"
#include "gmres_namespace_declarations.H"

#include "species.H"
#include "surfaces.H"

#include "StructFact_F.H"
#include "StructFact.H"

#include "StochMFlux.H"

#include "hydro_functions.H"
#include "hydro_functions_F.H"

#include "electrostatic.H"

#include "particle_functions.H"

// argv contains the name of the inputs file entered at the command line
void main_driver(const char* argv)
{
    // timer for total simulation time
    Real strt_time = ParallelDescriptor::second();

    std::string inputs_file = argv;

    // read in parameters from inputs file into F90 modules
    // we use "+1" because of amrex_string_c_to_f expects a null char termination
    read_common_namelist(inputs_file.c_str(),inputs_file.size()+1);
    read_gmres_namelist(inputs_file.c_str(),inputs_file.size()+1);

    // copy contents of F90 modules to C++ namespaces
    InitializeCommonNamespace();
    InitializeGmresNamespace();
    
    int step = 1;
    Real time = 0.;
    int statsCount = 1;

    /*
      Terms prepended with a 'C' are related to the particle grid; only used for finding neighbor lists
      Those with 'P' are for the electostatic grid.
      Those without are for the fluid grid.
      The particle grid and es grids are created as a corsening or refinement of the fluid grid.
    */

    // BoxArray for the fluid
    BoxArray ba;

    // BoxArray for the particles
    BoxArray bc;

    // BoxArray electrostatic grid
    BoxArray bp;
    
    // Box for the fluid
    IntVect dom_lo(AMREX_D_DECL(           0,            0,            0));
    IntVect dom_hi(AMREX_D_DECL(n_cells[0]-1, n_cells[1]-1, n_cells[2]-1));
    Box domain(dom_lo, dom_hi);
    
    // how boxes are distrubuted among MPI processes
    DistributionMapping dmap;

    // set number of ghost cells to fit whole peskin kernel
    // AJN - for perdictor/corrector do we need one more ghost cell if the predictor pushes
    //       a particle into a ghost region?
    int ang = 1;
    if (pkernel_fluid == 3) {
        ang = 2;
    }
    else if (pkernel_fluid == 4) {
        ang = 3;
    }
    else if (pkernel_fluid == 6) {
        ang = 4;
    }

    int ngp = 1;
    if (pkernel_es == 3) {
        ngp = 2;
    }
    else if (pkernel_es == 4) {
        ngp = 3;
    }
    else if (pkernel_es == 6) {
        ngp = 4;
    }
        
    // staggered velocities
    // umac needs extra ghost cells for Peskin kernels
    // note if we are restarting, these are defined and initialized to the checkpoint data
    std::array< MultiFab, AMREX_SPACEDIM > umac;
    std::array< MultiFab, AMREX_SPACEDIM > umacM;    // for storing basic fluid stats
    std::array< MultiFab, AMREX_SPACEDIM > umacV;    // for storing basic fluid stats

    // pressure for GMRES solve; 1 ghost cell
    MultiFab pres;

    // MFs for storing particle statistics
    // A lot of these relate to gas kinetics, but many are still useful so leave in for now.
    MultiFab particleMeans;
    MultiFab particleVars;

    // MF for electric potential
    MultiFab potential;
    
    if (restart < 0) {
        
        // zero is a clock-based seed
        int fhdSeed      = 0;
        int particleSeed = 0;
        int selectorSeed = 0;
        int thetaSeed    = 0;
        int phiSeed      = 0;
        int generalSeed  = 0;

        // "seed" controls all of them and gives distinct seeds to each physical process over each MPI process
        // this should be fixed so each physical process has its own seed control
        if (seed > 0) {
            fhdSeed      = 6*ParallelDescriptor::MyProc() + seed;
            particleSeed = 6*ParallelDescriptor::MyProc() + seed + 1;
            selectorSeed = 6*ParallelDescriptor::MyProc() + seed + 2;
            thetaSeed    = 6*ParallelDescriptor::MyProc() + seed + 3;
            phiSeed      = 6*ParallelDescriptor::MyProc() + seed + 4;
            generalSeed  = 6*ParallelDescriptor::MyProc() + seed + 5;
        }

        //Initialise rngs
        rng_initialize(&fhdSeed,&particleSeed,&selectorSeed,&thetaSeed,&phiSeed,&generalSeed);

        // Initialize the boxarray "ba" from the single box "bx"
        ba.define(domain);

        // Break up boxarray "ba" into chunks no larger than "max_grid_size" along a direction
        // note we are converting "Vector<int> max_grid_size" to an IntVect
        ba.maxSize(IntVect(max_grid_size));

        // how boxes are distrubuted among MPI processes
        dmap.define(ba);
        
        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            umac [d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, ang);
            umacM[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, ang);
            umacV[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, ang);
            umac [d].setVal(0.);
            umacM[d].setVal(0.);
            umacV[d].setVal(0.);
        }

        pres.define(ba,dmap,1,1);
        pres.setVal(0.);

        bc = ba;
        bp = ba;
        
        // particle grid_refine: <1 = refine, >1 = coarsen.
        // assume only powers of 2 for now
        if (particle_grid_refine < 1) {
            int sizeRatio = (int)(1.0/particle_grid_refine);
            bc.refine(sizeRatio);
        }
        else {
            int sizeRatio = (int)(particle_grid_refine);
            bc.coarsen(sizeRatio);
        }
        
        if (es_grid_refine < 1) {
            int sizeRatio = (int)(1.0/es_grid_refine);
            bp.refine(sizeRatio);
        }
        else {
            int sizeRatio = (int)(es_grid_refine);
            bp.coarsen(sizeRatio);
        }
        
        // Variables (C++ index)
        // ( 0) Members
        // ( 1) Density
        // ( 2) velx
        // ( 3) vely
        // ( 4) velz
        // ( 5) Temperature
        // ( 6) jx
        // ( 7) jy
        // ( 8) jz
        // ( 9) energyDensity
        // (10) pressure
        // (11) Cx
        // (12) Cy
        // (13) Cz
        particleMeans.define(bc, dmap, 14, 0);
        particleMeans.setVal(0.);
        
        // Variables (C++ index)
        // ( 0) Members
        // ( 1) Density
        // ( 2) velx
        // ( 3) vely
        // ( 4) velz
        // ( 5) Temperature
        // ( 6) jx
        // ( 7) jy
        // ( 8) jz
        // ( 9) energyDensity
        // (10) pressure
        // (11) GVar
        // (12) KGCross
        // (13) KRhoCross
        // (14) RhoGCross
        // (15) Cx
        // (16) Cy
        // (17) Cz 
        particleVars.define(bc, dmap, 18, 0);
        particleVars.setVal(0.);

        //Cell centred es potential
        potential.define(bp, dmap, 1, ngp);
        potential.setVal(0.);
    }
    else {
        
        // restart from checkpoint
        ReadCheckPoint(step,time,statsCount,umac,umacM,umacV,pres,
                       particleMeans,particleVars,potential);

        // grab DistributionMap from umac
        dmap = umac[0].DistributionMap();
        
        // grab fluid BoxArray from umac and convert to cell-centered
        ba = umac[0].boxArray();
        ba.enclosedCells();

        // grab particle BoxArray from particleMeans
        bc = particleMeans.boxArray();

        // grab electrostatic potential BoxArray from potential
        bp = potential.boxArray();
    }

    // Domain boxes for particle and electrostatic grids
    Box domainC = domain;
    Box domainP = domain;
    
    // particle grid and es grid_refine: <1 = refine, >1 = coarsen.
    // assume only powers of 2 for now
    // note particle grid BoxArray was handled above
    if (particle_grid_refine < 1) {
        int sizeRatio = (int)(1.0/particle_grid_refine);
        domainC.refine(sizeRatio);
    }
    else {
        int sizeRatio = (int)(particle_grid_refine);
        domainC.coarsen(sizeRatio);
    }
    if (es_grid_refine < 1) {
        int sizeRatio = (int)(1.0/es_grid_refine);
        domainP.refine(sizeRatio);
    }
    else {
        int sizeRatio = (int)(es_grid_refine);
        domainP.coarsen(sizeRatio);
    }

    // is the problem periodic?
    Vector<int> is_periodic  (AMREX_SPACEDIM,0);  // set to 0 (not periodic) by default
    Vector<int> is_periodic_c(AMREX_SPACEDIM,0);  // set to 0 (not periodic) by default
    Vector<int> is_periodic_p(AMREX_SPACEDIM,0);  // set to 0 (not periodic) by default
    for (int i=0; i<AMREX_SPACEDIM; ++i) {
        if (bc_vel_lo[i] == -1 && bc_vel_hi[i] == -1) {
            is_periodic  [i] = 1;
            is_periodic_c[i] = 1;
        }
        if (bc_es_lo[i] == -1 && bc_es_hi[i] == -1) {
            is_periodic_p[i] = 1;
        }
    }

    // This defines a Geometry object
    RealBox realDomain({AMREX_D_DECL(prob_lo[0],prob_lo[1],prob_lo[2])},
                       {AMREX_D_DECL(prob_hi[0],prob_hi[1],prob_hi[2])});

    Geometry geom (domain ,&realDomain,CoordSys::cartesian,is_periodic.  data());
    Geometry geomC(domainC,&realDomain,CoordSys::cartesian,is_periodic_c.data());
    Geometry geomP(domainP,&realDomain,CoordSys::cartesian,is_periodic_p.data());

    const Real* dx = geom.CellSize();
    const Real* dxc = geomC.CellSize();
    const Real* dxp = geomP.CellSize();

    // AJN - does cellVols have to be a MultiFab (could it just a Real?)
    MultiFab cellVols(bc, dmap, 1, 0);

#if (AMREX_SPACEDIM == 2)
    cellVols.setVal(dxc[0]*dxc[1]*cell_depth);
#elif (AMREX_SPACEDIM == 3)
    cellVols.setVal(dxc[0]*dxc[1]*dxc[2]);
#endif

    Real dt = fixed_dt;
    Real dtinv = 1.0/dt;

    // AJN - get rid of collision stuff?
#if (AMREX_SPACEDIM == 2)
    int totalCollisionCells = (domainC.hiVect()[0]+1)*(domainC.hiVect()[1]+1);
#elif (AMREX_SPACEDIM == 3)
    int totalCollisionCells = (domainC.hiVect()[0]+1)*(domainC.hiVect()[1]+1)*(domainC.hiVect()[2]+1);
#endif

#if (AMREX_SPACEDIM == 2)
    double domainVol = (prob_hi[0] - prob_lo[0])*(prob_hi[1] - prob_lo[1])*cell_depth;
#elif (AMREX_SPACEDIM == 3)
    double domainVol = (prob_hi[0] - prob_lo[0])*(prob_hi[1] - prob_lo[1])*(prob_hi[2] - prob_lo[2]);
#endif

    //Species type defined in species.H
    //array of length nspecies

    species ionParticle[nspecies];

    double realParticles = 0;
    double simParticles = 0;
    double wetRad;
    double dxAv = (dx[0] + dx[1] + dx[2])/3.0; //This is probably the wrong way to do this.

    if (pkernel_fluid == 3) {
        wetRad = 0.91*dxAv;
    }
    else if (pkernel_fluid == 4) {
        wetRad = 1.255*dxAv;
    }
    else if (pkernel_fluid == 6) {
        wetRad = 1.481*dxAv;
    }

    for(int i=0;i<nspecies;i++) {
        
        ionParticle[i].m = mass[i];
        ionParticle[i].q = qval[i];

        if (diameter[i] > 0) { // positive diameter

            // set diameter from inputs
            ionParticle[i].d = diameter[i];

            // compute total diffusion from input diameter
            ionParticle[i].totalDiff = (k_B*T_init[0])/(6*3.14159265359*(diameter[i]/2.0)*visc_coef);

            // compute wet diffusion from wetRad
            ionParticle[i].wetDiff = (k_B*T_init[0])/(6*3.14159265359*wetRad*visc_coef);

            if (all_dry == 1) {
                ionParticle[i].dryDiff = ionParticle[i].totalDiff;
            }
            else {            
                // dry = total - wet
                ionParticle[i].dryDiff = ionParticle[i].totalDiff - ionParticle[i].wetDiff;
            }

        }
        else { // zero or negative diameter

            // set total diffusion from inputs
            ionParticle[i].totalDiff = diff[i];            

            // set diameter from total diffusion (Stokes Einsten)
            ionParticle[i].d = 2.0*(k_B*T_init[0])/(6*3.14159265359*(ionParticle[i].totalDiff)*visc_coef);

            // compute wet diffusion from wetRad
            ionParticle[i].wetDiff = (k_B*T_init[0])/(6*3.14159265359*wetRad*visc_coef);

            if (all_dry == 1) {
                ionParticle[i].dryDiff = ionParticle[i].totalDiff;
            }
            else {            
                // dry = total - wet
                ionParticle[i].dryDiff = ionParticle[i].totalDiff - ionParticle[i].wetDiff;
            }
        }

        if (all_dry == 1 && fluid_tog != 0) {
            Abort("Abort: Don't use all_dry=1 and fluid_tog!=0 together");
        }

        Print() << "Species " << i << "\n"
                << " total diffusion: " << ionParticle[i].totalDiff << "\n"
                << " wet diffusion: " << ionParticle[i].wetDiff
                << " percent wet: " << 100.*ionParticle[i].wetDiff/ionParticle[i].totalDiff << "\n"
                << " dry diffusion: " << ionParticle[i].dryDiff
                << " percent dry: " << 100.*ionParticle[i].dryDiff/ionParticle[i].totalDiff << "\n"
                << " total radius: " << ionParticle[i].d/2.0 << "\n"
                << " wet radius: " << wetRad << "\n"
                << " dry radius: " << (k_B*T_init[0])/(6*3.14159265359*(ionParticle[i].dryDiff)*visc_coef) << "\n";

        if (ionParticle[i].dryDiff < 0) {
            Print() << "Negative dry diffusion in species " << i << "\n";
            Abort();
        }

        ionParticle[i].Neff = particle_neff; // From DSMC, this will be set to 1 for electolyte calcs
        ionParticle[i].R = k_B/ionParticle[i].m; //used a lot in kinetic stats cals, bu not otherwise necessary for electrolytes

        ionParticle[i].sigma = sigma[i];
        ionParticle[i].eepsilon = eepsilon[i];

        // round up particles so there are the same number in each box;
        // we have to divide them into whole numbers of particles somehow. 
        if (particle_count[i] >= 0) {
            ionParticle[i].ppb = (double)particle_count[i]/(double)ba.size();
            ionParticle[i].total = particle_count[i];
            ionParticle[i].n0 = ionParticle[i].total/domainVol;
            
            Print() << "Species " << i << " count adjusted to " << ionParticle[i].total << "\n";
        }
        else {
            // if particle count is negative, we instead compute the number of particles based on particle density and particle_neff
            ionParticle[i].total = (int)ceil(particle_n0[i]*domainVol/particle_neff);
            // adjust number of particles up so there is the same number per box  
            ionParticle[i].ppb = (int)ceil((double)ionParticle[i].total/(double)ba.size());
            //ionParticle[i].total = ionParticle[i].ppb*ba.size();
            ionParticle[i].n0 = ionParticle[i].total/domainVol;

            Print() << "Species " << i << " n0 adjusted to " << ionParticle[i].n0 << "\n";
        }

        Print() << "Species " << i << " particles per box: " <<  ionParticle[i].ppb << "\n";

        realParticles = realParticles + ionParticle[i].total*particle_neff;
        simParticles = simParticles + ionParticle[i].total;
    }
    
    Print() << "Total real particles: " << realParticles << "\n";
    Print() << "Total sim particles: " << simParticles << "\n";

    Print() << "Sim particles per box: " << simParticles/(double)ba.size() << "\n";

    Print() << "Collision cells: " << totalCollisionCells << "\n";
    Print() << "Sim particles per cell: " << simParticles/totalCollisionCells << "\n";

    // see the variable list used above above for particleMeans
    MultiFab particleInstant(bc, dmap, 14, 0);
    
    //-----------------------------
    //  Hydro setup

    ///////////////////////////////////////////
    // rho, alpha, beta, gamma:
    ///////////////////////////////////////////

    // this only needs 1 ghost cell    
    MultiFab rho(ba, dmap, 1, 1);
    rho.setVal(1.);

    // the number of ghost cells needed for the GMRES solve for alpha, beta, etc., is a fixed number
    // and not dependent on the Peskin kernels.
    // alpha_fc -> 1 ghost cell
    // beta -> 1
    // beta_ed -> 1
    // gamma -> 1
    
    // alpha_fc arrays
    std::array< MultiFab, AMREX_SPACEDIM > alpha_fc;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        alpha_fc[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 1);
        alpha_fc[d].setVal(dtinv);
    }

    // beta cell centred
    MultiFab beta(ba, dmap, 1, 1);
    beta.setVal(visc_coef);

    // beta on nodes in 2d
    // beta on edges in 3d
    std::array< MultiFab, NUM_EDGE > beta_ed;
#if (AMREX_SPACEDIM == 2)
    beta_ed[0].define(convert(ba,nodal_flag), dmap, 1, 1);
#elif (AMREX_SPACEDIM == 3)
    beta_ed[0].define(convert(ba,nodal_flag_xy), dmap, 1, 1);
    beta_ed[1].define(convert(ba,nodal_flag_xz), dmap, 1, 1);
    beta_ed[2].define(convert(ba,nodal_flag_yz), dmap, 1, 1);
#endif
    for (int d=0; d<NUM_EDGE; ++d) {
        beta_ed[d].setVal(visc_coef);
    }

    // cell-centered gamma
    MultiFab gamma(ba, dmap, 1, 1);
    gamma.setVal(0.);

    ///////////////////////////////////////////

    ///////////////////////////////////////////
    // Define & initalize eta & temperature multifabs
    ///////////////////////////////////////////
    
    // eta & temperature
    const Real eta_const = visc_coef;
    const Real temp_const = T_init[0];      // [units: K]

    // the number of ghost cells needed here is
    // eta_cc -> 1
    // temp_cc -> 1
    // eta_ed -> 0
    // temp_ed -> 0
    
    // eta and temperature; cell-centered
    MultiFab  eta_cc(ba, dmap, 1, 1);
    MultiFab temp_cc(ba, dmap, 1, 1);
    
    // eta and temperature; nodal
    std::array< MultiFab, NUM_EDGE >  eta_ed;
    std::array< MultiFab, NUM_EDGE > temp_ed;
#if (AMREX_SPACEDIM == 2)
    eta_ed [0].define(convert(ba,nodal_flag),    dmap, 1, 0);
    temp_ed[0].define(convert(ba,nodal_flag),    dmap, 1, 0);
#elif (AMREX_SPACEDIM == 3)
    eta_ed [0].define(convert(ba,nodal_flag_xy), dmap, 1, 0);
    eta_ed [1].define(convert(ba,nodal_flag_xz), dmap, 1, 0);
    eta_ed [2].define(convert(ba,nodal_flag_yz), dmap, 1, 0);
    temp_ed[0].define(convert(ba,nodal_flag_xy), dmap, 1, 0);
    temp_ed[1].define(convert(ba,nodal_flag_xz), dmap, 1, 0);
    temp_ed[2].define(convert(ba,nodal_flag_yz), dmap, 1, 0);
#endif

    // Initalize eta & temperature multifabs
    eta_cc .setVal(eta_const);
    temp_cc.setVal(temp_const);
    for (int d=0; d<NUM_EDGE; ++d) {
        eta_ed[d] .setVal(eta_const);
        temp_ed[d].setVal(temp_const);
    }
    ///////////////////////////////////////////

    ///////////////////////////////////////////
    // random fluxes:
    ///////////////////////////////////////////

    // mflux divergence, staggered in x,y,z
    // Define mfluxdiv predictor/corrector multifabs
    std::array< MultiFab, AMREX_SPACEDIM >  stochMfluxdiv;
    std::array< MultiFab, AMREX_SPACEDIM >  stochMfluxdivC;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        stochMfluxdiv [d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 0);
        stochMfluxdivC[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, 0);
        stochMfluxdiv [d].setVal(0.0);
        stochMfluxdivC[d].setVal(0.0);
    }

    // Declare object of StochMFlux class
    int n_rngs = 1; // we only need 1 stage of random numbers
    StochMFlux sMflux (ba,dmap,geom,n_rngs);

    // weights for random number stages
    Vector< amrex::Real> weights;
    weights = {1.0};

    ///////////////////////////////////////////

/*    
    // Setting the intial velocities can be useful for debugging, to get a known velocity field.
    // Note that we don't need to initialize velocities for the overdamped case.
    // They only matter as an initial guess to GMRES.
    // The first GMRES solve will compute the velocities as long as they start out with non-NaN values.
    int dm = 0;
    for ( MFIter mfi(beta); mfi.isValid(); ++mfi ) {
        const Box& bx = mfi.validbox();

        AMREX_D_TERM(dm=0; init_vel(BL_TO_FORTRAN_BOX(bx),
                                    BL_TO_FORTRAN_ANYD(umac[0][mfi]), geom.CellSize(),
                                    geom.ProbLo(), geom.ProbHi() ,&dm,
                                    ZFILL(realDomain.lo()), ZFILL(realDomain.hi()));,
                     dm=1; init_vel(BL_TO_FORTRAN_BOX(bx),
                                    BL_TO_FORTRAN_ANYD(umac[1][mfi]), geom.CellSize(),
                                    geom.ProbLo(), geom.ProbHi() ,&dm,
                                    ZFILL(realDomain.lo()), ZFILL(realDomain.hi()));,
                     dm=2; init_vel(BL_TO_FORTRAN_BOX(bx),
                                    BL_TO_FORTRAN_ANYD(umac[2][mfi]), geom.CellSize(),
                                    geom.ProbLo(), geom.ProbHi() ,&dm,
                                    ZFILL(realDomain.lo()), ZFILL(realDomain.hi())););

    }

    if (initial_variance_mom != 0.0) {
        // sMflux.addMfluctuations(umac, rho, temp_cc, initial_variance_mom);
        Abort("Initial momentum fluctuations not implemented; if you are overdamped they don't make sense anyway.");
    }
*/
        
    // additional staggered velocity MultiFabs
    std::array< MultiFab, AMREX_SPACEDIM > umacNew;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        umacNew[d].define(convert(ba,nodal_flag_dir[d]), dmap, 1, ang);
    }

    // staggered real coordinates - fluid grid
    std::array< MultiFab, AMREX_SPACEDIM > RealFaceCoords;
    AMREX_D_TERM(RealFaceCoords[0].define(convert(ba,nodal_flag_x), dmap, AMREX_SPACEDIM, ang);,
                 RealFaceCoords[1].define(convert(ba,nodal_flag_y), dmap, AMREX_SPACEDIM, ang);,
                 RealFaceCoords[2].define(convert(ba,nodal_flag_z), dmap, AMREX_SPACEDIM, ang););

    // staggered source terms - fluid grid
    std::array< MultiFab, AMREX_SPACEDIM > source;
    AMREX_D_TERM(source[0].define(convert(ba,nodal_flag_x), dmap, 1, ang);,
                 source[1].define(convert(ba,nodal_flag_y), dmap, 1, ang);,
                 source[2].define(convert(ba,nodal_flag_z), dmap, 1, ang););

    // staggered temporary holder for calculating source terms - This may not be necesssary, review later.
    std::array< MultiFab, AMREX_SPACEDIM > sourceTemp;
    AMREX_D_TERM(sourceTemp[0].define(convert(ba,nodal_flag_x), dmap, 1, ang);,
                 sourceTemp[1].define(convert(ba,nodal_flag_y), dmap, 1, ang);,
                 sourceTemp[2].define(convert(ba,nodal_flag_z), dmap, 1, ang););

    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        source    [d].setVal(0.0);
        sourceTemp[d].setVal(0.0);
    }

    //Define parametric surfaces for particle interaction - declare array for surfaces and then define properties in BuildSurfaces

    // AJN - we don't understand why you need this for ions
#if (BL_SPACEDIM == 3)
    int surfaceCount = 6;
    surface surfaceList[surfaceCount];
    BuildSurfaces(surfaceList,surfaceCount,realDomain.lo(),realDomain.hi());
#endif
#if (BL_SPACEDIM == 2)
    int surfaceCount = 5;
    surface surfaceList[surfaceCount];
    BuildSurfaces(surfaceList,surfaceCount,realDomain.lo(),realDomain.hi());
#endif

    // IBMarkerContainerBase default behaviour is to do tiling. Turn off here:

    //----------------------    
    // Particle tile size
    //----------------------
    Vector<int> ts(BL_SPACEDIM);
    
    for (int d=0; d<AMREX_SPACEDIM; ++d) {        
        if (max_particle_tile_size[d] > 0) {
            ts[d] = max_particle_tile_size[d];
        }
        else {
            ts[d] = max_grid_size[d];
        }
    }

    ParmParse pp ("particles");
    pp.addarr("tile_size", ts);

    //int num_neighbor_cells = 4; replaced by input var
    //Particles! Build on geom & box array for collision cells/ poisson grid?
    FhdParticleContainer particles(geomC, dmap, bc, crange);

    if (restart < 0 && particle_restart < 0) {
        // create particles
        particles.InitParticles(ionParticle, dxp);
    }
    else {
        ReadCheckPointParticles(particles);
    }

    //Find coordinates of cell faces (fluid grid). May be used for interpolating fields to particle locations
    FindFaceCoords(RealFaceCoords, geom); //May not be necessary to pass Geometry?
    
    //----------------------    
    // Electrostatic setup
    //----------------------

    // cell centered real coordinates - es grid
    MultiFab RealCenteredCoords;
    RealCenteredCoords.define(bp, dmap, AMREX_SPACEDIM, ngp);

    FindCenterCoords(RealCenteredCoords, geomP);

    //charage density for RHS of Poisson Eq.
    MultiFab charge(bp, dmap, 1, ngp);
    MultiFab chargeTemp(bp, dmap, 1, ngp);
    charge.setVal(0);
    chargeTemp.setVal(0);

    //mass density on ES grid - not necessary
    MultiFab massFrac(bp, dmap, 1, 1);
    MultiFab massFracTemp(bp, dmap, 1, 1);
    massFrac.setVal(0);
    massFracTemp.setVal(0);

    //Staggered electric fields
    std::array< MultiFab, AMREX_SPACEDIM > efield;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        efield[d].define(convert(bp,nodal_flag_dir[d]), dmap, 1, ngp);
    }

    //Centred electric fields (using an array instead of a multi-component MF)
    std::array< MultiFab, AMREX_SPACEDIM > efieldCC;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        efieldCC[d].define(bp, dmap, 1, ngp);
        efieldCC[d].setVal(0.);
    }

    // external field
    std::array< MultiFab, AMREX_SPACEDIM > external;
    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        external[d].define(bp, dmap, 1, ngp);
    }
    
    MultiFab dryMobility(ba, dmap, nspecies*AMREX_SPACEDIM, ang);

    ComputeDryMobility(dryMobility, ionParticle, geom);

    ///////////////////////////////////////////
    // structure factor:
    ///////////////////////////////////////////

    Vector< std::string > var_names;
    int nvar_sf = 1;
    // int nvar_sf = AMREX_SPACEDIM;
    var_names.resize(nvar_sf);
    var_names[0] = "charge";

    MultiFab struct_in_cc;
    struct_in_cc.define(bp, dmap, nvar_sf, 0);

    amrex::Vector< int > s_pairA(nvar_sf);
    amrex::Vector< int > s_pairB(nvar_sf);

    // Select which variable pairs to include in structure factor:
    for (int d=0; d<nvar_sf; d++) {
        s_pairA[d] = d;
        s_pairB[d] = d;
    }

    Real dVol = dx[0]*dx[1];
    int tot_n_cells = n_cells[0]*n_cells[1];
    if (AMREX_SPACEDIM == 2) {
        dVol *= cell_depth;
    } else if (AMREX_SPACEDIM == 3) {
        dVol *= dx[2];
        tot_n_cells = n_cells[2]*tot_n_cells;
    }

    Vector<Real> scaling;
    scaling.resize(nvar_sf);
    scaling[0] = 1.;

    StructFact structFact(bp,dmap,var_names,scaling,s_pairA,s_pairB);

/*
    // write a plotfile on restart
    // note particle data isn't updated for plotfile generation yet
    if (restart > 0) {    
        WritePlotFile(step-1, time, geom, geomC, geomP,
                      particleInstant, particleMeans, particleVars, particles,
                      charge, potential, efieldCC, dryMobility);
        
        WritePlotFileHydro(step-1,time,geom,umac,pres, umacM, umacV);
    }
*/
    
    //Time stepping loop
    for (int istep=step; istep<=max_step; ++istep) {

        // timer for time step
        Real time1 = ParallelDescriptor::second();
    
        //Most of these functions are sensitive to the order of execution. We can fix this, but for now leave them in this order.

        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            external[d].setVal(eamp[d]*cos(efreq[d]*time + ephase[d]));  // external field
            source    [d].setVal(0.0);      // reset source terms
            sourceTemp[d].setVal(0.0);      // reset source terms
        }

        //particles.BuildCorrectionTable(dxp,0);

        if (rfd_tog==1) {
            // Apply RFD force to fluid
            //particles.RFD(0, dx, sourceTemp, RealFaceCoords);
            //particles.ResetMarkers(0);
            particles.DoRFD(dt, dx, dxp, geom, umac, efieldCC, RealFaceCoords, RealCenteredCoords,
                            source, sourceTemp, surfaceList, surfaceCount, 3 /*this number currently does nothing, but we will use it later*/);
        }
        else {
            // set velx/y/z and forcex/y/z for each particle to zero
            particles.ResetMarkers(0);
        }

        // sr_tog is short range forces
        // es_tog is electrostatic solve (0=off, 1=Poisson, 2=Pairwise, 3=P3M)
        if (sr_tog==1 || es_tog==3) {
            // each tile clears its neighbors
            particles.clearNeighbors();
            
            // fill the neighbor buffers for each tile with the proper data
            particles.fillNeighbors();

            // compute short range forces (if sr_tog=1)
            // compute P3M short range correction (if es_tog=3)
            particles.computeForcesNL(charge, RealCenteredCoords, dxp);
        }

        if (es_tog==1 || es_tog==3) {
            // spreads charge density from ions onto multifab 'charge'.
            particles.collectFields(dt, dxp, RealCenteredCoords, geomP, charge, chargeTemp, massFrac, massFracTemp);
        }
        
        // do Poisson solve using 'charge' for RHS, and put potential in 'potential'.
        // Then calculate gradient and put in 'efieldCC', then add 'external'.
        esSolve(potential, charge, efieldCC, external, geomP);

        // compute other forces and spread to grid
        particles.SpreadIons(dt, dx, dxp, geom, umac, efieldCC, charge, RealFaceCoords, RealCenteredCoords, source, sourceTemp, surfaceList,
                             surfaceCount, 3 /*this number currently does nothing, but we will use it later*/);

        //particles.BuildCorrectionTable(dxp,1);

        if ((variance_coef_mom != 0.0) && fluid_tog != 0) {
            // compute the random numbers needed for the stochastic momentum forcing
            sMflux.fillMStochastic();

            // compute stochastic momentum force
            sMflux.StochMFluxDiv(stochMfluxdiv,0,eta_cc,eta_ed,temp_cc,temp_ed,weights,dt);

            // integrator containing inertial terms and predictor/corrector requires 2 RNG stages
            if (fluid_tog ==2) {
                sMflux.StochMFluxDiv(stochMfluxdivC,0,eta_cc,eta_ed,temp_cc,temp_ed,weights,dt);
            }
        }

        // AJN - should this be an if/else fluid_tog==2?
        if (fluid_tog == 1) {
            advanceStokes(umac,pres,stochMfluxdiv,source,alpha_fc,beta,gamma,beta_ed,geom,dt);
        }
        else if (fluid_tog == 2) {
            Abort("Don't use fluid_tog=2 (inertial Low Mach solver)");
/*            
            MultiFab tracer(ba, dmap, 1,1);
            tracer.setVal(0.);
            advanceLowMach(umac, umacNew, pres, tracer, stochMfluxdiv, stochMfluxdivC,
                           alpha_fc, beta, gamma, beta_ed, geom,dt);
*/
        }

        // total particle move (1=single step, 2=midpoint)
        if (move_tog != 0)
        {
            //Calls wet ion interpolation and movement.
            Print() << "Start move.\n";
            particles.MoveIons(dt, dx, dxp, geom, umac, efield, RealFaceCoords, source, sourceTemp, dryMobility, surfaceList,
                               surfaceCount, 3 /*this number currently does nothing, but we will use it later*/);

            // reset statistics after step n_steps_skip
            // if n_steps_skip is negative, we use it as an interval
            if ((n_steps_skip > 0 && istep == n_steps_skip) ||
                (n_steps_skip < 0 && istep%n_steps_skip == 0) ) {

                particles.MeanSqrCalc(0, 1);
            }
            else {
                particles.MeanSqrCalc(0, 0);
            }

            particles.Redistribute();
            particles.ReBin();
            Print() << "Finish move.\n";
        }

        // reset statistics after step n_steps_skip
        // if n_steps_skip is negative, we use it as an interval
        if ((n_steps_skip > 0 && istep == n_steps_skip) ||
            (n_steps_skip < 0 && istep%n_steps_skip == 0) ) {
            
            particleMeans.setVal(0.0);
            particleVars.setVal(0);

            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                umacM[d].setVal(0.);
                umacV[d].setVal(0.);
            }
                
            Print() << "Resetting stat collection.\n";

            statsCount = 1;
        }

        // g(r)
        if(radialdist_int>0 && istep%radialdist_int == 0) {
            
            // timer
            Real time_PC1 = ParallelDescriptor::second();

            // compute g(r)
            particles.RadialDistribution(simParticles, istep, ionParticle);

            // timer
            Real time_PC2 = ParallelDescriptor::second() - time_PC1;
            ParallelDescriptor::ReduceRealMax(time_PC2);
            amrex::Print() << "Time spend computing radial distribution = " << time_PC2 << std::endl;
        }

        // g(x), g(y), g(z)
        if(cartdist_int>0 && istep%cartdist_int == 0) {

            // timer
            Real time_PC1 = ParallelDescriptor::second();
        
            // compute g(x), g(y), g(z)
            particles.CartesianDistribution(simParticles, istep, ionParticle);
            
            // timer
            Real time_PC2 = ParallelDescriptor::second() - time_PC1;
            ParallelDescriptor::ReduceRealMax(time_PC2);
            amrex::Print() << "Time spend computing Cartesian distribution = " << time_PC2 << std::endl;
        }
        
        particles.EvaluateStats(particleInstant, particleMeans, particleVars, cellVols, ionParticle[0], dt,statsCount);

        for (int d=0; d<AMREX_SPACEDIM; ++d) {
            ComputeBasicStats(umac[d], umacM[d], umacV[d], 1, 1, statsCount);
        }

        statsCount++;
        
	//_______________________________________________________________________
	// Update structure factor
        if (istep > n_steps_skip && struct_fact_int > 0 && (istep-n_steps_skip-1)%struct_fact_int == 0) {
            MultiFab::Copy(struct_in_cc, charge, 0, 0, nvar_sf, 0);
            structFact.FortStructure(struct_in_cc,geomP);
            // plot structure factor on plot_int
            if (istep%plot_int == 0) {
                structFact.WritePlotFile(istep,time,geomP,"plt_SF");
            }
        }

        if (plot_int > 0 && istep%plot_int == 0) {

            // This write particle data and associated fields and electrostatic fields
            WritePlotFile(istep, time, geom, geomC, geomP,
                          particleInstant, particleMeans, particleVars, particles,
                          charge, potential, efieldCC, dryMobility);

            // Writes instantaneous flow field and some other stuff? Check with Guy.
            WritePlotFileHydro(istep, time, geom, umac, pres, umacM, umacV);
        }

        if (chk_int > 0 && istep%chk_int == 0) {
            WriteCheckPoint(istep, time, statsCount, umac, umacM, umacV, pres,
                            particles, particleMeans, particleVars, potential);
        }

        // timer for time step
        Real time2 = ParallelDescriptor::second() - time1;
        ParallelDescriptor::ReduceRealMax(time2);
        amrex::Print() << "Advanced step " << istep << " in " << time2 << " seconds\n";
        
        time = time + dt;
    }
    ///////////////////////////////////////////

    // timer for total simulation time
    Real stop_time = ParallelDescriptor::second() - strt_time;
    ParallelDescriptor::ReduceRealMax(stop_time);
    amrex::Print() << "Run time = " << stop_time << std::endl;

}

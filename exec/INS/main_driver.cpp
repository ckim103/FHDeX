#include "common_functions.H"
#include "gmres_functions.H"

#include "common_functions_F.H"
#include "common_namespace.H"
#include "common_namespace_declarations.H"

#include "gmres_functions_F.H"
#include "gmres_namespace.H"
#include "gmres_namespace_declarations.H"

#include "INS_functions.H"

using namespace common;
using namespace gmres;

// argv contains the name of the inputs file entered at the command line
void main_driver(const char* argv)
{

    // store the current time so we can later compute total run time.
    Real strt_time = ParallelDescriptor::second();

    std::string inputs_file = argv;

    // read in parameters from inputs file into F90 modules
    // we use "+1" because of amrex_string_c_to_f expects a null char termination
    read_common_namelist(inputs_file.c_str(),inputs_file.size()+1);
    read_gmres_namelist(inputs_file.c_str(),inputs_file.size()+1);

    // copy contents of F90 modules to C++ namespaces
    InitializeCommonNamespace();
    InitializeGmresNamespace();

    // is the problem periodic?
    Vector<int> is_periodic(AMREX_SPACEDIM,0);  // set to 0 (not periodic) by default
    for (int i=0; i<AMREX_SPACEDIM; ++i) {
        if (bc_lo[i] == 0 && bc_hi[i] == 0) {
            is_periodic[i] = 1;
        }
    }

    // make BoxArray and Geometry
    BoxArray ba;
    Geometry geom;
    {
        IntVect dom_lo(AMREX_D_DECL(           0,            0,            0));
        IntVect dom_hi(AMREX_D_DECL(n_cells[0]-1, n_cells[1]-1, n_cells[2]-1));
        Box domain(dom_lo, dom_hi);

        // Initialize the boxarray "ba" from the single box "bx"
        ba.define(domain);

        // Break up boxarray "ba" into chunks no larger than "max_grid_size" along a direction
        // note we are converting "Vector<int> max_grid_size" to an IntVect
        ba.maxSize(IntVect(max_grid_size));

       // This defines the physical box, [-1,1] in each direction.
        RealBox real_box({AMREX_D_DECL(prob_lo[0],prob_lo[1],prob_lo[2])},
                         {AMREX_D_DECL(prob_hi[0],prob_hi[1],prob_hi[2])});

        // This defines a Geometry object
        geom.define(domain,&real_box,CoordSys::cartesian,is_periodic.data());
    }
  
    // how boxes are distrubuted among MPI processes
    DistributionMapping dmap(ba);

    // total density
    MultiFab rhotot(ba, dmap, 1, 1);

    // divergence
    MultiFab div(ba, dmap, 1, 1);

    // set density to 1
    rhotot.setVal(1.);

    // staggered velocities
    std::array< MultiFab, AMREX_SPACEDIM > umac;
    AMREX_D_TERM(umac[0].define(convert(ba,nodal_flag_x), dmap, 1, 1);,
                 umac[1].define(convert(ba,nodal_flag_y), dmap, 1, 1);,
                 umac[2].define(convert(ba,nodal_flag_z), dmap, 1, 1););

    // ***REPLACE THIS WITH A FUNCTION THAT SETS THE INITIAL VELOCITY***
    // ***SETTING THESE TO DUMMY VALUES FOR NOW***
    AMREX_D_TERM(umac[0].setVal(100.);,
                 umac[1].setVal(100.);,
                 umac[2].setVal(100.););

	int dm = 0;
	for ( MFIter mfi(rhotot); mfi.isValid(); ++mfi )
    {
        const Box& bx = mfi.validbox();

		AMREX_D_TERM(dm=0; init_vel(BL_TO_FORTRAN_BOX(bx),
							BL_TO_FORTRAN_ANYD(umac[0][mfi]), geom.CellSize(),
            				geom.ProbLo(), geom.ProbHi() ,&dm);,
					dm=1; init_vel(BL_TO_FORTRAN_BOX(bx),
            			    BL_TO_FORTRAN_ANYD(umac[1][mfi]), geom.CellSize(),
            			    geom.ProbLo(), geom.ProbHi() ,&dm);,
					dm=2; init_vel(BL_TO_FORTRAN_BOX(bx),
                  			BL_TO_FORTRAN_ANYD(umac[2][mfi]), geom.CellSize(),
                   			geom.ProbLo(), geom.ProbHi() ,&dm););
    }

	for ( MFIter mfi(div); mfi.isValid(); ++mfi )
    {
        const Box& bx = mfi.validbox();
#if AMREX_SPACEDIM == 2
		compute_div2d(BL_TO_FORTRAN_BOX(bx),
					BL_TO_FORTRAN_ANYD(umac[0][mfi]), BL_TO_FORTRAN_ANYD(umac[1][mfi]), BL_TO_FORTRAN_ANYD(div[mfi]), 
					geom.CellSize());
#endif

#if AMREX_SPACEDIM == 3
		compute_div2d(BL_TO_FORTRAN_BOX(bx),
					BL_TO_FORTRAN_ANYD(umac[0][mfi]), BL_TO_FORTRAN_ANYD(umac[1][mfi]), BL_TO_FORTRAN_ANYD(umac[2][mfi]), BL_TO_FORTRAN_ANYD(div[mfi]), 
					geom.CellSize());
#endif

    }


    int step = 0;
    Real time = 0.;

    // write out rhotot and umac to a plotfile
    WritePlotFile(step,time,geom,rhotot,umac,div);

    // write a loop here to advance the solution in time


    // Call the timer again and compute the maximum difference between the start time 
    // and stop time over all processors
    Real stop_time = ParallelDescriptor::second() - strt_time;
    ParallelDescriptor::ReduceRealMax(stop_time);
    amrex::Print() << "Run time = " << stop_time << std::endl;
}
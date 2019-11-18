#include <AMReX.H>
#include <iostream>

// additional header files needed by MUI
#include <mpi.h>
#include <lib_mpi_split.h>
#include <mui.h>

// function declaration
void main_driver (const char* argv);

using namespace std;

int main (int argc, char* argv[])
{
    cout << "Hello World from compressible_mui\n";

    MPI_Comm comm = mui::mpi_split_by_app( argc, argv );
    amrex::Initialize(argc,argv,true,comm);
    //amrex::Initialize(argc,argv);

    // argv[1] contains the name of the inputs file entered at the command line
    main_driver(argv[1]);

    amrex::Finalize();

    return 0;
}

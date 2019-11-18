#include <AMReX.H>
#include <iostream>

// function declaration
void main_driver (const char* argv);

using namespace std;

int main (int argc, char* argv[])
{
    cout << "Hello World from compressible_mui\n";

    amrex::Initialize(argc,argv);

    // argv[1] contains the name of the inputs file entered at the command line
    main_driver(argv[1]);

    amrex::Finalize();

    return 0;
}

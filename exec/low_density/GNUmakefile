# AMREX_HOME defines the directory in which we will find all the AMReX code.
# If you set AMREX_HOME as an environment variable, this line will be ignored
AMREX_HOME ?= ../../../amrex/
FBOXLIB_HOME ?= ../../../FBoxLib/

DEBUG     = FALSE
USE_MPI   = TRUE
USE_OMP   = FALSE
COMP      = gnu
DIM       = 3
DSMC      = TRUE

USE_PARTICLES = FALSE

include $(AMREX_HOME)/Tools/GNUMake/Make.defs

include ./Make.package
VPATH_LOCATIONS   += .
INCLUDE_LOCATIONS += .


include ../../src_low-density/src_F90/Make.package
VPATH_LOCATIONS   += ../../src_low-density/src_F90
INCLUDE_LOCATIONS += ../../src_low-density/src_F90

include ../../src_low-density/Make.package
VPATH_LOCATIONS   += ../../src_low-density/
INCLUDE_LOCATIONS += ../../src_low-density/

include ../../src_analysis/src_F90/Make.package
VPATH_LOCATIONS   += ../../src_analysis/src_F90
INCLUDE_LOCATIONS += ../../src_analysis/src_F90

include ../../src_analysis/Make.package
VPATH_LOCATIONS   += ../../src_analysis
INCLUDE_LOCATIONS += ../../src_analysis

include ../../src_rng/src_F90/Make.package
VPATH_LOCATIONS   += ../../src_rng/src_F90
INCLUDE_LOCATIONS += ../../src_rng/src_F90

include ../../src_rng/Make.package
VPATH_LOCATIONS   += ../../src_rng/
INCLUDE_LOCATIONS += ../../src_rng/

include ../../src_common/src_F90/Make.package
VPATH_LOCATIONS   += ../../src_common/src_F90
INCLUDE_LOCATIONS += ../../src_common/src_F90

include ../../src_common/Make.package
VPATH_LOCATIONS   += ../../src_common/
INCLUDE_LOCATIONS += ../../src_common/

include $(AMREX_HOME)/Src/Base/Make.package
include $(AMREX_HOME)/Src/Boundary/Make.package

 include $(AMREX_HOME)/Src/Extern/SWFFT/Make.package
 INCLUDE_LOCATIONS	+= $(AMREX_HOME)/Src/Extern/SWFFT
 VPATH_LOCATIONS	+= $(AMREX_HOME)/Src/Extern/SWFFT

 #LIBRARIES += -L$(FFTW_DIR) -lfftw3_mpi -lfftw3_omp -lfftw3
 LIBRARIES += -L$(FFTW_DIR) -lfftw3_mpi -lfftw3

include $(AMREX_HOME)/Tools/GNUMake/Make.rules

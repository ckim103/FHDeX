module membrane_forces_module
  use iso_c_binding
  use amrex_fort_module, only: amrex_real
  use amrex_string_module, only: amrex_string_c_to_f, amrex_string_f_to_c
  use common_namelist_module

  public
   
contains

! For Daniel and Andy:
! 4) Can we get time AND time step as an input argument of this routine also please? This should be based on the temporal integrator used, i.e., at which position in time the particle positions are.
! 5) I have added routines user_force_calc_init and user_force_calc_destroy here, which are to be called to initialize the user force code (e.g., read namelist with parameters for membrane) and to open files and then close them. We can adjust the interfaces as needed.
! 6) When the particle positions are output from inside the C++ code, are they sorted in original order? It is only in binary in paraview

subroutine user_force_calc_init(inputs_file,length) bind(c,name="user_force_calc_init")
   ! Read namelists, open files, etc.
   integer(c_int), value                 :: length                   !Note this is changed to pass by value, for consistency with rest of FHDeX
   character(kind=c_char), intent(in   ) :: inputs_file(length)
  
   print *, "Stub! Do not use"

end subroutine user_force_calc_init

subroutine user_force_calc(spec3xPos, spec3yPos, spec3zPos, spec3xForce, spec3yForce, spec3zForce, length, step, particleInfo) bind(c,name="user_force_calc")
  ! This routine should increment the forces supplied here
  ! CHANGE: Make length an array of length nspecies -- particleInfo is an array of length nspecies of type 'species_t', found in /src_common/src_F90/species_type.F90. This type stores a variety of info about each species, including the total number of particles - see example below.
  ! CHANGE: Add time and time_step as argument  -- fixed_dt in common namespace is time step, step number added
  
  implicit none

  integer(c_int),          intent(in   )         :: length, step
  real(c_double),          intent(in   )         :: spec3xPos(length), spec3yPos(length), spec3zPos(length)
  real(c_double),          intent(out  )         :: spec3xForce(length), spec3yForce(length), spec3zForce(length)
  real(c_double),         intent(in   )         :: particleInfo(nspecies)

  integer :: i
  real(c_double) :: time

   print *, "Stub! Do not use"
  
end subroutine user_force_calc

subroutine user_force_calc_destroy() bind(c,name="user_force_calc_destroy")
   ! Free files etc
 
end subroutine user_force_calc_destroy

end module membrane_forces_module

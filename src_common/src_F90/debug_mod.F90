module debug_module

  implicit none

  private

contains

  subroutine print_mf(lo, hi, mf, m_lo, m_hi, nc_m) bind (C,name="print_mf")

    integer         , intent (in   ) :: lo(3), hi(3)
    integer         , intent (in   ) :: m_lo(3), m_hi(3), nc_m
    double precision, intent (inout) :: mf(m_lo(1):m_hi(1),m_lo(2):m_hi(2),m_lo(3):m_hi(3),nc_m)

    integer comp,i,j,k
    double precision test

    test = 0

    print*,"valid box",lo(1:AMREX_SPACEDIM),hi(1:AMREX_SPACEDIM)

    ! loop over the data
    do comp = 1, nc_m
    do k = m_lo(3),m_hi(3)
    do j = m_lo(2),m_hi(2)
    do i = m_lo(1),m_hi(1)
#if (AMREX_SPACEDIM == 1)
       print*,'i,comp',i,comp,mf(i,j,k,comp)
#elif (AMREX_SPACEDIM == 2)
       print*,'i,j,comp',i,j,comp,mf(i,j,k,comp)
#elif (AMREX_SPACEDIM == 3)
       print*,'i,j,k,comp',i,j,k,comp,mf(i,j,k,comp)
       test = test + mf(i,j,k,comp)
#endif
    enddo
    enddo
    enddo
    enddo

    call flush(6)
    print *, "TOTAL: ", test

  end subroutine print_mf

end module debug_module

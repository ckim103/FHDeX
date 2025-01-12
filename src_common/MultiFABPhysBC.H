#ifndef MULTIFABPHYSBC_H
#define MULTIFABPHYSBC_H

// GPU kernels

AMREX_GPU_HOST_DEVICE
inline void physbc_pres_fab(const Box & tbx,
                            const Box & dom,
                            const Array4<Real> & data,
                            const GpuArray<int, AMREX_SPACEDIM> & bc_lo,
                            const GpuArray<int, AMREX_SPACEDIM> & bc_hi,
                            int start_cmp, int n_cmp) {

    //___________________________________________________________________________
    // Total work region => the loops below will actually only iterate over
    // cells between tbx and dom
    const Dim3 tlo    = amrex::lbound(tbx);
    const Dim3 thi    = amrex::ubound(tbx);
    const Dim3 dom_lo = amrex::lbound(dom);
    const Dim3 dom_hi = amrex::ubound(dom);

    //___________________________________________________________________________
    // Apply x-physbc to data
    if (((bc_lo[0] == 1) || (bc_lo[0] == 2)) && (tlo.x <= dom_lo.x)) {
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i < dom_lo.x; ++i) {
            int offset = dom_lo.x - i;
            int i_real = dom_lo.x + offset - 1;
            data(i, j, k, n) = data(i_real, j, k, n);
        }
        }
        }
        }
    }

    if (((bc_hi[0] == 1) || (bc_hi[0] == 2)) && (thi.x >= dom_hi.x)){
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = dom_hi.x + 1; i <= thi.x; ++i) {
            int offset = i - dom_hi.x;
            int i_real = dom_hi.x - offset + 1;
            data(i, j, k, n) = data(i_real, j, k, n);
        }
        }
        }
        }
    }


    //___________________________________________________________________________
    // Apply y-physbc to data
#if (AMREX_SPACEDIM >= 2)
    if (((bc_lo[1] == 1) || (bc_lo[1] == 2)) && (tlo.y <= dom_lo.y)) {
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j < dom_lo.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = dom_lo.y - j;
            int j_real = dom_lo.y + offset - 1;
            data(i, j, k, n) = data(i, j_real, k, n);
        }
        }
        }
        }
    }

    if (((bc_hi[1] == 1) || (bc_hi[1] == 2)) && (thi.y >= dom_hi.y)) {
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = dom_hi.y + 1; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = j - dom_hi.y;
            int j_real = dom_hi.y - offset + 1;
            data(i, j, k, n) = data(i, j_real, k, n);
        }
        }
        }
        }
    }
#endif

    //___________________________________________________________________________
    // Apply z-physbc to data
#if (AMREX_SPACEDIM >= 3)
    if (((bc_lo[2] == 1) || (bc_lo[2] == 2)) && (tlo.z <= dom_lo.z)) {
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k < dom_lo.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = dom_lo.z - k;
            int k_real = dom_lo.z + offset - 1;
            data(i, j, k, n) = data(i, j, k_real, n);
        }
        }
        }
        }
    }

    if (((bc_hi[2] == 1) || (bc_hi[2] == 2)) && (thi.z >= dom_hi.z)) {
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = dom_hi.z + 1; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = k - dom_hi.z;
            int k_real = dom_hi.z - offset + 1;
            data(i, j, k, n) = data(i, j, k_real, n);
        }
        }
        }
        }
    }
#endif
}

AMREX_GPU_HOST_DEVICE
inline void physbc_domainvel_fab(const Box & tbx,
                                 const Box & dom,
                                 const Array4<Real> & data,
                                 const GpuArray<int, AMREX_SPACEDIM> & bc_lo,
                                 const GpuArray<int, AMREX_SPACEDIM> & bc_hi,
                                 int start_cmp, int n_cmp, int dim) {

    //___________________________________________________________________________
    // Total work region => the loops below will actually only iterate over
    // cells between tbx and dom
    const Dim3 tlo    = amrex::lbound(tbx);
    const Dim3 thi    = amrex::ubound(tbx);
    const Dim3 dom_lo = amrex::lbound(dom);
    const Dim3 dom_hi = amrex::ubound(dom);
    // Compute valid parts only:
    const Dim3 vlo = amrex::elemwiseMax(tlo, dom_lo);
    const Dim3 vhi = amrex::elemwiseMin(thi, dom_hi);

    //___________________________________________________________________________
    // Apply x-physbc to data
    if ((dim == 0) && (bc_lo[0] == 1 || bc_lo[0] == 2) && (tlo.x <= dom_lo.x)) {

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = vlo.z; k <= vhi.z; ++k) {
        AMREX_PRAGMA_SIMD
        for (int j = vlo.y; j <= vhi.y; ++j) {
            data(dom_lo.x, j, k, n) = 0;
        }
        }
        }
        
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i < dom_lo.x; ++i) {
            int offset = dom_lo.x - i;
            int i_real = dom_lo.x + offset;
            data(i, j, k, n) = - data(i_real, j, k, n);
        }
        }
        }
        }
    }

    if ((dim == 0) && (bc_hi[0] == 1 || bc_hi[0] == 2) && (thi.x >= dom_hi.x)) {

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = vlo.z; k <= vhi.z; ++k) {
        AMREX_PRAGMA_SIMD
        for (int j = vlo.y; j <= vhi.y; ++j) {
            data(dom_hi.x, j, k, n) = 0;
        }
        }
        }

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = dom_hi.x + 1; i <= thi.x; ++i) {
            int offset = i - dom_hi.x;
            int i_real = dom_hi.x - offset;
            data(i, j, k, n) = - data(i_real, j, k, n);
        }
        }
        }
        }
    }


    //___________________________________________________________________________
    // Apply y-physbc to data
#if (AMREX_SPACEDIM >= 2)
    if ((dim == 1) && (bc_lo[1] == 1 || bc_lo[1] == 2) && (tlo.y <= dom_lo.y)){

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = vlo.z; k <= vhi.z; ++k) {
        AMREX_PRAGMA_SIMD
        for (int i = vlo.x; i <= vhi.x; ++i) {
            data(i, dom_lo.y, k, n) = 0;
        }
        }
        }

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j < dom_lo.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = dom_lo.y - j;
            int j_real = dom_lo.y + offset;
            data(i, j, k, n) = - data(i, j_real, k, n);
        }
        }
        }
        }
    }

    if ((dim == 1) && (bc_hi[1] == 1 || bc_hi[1] == 2) && (thi.y >= dom_hi.y)) {

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = vlo.z; k <= vhi.z; ++k) {
        AMREX_PRAGMA_SIMD
        for (int i = vlo.x; i <= vhi.x; ++i) {
            data(i, dom_hi.y, k, n) = 0;
        }
        }
        }

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = dom_hi.y + 1; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = j - dom_hi.y;
            int j_real = dom_hi.y - offset;
            data(i, j, k, n) = - data(i, j_real, k, n);
        }
        }
        }
        }
    }
#endif

    //___________________________________________________________________________
    // Apply z-physbc to data
#if (AMREX_SPACEDIM >= 3)
    if ((dim == 2) && (bc_lo[2] == 1 || bc_lo[2] == 2) && (tlo.z <= dom_lo.z)) {

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int j = vlo.y; j <= vhi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = vlo.x; i <= vhi.x; ++i) {
            data(i, j, dom_lo.z, n) = 0;
        }
        }
        }

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k < dom_lo.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = dom_lo.z - k;
            int k_real = dom_lo.z + offset;
            data(i, j, k, n) = - data(i, j, k_real, n);
        }
        }
        }
        }
    }

    if ((dim == 2) && (bc_hi[2] == 1 || bc_hi[2] == 2) && (thi.z >= dom_hi.z) ) {

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int j = vlo.y; j <= vhi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = vlo.x; i <= vhi.x; ++i) {
            data(i, j, dom_hi.z, n) = 0;
        }
        }
        }

        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = dom_hi.z + 1; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = k - dom_hi.z;
            int k_real = dom_hi.z - offset;
            data(i, j, k, n) = - data(i, j, k_real, n);
        }
        }
        }
        }
    }
#endif
}

AMREX_GPU_HOST_DEVICE
inline void physbc_macvel_fab(const Box & tbx,
                              const Box & dom,
                              const Array4<Real> & data,
                              const GpuArray<int, AMREX_SPACEDIM> & bc_lo,
                              const GpuArray<int, AMREX_SPACEDIM> & bc_hi,
                              int start_cmp, int n_cmp, int dim) {

    //___________________________________________________________________________
    // Total work region => the loops below will actually only iterate over
    // cells between tbx and dom
    const Dim3 tlo    = amrex::lbound(tbx);
    const Dim3 thi    = amrex::ubound(tbx);
    const Dim3 dom_lo = amrex::lbound(dom);
    const Dim3 dom_hi = amrex::ubound(dom);


    //___________________________________________________________________________
    // Apply x-physbc to data
    if ((dim != 0) && (bc_lo[0] == 1 || bc_lo[0] == 2) && (tlo.x <= dom_lo.x)) {
        Real fac = (bc_lo[0] == 1) ? 1. : -1.;
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i < dom_lo.x; ++i) {
            int offset = dom_lo.x - i;
            int i_real = dom_lo.x + offset - 1;
            data(i, j, k, n) = fac*data(i_real, j, k, n);
        }
        }
        }
        }
    }

    if ((dim != 0) && (bc_lo[0] == 1 || bc_hi[0] == 2) && (thi.x >= dom_hi.x)) {
        Real fac = (bc_hi[0] == 1) ? 1. : -1.;
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = dom_hi.x + 1; i <= thi.x; ++i) {
            int offset = i - dom_hi.x;
            int i_real = dom_hi.x - offset + 1;
            data(i, j, k, n) = fac*data(i_real, j, k, n);
        }
        }
        }
        }
    }


    //___________________________________________________________________________
    // Apply y-physbc to data
#if (AMREX_SPACEDIM >= 2)
    if ((dim != 1) && (bc_lo[1] == 1 || bc_lo[1] == 2) && (tlo.y <= dom_lo.y)) {
        Real fac = (bc_lo[1] == 1) ? 1. : -1.;
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = tlo.y; j < dom_lo.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = dom_lo.y - j;
            int j_real = dom_lo.y + offset - 1;
            data(i, j, k, n) = fac*data(i, j_real, k, n);
        }
        }
        }
        }
    }

    if ((dim != 1) && (bc_hi[1] == 1 || bc_hi[1] == 2) && (thi.y >= dom_hi.y)) {
        Real fac = (bc_hi[1] == 1) ? 1. : -1.;
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k <= thi.z; ++k) {
        for (int j = dom_hi.y + 1; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = j - dom_hi.y;
            int j_real = dom_hi.y - offset + 1;
            data(i, j, k, n) = fac*data(i, j_real, k, n);
        }
        }
        }
        }
    }
#endif

    //___________________________________________________________________________
    // Apply z-physbc to data
#if (AMREX_SPACEDIM >= 3)
    if ((dim != 2) && (bc_lo[2] == 1 || bc_lo[2] == 2) && (tlo.z <= dom_lo.z)) {
        Real fac = (bc_lo[2] == 1) ? 1. : -1.;
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = tlo.z; k < dom_lo.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = dom_lo.z - k;
            int k_real = dom_lo.z + offset - 1;
            data(i, j, k, n) = fac*data(i, j, k_real, n);
        }
        }
        }
        }
    }

    if ((dim != 2) && (bc_hi[2] == 1 || bc_hi[2] == 2) && (thi.z >= dom_hi.z)) {
        Real fac = (bc_hi[2] == 1) ? 1. : -1.;
        for (int n = start_cmp; n < start_cmp + n_cmp; ++n) {
        for (int k = dom_hi.z + 1; k <= thi.z; ++k) {
        for (int j = tlo.y; j <= thi.y; ++j) {
        AMREX_PRAGMA_SIMD
        for (int i = tlo.x; i <= thi.x; ++i) {
            int offset = k - dom_hi.z;
            int k_real = dom_hi.z - offset + 1;
            data(i, j, k, n) = fac*data(i, j, k_real, n);
        }
        }
        }
        }
    }
#endif
}

#endif

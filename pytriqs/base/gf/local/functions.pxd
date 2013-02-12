###############  Density  #########################

cdef extern from "triqs/gf/local/functions.hpp":
    matrix_view density(gf_imfreq &)
    matrix_view density(gf_legendre &)
    void enforce_discontinuity(gf_legendre &, array_view[double,TWO,COrder])

###############  Fourier  #########################

cdef extern from "triqs/gf/local/fourier_matsubara.hpp" : 
    gf_imfreq lazy_fourier          (gf_imtime & )
    gf_imtime lazy_inverse_fourier  (gf_imfreq & )

###############  Legendre  #########################

cdef extern from "triqs/gf/local/legendre_matsubara.hpp" : 
    gf_imfreq lazy_legendre_imfreq    (gf_legendre &)
    gf_imtime lazy_legendre_imtime    (gf_legendre &)
    gf_legendre lazy_imfreq_legendre  (gf_imfreq &)
    gf_legendre lazy_imtime_legendre  (gf_imtime &)

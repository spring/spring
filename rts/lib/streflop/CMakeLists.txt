# Spring supplied CMake build file

SET(libm_flt32_source
	"libm/flt-32/e_acosf.cpp"
	"libm/flt-32/e_acoshf.cpp"
	"libm/flt-32/e_asinf.cpp"
	"libm/flt-32/e_atan2f.cpp"
	"libm/flt-32/e_atanhf.cpp"
	"libm/flt-32/e_coshf.cpp"
	"libm/flt-32/e_exp2f.cpp"
	"libm/flt-32/e_expf.cpp"
	"libm/flt-32/e_fmodf.cpp"
	"libm/flt-32/e_gammaf_r.cpp"
	"libm/flt-32/e_hypotf.cpp"
	"libm/flt-32/e_j0f.cpp"
	"libm/flt-32/e_j1f.cpp"
	"libm/flt-32/e_jnf.cpp"
	"libm/flt-32/e_lgammaf_r.cpp"
	"libm/flt-32/e_log10f.cpp"
	"libm/flt-32/e_log2f.cpp"
	"libm/flt-32/e_logf.cpp"
	"libm/flt-32/e_powf.cpp"
	"libm/flt-32/e_rem_pio2f.cpp"
	"libm/flt-32/e_remainderf.cpp"
	"libm/flt-32/e_sinhf.cpp"
	"libm/flt-32/e_sqrtf.cpp"
	"libm/flt-32/k_cosf.cpp"
	"libm/flt-32/k_rem_pio2f.cpp"
	"libm/flt-32/k_sinf.cpp"
	"libm/flt-32/k_tanf.cpp"
	"libm/flt-32/s_asinhf.cpp"
	"libm/flt-32/s_atanf.cpp"
	"libm/flt-32/s_cbrtf.cpp"
	"libm/flt-32/s_ceilf.cpp"
	"libm/flt-32/s_copysignf.cpp"
	"libm/flt-32/s_cosf.cpp"
	"libm/flt-32/s_erff.cpp"
	"libm/flt-32/s_expm1f.cpp"
	"libm/flt-32/s_fabsf.cpp"
	"libm/flt-32/s_finitef.cpp"
	"libm/flt-32/s_floorf.cpp"
	"libm/flt-32/s_fpclassifyf.cpp"
	"libm/flt-32/s_frexpf.cpp"
	"libm/flt-32/s_ilogbf.cpp"
	"libm/flt-32/s_isinff.cpp"
	"libm/flt-32/s_isnanf.cpp"
	"libm/flt-32/s_ldexpf.cpp"
	"libm/flt-32/s_llrintf.cpp"
	"libm/flt-32/s_llroundf.cpp"
	"libm/flt-32/s_log1pf.cpp"
	"libm/flt-32/s_logbf.cpp"
	"libm/flt-32/s_lrintf.cpp"
	"libm/flt-32/s_lroundf.cpp"
	"libm/flt-32/s_modff.cpp"
	"libm/flt-32/s_nearbyintf.cpp"
	"libm/flt-32/s_nextafterf.cpp"
	"libm/flt-32/s_remquof.cpp"
	"libm/flt-32/s_rintf.cpp"
	"libm/flt-32/s_roundf.cpp"
	"libm/flt-32/s_scalblnf.cpp"
	"libm/flt-32/s_scalbnf.cpp"
	"libm/flt-32/s_signbitf.cpp"
	"libm/flt-32/s_sincosf.cpp"
	"libm/flt-32/s_sinf.cpp"
	"libm/flt-32/s_tanf.cpp"
	"libm/flt-32/s_tanhf.cpp"
	"libm/flt-32/s_truncf.cpp"
	"libm/flt-32/w_expf.cpp"
	)

SET(cxxflags "-I\"${CMAKE_CURRENT_SOURCE_DIR}/libm/headers\"")
if   (NOT MSVC)
	SET(cxxflags "${cxxflags} -w -O3")
	if (NOT $ENV{CXX} MATCHES "icpc")
		SET(cxxflags "${cxxflags} -mfpmath=sse -msse")
	endif (NOT $ENV{CXX} MATCHES "icpc")
endif(NOT MSVC)
SET_SOURCE_FILES_PROPERTIES(${libm_flt32_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_FLT32 ${cxxflags}")

ADD_LIBRARY(streflop STATIC EXCLUDE_FROM_ALL
	SMath.cpp
	Random.cpp
	streflopC.cpp
	${libm_flt32_source}
	)
set_target_properties(streflop PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
#TODO do not use -fPIC for streflop (decreases performance)

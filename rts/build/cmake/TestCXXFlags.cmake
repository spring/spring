# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Test whether the C++ compiler supports certain flags.
# Once done, this will define the following vars.
# They will be empty if the flag is not supported,
# or contain the flag if it is supported.
#
# VISIBILITY_HIDDEN            -fvisibility=hidden
# VISIBILITY_INLINES_HIDDEN    -fvisibility-inlines-hidden
# SSE_FLAGS                    -msse -mfpmath=sse
# IEEE_FP_FLAG                 -fvisibility-inlines-hidden
# LTO_FLAGS                    -flto -fwhopr
#
# Note: gcc for windows supports these flags, but gives lots of errors when
#       compiling, so use them only for linux builds.

Include(TestCXXAcceptsFlag)


If    (NOT DEFINED VISIBILITY_HIDDEN)
	Set(VISIBILITY_HIDDEN "")
	If    (NOT MINGW AND NOT APPLE)
		CHECK_CXX_ACCEPTS_FLAG(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
		If    (HAS_VISIBILITY_HIDDEN)
			Set(VISIBILITY_HIDDEN "-fvisibility=hidden")
		EndIf (HAS_VISIBILITY_HIDDEN)
	EndIf (NOT MINGW AND NOT APPLE)
EndIf (NOT DEFINED VISIBILITY_HIDDEN)


If    (NOT DEFINED VISIBILITY_INLINES_HIDDEN)
	Set(VISIBILITY_INLINES_HIDDEN "")
	If    (NOT MINGW)
		CHECK_CXX_ACCEPTS_FLAG(-fvisibility-inlines-hidden HAS_VISIBILITY_INLINES_HIDDEN)
		If    (HAS_VISIBILITY_INLINES_HIDDEN)
			Set(VISIBILITY_INLINES_HIDDEN "-fvisibility-inlines-hidden")
		EndIf (HAS_VISIBILITY_INLINES_HIDDEN)
	EndIf (NOT MINGW)
EndIf (NOT DEFINED VISIBILITY_INLINES_HIDDEN)


If    (NOT DEFINED SSE_FLAGS)
	CHECK_CXX_ACCEPTS_FLAG("-msse -mfpmath=sse" HAS_SSE_FLAGS)
	CHECK_CXX_ACCEPTS_FLAG("-mno-avx" HAS_AVX_FLAGS)
	CHECK_CXX_ACCEPTS_FLAG("-mno-avx2" HAS_AVX2_FLAGS)
	If    (HAS_SSE_FLAGS)
		# activate SSE1 only
		Set(SSE_FLAGS "-msse -mfpmath=sse")
		#Set(SSE_FLAGS "${SSE_FLAGS} -mmmx")

		# disable rest
		#Set(SSE_FLAGS "${SSE_FLAGS} -mno-3dnow") tests showed it might sync
		Set(SSE_FLAGS "${SSE_FLAGS} -mno-sse2 -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2 -mno-sse4 -mno-sse4a")
		If    (HAS_AVX_FLAGS)
			Set(SSE_FLAGS "${SSE_FLAGS} -mno-avx -mno-fma -mno-fma4 -mno-xop -mno-lwp")
		EndIf (HAS_AVX_FLAGS)
		If    (HAS_AVX2_FLAGS)
			Set(SSE_FLAGS "${SSE_FLAGS} -mno-avx2")
		EndIf (HAS_AVX2_FLAGS)
	Else  (HAS_SSE_FLAGS)
		Set(SSE_FLAGS "-DDEDICATED_NOSSE")
		Message(WARNING "SSE1 support is missing, online play is highly discouraged with this build")
	EndIf (HAS_SSE_FLAGS)
EndIf (NOT DEFINED SSE_FLAGS)


If    (NOT DEFINED IEEE_FP_FLAG)
	CHECK_CXX_ACCEPTS_FLAG("-mieee-fp" HAS_IEEE_FP_FLAG)
	If    (HAS_IEEE_FP_FLAG)
		Set(IEEE_FP_FLAG "-mieee-fp")
	Else  (HAS_IEEE_FP_FLAG)
		Message(WARNING "IEEE-FP support is missing, online play is highly discouraged with this build")
		Set(IEEE_FP_FLAG "")
	EndIf (HAS_IEEE_FP_FLAG)
EndIf (NOT DEFINED IEEE_FP_FLAG)


If    (NOT DEFINED LTO_FLAGS)
	Set(LTO_FLAGS "")

	Set(LTO       FALSE CACHE BOOL "Link Time Optimizations (LTO)")
	If    (LTO)
		CHECK_CXX_ACCEPTS_FLAG("-flto" HAS_LTO_FLAG)
		If    (HAS_LTO_FLAG)
			Set(LTO_FLAGS "${LTO_FLAGS} -flto")
		Else  (HAS_LTO_FLAG)
			Set(LTO_FLAGS "${LTO_FLAGS} -flto")
		EndIf (HAS_LTO_FLAG)
	EndIf (LTO)

	Set(LTO_WHOPR FALSE CACHE BOOL "Link Time Optimizations (LTO) - Whole program optimizer (WHOPR)")
	If    (LTO_WHOPR)
		CHECK_CXX_ACCEPTS_FLAG("-fwhopr" HAS_LTO_WHOPR_FLAG)
		If    (HAS_LTO_WHOPR_FLAG)
			Set(LTO_FLAGS "${LTO_FLAGS} -fwhopr")
		EndIf (HAS_LTO_WHOPR_FLAG)
	EndIf (LTO_WHOPR)
	
	If (LTO AND LTO_WHOPR)
		Message( FATAL_ERROR "LTO and LTO_WHOPR are mutually exclusive, please enable only one at a time." )
	EndIf (LTO AND LTO_WHOPR)
EndIf (NOT DEFINED LTO_FLAGS)


IF    (NOT DEFINED MARCH)
	Set(MARCH "")

	# 32bit
	CHECK_CXX_ACCEPTS_FLAG("-march=i686" HAS_I686_FLAG_)
	IF    (HAS_I686_FLAG_)
		Set(MARCH "i686")
	EndIf (HAS_I686_FLAG_)

	# 64bit
	if    ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (NOT MARCH))
		# always syncs with 32bit
		CHECK_CXX_ACCEPTS_FLAG("-march=x86_64" HAS_X86_64_FLAG_)
		IF    (HAS_X86_64_FLAG_)
			Set(MARCH "x86_64")
		EndIf (HAS_X86_64_FLAG_)

		if    (NOT MARCH)
			# _should_ sync with 32bit
			CHECK_CXX_ACCEPTS_FLAG("-march=k8" HAS_K8_FLAG_)
			IF    (HAS_K8_FLAG_)
				Set(MARCH "k8")
			EndIf (HAS_K8_FLAG_)
		endif (NOT MARCH)
	endif ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (NOT MARCH))

	# no compatible arch found
	if    (NOT MARCH)
		Message(WARNING "Neither i686, x86_64 nor k8 are accepted by the compiler! (`march=native` _may_ cause sync errors!)")
	endif (NOT MARCH)
EndIf (NOT DEFINED MARCH)

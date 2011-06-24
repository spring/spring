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
	If    (NOT MINGW)
		CHECK_CXX_ACCEPTS_FLAG(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
		If    (HAS_VISIBILITY_HIDDEN)
			Set(VISIBILITY_HIDDEN "-fvisibility=hidden")
		EndIf (HAS_VISIBILITY_HIDDEN)
	EndIf (NOT MINGW)
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
	If    (HAS_SSE_FLAGS)
		Set(SSE_FLAGS "-msse -mfpmath=sse")
	Else  (HAS_SSE_FLAGS)
		Set(SSE_FLAGS "")
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
EndIf (NOT DEFINED LTO_FLAGS)


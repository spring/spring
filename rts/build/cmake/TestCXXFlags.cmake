# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Test whether the C++ compiler supports "-fvisibility=hidden"
# Once done this will define
#
# VISIBILITY_HIDDEN - -fvisibility=hidden   if supported, an empty string otherwise.
# VISIBILITY_INLINES_HIDDEN - -fvisibility-inlines-hidden   likewise
#
# Note: gcc for windows supports these flags, but give lots of errors when compiling, so use only for linux builds

INCLUDE(TestCXXAcceptsFlag)

IF(NOT DEFINED VISIBILITY_HIDDEN)
	CHECK_CXX_ACCEPTS_FLAG(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
	IF(HAS_VISIBILITY_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_HIDDEN "-fvisibility=hidden")
	ELSE(HAS_VISIBILITY_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_HIDDEN "")
	ENDIF(HAS_VISIBILITY_HIDDEN AND NOT MINGW)
ENDIF(NOT DEFINED VISIBILITY_HIDDEN)
	
IF(NOT DEFINED VISIBILITY_INLINES_HIDDEN)
	CHECK_CXX_ACCEPTS_FLAG(-fvisibility-inlines-hidden HAS_VISIBILITY_INLINES_HIDDEN)
	IF(HAS_VISIBILITY_INLINES_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_INLINES_HIDDEN "-fvisibility-inlines-hidden")
	ELSE(HAS_VISIBILITY_INLINES_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_INLINES_HIDDEN "")
	ENDIF(HAS_VISIBILITY_INLINES_HIDDEN AND NOT MINGW)
ENDIF(NOT DEFINED VISIBILITY_INLINES_HIDDEN)

IF(NOT DEFINED SSE_FLAGS)
	CHECK_CXX_ACCEPTS_FLAG("-msse -mfpmath=sse" HAS_SSE_FLAGS)
	IF(HAS_SSE_FLAGS)
		SET(SSE_FLAGS "-msse -mfpmath=sse")
	ELSE(HAS_SSE_FLAGS)
		SET(SSE_FLAGS "")
		message(WARNING "SSE support missing, online play is highly discouraged with this build")
	ENDIF(HAS_SSE_FLAGS)
ENDIF(NOT DEFINED SSE_FLAGS)

IF(NOT DEFINED IEEE_FP_FLAG)
	CHECK_CXX_ACCEPTS_FLAG("-mieee-fp" HAS_IEEE_FP_FLAG)
	IF(HAS_IEEE_FP_FLAG)
		SET(IEEE_FP_FLAG "-mieee-fp")
	ELSE(HAS_IEEE_FP_FLAG)
		message(WARNING "IEEE-FP support missing, online play is highly discouraged with this build")
		SET(IEEE_FP_FLAG "")
	ENDIF(HAS_IEEE_FP_FLAG)
ENDIF(NOT DEFINED IEEE_FP_FLAG)

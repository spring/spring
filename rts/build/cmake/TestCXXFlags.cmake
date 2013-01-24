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

# Helper
Macro    (CHECK_AND_ADD_FLAGS dest)
	FOREACH    (flag ${ARGN})
		CHECK_CXX_ACCEPTS_FLAG("${flag}" has_${flag})
		if    (has_${flag})
			Set(${dest} "${${dest}} ${flag}")
		else (has_${flag})
			Message("compiler doesn't support: ${flag}")
		endif (has_${flag})
	ENDFOREACH (flag ${ARGN})
EndMacro (CHECK_AND_ADD_FLAGS)



If    (NOT DEFINED VISIBILITY_HIDDEN)
	Set(VISIBILITY_HIDDEN "")
	If    (NOT WIN32 AND NOT APPLE)
		CHECK_AND_ADD_FLAGS(VISIBILITY_HIDDEN -fvisibility=hidden)
	EndIf (NOT WIN32 AND NOT APPLE)
EndIf (NOT DEFINED VISIBILITY_HIDDEN)


If    (NOT DEFINED VISIBILITY_INLINES_HIDDEN)
	Set(VISIBILITY_INLINES_HIDDEN "")
	If    (NOT WIN32)
		CHECK_AND_ADD_FLAGS(VISIBILITY_INLINES_HIDDEN -fvisibility-inlines-hidden)
	EndIf (NOT WIN32)
EndIf (NOT DEFINED VISIBILITY_INLINES_HIDDEN)


If    (NOT DEFINED SSE_FLAGS)
	If   (MSVC)
		#Set(SSE_FLAGS "/arch:SSE2")
	Else (MSVC)
		CHECK_CXX_ACCEPTS_FLAG("-msse -mfpmath=sse" HAS_SSE_FLAGS)
		If    (HAS_SSE_FLAGS)
			# activate SSE1 only
			Set(SSE_FLAGS "-msse -mfpmath=sse")
			#Set(SSE_FLAGS "${SSE_FLAGS} -mmmx")

			# worth to test if sync
			#Set(SSE_FLAGS "${SSE_FLAGS} -mpopcnt -mlzcnt -mabm")

			# disable rest
			#Set(SSE_FLAGS "${SSE_FLAGS} -mno-3dnow") tests showed it might sync
			CHECK_AND_ADD_FLAGS(SSE_FLAGS -mno-sse2 -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2 -mno-sse4 -mno-sse4a)
			CHECK_AND_ADD_FLAGS(SSE_FLAGS -mno-avx -mno-fma -mno-fma4 -mno-xop -mno-lwp)
			CHECK_AND_ADD_FLAGS(SSE_FLAGS -mno-avx2)
		Else  (HAS_SSE_FLAGS)
			Set(SSE_FLAGS "-DDEDICATED_NOSSE")
			Message(WARNING "SSE1 support is missing, online play is highly discouraged with this build")
		EndIf (HAS_SSE_FLAGS)
	Endif (MSVC)
EndIf (NOT DEFINED SSE_FLAGS)


If    (NOT DEFINED IEEE_FP_FLAG)
	If   (MSVC)
		Set(IEEE_FP_FLAG "/fp:strict")
	Else (MSVC)
	CHECK_CXX_ACCEPTS_FLAG("-mieee-fp" HAS_IEEE_FP_FLAG)
	If    (HAS_IEEE_FP_FLAG)
		Set(IEEE_FP_FLAG "-mieee-fp")
	Else  (HAS_IEEE_FP_FLAG)
		Message(WARNING "IEEE-FP support is missing, online play is highly discouraged with this build")
		Set(IEEE_FP_FLAG "")
	EndIf (HAS_IEEE_FP_FLAG)
	Endif(MSVC)
EndIf (NOT DEFINED IEEE_FP_FLAG)


If    (NOT MSVC AND NOT DEFINED LTO_FLAGS)
	Set(LTO_FLAGS "")

	Set(LTO       FALSE CACHE BOOL "Link Time Optimizations (LTO)")
	If    (LTO)
		CHECK_AND_ADD_FLAGS(LTO_FLAGS -flto)
		if    (NOT LTO_FLAGS)
			Message(WARNING "Tried to enable LTO, but compiler doesn't support it!")
		endif (NOT LTO_FLAGS)
	EndIf (LTO)
EndIf (NOT MSVC AND NOT DEFINED LTO_FLAGS)


IF    (NOT MSVC AND NOT DEFINED MARCH)
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
	endif ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (NOT MARCH))
EndIf (NOT MSVC AND NOT DEFINED MARCH)

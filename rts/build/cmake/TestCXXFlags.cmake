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

include(TestCXXAcceptsFlag)

# Helper
macro    (CHECK_AND_ADD_FLAGS dest)
	foreach    (flag ${ARGN})
		check_cxx_accepts_flag("${flag}" has_${flag})
		if    (has_${flag})
			set(${dest} "${${dest}} ${flag}")
		else (has_${flag})
			message("compiler doesn't support: ${flag}")
		endif (has_${flag})
	endforeach (flag ${ARGN})
endmacro (CHECK_AND_ADD_FLAGS)



if    (NOT DEFINED VISIBILITY_HIDDEN)
	set(VISIBILITY_HIDDEN "")
	if    (NOT WIN32 AND NOT APPLE)
		check_and_add_flags(VISIBILITY_HIDDEN -fvisibility=hidden)
	endif (NOT WIN32 AND NOT APPLE)
endif (NOT DEFINED VISIBILITY_HIDDEN)


if    (NOT DEFINED VISIBILITY_INLINES_HIDDEN)
	set(VISIBILITY_INLINES_HIDDEN "")
	if    (NOT WIN32)
		check_and_add_flags(VISIBILITY_INLINES_HIDDEN -fvisibility-inlines-hidden)
	endif (NOT WIN32)
endif (NOT DEFINED VISIBILITY_INLINES_HIDDEN)


if    (NOT DEFINED SSE_FLAGS)
	if   (MSVC)
		#Set(SSE_FLAGS "/arch:SSE2")
	else (MSVC)
		check_cxx_accepts_flag("-msse -mfpmath=sse" HAS_SSE_FLAGS)
		if    (HAS_SSE_FLAGS)
			# activate SSE1 only
			set(SSE_FLAGS "-msse -mfpmath=sse")
			#Set(SSE_FLAGS "${SSE_FLAGS} -mmmx")

			# worth to test if sync
			#Set(SSE_FLAGS "${SSE_FLAGS} -mpopcnt -mlzcnt -mabm")

			# disable rest
			#Set(SSE_FLAGS "${SSE_FLAGS} -mno-3dnow") tests showed it might sync
			check_and_add_flags(SSE_FLAGS -mno-sse2 -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2 -mno-sse4 -mno-sse4a)
			check_and_add_flags(SSE_FLAGS -mno-avx -mno-fma -mno-fma4 -mno-xop -mno-lwp)
			check_and_add_flags(SSE_FLAGS -mno-avx2)
		else  (HAS_SSE_FLAGS)
			set(SSE_FLAGS "-DDEDICATED_NOSSE")
			message(WARNING "SSE1 support is missing, online play is highly discouraged with this build")
		endif (HAS_SSE_FLAGS)
	endif (MSVC)
endif (NOT DEFINED SSE_FLAGS)


If    (NOT DEFINED IEEE_FP_FLAG)
	If   (MSVC)
		Set(IEEE_FP_FLAG "/fp:strict")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		Message(WARNING "Clang detected, disabled IEEE-FP")
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


If    (NOT DEFINED CXX11_FLAGS)
	If   (MSVC)
		# Nothing needed
		Set(CXX11_FLAGS "")
	ElseIf ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		CHECK_AND_ADD_FLAGS(CXX11_FLAGS "-std=c++11")
	Else (MSVC)
		# note, we need gnu++11 instead of c++11, cause else we get compile errors
		# under mingw (pthread.h missing, sys/utsname.h missing, M_PI not defined and more)
		CHECK_AND_ADD_FLAGS(CXX11_FLAGS "-std=gnu++11")
		If    (NOT CXX11_FLAGS)
			CHECK_AND_ADD_FLAGS(CXX11_FLAGS "-std=c++11")
		EndIf (NOT CXX11_FLAGS)
		If    (NOT CXX11_FLAGS)
			# xcode
			CHECK_AND_ADD_FLAGS(CXX11_FLAGS "-std=gnu++0x")
		EndIf (NOT CXX11_FLAGS)
		If    (NOT CXX11_FLAGS)
			# xcode 2
			CHECK_AND_ADD_FLAGS(CXX11_FLAGS "-std=c++0x")
		EndIf (NOT CXX11_FLAGS)

		If    (NOT CXX11_FLAGS)
			Message(WARNING "C++11 support missing")
		EndIf (NOT CXX11_FLAGS)
	Endif(MSVC)
EndIf (NOT DEFINED CXX11_FLAGS)


If    (NOT MSVC AND NOT DEFINED LTO_FLAGS)
	Set(LTO_FLAGS "")
	CHECK_AND_ADD_FLAGS(LTO_FLAGS -flto)
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
		check_cxx_accepts_flag("-march=x86_64" HAS_X86_64_FLAG_)
		if    (HAS_X86_64_FLAG_)
			set(MARCH "x86_64")
		endif (HAS_X86_64_FLAG_)
	endif ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (NOT MARCH))
endif (NOT MSVC AND NOT DEFINED MARCH)

if   (NOT MSVC)
	if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		CHECK_CXX_ACCEPTS_FLAG("-mno-tls-direct-seg-refs" HAS_NO_TLS_DIRECT_SEG_REFS_FLAG)
		if (HAS_NO_TLS_DIRECT_SEG_REFS_FLAG)
			set(NO_TLS_DIRECT_SEG_REFS -mno-tls-direct-seg-refs)
		endif()
	endif()
endif()


if   (CMAKE_COMPILER_IS_GNUCXX)
	# check if default linker is ld.gold
	execute_process(COMMAND ${CMAKE_LINKER} "-v"
		OUTPUT_VARIABLE linkerVersion
		ERROR_VARIABLE linkerVersion
	)
	set(hasGold FALSE)
	if ("${linkerVersion}" MATCHES "gold")
		set(hasGold TRUE)
	endif()


	if    (NOT hasGold AND NOT WIN32) #FIND_PROGRAM fails in crosscompile environments (it detects the native ld.gold)
		# since gcc 4.8 it is possible to switch the linker via that argument
		check_cxx_accepts_flag("-fuse-ld=gold" HAS_USE_LD)
		if    (HAS_USE_LD)
			find_program(LD_GOLD ld.gold)
			if    (LD_GOLD)
				set(hasGold TRUE)
				set(LDGOLD_CXX_FLAGS "-fuse-ld=gold")
			endif ()
		endif ()
	endif ()

	if    (hasGold)
		set(LDGOLD_FOUND TRUE)
		set(LDGOLD_LINKER_FLAGS "")
		#set(LDGOLD_LINKER_FLAGS " -Wl,--stats ${LDGOLD_LINKER_FLAGS}")
		set(LDGOLD_LINKER_FLAGS " -Wl,-O3 ${LDGOLD_LINKER_FLAGS}")       # e.g. tries to optimize duplicated strings across the binary
		set(LDGOLD_LINKER_FLAGS " -Wl,--icf=all ${LDGOLD_LINKER_FLAGS}") # Identical Code Folding
	endif ()

	mark_as_advanced(LDGOLD_FOUND LDGOLD_LINKER_FLAGS LDGOLD_CXX_FLAGS)
endif()


if   (CMAKE_COMPILER_IS_GNUCXX)
	set(MPX_FLAGS "")
	check_and_add_flags(MPX_FLAGS -fcheck-pointer-bounds -mmpx -Wchkp)
endif()

# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Test whether the C++ compiler supports certain flags.
# Once done, this will define the following vars.
# They will be empty if the flag is not supported,
# or contain the flag if it is supported.
#
# SSE_FLAGS                    -msse -mfpmath=sse
# IEEE_FP_FLAG                 -fvisibility-inlines-hidden
# LTO_FLAGS                    -flto -fwhopr
#
# Note: gcc for windows supports these flags, but gives lots of errors when
#       compiling, so use them only for linux builds.

include(TestCXXAcceptsFlag)

# Helper
macro (CHECK_AND_ADD_FLAGS dest)
	foreach    (flag ${ARGN})
		check_cxx_accepts_flag("${flag}" has_${flag})
		if (has_${flag})
			set(${dest} "${${dest}} ${flag}")
		else ()
			message ("compiler doesn't support: ${flag}")
		endif ()
	endforeach ()
endmacro ()

if (NOT DEFINED SSE_FLAGS)
	if (MSVC)
		#Set(SSE_FLAGS "/arch:SSE2")
	else ()
		check_cxx_accepts_flag("-msse -mfpmath=sse" HAS_SSE_FLAGS)
		if (HAS_SSE_FLAGS)
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
		else ()
			set(SSE_FLAGS "-DDEDICATED_NOSSE")
			message (WARNING "SSE1 support is missing, online play is highly discouraged with this build")
		endif ()
	endif ()
endif ()


if (NOT DEFINED IEEE_FP_FLAG)
	if (MSVC)
		Set(IEEE_FP_FLAG "/fp:strict")
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		message (WARNING "Clang detected, disabled IEEE-FP")
	else ()
		CHECK_CXX_ACCEPTS_FLAG("-mieee-fp" HAS_IEEE_FP_FLAG)
		if (HAS_IEEE_FP_FLAG)
			Set(IEEE_FP_FLAG "-mieee-fp")
		else ()
			message (WARNING "IEEE-FP support is missing, online play is highly discouraged with this build")
			Set(IEEE_FP_FLAG "")
		endif ()
	endif ()
endif ()


if (NOT DEFINED CXX17_FLAGS)
	CHECK_AND_ADD_FLAGS(CXX17_FLAGS "-std=c++17")
endif ()


if (NOT MSVC AND NOT DEFINED LTO_FLAGS)
	Set(LTO_FLAGS "")
	CHECK_AND_ADD_FLAGS(LTO_FLAGS -flto)
endif ()



if (NOT MSVC AND NOT DEFINED MARCH)
	Set(MARCH "")

	# 32bit
	CHECK_CXX_ACCEPTS_FLAG("-march=i686" HAS_I686_FLAG_)
	if (HAS_I686_FLAG_)
		Set(MARCH "i686")
	endif ()

	# 64bit
	if ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (NOT MARCH))
		# always syncs with 32bit
		check_cxx_accepts_flag("-march=x86-64" HAS_X86_64_FLAG_)
		if (HAS_X86_64_FLAG_)
			set(MARCH "x86-64")
		endif ()
		# MCST lcc compiler accept -march=elbrus-v2/v3/v4/v5/v6 and -march=native
		check_cxx_accepts_flag("-march=elbrus-v2" HAS_E2K_FLAG_)
		if (HAS_E2K_FLAG_)
			set(MARCH "native")
		endif ()
	endif ()
endif ()

if (NOT MSVC)
	if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		CHECK_CXX_ACCEPTS_FLAG("-mno-tls-direct-seg-refs" HAS_NO_TLS_DIRECT_SEG_REFS_FLAG)
		if (HAS_NO_TLS_DIRECT_SEG_REFS_FLAG)
			set(NO_TLS_DIRECT_SEG_REFS -mno-tls-direct-seg-refs)
		endif ()
	endif ()
endif ()


if (CMAKE_COMPILER_IS_GNUCXX)
	# check if default linker is ld.gold
	execute_process(COMMAND ${CMAKE_LINKER} "-v"
		OUTPUT_VARIABLE linkerVersion
		ERROR_VARIABLE linkerVersion
	)
	set(hasGold FALSE)
	if ("${linkerVersion}" MATCHES "gold")
		set(hasGold TRUE)
	endif ()


	if (NOT hasGold AND NOT WIN32) #FIND_PROGRAM fails in crosscompile environments (it detects the native ld.gold)
		# since gcc 4.8 it is possible to switch the linker via that argument
		check_cxx_accepts_flag("-fuse-ld=gold" HAS_USE_LD)
		if (HAS_USE_LD)
			find_program(LD_GOLD ld.gold)
			if (LD_GOLD)
				set(hasGold TRUE)
				set(LDGOLD_CXX_FLAGS "-fuse-ld=gold")
			endif ()
		endif ()
	endif ()

	if (hasGold)
		set(LDGOLD_FOUND TRUE)
		set(LDGOLD_LINKER_FLAGS "")
		#set(LDGOLD_LINKER_FLAGS " -Wl,--stats ${LDGOLD_LINKER_FLAGS}")
		set(LDGOLD_LINKER_FLAGS " -Wl,-O3 ${LDGOLD_LINKER_FLAGS}")       # e.g. tries to optimize duplicated strings across the binary
		set(LDGOLD_LINKER_FLAGS " -Wl,--icf=all ${LDGOLD_LINKER_FLAGS}") # Identical Code Folding
	endif ()

	mark_as_advanced(LDGOLD_FOUND LDGOLD_LINKER_FLAGS LDGOLD_CXX_FLAGS)
endif ()


if (CMAKE_COMPILER_IS_GNUCXX)
	set(MPX_FLAGS "")
	check_and_add_flags(MPX_FLAGS -fcheck-pointer-bounds -mmpx -Wchkp)
endif ()


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts)

ADD_SUBDIRECTORY(lua)
ADD_SUBDIRECTORY(luasocket)
ADD_SUBDIRECTORY(7z)
ADD_SUBDIRECTORY(minizip)
ADD_SUBDIRECTORY(headlessStubs)
if (ENABLE_STREFLOP)
ADD_SUBDIRECTORY(streflop)
endif ()

SET(GFLAGS_BUILD_SHARED_LIBS          FALSE)
SET(GFLAGS_BUILD_STATIC_LIBS          TRUE)
SET(GFLAGS_BUILD_gflags_LIB           FALSE)
SET(GFLAGS_BUILD_gflags_nothreads_LIB TRUE)
SET(GFLAGS_BUILD_PACKAGING            FALSE)
SET(GFLAGS_BUILD_TESTING              FALSE)
SET(GFLAGS_INSTALL_HEADERS            FALSE)
SET(GFLAGS_INSTALL_SHARED_LIBS        FALSE)
SET(GFLAGS_INSTALL_STATIC_LIBS        FALSE)

ADD_SUBDIRECTORY(gflags)


# to change assimp for springs need, see fix_assimp.sh
# sadly still manual changes are needed!

set(DISABLED_ASSIMP_FORMATS "")
foreach(var AMF AC ASE ASSBIN ASSXML B3D BVH DXF CSM HMP IRRMESH IRR LWS MD2 MD3 MD5 MDC MDL MMD NFF NDO OFF OGRE OPENGEX PLY MS3D COB IFC XGL FBX Q3D Q3BSP RAW SIB SMD STL TERRAGEN 3D X X3D GLTF 3MF)
	list(APPEND DISABLED_ASSIMP_FORMATS "ASSIMP_BUILD_${var}_IMPORTER")
endforeach()
set(ENABLED_ASSIMP_FORMATS "")
foreach(var 3DS COLLADA LWO OBJ BLEND)
	list(APPEND ENABLED_ASSIMP_FORMATS "ASSIMP_BUILD_${var}_IMPORTER")
endforeach()

foreach(var
		${DISABLED_ASSIMP_FORMATS}
		BUILD_SHARED_LIBS
		ASSIMP_BUILD_ASSIMP_TOOLS
		ASSIMP_BUILD_TESTS
		ASSIMP_NO_EXPORT
		ASSIMP_OPT_BUILD_PACKAGES
		ASSIMP_BUILD_ASSIMP_TOOLS
		ASSIMP_BUILD_NONFREE_C4D_IMPORTER
		ASSIMP_DOUBLE_PRECISION
	)
	set("${var}" OFF CACHE STRING "forced off by spring build env" FORCE)
endforeach()

foreach(var
		${ENABLED_ASSIMP_FORMATS}
	)
	set("${var}" ON CACHE STRING "forced on by spring build env" FORCE)
endforeach()

foreach(var ASSIMP_BUILD_STATIC_LIB ASSIMP_NO_EXPORT)
	message(STATUS "Forcing assimp option ${var} to on")
	set("${var}" ON CACHE STRING "forced on by spring build env" FORCE)
endforeach()

ADD_SUBDIRECTORY(assimp)
target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OWN_ZLIB)

# hide assimp compile warnings with g++
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag(-Wno-class-memaccess CXX_SUPPORT_NO_CLASS_MEMACCESS)
	set(flagstoadd "")

	if(CXX_SUPPORT_NO_CLASS_MEMACCESS)
		set(flagstoadd "${flagstoadd} -Wno-class-memaccess")
	endif()
	check_cxx_compiler_flag(-Wno-tautological-compare CXX_SUPPORT_NO_AUTOLOGICAL_COMPARE)
	if(CXX_SUPPORT_NO_AUTOLOGICAL_COMPARE)
		set(flagstoadd "${flagstoadd} -Wno-tautological-compare")
	endif()
	set_target_properties(assimp PROPERTIES COMPILE_FLAGS ${flagstoadd})

endif()


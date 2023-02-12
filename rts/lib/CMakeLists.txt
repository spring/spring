
add_subdirectory(lua)
add_subdirectory(luasocket)
add_subdirectory(minizip)
add_subdirectory(headlessStubs)
if (ENABLE_STREFLOP)
	add_subdirectory(streflop)
endif ()

set(GFLAGS_BUILD_SHARED_LIBS          FALSE)
set(GFLAGS_BUILD_STATIC_LIBS          TRUE)
set(GFLAGS_BUILD_gflags_LIB           FALSE)
set(GFLAGS_BUILD_gflags_nothreads_LIB TRUE)
set(GFLAGS_BUILD_PACKAGING            FALSE)
set(GFLAGS_BUILD_TESTING              FALSE)
set(GFLAGS_INSTALL_HEADERS            FALSE)
set(GFLAGS_INSTALL_SHARED_LIBS        FALSE)
set(GFLAGS_INSTALL_STATIC_LIBS        FALSE)

add_subdirectory(gflags)


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

add_subdirectory(assimp)
target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OWN_ZLIB)
target_include_directories(assimp
PRIVATE
	"${CMAKE_SOURCE_DIR}/rts"
	assimp/include
	assimp
)
target_include_directories(IrrXML
PRIVATE
	"${CMAKE_SOURCE_DIR}/rts"
	assimp/include
)

# hide assimp compile warnings with g++
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag(-Wno-class-memaccess CXX_SUPPORT_NO_CLASS_MEMACCESS)
	set(flagstoadd "")

	if(CXX_SUPPORT_NO_CLASS_MEMACCESS)
		set(flagstoadd "${flagstoadd} -Wno-class-memaccess")
	endif ()
	check_cxx_compiler_flag(-Wno-tautological-compare CXX_SUPPORT_NO_AUTOLOGICAL_COMPARE)
	if(CXX_SUPPORT_NO_AUTOLOGICAL_COMPARE)
		set(flagstoadd "${flagstoadd} -Wno-tautological-compare")
	endif ()
	set_target_properties(assimp PROPERTIES COMPILE_FLAGS ${flagstoadd})

endif ()


set (AIDIR ${DATADIR} CACHE STRING "Where AIs are installed")

add_definitions(-DBUILDING_AI)

include_directories(
	${CMAKE_SOURCE_DIR}/rts
	${CMAKE_SOURCE_DIR}/rts/System
	${CMAKE_SOURCE_DIR}/rts/ExternalAI/Interface
	Wrappers
	${SDL_INCLUDE_DIR})


# set these for C Interfaces and AIs
SET(CMAKE_C_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}")
SET(CMAKE_C_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2}")
SET(CMAKE_C_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3}")
SET(CMAKE_C_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
SET(CMAKE_C_FLAGS_PROFILE        "${CMAKE_CXX_FLAGS_PROFILE}")

remove_definitions(-DSTREFLOP_SSE)
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)

if (WIN32)
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--kill-at -Wl,--add-stdcall-alias")
endif (WIN32)

list (APPEND ai_common ${CMAKE_SOURCE_DIR}/rts/Game/GameVersion)

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Wrappers/LegacyCpp legacycppaifiles)
list (APPEND legacycppaifiles ${ai_common})
list (APPEND legacycppaifiles ${CMAKE_SOURCE_DIR}/rts/System/float3)
list (APPEND legacycppaifiles ${CMAKE_SOURCE_DIR}/rts/Sim/Misc/DamageArray)
add_library(legacycpp STATIC ${legacycppaifiles})

aux_source_directory(${CMAKE_SOURCE_DIR}/rts/System/creg creg)
add_library(legacycpp-creg STATIC ${legacycppaifiles} ${creg})
set_target_properties(legacycpp-creg PROPERTIES COMPILE_FLAGS "-DUSING_CREG")

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils cutilsenv)

Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)

include_directories(${CMAKE_SOURCE_DIR}/rts ${CMAKE_SOURCE_DIR}/rts/ExternalAI ${CMAKE_SOURCE_DIR}/AI/Wrappers ${CMAKE_SOURCE_DIR}/rts/System ${SDL_INCLUDE_DIR})

remove_definitions(-DSTREFLOP_SSE) # would break AI compiling
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)
aux_source_directory(${CMAKE_SOURCE_DIR}/rts/System/creg creg)
#list (APPEND creg ${CMAKE_SOURCE_DIR}/rts/System/float3)

aux_source_directory(${CMAKE_SOURCE_DIR}/rts/ExternalAI/Interface aienv)
list (APPEND aienv ${CMAKE_SOURCE_DIR}/rts/Game/GameVersion)

list (APPEND errorhandler ${CMAKE_SOURCE_DIR}/rts/System/Platform/errorhandler)
if (UNIX AND NOT MINGW)
	list (APPEND errorhandler ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/X_MessageBox)
endif (UNIX AND NOT MINGW)

#aux_source_directory(${CMAKE_SOURCE_DIR}/rts/Lua/LuaParser luaparsingenv)
#aux_source_directory(${CMAKE_SOURCE_DIR}/rts/Map/MapParser luaparsingenv)

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Wrappers/LegacyCpp legacycppaienv)
list (APPEND legacycppaienv ${aienv})
list (APPEND legacycppaienv ${CMAKE_SOURCE_DIR}/rts/System/float3)
list (APPEND legacycppaienv ${CMAKE_SOURCE_DIR}/rts/Sim/Misc/DamageArray)
#list (APPEND legacycppaienv ${errorhandler})
#list (APPEND legacycppaienv ${CMAKE_SOURCE_DIR}/rts/System/LogOutput)


Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)
#Add_Subdirectory(Group)

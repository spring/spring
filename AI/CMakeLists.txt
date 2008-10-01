include_directories(${CMAKE_SOURCE_DIR}/rts ${CMAKE_SOURCE_DIR}/rts/ExternalAI ${CMAKE_SOURCE_DIR}/rts/System ${SDL_INCLUDE_DIR})

add_definitions(-fPIC)
#remove_definitions(-DSTREFLOP_X87)

aux_source_directory(${CMAKE_SOURCE_DIR}/rts/System/creg creg)

aux_source_directory(${CMAKE_SOURCE_DIR}/rts/ExternalAI/Interface aienv)
list (APPEND aienv ${CMAKE_SOURCE_DIR}/rts/Game/GameVersion)
list (APPEND aienv ${CMAKE_SOURCE_DIR}/rts/System/Platform/errorhandler)

aux_source_directory(${CMAKE_SOURCE_DIR}/rts/Lua/LuaParser luaparsingenv)
#aux_source_directory(${CMAKE_SOURCE_DIR}/rts/Map/MapParser luaparsingenv)

list (APPEND legacycppaienv ${aienv})
aux_source_directory(${CMAKE_SOURCE_DIR}/rts/ExternalAI/Interface/LegacyCppWrapper legacycppaienv)
list (APPEND legacycppaienv ${CMAKE_SOURCE_DIR}/rts/System/float3)
#list (APPEND legacycppaienv ${CMAKE_SOURCE_DIR}/rts/System/LogOutput)



Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)
Add_Subdirectory(Group)



### Generic native Skirmish AI config
##
## set(mySourceDirRel         "")
## set(additionalSources      ${CUtils_SRC})
## set(additionalCompileFlags "-DUSING_CREG -DLUA_LIB_EXT")
## set(additionalLibraries    ${LegacyCpp_Creg_AIWRAPPER_TARGET} "lua5.1")
##
## ConfigureNativeSkirmishAI(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)



GetNativeSourcesRecursive(luaSources "${CMAKE_SOURCE_DIR}/rts/lib/lua/src" "")

set(mySourceDirRel         "")
set(additionalSources      ${CUtils_SRC} ${luaSources} ${CMAKE_SOURCE_DIR}/rts/System/Util)
set(additionalCompileFlags "-DUSING_CREG -D__APPLE__") ## HACK: the x86-asm in System/Util conflicts with -fPIC
set(additionalCompileFlags "${additionalCompileFlags} -I ${CMAKE_SOURCE_DIR}/rts/lib/lua/include/")
set(additionalCompileFlags "${additionalCompileFlags} -I ${CMAKE_SOURCE_DIR}/rts/lib/lua/src/")
set(additionalCompileFlags "${additionalCompileFlags} -I ${CMAKE_SOURCE_DIR}/rts/lib/streflop/")
set(additionalLibraries    ${LegacyCpp_Creg_AIWRAPPER_TARGET})

ConfigureNativeSkirmishAI(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)

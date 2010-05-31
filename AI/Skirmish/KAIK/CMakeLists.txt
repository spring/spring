### Generic native Skirmish AI config
#

GetNativeSourcesRecursive(luaSources "${CMAKE_SOURCE_DIR}/rts/lib/lua/src" "")

set(mySourceDirRel         "") # Common values are "" or "src"
set(additionalSources      ${CUtils_SRC} ${luaSources} ${CMAKE_SOURCE_DIR}/rts/System/Util)
set(additionalCompileFlags "-DUSING_CREG")
set(additionalCompileFlags "${additionalCompileFlags} -I ${CMAKE_SOURCE_DIR}/rts/lib/lua/include/")
set(additionalCompileFlags "${additionalCompileFlags} -I ${CMAKE_SOURCE_DIR}/rts/lib/lua/src/")
set(additionalCompileFlags "${additionalCompileFlags} -I ${CMAKE_SOURCE_DIR}/rts/lib/streflop/")
set(additionalLibraries    ${LegacyCpp_Creg_AIWRAPPER_TARGET})

ConfigureNativeSkirmishAI(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)

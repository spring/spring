### Generic native Skirmish AI config
#

set(mySourceDirRel         "AI/NTai") # Common values are "" or "src"
set(additionalSources      "${CUtils_SRC}")
set(additionalCompileFlags "")
set(additionalLibraries    legacycpp ${Boost_THREAD_LIBRARY})

ConfigureNativeSkirmishAI(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)

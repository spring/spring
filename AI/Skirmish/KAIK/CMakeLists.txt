### Generic native Skirmish AI config
#

set(mySourceDirRel         "") # Common values are "" or "src"
set(additionalSources      ${CUtils_SRC})
set(additionalCompileFlags "-DUSING_CREG")
set(additionalLibraries    legacycpp-creg)

ConfigureNativeSkirmishAI(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)

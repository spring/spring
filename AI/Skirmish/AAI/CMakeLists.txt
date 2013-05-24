### Generic native Skirmish AI config
#

set(mySourceDirRel         "") # Common values are "" or "src"
set(additionalSources      "")
set(additionalCompileFlags "")
set(additionalLibraries    CUtils ${LegacyCpp_AIWRAPPER_TARGET})

ConfigureNativeSkirmishAI(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)

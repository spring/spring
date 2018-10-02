### Generic native Skirmish AI config
#

set(mySourceDirRel         "") # Common values are "" or "src"
set(additionalSources      "")
set(additionalCompileFlags "")
set(additionalLibraries    ${LegacyCpp_AIWRAPPER_TARGET} CUtils)

configure_native_skirmish_ai(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)

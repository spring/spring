### Generic native Skirmish AI config
#

set(mySourceDirRel         "src") # Common values are "" or "src"
set(additionalSources      "")
set(additionalCompileFlags "")
set(additionalLibraries    ${Cpp_AIWRAPPER_TARGET} CUtils)


if    (BUILD_Cpp_AIWRAPPER)
	include_directories(BEFORE ${Cpp_AIWRAPPER_INCLUDE_DIRS})
	ConfigureNativeSkirmishAI(mySourceDirRel additionalSources additionalCompileFlags additionalLibraries)
else  (BUILD_Cpp_AIWRAPPER)
	message ("warning: (New) C++ Test AI will not be built! (missing Cpp Wrapper)")
endif (BUILD_Cpp_AIWRAPPER)

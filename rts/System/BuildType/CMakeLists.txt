
set(build_type_sources
	BuildType.cpp
)

foreach(buildtype opengl4 dedicated headless unitsync)
	string(TOUPPER ${buildtype} buildtype_upper)
	set(target buildtype_${buildtype})

	add_library(${target} EXCLUDE_FROM_ALL STATIC ${build_type_sources})
	target_compile_definitions(${target} PRIVATE ${buildtype_upper})
endforeach()

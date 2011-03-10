# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#
# This is to be used through
# ADD_CUSTOM_COMMAND(${CMAKE_COMMAND} -P ConfigureFile.cmake).
#
# example usage:
#ADD_CUSTOM_COMMAND(
#	DEPENDS
#		"output.txt"
#	COMMAND "${CMAKE_COMMAND}"
#		"-Dfile.in=input.txt"
#		"-Dfile.out=output.txt"
#		"-DmyReplaceProperty=ABC"
#		"-P" "${CMAKE_MODULES_SPRING}/ConfigureFile.cmake"
#	DEPENDS
#		"input.txt"
#	COMMENT
#		"Configure output.txt" VERBATIM
#	)

CONFIGURE_FILE("${file.in}" "${file.out}")

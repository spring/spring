# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#
# example usage:
#Add_Custom_Command(
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

configure_file("${file.in}" "${file.out}")

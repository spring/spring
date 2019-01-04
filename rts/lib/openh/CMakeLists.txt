# Spring supplied CMake build file

#trigger compiling via make file
add_custom_target(
		COMMAND make all
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openh264
	)

#add the resulting artefacts

SET( openh264ressources,
	"openh264/libencoder.a"
	"openh264/libcommon.a"
	"openh264/libconsole_common.a"
	"openh264/libopenh264.a"
	"openh264/libprocessing.a"
	)


#add the resulting binaries as libraries
ADD_LIBRARY(encoder STATIC ${openh264ressources})
target_link_libraries(encoder ${OPENH_LIBRARY})


ADD_LIBRARY(common STATIC ${openh264ressources})
SET_TARGET_PROPERTIES(common ${OPENH_LIBRARY})

ADD_LIBRARY(console STATIC ${openh264ressources})
SET_TARGET_PROPERTIES(console ${OPENH_LIBRARY})

ADD_LIBRARY(processing STATIC ${openh264ressources})
SET_TARGET_PROPERTIES(processing ${OPENH_LIBRARY})

#add the header files to use the decoder/encorder in the project
target_include_directories(code_api ${CMAKE_CURRENT_SOURCE_DIR}/openh264/codec/api/svc/)

set_global(SPRING_OPENH_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/rts/lib/openh/openh264/")
set_global(SPRING_OPENH264_LIBRARY encoder)


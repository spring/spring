# Spring supplied CMake build file

#trigger compiling via make file
add_custom_target(ARCH=i386 
		COMMAND make all
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openh264
	)

#add the resulting artefacts
SET( openh264ressources,
	openh264/libencoder.a,
	openh264/libcommon.a
	openh264/libconsole_common.a
	#openh264/libdecoder.a
	openh264/libopenh264.a
	openh264/libprocessing.a)


#add the resulting binaries as libraries
ADD_LIBRARY(encoder STATIC EXCLUDE_FROM_ALL ${openh264ressources})
SET_TARGET_PROPERTIES(encoder PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

ADD_LIBRARY(common STATIC EXCLUDE_FROM_ALL ${openh264ressources})
SET_TARGET_PROPERTIES(common PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

ADD_LIBRARY(console STATIC EXCLUDE_FROM_ALL ${openh264ressources})
SET_TARGET_PROPERTIES(console PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

ADD_LIBRARY(processing STATIC EXCLUDE_FROM_ALL ${openh264ressources})
SET_TARGET_PROPERTIES(processing PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

#add the header files to use the decoder/encorder in the project

target_include_directories(code_api ${WORKING_DIRECTORY})


#TODO work continous

set_global(SPRING_OPENH264_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/rts/lib/openh264/codec/api/")
set_global(SPRING_OPENH264_LIBRARY encoder)

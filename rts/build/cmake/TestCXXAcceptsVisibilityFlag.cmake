# - Test whether the C++ compiler supports "-fvisibility=hidden"
# Once done this will define
#
# VISIBILITY_HIDDEN - -fvisibility=hidden if supported, an empty string otherwise.
#
# Copyright (C) 2008 Tobi Vollebregt
#

IF(NOT DEFINED CMAKE_CXX_FLAGS)
	INCLUDE(TestCXXAcceptsFlag)
	CHECK_CXX_ACCEPTS_FLAG(-fvisibility=hidden VISIBILITY_HIDDEN)
	IF(VISIBILITY_HIDDEN)
		SET(VISIBILITY_HIDDEN -fvisibility=hidden)
	ELSE(VISIBILITY_HIDDEN)
		SET(VISIBILITY_HIDDEN "")
	ENDIF(VISIBILITY_HIDDEN )
ENDIF(NOT DEFINED CMAKE_CXX_FLAGS) 

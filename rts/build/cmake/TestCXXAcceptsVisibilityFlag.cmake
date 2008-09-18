# - Test whether the C++ compiler supports "-fvisibility=hidden"
# Once done this will define
#
# VISIBILITY_HIDDEN - -fvisibility=hidden   if supported, an empty string otherwise.
# VISIBILITY_INLINES_HIDDEN - -fvisibility-inlines-hidden   likewise
#
# Copyright (C) 2008 Tobi Vollebregt
# Copyright (C) 2008 Karl-Robert Ernst
#
# Note: gcc for windows supports these flags, but give lots of errors when compiling, so use only for linux builds

INCLUDE(TestCXXAcceptsFlag)

IF(NOT DEFINED VISIBILITY_HIDDEN)
	CHECK_CXX_ACCEPTS_FLAG(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
	IF(HAS_VISIBILITY_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_HIDDEN "-fvisibility=hidden")
	ELSE(HAS_VISIBILITY_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_HIDDEN "")
	ENDIF(HAS_VISIBILITY_HIDDEN AND NOT MINGW)
ENDIF(NOT DEFINED VISIBILITY_HIDDEN)
	
IF(NOT DEFINED VISIBILITY_INLINES_HIDDEN)
	CHECK_CXX_ACCEPTS_FLAG(-fvisibility-inlines-hidden HAS_VISIBILITY_INLINES_HIDDEN)
	IF(HAS_VISIBILITY_INLINES_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_INLINES_HIDDEN "-fvisibility-inlines-hidden")
	ELSE(HAS_VISIBILITY_INLINES_HIDDEN AND NOT MINGW)
		SET(VISIBILITY_INLINES_HIDDEN "")
	ENDIF(HAS_VISIBILITY_INLINES_HIDDEN AND NOT MINGW)
ENDIF(NOT DEFINED VISIBILITY_INLINES_HIDDEN)

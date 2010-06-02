# Spring supplied CMake build file

SET(7zSources
		"7zBuf"
		"7zBuf2"
		"7zCrc"
		"7zFile"
		"7zStream"
		"Bcj2"
		"Bra"
		"Bra86"
		"LzmaDec"
		"Archive/7z/7zAlloc"
		"Archive/7z/7zDecode"
		"Archive/7z/7zExtract"
		"Archive/7z/7zHeader"
		"Archive/7z/7zIn"
		"Archive/7z/7zItem"
	)

ADD_LIBRARY(7zip STATIC EXCLUDE_FROM_ALL ${7zSources})
SET_TARGET_PROPERTIES(7zip PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")


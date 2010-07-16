# Spring supplied CMake build file

SET(md5Sources
		"md5"
		"base64"
	)

ADD_LIBRARY(md5 STATIC EXCLUDE_FROM_ALL ${md5Sources})


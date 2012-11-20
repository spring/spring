SET(7zSources
	7zAlloc.c
	7zBuf.c
	7zBuf2.c
	7zCrc.c
	7zCrcOpt.c
	7zDec.c
	7zIn.c
	CpuArch.c
	LzmaDec.c
	Lzma2Dec.c
	Bra.c
	Bra86.c
	Bcj2.c
	Ppmd7.c
	Ppmd7Dec.c
	7zFile.c
	7zStream.c
)

ADD_LIBRARY(7zip STATIC EXCLUDE_FROM_ALL ${7zSources})
SET_TARGET_PROPERTIES(7zip PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")


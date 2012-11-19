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

## FIXME: 7z doesn't compile on old compilers / 32bit (?) with -fPIC
##   CpuArch.c: In function 'MyCPUID':
##   CpuArch.c:75:3: error: can't find a register in class 'BREG' while reloading 'asm'
##   CpuArch.c:75:3: error: 'asm' operand has impossible constraints

# enable fPIC only for 64 bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET_TARGET_PROPERTIES(7zip PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
endif()


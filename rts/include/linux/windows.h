/**
This file is here only for fast porting. 
It has to be removed so that no #include <windows.h>
appears in the common code

@author gnibu

*/

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <fenv.h>
#include <iostream>
#include <byteswap.h>
#include <boost/cstdint.hpp>

//FIXME remove following line and include everywhere required
#include <inputs.h>

#define  WINAPI

#ifdef NO_DLL
#define FreeLibrary(a) (0)
#define LoadLibrary(a) (0)
#define GetProcAddress(a,b) (0)
#endif


#ifdef EMULE_WINTYPES
#include "wintypes.h"
typedef void* HINSTANCE ;
typedef void *HDC; 
typedef void *HANDLE;
#define __int64 int64_t
#define BOOL bool
#define VOID void
#define CALLBACK
#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif
typedef void *LPVOID;
typedef char *LPSTR;
typedef int64_t INT64;
typedef uint64_t UINT64;

#endif //EMULE_WINTYPES

#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) |\\
			((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) |\\
			((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD)(w) >> 8))

#ifdef USE_GLUT
#define ShowCursor(a)					\
do {							\
	if (a)						\
		glutSetCursor(GLUT_CURSOR_INHERIT);	\
	else						\
		glutSetCursor(GLUT_CURSOR_NONE);	\
} while (0)
#else
#define ShowCursor(a) do{}while(0)
#endif

#define Sleep(a) usleep((a)*1000)

#define _hypot(x,y) hypot(x,y)

/*
 * Error handling
 */
#ifdef DEBUG
#ifdef __GNUC__
#define DEBUGSTRING std::cerr << "  " << __FILE__ << ":" << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << std::endl;
#else
#define DEBUGSTRING std::cerr << "  " << __FILE__ << ":" << std::dec << __LINE__ << std::endl;
#endif
#else
#define DEBUGSTRING
#endif

#define MB_OK 			0x00000001
#define MB_ICONINFORMATION	0x00000002
#define MB_ICONEXCLAMATION	0x00000004

#define MessageBox(o, m, c, f)					\
do {								\
	if (f & MB_ICONINFORMATION)				\
	    std::cerr << "Info: ";				\
	else if (f & MB_ICONEXCLAMATION)			\
	    std::cerr << "Warning: ";				\
	else							\
	    std::cerr << "ERROR: ";				\
	std::cerr << c << std::endl;				\
	std::cerr << "  " << m << std::endl;			\
	if (!(f&(MB_ICONINFORMATION|MB_ICONEXCLAMATION)))	\
		do {DEBUGSTRING} while (0);			\
} while (0)

/*
 * Bitmap handling
 *
 * The swabbing stuff looks backwards, but the bitmaps
 * are _originally_ little endian (win32 x86).
 * So in this case little endian is the standard while
 * big endian is the exception.
 */
#if __BYTE_ORDER == __BIG_ENDIAN
#define swabword(w)	(bswap_16(w))
#define swabdword(w)	(bswap_32(w))
#else
#define swabword(w)	(w)
#define swabdword(w)	(w)
#endif
struct bitmapfileheader_s {
	unsigned short bfType;
	unsigned int bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int bfOffBits;
};
struct bitmapinfoheader_s {
	unsigned int biSize; 
	int biWidth; 
	int biHeight; 
	unsigned short biPlanes; 
	unsigned short biBitCount; 
	unsigned int  biCompression; 
	unsigned int  biSizeImage; 
	int biXPelsPerMeter; 
	int biYPelsPerMeter; 
	unsigned int biClrUsed; 
	unsigned int biClrImportant; 
};
struct paletteentry_s {
	unsigned char peRed;
	unsigned char peGreen;
	unsigned char peBlue;
	unsigned char peFlags;
};
typedef struct bitmapfileheader_s BITMAPFILEHEADER;
typedef struct bitmapinfoheader_s BITMAPINFOHEADER;
typedef struct paletteentry_s PALETTEENTRY;

#define BI_RGB 0

/*
 * Floating point
 */
#define _EM_INVALID FE_INVALID
#define _EM_DENORMAL __FE_DENORM
#define _EM_ZERODIVIDE FE_DIVBYZERO
#define _EM_OVERFLOW FE_OVERFLOW
#define _EM_UNDERFLOW FE_UNDERFLOW
#define _EM_INEXACT FE_INEXACT
#define _MCW_EM FE_ALL_EXCEPT

static inline unsigned int _control87(unsigned int newflags, unsigned int mask)
{
	fenv_t cur;
	fegetenv(&cur);
	if (mask) {
		cur.__control_word = ((cur.__control_word & ~mask)|(newflags & mask));
		fesetenv(&cur);
	}
	return (unsigned int)(cur.__control_word);
}

static inline unsigned int _clearfp(void)
{
	fenv_t cur;
	fegetenv(&cur);
#if 0	/* Windows control word default */
	cur.__control_word &= ~FE_ALL_EXCEPT;
	fesetenv(cur);
#else	/* Posix control word default */
	fesetenv(FE_DFL_ENV);
#endif
	return (unsigned int)(cur.__status_word);
}

/*
 * Performance testing
 */
#include <sys/time.h>
static inline bool QueryPerformanceCounter(LARGE_INTEGER *count)
{
#if defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
	/*
	 * Reading from the tsc is only available on
	 * pentium class chips (x86 compatible and x86_64)
	 */
	int64_t val;
	__asm__ __volatile__("rdtsc" : "=A" (val));
	count->QuadPart = val;
#elif defined(__GNUC__) && defined(__ia64__)
	/*
	 * Reading from the itc is only available on
	 * ia64
	 */
	int64_t val;
	__asm__ __volatile__("mov %0=ar.itc" : "=r" (val));
	count->QuadPart = val;
#else
	/*
	 * Have to go generic
	 */
	struct timeval tv;
	gettimeofday(&tv,NULL);
	count->QuadPart = (int64_t)tv.tv_usec;
#endif
	return true;
}


/*
 * Avoid repeated recalculations
 */
static int64_t _cpufreq;

/*
 * Generic solution
 */
static inline bool _freqfallback(LARGE_INTEGER *frequence)
{
	if (!_cpufreq) {
		int64_t tscstart,tscend;
		struct timeval tvstart,tvend;
		long usec;
		LARGE_INTEGER tmp;
		QueryPerformanceCounter(&tmp);
		tscstart = tmp.QuadPart;
		gettimeofday(&tvstart,NULL);
		usleep(100000);
		QueryPerformanceCounter(&tmp);
		tscend = tmp.QuadPart;
		gettimeofday(&tvend,NULL);
		usec = 1000000 * (tvend.tv_sec - tvstart.tv_sec) + (tvend.tv_usec - tvstart.tv_usec);
		_cpufreq = (tscend - tscstart) / usec;
	}
	frequence->QuadPart = _cpufreq;
	return !!_cpufreq;
}

#define CPUINFO "/proc/cpuinfo"
static inline bool QueryPerformanceFrequency(LARGE_INTEGER *frequence)
{
#if defined(__linux__)  /* /proc/cpuinfo on linux */
	if (!_cpufreq) {
		FILE *f = fopen(CPUINFO,"r");
		if (!f) {
			char err[50];
			sprintf(err,"Can't read from %s",CPUINFO);
			MessageBox(0,err,"System error",0);
			return _freqfallback(frequence);
		}
		for (;;) {
			int r;
			char buf[1000],tmp[24];
			if (fgets(buf, sizeof(buf), f) == NULL) {
				char err[50];
				sprintf(err,"Can't locate cpu MHz in %s",CPUINFO);
				MessageBox(0,err,"System error",0);
				return _freqfallback(frequence);
			}
#if defined(__powerpc__)
			r = sscanf(buf, "clock\t: %s", &tmp);
#elif defined( __i386__ ) || defined (__hppa__)  || defined (__ia64__) || defined(__x86_64__)
			r = sscanf(buf, "cpu MHz         : %s", &tmp);
#elif defined( __sparc__ )
			r = sscanf(buf, "Cpu0Bogo        : %s", &tmp);
#elif defined( __mc68000__ )
			r = sscanf(buf, "Clocking:       %s", &tmp);
#elif defined( __s390__  )
			r = sscanf(buf, "bogomips per cpu: %s", &tmp);
#else /* MIPS, ARM, and alpha */
			r = sscanf(buf, "BogoMIPS        : %s", &tmp);
#endif 
			if (r == 1) {
				fclose(f);
				_cpufreq = (int64_t)atoi(tmp);
				frequence->QuadPart = _cpufreq;
				return !!_cpufreq;
			}
		}
	}
#endif /* __linux__ */
	return _freqfallback(frequence);
}

#endif //__WINDOWS_H__

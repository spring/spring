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
#include <fenv.h>
#include <glib.h>
#include <iostream>

//FIXME remove following line and include everywhere required
#include <inputs.h>

#define  WINAPI

#ifdef NO_DLL
#define FreeLibrary(a) (0)
#define LoadLibrary(a) (0)
#define GetProcAddress(a,b) (0)
#endif


#ifdef EMULE_WINTYPES
//FIXME check following lines
typedef void *HANDLE;
#define CALLBACK
typedef void* HINSTANCE ;
typedef void *HDC; 
//following might stay like as is in a first time
typedef unsigned char BYTE;
typedef int WORD;
typedef bool BOOL;
typedef void *LPVOID;
typedef long int DWORD;
typedef unsigned int UINT;
typedef void VOID;
typedef long int DWORD;
typedef DWORD *DWORD_PTR;
typedef int LRESULT;
#include <string>
using namespace std;
typedef string LPSTR;
typedef int* LPINT;
/* TODO check if replacing
 	gu->lastFrameTime = (double)(start.QuadPart - lastMoveUpdate.QuadPart)/timeSpeed.QuadPart;

by 
	gu->lastFrameTime = (double)(start - lastMoveUpdate)/timeSpeed;
is OK for LARGE_INTEGER
Game.cpp:768: error: request for member `QuadPart' in `this->CGame::timeSpeed', 
   which is of non-class type `long int'
*/
#define LARGE_INTEGER long int
#define __int64 gint64
#else
#error please check the above types and maybe remove them from the code cf. http://www.jniwrapper.com/wintypes.jsp
#endif //EMULE_WINTYPES

#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) |\\
			((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) |\\
			((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD)(w) >> 8))

#ifdef ENABLE_SMALLFIXES
#define ShowCursor(a) while(0){}
#endif

#ifdef NO_MUTEXTHREADS
#define CreateMutex(a,b,c) (0)
#define ReleaseMutex(a) (0)
#define	CloseHandle(netMutex) while(0){}
#define WaitForSingleObject(a,b)  while(0){}
#endif

#define Sleep(a) sleep(a)

#define _hypot(x,y) hypot(x,y)

/*
 * Error handling
 */
#define MB_OK 			0x00000001
#define MB_ICONINFORMATION	0x00000002
#define MB_ICONEXCLAMATION	0x00000004

#define MessageBox(o, m, c, f)				\
do {							\
	if (f & MB_ICONINFORMATION)			\
	    std::cerr << "Info: ";			\
	else if (f & MB_ICONEXCLAMATION)		\
	    std::cerr << "Warning: ";			\
	else						\
	    std::cerr << "ERROR: ";			\
	std::cerr << c << std::endl;			\
	std::cerr << "  " << m << std::endl;		\
} while (0)

/*
 * Bitmap handling
 */
struct bitmapfileheader_s {
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
};
struct bitmapinfoheader_s {
	DWORD  biSize; 
	long   biWidth; 
	long   biHeight; 
	DWORD  biPlanes; 
	WORD   biBitCount; 
	DWORD  biCompression; 
	DWORD  biSizeImage; 
	long   biXPelsPerMeter; 
	long   biYPelsPerMeter; 
	DWORD  biClrUsed; 
	DWORD  biClrImportant; 
};
struct paletteentry_s {
	BYTE peRed;
	BYTE peGreen;
	BYTE peBlue;
	BYTE peFlags;
};
typedef struct bitmapfileheader_s BITMAPFILEHEADER;
typedef struct bitmapinfoheader_s BITMAPINFOHEADER;
typedef struct paletteentry_s PALETTEENTRY;

#define BI_RGB 0

/*
 * Floating point
 */
#define FPE_INVALID FE_INVALID
#define FPE_DENORMAL __FE_DENORM
#define FPE_ZERODIVIDE FE_DIVBYZERO
#define FPE_OVERFLOW FE_OVERFLOW
#define FPE_UNDERFLOW FE_UNDERFLOW
#define FPE_INEXACT FE_INEXACT
#define FPE_ALL FE_ALL_EXCEPT
#define _EM_INVALID FE_INVALID
#define _EM_DENORMAL __FE_DENORM
#define _EM_ZERODIVIDE FE_DIVBYZERO
#define _EM_OVERFLOW FE_OVERFLOW
#define _EM_UNDERFLOW FE_UNDERFLOW
#define _EM_INEXACT FE_INEXACT
#define _MCW_EM FE_ALL_EXCEPT

static inline unsigned int _control87(unsigned int newflags, unsigned int mask)
{
	fenv_t *cur;
	fegetenv(cur);
	if (mask) {
		cur->__control_word = ((cur->__control_word & ~mask)|(newflags & mask));
		fesetenv(cur);
	}
	return (unsigned int)(cur->__control_word);
}

static inline unsigned int _clearfp(void)
{
	fenv_t *cur;
	fegetenv(cur);
#if 0	/* Windows control word default */
	cur.__control_word &= ~FE_ALL_EXCEPT;
	fesetenv(cur);
#else	/* Posix control word default */
	fesetenv(FE_DFL_ENV);
#endif
	return (unsigned int)(cur->__status_word);
}

/*
 * Performance testing
 */
#include <sys/time.h>
static inline bool QueryPerformanceCounter(long int *count)
{
#if defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
	/*
	 * Reading from the tsc is only available on
	 * pentium class chips (x86 compatible and x86_64)
	 */
	unsigned long long val;
	__asm__ __volatile__("rdtsc" : "=A" (val));
	*count = (long int)val;
#elif defined(__GNUC__) && defined(__ia64__)
	/*
	 * Reading from the itc is only available on
	 * ia64
	 */
	unsigned long val;
	__asm__ __volatile__("mov %0=ar.itc" : "=r" (val));
	*count = (long int)val;
#else
	/*
	 * Have to go generic
	 */
	struct timeval tv;
	gettimeofday(&tv,NULL);
	*count = (long int)tv.tv_usec;
#endif
	return true;
}


/*
 * Avoid repeated recalculations
 */
static long int _cpufreq;

/*
 * Generic solution
 */
static inline bool _freqfallback(long int *frequence)
{
	if (!_cpufreq) {
		uint64_t tscstart,tscend, mhz;
		struct timeval tvstart,tvend;
		long usec;
		long int tmp;
		QueryPerformanceCounter(&tmp);
		tscstart = (uint64_t)tmp;
		gettimeofday(&tvstart,NULL);
		usleep(100000);
		QueryPerformanceCounter(&tmp);
		tscend = (uint64_t)tmp;
		gettimeofday(&tvend,NULL);
		usec = 1000000 * (tvend.tv_sec - tvstart.tv_sec) + (tvend.tv_usec - tvstart.tv_usec);
		mhz = (tscend - tscstart) / usec;
		_cpufreq = mhz;
		*frequence = mhz;
	} else
		*frequence = _cpufreq;
	return !!_cpufreq;
}

#define CPUINFO "/proc/cpuinfo"
static inline bool QueryPerformanceFrequency(long int *frequence)
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
			unsigned long mhz;
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
				mhz = atoi(tmp);
				fclose(f);
				_cpufreq = (long int)mhz;
				*frequence = _cpufreq;
				return !!_cpufreq;
			}
		}
	}
#endif /* __linux__ */
	return _freqfallback(frequence);
}


#endif //__WINDOWS_H__

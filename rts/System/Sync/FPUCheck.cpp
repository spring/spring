/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef USE_VALGRIND
	#include <valgrind/valgrind.h>
#endif

#include "FPUCheck.h"
#include <cstddef>
#include "lib/streflop/streflop_cond.h"
#include "System/Exceptions.h"
#include "lib/gml/gml_base.h"
#include "System/OpenMP_cond.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef STREFLOP_H
	#ifdef STREFLOP_SSE
	#elif STREFLOP_X87
	#else
		#error "streflop FP-math mode must be either SSE or X87"
	#endif
#endif

/**
	@brief checks FPU control registers.
	Checks the FPU control registers MXCSR and FPUCW,

For reference, the layout of the MXCSR register:
            FZ:RC:RC:PM:UM:OM:ZM:DM:IM: Rsvd:PE:UE:OE:ZE:DE:IE
            15 14 13 12 11 10  9  8  7|   6   5  4  3  2  1  0
Spring1:     0  0  0  1  1  1  0  1  0|   0   0  0  0  0  0  0 = 0x1D00 = 7424
Spring2:     0  0  0  1  1  1  1  1  1|   0   0  0  0  0  0  0 = 0x1F80 = 8064
Spring3:     0  0  0  1  1  0  0  1  0|   0   0  0  0  0  0  0 = 0x1900 = 6400  (signan)
Default:     0  0  0  1  1  1  1  1  1|   0   0  0  0  0  0  0 = 0x1F80 = 8064
MaskRsvd:    1  1  1  1  1  1  1  1  1|   0   0  0  0  0  0  0 = 0xFF80

And the layout of the 387 FPU control word register:
           Rsvd:Rsvd:Rsvd:X:RC:RC:PC:PC: Rsvd:Rsvd:PM:UM:OM:ZM:DM:IM
            15   14   13 12 11 10  9  8|   7    6   5  4  3  2  1  0
Spring1:     0    0    0  0  0  0  0  0|   0    0   1  1  1  0  1  0 = 0x003A = 58
Spring2:     0    0    0  0  0  0  0  0|   0    0   1  1  1  1  1  1 = 0x003F = 63
Spring3:     0    0    0  0  0  0  0  0|   0    0   1  1  0  0  1  0 = 0x0032 = 50   (signan)
Default:     0    0    0  0  0  0  1  1|   0    0   1  1  1  1  1  1 = 0x033F = 831
MaskRsvd:    0    0    0  1  1  1  1  1|   0    0   1  1  1  1  1  1 = 0x1F3F

	Where:
		Rsvd - Reserved
		FZ   - Flush to Zero
		RC   - Rounding Control
		PM   - Precision Mask
		UM   - Underflow Mask
		OM   - Overflow Mask
		ZM   - Zerodivide Mask
		DM   - Denormal Mask
		IM   - Invalid Mask
		PE   - Precision Exception
		UE   - Underflow Exception
		OE   - Overflow Exception
		ZE   - Zerodivide Exception
		DE   - Denormal Exception
		IE   - Invalid Exception
		X    - Infinity control (unused on 387 and higher)
		PC   - Precision Control

		Spring1  - Control word used by spring in code in CGame::SimFrame().
		Spring2  - Control word used by spring in code everywhere else.
		Default  - Default control word according to Intel.
		MaskRsvd - Masks out the reserved bits.

	Source: Intel Architecture Software Development Manual, Volume 1, Basic Architecture
*/
void good_fpu_control_registers(const char* text)
{
#ifdef USE_VALGRIND
	static const bool valgrindRunning = RUNNING_ON_VALGRIND;
	if (valgrindRunning) {
		// Valgrind doesn't allow us setting the FPU, so syncing is impossible
		return;
	}
#endif

	// accepted/syncsafe FPU states:
	int sse_a, sse_b, sse_c, x87_a, x87_b, x87_c;
	{
		sse_a = 0x1D00;
		sse_b = 0x1F80;
		sse_c = 0x1900; // signan
		x87_a = 0x003A;
		x87_b = 0x003F;
		x87_c = 0x0032; // signan
	}

#if defined(STREFLOP_SSE)
	// struct
	streflop::fpenv_t fenv;
	streflop::fegetenv(&fenv);

	const int fsse = fenv.sse_mode & 0xFF80;
	const int fx87 = fenv.x87_mode & 0x1F3F;

	bool ret = ((fsse == sse_a) || (fsse == sse_b) || (fsse == sse_c)) &&
	           ((fx87 == x87_a) || (fx87 == x87_b) || (fx87 == x87_c));

	if (!ret) {
		LOG_L(L_WARNING, "[%s] Sync warning: (env.sse_mode) MXCSR 0x%04X instead of 0x%04X or 0x%04X (\"%s\")", __FUNCTION__, fsse, sse_a, sse_b, text);
		LOG_L(L_WARNING, "[%s] Sync warning: (env.x87_mode) FPUCW 0x%04X instead of 0x%04X or 0x%04X (\"%s\")", __FUNCTION__, fx87, x87_a, x87_b, text);

		// Set single precision floating point math.
		streflop::streflop_init<streflop::Simple>();
	#if defined(__SUPPORT_SNAN__)
		if (!GML::Enabled() || Threading::IsSimThread())
			streflop::feraiseexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
	#endif
	}

#elif defined(STREFLOP_X87)
	// short int
	streflop::fpenv_t fenv;
	streflop::fegetenv(&fenv);

	bool ret = (fenv & 0x1F3F) == x87_a || (fenv & 0x1F3F) == x87_b || (fenv & 0x1F3F) == x87_c;

	if (!ret) {
		LOG_L(L_WARNING, "[%s] Sync warning: FPUCW 0x%04X instead of 0x%04X or 0x%04X (\"%s\")", __FUNCTION__, fenv, x87_a, x87_b, text);

		// Set single precision floating point math.
		streflop::streflop_init<streflop::Simple>();
	#if defined(__SUPPORT_SNAN__)
		if (!GML::Enabled() || Threading::IsSimThread())
			streflop::feraiseexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
	#endif
	}
#endif
}

void good_fpu_init()
{
	const unsigned int sseBits = proc::GetProcSSEBits();
		LOG("[CMyMath::Init] CPU SSE mask: %u, flags:", sseBits);
		LOG("\tSSE 1.0:  %d,  SSE 2.0:  %d", (sseBits >> 5) & 1, (sseBits >> 4) & 1);
		LOG("\tSSE 3.0:  %d, SSSE 3.0:  %d", (sseBits >> 3) & 1, (sseBits >> 2) & 1);
		LOG("\tSSE 4.1:  %d,  SSE 4.2:  %d", (sseBits >> 1) & 1, (sseBits >> 0) & 1);
		LOG("\tSSE 4.0A: %d,  SSE 5.0A: %d", (sseBits >> 8) & 1, (sseBits >> 7) & 1);

#ifdef STREFLOP_H
	const bool hasSSE1 = (sseBits >> 5) & 1;

	#ifdef STREFLOP_SSE
	if (hasSSE1) {
		LOG("\tusing streflop SSE FP-math mode, CPU supports SSE instructions");
	} else {
		throw unsupported_error("CPU is missing SSE instruction support");
	}
	#else
	if (hasSSE1) {
		LOG_L(L_WARNING, "\tStreflop floating-point math is set to X87 mode");
		LOG_L(L_WARNING, "\tThis may cause desyncs during multi-player games");
		LOG_L(L_WARNING, "\tThis CPU is SSE-capable; consider recompiling");
	} else {
		LOG_L(L_WARNING, "\tStreflop floating-point math is not SSE-enabled");
		LOG_L(L_WARNING, "\tThis may cause desyncs during multi-player games");
		LOG_L(L_WARNING, "\tThis CPU is not SSE-capable; it can only use X87 mode");
	}
	#endif

	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();
	#if defined(__SUPPORT_SNAN__)
		if (!GML::Enabled() || Threading::IsSimThread())
			streflop::feraiseexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
	#endif

#else
	// probably should check if SSE was enabled during
	// compilation and issue a warning about illegal
	// instructions if so (or just die with an error)
	LOG_L(L_WARNING, "Floating-point math is not controlled by streflop");
	LOG_L(L_WARNING, "This makes keeping multi-player sync 99% impossible");
#endif
}

void streflop_init_omp() {
#ifndef DEDICATED
	// Initialize FPU in all OpenMP threads, too
	// Note: Tested on Linux it seems it's not needed to do this.
	//       Either OMP threads copy the FPU state of the mainthread
	//       or the FPU state per-process on Linux.
	//       Still it hurts nobody to call these functions ;-)
#ifdef _OPENMP
	Threading::OMPCheck();
	#pragma omp parallel
	{
		//good_fpu_control_registers("OMP-Init");
		streflop::streflop_init<streflop::Simple>();
	#if defined(__SUPPORT_SNAN__)
		if (!GML::Enabled() || Threading::IsSimThread())
			streflop::feraiseexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
	#endif
	}
#endif
#endif
}

namespace proc {
	#if defined(__GNUC__)
		// function inlining breaks this
		__attribute__((__noinline__))
		void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
		{
		#ifndef __APPLE__
			__asm__ __volatile__(
				"cpuid"
				: "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
				: "0" (*a)
			);
		#else
			#ifdef __x86_64__
				__asm__ __volatile__(
					"pushq %%rbx\n\t"
					"cpuid\n\t"
					"movl %%ebx, %1\n\t"
					"popq %%rbx"
					: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
					: "0" (*a)
				);
			#else
				__asm__ __volatile__(
					"pushl %%ebx\n\t"
					"cpuid\n\t"
					"movl %%ebx, %1\n\t"
					"popl %%ebx"
					: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
					: "0" (*a)
				);
			#endif
		#endif
		}
	#elif defined(_MSC_VER) && (_MSC_VER >= 1310)
		void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
		{
			int features[4];
			__cpuid(features, *a);
			*a=features[0];
			*b=features[1];
			*c=features[2];
			*d=features[3];
		}
	#else
		// no-op on other compilers
		void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
		{
		}
	#endif

	unsigned int GetProcMaxStandardLevel()
	{
		unsigned int rEAX = 0x00000000;
		unsigned int rEBX =          0;
		unsigned int rECX =          0;
		unsigned int rEDX =          0;

		ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

		return rEAX;
	}

	unsigned int GetProcMaxExtendedLevel()
	{
		unsigned int rEAX = 0x80000000;
		unsigned int rEBX =          0;
		unsigned int rECX =          0;
		unsigned int rEDX =          0;

		ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

		return rEAX;
	}

	unsigned int GetProcSSEBits()
	{
		unsigned int rEAX = 0;
		unsigned int rEBX = 0;
		unsigned int rECX = 0;
		unsigned int rEDX = 0;
		unsigned int bits = 0;

		if (GetProcMaxStandardLevel() >= 0x00000001U) {
			rEAX = 0x00000001U; ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

			int SSE42  = (rECX >> 20) & 1; bits |= ( SSE42 << 0); // SSE 4.2
			int SSE41  = (rECX >> 19) & 1; bits |= ( SSE41 << 1); // SSE 4.1
			int SSSE30 = (rECX >>  9) & 1; bits |= (SSSE30 << 2); // Supplemental SSE 3.0
			int SSE30  = (rECX >>  0) & 1; bits |= ( SSE30 << 3); // SSE 3.0

			int SSE20  = (rEDX >> 26) & 1; bits |= ( SSE20 << 4); // SSE 2.0
			int SSE10  = (rEDX >> 25) & 1; bits |= ( SSE10 << 5); // SSE 1.0
			int MMX    = (rEDX >> 23) & 1; bits |= ( MMX   << 6); // MMX
		}

		if (GetProcMaxExtendedLevel() >= 0x80000001U) {
			rEAX = 0x80000001U; ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

			int SSE50A = (rECX >> 11) & 1; bits |= (SSE50A << 7); // SSE 5.0A
			int SSE40A = (rECX >>  6) & 1; bits |= (SSE40A << 8); // SSE 4.0A
			int MSSE   = (rECX >>  7) & 1; bits |= (MSSE   << 9); // Misaligned SSE
		}

		return bits;
	}
}

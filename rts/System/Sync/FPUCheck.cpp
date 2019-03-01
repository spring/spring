/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef USE_VALGRIND
	#include <valgrind/valgrind.h>
#endif

#include "FPUCheck.h"
#include "lib/streflop/streflop_cond.h"
#include "System/Exceptions.h"
#include "System/Threading/ThreadPool.h"
#include "System/Log/ILog.h"
#include "System/Platform/CpuID.h"

#ifndef STREFLOP_H
void good_fpu_control_registers(const char* text) { LOG_L(L_WARNING, "[%s](%s) streflop is disabled", __func__, text); }
void good_fpu_init() { LOG_L(L_WARNING, "[%s] streflop is disabled", __func__); }

#else

#ifdef STREFLOP_SSE
#elif STREFLOP_X87
#else
	#error "streflop FP-math mode must be either SSE or X87"
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
	constexpr int sse_a = 0x1D00;
	constexpr int sse_b = 0x1F80;
	constexpr int sse_c = 0x1900; // signan
	constexpr int x87_a = 0x003A;
	constexpr int x87_b = 0x003F;
	constexpr int x87_c = 0x0032; // signan

#ifdef STREFLOP_H
	streflop::fpenv_t fenv;
	streflop::fegetenv(&fenv);

	#if defined(STREFLOP_SSE)
	const int sse_flag = fenv.sse_mode & 0xFF80;
	const int x87_flag = fenv.x87_mode & 0x1F3F;

	const bool ret_sse = ((sse_flag == sse_a) || (sse_flag == sse_b) || (sse_flag == sse_c));
	const bool ret_x87 = ((x87_flag == x87_a) || (x87_flag == x87_b) || (x87_flag == x87_c));
	const bool ret_all = (ret_sse && ret_x87);

	if (!ret_all) {
		LOG_L(L_WARNING, "[%s] Sync warning: (env.sse_mode) MXCSR 0x%04X instead of 0x%04X or 0x%04X (\"%s\")", __func__, sse_flag, sse_a, sse_b, text);
		LOG_L(L_WARNING, "[%s] Sync warning: (env.x87_mode) FPUCW 0x%04X instead of 0x%04X or 0x%04X (\"%s\")", __func__, x87_flag, x87_a, x87_b, text);

		// Set single precision floating point math.
		streflop::streflop_init<streflop::Simple>();
		#if defined(__SUPPORT_SNAN__)
		streflop::feraiseexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
		#endif
	}

	#elif defined(STREFLOP_X87)
	if ((fenv & 0x1F3F) == x87_a || (fenv & 0x1F3F) == x87_b || (fenv & 0x1F3F) == x87_c)
		return;

	LOG_L(L_WARNING, "[%s] Sync warning: FPUCW 0x%04X instead of 0x%04X or 0x%04X (\"%s\")", __func__, fenv, x87_a, x87_b, text);

	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();
	#if defined(__SUPPORT_SNAN__)
	streflop::feraiseexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
	#endif
	#endif
#endif
}

void good_fpu_init()
{
	const unsigned int sseBits = springproc::GetProcSSEBits();
	const unsigned int sseFlag = (sseBits >> 5) & 1;

#ifdef STREFLOP_H
	#if (defined(STREFLOP_SSE))
	LOG("[%s][STREFLOP_SSE]", __func__);
	#elif (defined(STREFLOP_X87))
	LOG("[%s][STREFLOP_X87]", __func__);
	#else
	#error
	#endif
#endif

	LOG("\tSSE 1.0 : %d,  SSE 2.0 : %d", (sseBits >> 5) & 1, (sseBits >> 4) & 1);
	LOG("\tSSE 3.0 : %d, SSSE 3.0 : %d", (sseBits >> 3) & 1, (sseBits >> 2) & 1);
	LOG("\tSSE 4.1 : %d,  SSE 4.2 : %d", (sseBits >> 1) & 1, (sseBits >> 0) & 1);
	LOG("\tSSE 4.0A: %d,  SSE 5.0A: %d", (sseBits >> 8) & 1, (sseBits >> 7) & 1);

#ifdef STREFLOP_H
	#if (defined(STREFLOP_SSE))
	if (sseFlag == 0)
		throw unsupported_error("CPU is missing SSE 1.0 instruction support");
	#elif (defined(STREFLOP_X87))
	LOG_L(L_WARNING, "\tStreflop floating-point math is set to X87 mode");
	LOG_L(L_WARNING, "\tThis may cause desyncs during multi-player games");
	LOG_L(L_WARNING, "\tYour CPU is %s SSE-capable; consider %s", (sseFlag == 0)? "not": "", (sseFlag == 1)? "recompiling": "upgrading");
	#else
	#error
	#endif

	// Set single precision floating point math.
	streflop::streflop_init<streflop::Simple>();
	#if defined(__SUPPORT_SNAN__)
		streflop::feraiseexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
	#endif

#else
	// probably should check if SSE was enabled during
	// compilation and issue a warning about illegal
	// instructions if so (or just die with an error)
	LOG_L(L_WARNING, "\tFPU math is not controlled by streflop; multi-player games will desync");
#endif
}
#endif

namespace springproc {
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

			const int SSE42  = (rECX >> 20) & 1; bits |= ( SSE42 << 0); // SSE 4.2
			const int SSE41  = (rECX >> 19) & 1; bits |= ( SSE41 << 1); // SSE 4.1
			const int SSSE30 = (rECX >>  9) & 1; bits |= (SSSE30 << 2); // Supplemental SSE 3.0
			const int SSE30  = (rECX >>  0) & 1; bits |= ( SSE30 << 3); // SSE 3.0

			const int SSE20  = (rEDX >> 26) & 1; bits |= ( SSE20 << 4); // SSE 2.0
			const int SSE10  = (rEDX >> 25) & 1; bits |= ( SSE10 << 5); // SSE 1.0
			const int MMX    = (rEDX >> 23) & 1; bits |= ( MMX   << 6); // MMX
		}

		if (GetProcMaxExtendedLevel() >= 0x80000001U) {
			rEAX = 0x80000001U; ExecCPUID(&rEAX, &rEBX, &rECX, &rEDX);

			const int SSE50A = (rECX >> 11) & 1; bits |= (SSE50A << 7); // SSE 5.0A
			const int SSE40A = (rECX >>  6) & 1; bits |= (SSE40A << 8); // SSE 4.0A
			const int MSSE   = (rECX >>  7) & 1; bits |= (MSSE   << 9); // Misaligned SSE
		}

		return bits;
	}
}


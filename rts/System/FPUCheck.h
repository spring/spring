/* Author: Tobi Vollebregt */

/* Assertions on floating point unit control registers. */

#ifndef FPUCHECK_H
#define FPUCHECK_H

#include "Game/UI/InfoConsole.h"

/*
	For reference, the layout of the MXCSR register:
        FZ:RC:RC:PM:UM:OM:ZM:DM:IM:Rsvd:PE:UE:OE:ZE:DE:IE
        15 14 13 12 11 10  9  8  7   6   5  4  3  2  1  0
Spring:  0  0  0  1  1  1  0  1  0   0   0  0  0  0  0  0 = 0x1D00 = 7424
Default: 0  0  0  1  1  1  1  1  1   0   0  0  0  0  0  0 = 0x1F80 = 8064
MaskRsvd:1  1  1  1  1  1  1  1  1   0   0  0  0  0  0  0 = 0xFF80

	And the layout of the 387 FPU control word register:
        Rsvd:Rsvd:Rsvd:X:RC:RC:PC:PC:Rsvd:Rsvd:PM:UM:OM:ZM:DM:IM
         15   14   13 12 11 10  9  8   7    6   5  4  3  2  1  0
Spring:   0    0    0  0  0  0  0  0   0    0   1  1  1  0  1  0 = 0x003A = 58
Default:  0    0    0  0  0  0  1  1   0    0   1  1  1  1  1  1 = 0x033F = 831
MaskRsvd: 0    0    0  1  1  1  1  1   0    0   1  1  1  1  1  1 = 0x1F3F

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

	Source: Intel Architecture Software Development Manual, Volume 1, Basic Architecture
*/

static inline bool good_fpu_control_registers()
{
	// We are paranoid.
	// We don't trust the enumeration constants from streflop / (g)libc.
#if defined(STREFLOP_SSE)
	fenv_t fenv;
	fegetenv(&fenv);
	bool ret = (fenv.sse_mode & 0xFF80) == 0x1D00 &&
	           (fenv.x87_mode & 0x1F3F) == 0x003A;
#ifndef NDEBUG
	if (!ret) {
		info->AddLine("MXCSR: 0x%04X instead of 0x1D00", fenv.sse_mode);
		info->AddLine("FPUCW: 0x%04X instead of 0x003A", fenv.x87_mode);
	}
#endif
	return ret;
#elif defined(STREFLOP_X87)
	fenv_t fenv;
	fegetenv(&fenv);
	bool ret = (fenv & 0x1F3F) == 0x003A;
#ifndef NDEBUG
	if (!ret)
		info->AddLine("FPUCW: 0x%04X instead of 0x003A", fenv);
#endif
	return ret;
#endif
}

#endif // !FPUCHECK_H

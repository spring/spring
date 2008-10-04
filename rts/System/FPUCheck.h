/**
 * @file FPUCheck.h
 * @brief good_fpu_control_registers function
 * @author Tobi Vollebregt
 *
 * Assertions on floating point unit control registers.
 * For now it only defines the good_fpu_control_registers() function.
 *
 * Copyright (C) 2006.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#ifndef FPUCHECK_H
#define FPUCHECK_H

#include "LogOutput.h"
#include "lib/streflop/streflop_cond.h"

/**
	@brief checks FPU control registers.
	@return true if everything is fine, false otherwise

	Can be used in an assert() to check the FPU control registers MXCSR and FPUCW,
	e.g. `assert(good_fpu_control_registers());'

	For reference, the layout of the MXCSR register:
        FZ:RC:RC:PM:UM:OM:ZM:DM:IM:Rsvd:PE:UE:OE:ZE:DE:IE
        15 14 13 12 11 10  9  8  7   6   5  4  3  2  1  0
Spring1: 0  0  0  1  1  1  0  1  0   0   0  0  0  0  0  0 = 0x1D00 = 7424
Spring2: 0  0  0  1  1  1  1  1  1   0   0  0  0  0  0  0 = 0x1F80 = 8064
Default: 0  0  0  1  1  1  1  1  1   0   0  0  0  0  0  0 = 0x1F80 = 8064
MaskRsvd:1  1  1  1  1  1  1  1  1   0   0  0  0  0  0  0 = 0xFF80

	And the layout of the 387 FPU control word register:
        Rsvd:Rsvd:Rsvd:X:RC:RC:PC:PC:Rsvd:Rsvd:PM:UM:OM:ZM:DM:IM
         15   14   13 12 11 10  9  8   7    6   5  4  3  2  1  0
Spring1:  0    0    0  0  0  0  0  0   0    0   1  1  1  0  1  0 = 0x003A = 58
Spring2:  0    0    0  0  0  0  0  0   0    0   1  1  1  1  1  1 = 0x003F = 63
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

		Spring1  - Control word used by spring in code in CGame::SimFrame().
		Spring2  - Control word used by spring in code everywhere else.
		Default  - Default control word according to Intel.
		MaskRsvd - Masks out the reserved bits.

	Source: Intel Architecture Software Development Manual, Volume 1, Basic Architecture
*/
static inline void good_fpu_control_registers(const char* text)
{
	// We are paranoid.
	// We don't trust the enumeration constants from streflop / (g)libc.
#if defined(STREFLOP_SSE)
	fenv_t fenv;
	fegetenv(&fenv);
	bool ret = ((fenv.sse_mode & 0xFF80) == 0x1D00 || (fenv.sse_mode & 0xFF80) == 0x1F80) &&
	           ((fenv.x87_mode & 0x1F3F) == 0x003A || (fenv.x87_mode & 0x1F3F) == 0x003F);
	if (!ret) {
		logOutput.Print("Sync warning: MXCSR 0x%04X instead of 0x1D00 or 0x1F80 (\"%s\")", fenv.sse_mode, text);
		logOutput.Print("Sync warning: FPUCW 0x%04X instead of 0x003A or 0x003F (\"%s\")", fenv.x87_mode, text);
		// Set single precision floating point math.
		streflop_init<streflop::Simple>();
	}
#elif defined(STREFLOP_X87)
	fenv_t fenv;
	fegetenv(&fenv);
	bool ret = (fenv & 0x1F3F) == 0x003A || (fenv & 0x1F3F) == 0x003F;
	if (!ret) {
		logOutput.Print("Sync warning: FPUCW 0x%04X instead of 0x003A or 0x003F (\"%s\")", fenv, text);
		// Set single precision floating point math.
		streflop_init<streflop::Simple>();
	}
#endif
}

#endif // !FPUCHECK_H

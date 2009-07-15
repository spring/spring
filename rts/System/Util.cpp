#include "Util.h"

std::string StringReplace(const std::string& text,
                          const std::string& from,
                          const std::string& to)
{
	std::string working = text;
	std::string::size_type pos = 0;
	while (true) {
		pos = working.find(from, pos);
		if (pos == std::string::npos) {
			break;
		}
		std::string tmp = working.substr(0, pos);
		tmp += to;
		tmp += working.substr(pos + from.size(), std::string::npos);
		pos += to.size();
		working = tmp;
	}
	return working;
}

namespace proc {
	#if defined(__GNUC__)
	// function inlining breaks this
	__attribute__((__noinline__))
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		__asm__ __volatile__(
			"cpuid"
			: "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
			: "0" (*a)
		);
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

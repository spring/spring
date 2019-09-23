/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCCHECKER_H
#define SYNCCHECKER_H

#ifdef SYNCCHECK

#ifdef TRACE_SYNC_HEAVY
	#include "HsiehHash.h"
#endif

#include <assert.h>

/**
 * @brief sync checker class
 *
 * A Lightweight sync debugger that just keeps a running checksum over all
 * assignments to synced variables.
 */
class CSyncChecker {

	public:
		/**
		 * Whether one thread (doesn't have to be the current thread!!!) is currently processing a SimFrame.
		 */
		static bool InSyncedCode()    { return (inSyncedCode > 0); }
		static void EnterSyncedCode() { ++inSyncedCode; }
		static void LeaveSyncedCode() { assert(InSyncedCode()); --inSyncedCode; }

		/**
		 * Keeps a running checksum over all assignments to synced variables.
		 */
		static unsigned GetChecksum() { return g_checksum; }
		static void NewFrame() { g_checksum = 0xfade1eaf; }

		static void Sync(const void* p, unsigned size) {
			// most common cases first, make it easy for compiler to optimize for it
			// simple xor is not enough to detect multiple zeroes, e.g.
#ifdef TRACE_SYNC_HEAVY
			g_checksum = HsiehHash((const char*)p, size, g_checksum);
#else
			switch(size) {
			case 1:
				g_checksum += *(const unsigned char*)p;
				g_checksum ^= g_checksum << 10;
				g_checksum += g_checksum >> 1;
				break;
			case 2:
				g_checksum += *(const unsigned short*)(const char*)p;
				g_checksum ^= g_checksum << 11;
				g_checksum += g_checksum >> 17;
				break;
			case 3:
				// just here to make the switch statements contiguous (so it can be optimized)
				for (unsigned i = 0; i < 3; ++i) {
					g_checksum += *(const unsigned char*)p + i;
					g_checksum ^= g_checksum << 10;
					g_checksum += g_checksum >> 1;
				}
				break;
			case 4:
				g_checksum += *(const unsigned int*)(const char*)p;
				g_checksum ^= g_checksum << 16;
				g_checksum += g_checksum >> 11;
				break;
			default:
			{
				unsigned i = 0;
				for (; i < (size & ~3) / 4; ++i) {
					g_checksum += *(reinterpret_cast<const unsigned int*>(p) + i);
					g_checksum ^= g_checksum << 16;
					g_checksum += g_checksum >> 11;
				}
				for (; i < size; ++i) {
					g_checksum += *(const unsigned char*)p + i;
					g_checksum ^= g_checksum << 10;
					g_checksum += g_checksum >> 1;
				}
				break;
			}
			}
#endif
		}

	private:

		/**
		 * The sync checksum
		 */
		static unsigned g_checksum;

		/**
		 * @brief in synced code
		 *
		 * Whether one thread (doesn't have to current thread!!!) is currently processing a SimFrame.
		 */
		static int inSyncedCode;
};

#endif // SYNCDEBUG

#endif // SYNCDEBUGGER_H

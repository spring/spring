/* Author: Tobi Vollebregt */

#ifndef SYNCCHECKER_H
#define SYNCCHECKER_H

#ifdef SYNCCHECK

#include <assert.h>
#include <deque>
#include <vector>
#include <SDL_types.h>

/**
 * @brief sync checker class
 *
 * Lightweight sync debugger that just keeps a running checksum over all
 * assignments to synced variables.
 */
class CSyncChecker {

	public:

		static unsigned GetChecksum() { return g_checksum; }
		static void NewFrame() { g_checksum = 0; }

	private:

		static unsigned g_checksum;

		static inline void Sync(void* p, unsigned size) {
			// most common cases first, make it easy for compiler to optimize for it
			if (size == 1) {
				g_checksum ^= *(unsigned char*)p;
			} else if (size == 2) {
				g_checksum ^= *(unsigned short*)(char*)p;
			} else if (size == 4) {
				g_checksum ^= *(unsigned int*)(char*)p;
			} else {
				// generic case
				unsigned i = 0;
				for (; i < (size & ~3); i += 4)
					g_checksum ^= *(unsigned*) ((char*) p + i);
				for (; i < size; ++i)
					g_checksum ^= *((unsigned char*) p + i);
			}
		}

		friend class CSyncedPrimitiveBase;
};


/**
 * @brief base class to use for synced classes
 */
class CSyncedPrimitiveBase {

	protected:

		/**
		 * @brief wrapper to call the private CSyncChecker::Sync()
		 */
		inline void Sync(void* p, unsigned size, const char*) {
			CSyncChecker::Sync(p, size);
		}
};

#endif // SYNCDEBUG

#endif // SYNCDEBUGGER_H

/* Author: Tobi Vollebregt */

#ifndef SYNCCHECKER_H
#define SYNCCHECKER_H

#ifdef SYNCCHECK

#include <assert.h>
#include <deque>
#include <vector>

#ifdef TRACE_SYNC
#include "SyncTracer.h"
#endif

#include <boost/cstdint.hpp> /* Replace with <stdint.h> if appropriate */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const boost::uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((boost::uint32_t)(((const boost::uint8_t *)(d))[1])) << 8)\
	+(boost::uint32_t)(((const boost::uint8_t *)(d))[0]) )
#endif


/**
 * @brief sync checker class
 *
 * Lightweight sync debugger that just keeps a running checksum over all
 * assignments to synced variables.
 */
class CSyncChecker {

	public:

		static unsigned GetChecksum() { return g_checksum; }
		static void NewFrame() { g_checksum = 0xfade1eaf; }

                /** @brief a fast hash function
                 *
                 * This hash function is roughly 4x as fast as CRC32, but even that is too slow.
                 * We use a very simplistic add/xor feedback scheme when not debugging. */
                static inline boost::uint32_t HsiehHash (const char * data, int len, boost::uint32_t hash) {
                        boost::uint32_t tmp;
                        int rem;

                        if (len <= 0 || data == NULL) return 0;

                        rem = len & 3;
                        len >>= 2;

                        /* Main loop */
                        for (;len > 0; len--) {
                                hash  += get16bits (data);
                                tmp    = (get16bits (data+2) << 11) ^ hash;
                                hash   = (hash << 16) ^ tmp;
                                data  += 2*sizeof (boost::uint16_t);
                                hash  += hash >> 11;
                        }

                        /* Handle end cases */
                        switch (rem) {
                        case 3: hash += get16bits (data);
                                hash ^= hash << 16;
                                hash ^= data[sizeof (boost::uint16_t)] << 18;
                                hash += hash >> 11;
                                break;
                        case 2: hash += get16bits (data);
                                hash ^= hash << 11;
                                hash += hash >> 17;
                                break;
                        case 1: hash += *data;
                                hash ^= hash << 10;
                                hash += hash >> 1;
                        }

                        /* Force "avalanching" of final 127 bits */
                        hash ^= hash << 3;
                        hash += hash >> 5;
                        hash ^= hash << 4;
                        hash += hash >> 17;
                        hash ^= hash << 25;
                        hash += hash >> 6;

                        return hash;
                }

	private:

		static unsigned g_checksum;

		static inline void Sync(void* p, unsigned size) {
			// most common cases first, make it easy for compiler to optimize for it
			// simple xor is not enough to detect multiple zeroes, e.g.
#ifdef TRACE_SYNC_HEAVY
			g_checksum = HsiehHash((char*)p, size, g_checksum);
#else
			switch(size) {
			case 1:
				g_checksum += *(unsigned char*)p;
				g_checksum ^= g_checksum << 10;
				g_checksum += g_checksum >> 1;
				break;
			case 2:
				g_checksum += *(unsigned short*)(char*)p;
				g_checksum ^= g_checksum << 11;
				g_checksum += g_checksum >> 17;
				break;
			case 4:
				g_checksum += *(unsigned int*)(char*)p;
				g_checksum ^= g_checksum << 16;
				g_checksum += g_checksum >> 11;
				break;
			default:
			{
				unsigned i = 0;
				for (; i < (size & ~3); i += 4) {
					g_checksum += *(unsigned int*)(char*)p + i;
					g_checksum ^= g_checksum << 16;
					g_checksum += g_checksum >> 11;
				}
				for (; i < size; ++i) {
					g_checksum += *(unsigned char*)p + i;
					g_checksum ^= g_checksum << 10;
					g_checksum += g_checksum >> 1;
				}
				break;
			}
			}
#endif
		}

		friend class CSyncedPrimitiveBase;
};

#endif // SYNCDEBUG

#endif // SYNCDEBUGGER_H

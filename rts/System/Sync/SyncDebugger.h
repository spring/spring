/* Author: Tobi Vollebregt */

#ifndef SYNCDEBUGGER_H
#define SYNCDEBUGGER_H

#ifdef SYNCDEBUG

#include <assert.h>
#include <deque>
#include <vector>
#include <SDL_types.h>

#include "Sim/Misc/GlobalConstants.h"

/**
 * @brief sync debugger class
 *
 * The sync debugger keeps track of the results of the last assignments of
 * synced variables, and, if compiled with HAVE_BACKTRACE, it keeps a backtrace
 * for every assignment. This allows communication between client and server
 * to figure out which exact assignment (including backtrace) was the first
 * assignment to differ between client and server.
 */
class CSyncDebugger {

	public:

		static CSyncDebugger* GetInstance();

	private:

		enum {
			MAX_STACK = 5,       ///< Maximum number of stackframes per HistItemWithBacktrace.
			BLOCK_SIZE = 2048,   ///< Number of \p HistItem per block history.
			HISTORY_SIZE = 2048, ///< Number of blocks of the entire history.
		};

		/**
		 * @brief a clientside history item representing one assignment
		 *
		 * One clientside item in the history (without backtrace) is
		 * represented by this structure.
		 */
		struct HistItem {
			unsigned chk; ///< Checksum (XOR of 32 bit dwords of the data).
			unsigned data; ///< First four bytes of data
		};

		/**
		 * @brief a serverside history item representing one assignment
		 *
		 * One serverside item in the history (with backtrace) is represented
		 * by this structure.
		 */
		struct HistItemWithBacktrace: public HistItem {
			const char* op;      ///< Pointer to short static string giving operator type (e.g. "+=").
			unsigned frameNum;   ///< gs->frameNum at the time this entry was committed.
			unsigned bt_size;    ///< Number of entries in the stacktrace.
			void* bt[MAX_STACK]; ///< The stacktrace (frame pointers).
		};

	private:

		// client thread

		/**
		 * @brief the history on clients
		 *
		 * This points to the assignment history that doesn't have backtraces.
		 * It is used on platforms which don't have a backtrace() function
		 * (Windows) and on game clients. The game host uses historybt instead.
		 *
		 * Results of the last HISTORY_SIZE * BLOCK_SIZE = 2048 * 2048 = 4194304
		 * assignments to synced variables are stored in it.
		 *
		 * The size of the array is HISTORY_SIZE * BLOCK_SIZE * sizeof(HistItem),
		 * that is 2048*2048*4 = 16 megabytes.
		 */
		HistItem* history;

		/**
		 * @brief the history on the host
		 *
		 * This points to the assignment history that does have backtraces.
		 * It is used when running as server on platforms that have a
		 * backtrace() function. Game clients use the history array instead.
		 *
		 * Results of the last 4194304 assignments to synced variables are
		 * stored in it, with a backtrace attached to each of these results.
		 *
		 * This makes the size of the array HISTORY_SIZE * BLOCK_SIZE *
		 * sizeof(HistItemWithBacktrace), that is
		 * 2048*2048*(8+(MAX_STACK+1)*sizeof(void*)) = 128 megabytes on 32 bit
		 * systems and 224 megabytes on 64 bit systems.
		 */
		HistItemWithBacktrace* historybt;

		unsigned historyIndex;         ///< Where are we in the history buffer?
		volatile bool disable_history; ///< Volatile because it is read by server thread and written by client thread.
		bool may_enable_history;       ///< Is it safe already to set disable_history = false?
		Uint64 flop;                   ///< Current (local) operation number.

		// server thread

		std::vector<unsigned> checksumResponses[MAX_PLAYERS];    ///< Received checksums after a checkum request.
		Uint64 remoteFlop[MAX_PLAYERS];                          ///< Received operation number.
		std::deque<unsigned> requestedBlocks;        ///< We are processing these blocks.
		std::deque<unsigned> pendingBlocksToRequest; ///< We still need to receive these blocks (slowly emptied).
		bool waitingForBlockResponse;                ///< Are we still waiting for a block response?
		std::vector<unsigned> remoteHistory[MAX_PLAYERS];        ///< Chk field of history of clients (only of differing blocks).

	private:

		// don't construct or copy
		CSyncDebugger();
		CSyncDebugger(const CSyncDebugger&);
		CSyncDebugger& operator=(const CSyncDebugger&);
		~CSyncDebugger();

		void Backtrace(int index, const char* prefix) const;
		unsigned GetBacktraceChecksum(int index) const;
		void ClientSendChecksumResponse();
		void ServerQueueBlockRequests();
		void ClientSendBlockResponse(int block);
		void ServerReceivedBlockResponses();
		void ServerDumpStack();
		void Sync(void* p, unsigned size, const char* op);

	public:

		void Initialize(bool useBacktrace);
		void ServerTriggerSyncErrorHandling(int serverframenum);
		bool ServerReceived(const unsigned char* inbuf);
		void ServerHandlePendingBlockRequests();
		bool ClientReceived(const unsigned char* inbuf);
		void Reset();

		friend class CSyncedPrimitiveBase;
};

#endif // SYNCDEBUG

#endif // SYNCDEBUGGER_H

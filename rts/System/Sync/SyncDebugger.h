/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNC_DEBUGGER_H
#define SYNC_DEBUGGER_H

#ifdef SYNCDEBUG

#include <atomic>
#include <cassert>
#include <deque>
#include <vector>
#include <cinttypes>

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

		/**
		 * @brief get sync debugger instance
		 *
		 * The sync debugger is a singleton (for now).
		 * @return the instance
		 */
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
			/// Checksum (XOR of 32 bit dwords of the data),
			/// or the data itself if it is 32 bits or less.
			unsigned data;
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
		 */
		HistItemWithBacktrace* historybt;

		unsigned historyIndex;             ///< Where are we in the history buffer?
		std::atomic<bool> disableHistory;  ///< atomic because it is read by server thread and written by client thread.
		bool mayEnableHistory;             ///< Is it safe already to set disableHistory = false?
		std::uint64_t flop;                ///< Current (local) operation number.

		// server thread

		struct PlayerStruct
		{
			std::vector<unsigned> checksumResponses;
			std::vector<unsigned> remoteHistory;
			std::uint64_t remoteFlop = 0;
		};
		typedef std::vector<PlayerStruct> PlayerVec;
		PlayerVec players;

		std::deque<unsigned> requestedBlocks;        ///< We are processing these blocks.
		std::deque<unsigned> pendingBlocksToRequest; ///< We still need to receive these blocks (slowly emptied).

		bool waitingForBlockResponse;                ///< Are we still waiting for a block response?

	private:

		// don't construct or copy
		CSyncDebugger();
		CSyncDebugger(const CSyncDebugger&);
		CSyncDebugger& operator=(const CSyncDebugger&);
		~CSyncDebugger();

		/**
		 * @brief output a backtrace to the log
		 *
		 * Writes the backtrace attached to history item # index to the log.
		 * The backtrace is prefixed with prefix.
		 */
		void Backtrace(int index, const char* prefix) const;
		/**
		 * @brief get a checksum for a backtrace in the history
		 *
		 * @return a checksum for backtrace # index in the history.
		 */
		unsigned GetBacktraceChecksum(int index) const;
		/**
		 * @brief second step after desync
		 *
		 * Called by client to send a response to a checksum request.
		 */
		void ClientSendChecksumResponse();
		/**
		 * @brief third step after desync
		 *
		 * Called by server after all checksum responses have been received.
		 * Compares the checksumResponses and figures out which blocks are out
		 * of sync (have different checksum). For these blocks requests are
		 * queued which will be send next frames (one request at a time, see
		 * CSyncDebugger::ServerHandlePendingBlockRequests()).
		 */
		void ServerQueueBlockRequests();
		/**
		 * @brief fourth step after desync
		 *
		 * Called by client to send a response to a block request.
		 */
		void ClientSendBlockResponse(int block);
		/**
		 * @brief fifth step after desync
		 *
		 * Called each time a set of blockResponses (one for every client) is
		 * received.
		 * If there are no more pendingBlocksToRequest,
		 * it triggers the sixth step, ServerDumpStack().
		 */
		void ServerReceivedBlockResponses();
		/**
		 * @brief sixth step after desync
		 *
		 * Called by server once all blockResponses are received. It dumps a
		 * backtrace to the logger for every checksum mismatch in the block
		 * which was out of sync. The backtraces are passed to the logger in a
		 * fairly simple form consisting basically only of hexadecimal
		 * addresses. The logger class resolves those to function, file-name
		 * & line number.
		 */
		void ServerDumpStack();

	public:

		/**
		 * @brief the backbone of the sync debugger
		 *
		 * This function adds an item to the history and appends a backtrace and
		 * an operator (op) to it. p must point to the data to checksum and size
		 * must be the size of that data.
		 */
		void Sync(const void* p, unsigned size, const char* op);

		/**
		 * @brief initialize
		 *
		 * Initialize the sync debugger.
		 * @param useBacktrace true for a server (requires approx. 160 MegaBytes
		 *   on 32 bit systems and 256 MegaBytes on 64 bit systems)
		 *   and false for a client (requires only 32 MegaBytes extra)
		 */
		void Initialize(bool useBacktrace, unsigned numPlayers);
		/**
		 * @brief first step after desync
		 *
		 * Does nothing
		 */
		void ServerTriggerSyncErrorHandling(int serverframenum);
		/**
		 * @brief serverside network receiver
		 *
		 * Plugin for the CGameServer network code in GameServer.cpp.
		 * @return the number of bytes read from the network stream
		 */
		bool ServerReceived(const unsigned char* inbuf);
		/**
		 * @brief helper for the third step
		 *
		 * Must be called by the server in GameServer.cpp once every frame to
		 * handle queued block requests.
		 * @see #ServerQueueBlockRequests()
		 */
		void ServerHandlePendingBlockRequests();
		/**
		 * @brief clientside network receiver
		 *
		 * Plugin for the CGame network code in Game.cpp.
		 * @return the number of bytes read from the network stream
		 */
		bool ClientReceived(const unsigned char* inbuf);
		/**
		 * @brief re-enable the history
		 *
		 * Restart the sync debugger lifecycle, so it can be used again (if the
		 * sync errors are resolved somehow or you were just testing it using
		 * /fakedesync).
		 *
		 * Called after typing '/reset' in chat area.
		 */
		void Reset();

		friend class CSyncedPrimitiveBase;
};

#endif // SYNCDEBUG

#endif // SYNC_DEBUGGER_H

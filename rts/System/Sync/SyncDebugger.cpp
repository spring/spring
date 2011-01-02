/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#ifdef SYNCDEBUG

#include <boost/format.hpp>

#include "LogOutput.h"
#include "GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/PlayerHandler.h"
#include "BaseNetProtocol.h"
#include "NetProtocol.h"
#include "SyncDebugger.h"
#include "SyncChecker.h"
#include "SyncTracer.h"
#include "Logger.h"

#include <string.h>
#include <map>
#ifndef WIN32
/* for backtrace() function */
# include <execinfo.h>
# define HAVE_BACKTRACE
#elif defined __MINGW32__
/* from backtrace.c: */
extern "C" int backtrace (void **array, int size);
# define HAVE_BACKTRACE
#else
# undef HAVE_BACKTRACE
#endif


#define LOGFILE_SERVER   "syncdebug-server.log"
#define LOGFILE_CLIENT   "syncdebug-client.log"



/**
 * @brief logging instance
 */
CLogger logger;


/**
 * @brief get sync debugger instance
 *
 * The sync debugger is a singleton (for now).
 * @return the instance
 */
CSyncDebugger* CSyncDebugger::GetInstance() {
	static CSyncDebugger instance;
	return &instance;
}


/**
 * @brief default-noarg-constructor
 */
CSyncDebugger::CSyncDebugger():
		history(NULL), historybt(NULL), historyIndex(0),
		disable_history(false), may_enable_history(false),
		flop(0), waitingForBlockResponse(false)
{
}


/**
 * @brief destructor
 */
CSyncDebugger::~CSyncDebugger()
{
	delete[] history;
	delete[] historybt;
}


/**
 * @brief initialize
 *
 * Initialize the sync debugger. Pass true for a server (this requires approx.
 * 144 megabytes on 32 bit systems and 240 megabytes on 64 bit systems) and
 * false for a client (requires only 16 megabytes extra).
 * FIXME update this comment to reflect new values
 */
void CSyncDebugger::Initialize(bool useBacktrace, unsigned numPlayers)
{
	delete[] history;
	history = 0;
	delete[] historybt;
	historybt = 0;

#ifdef HAVE_BACKTRACE
	if (useBacktrace) {
		historybt = new HistItemWithBacktrace[HISTORY_SIZE * BLOCK_SIZE];
		memset(historybt, 0, HISTORY_SIZE * BLOCK_SIZE * sizeof(HistItemWithBacktrace));
	} else
#endif
	{
		history = new HistItem[HISTORY_SIZE * BLOCK_SIZE];
		memset(history, 0, HISTORY_SIZE * BLOCK_SIZE * sizeof(HistItem));
	}

	//cleanup
	historyIndex = 0;
	disable_history = false;
	may_enable_history = false;
	flop = 0;
	for (unsigned j = 0; j < numPlayers; ++j) {
		PlayerStruct buf;
		buf.checksumResponses.clear();
		buf.remoteHistory.clear();
		buf.remoteFlop = 0;
		players.push_back(buf);
	}
	pendingBlocksToRequest.clear();
	waitingForBlockResponse = false;

	// init logger
	logger.SetFilename(useBacktrace ? LOGFILE_SERVER : LOGFILE_CLIENT);
	logger.AddLine("Syncdebugger initialised");
	logger.FlushBuffer();
}


/**
 * @brief the backbone of the sync debugger
 *
 * This function adds an item to the history and appends a backtrace and an
 * operator (op) to it. p must point to the data to checksum and size must be
 * the size of that data.
 */
void CSyncDebugger::Sync(void* p, unsigned size, const char* op)
{
	if (!history && !historybt)
		return;

	HistItem* h = &history[historyIndex];

#ifdef HAVE_BACKTRACE
	if (historybt) {
		// dirty hack to skip the uppermost 2 or 3 (32 resp. 64 bit) frames without memcpy'ing the whole backtrace...
		const int frameskip = (8 + sizeof(void*)) / sizeof(void*);
		historybt[historyIndex].bt_size = backtrace(historybt[historyIndex].bt - frameskip, MAX_STACK + frameskip) - frameskip;
		historybt[historyIndex].op = op;
		historybt[historyIndex].frameNum = gs->frameNum;
		h = &historybt[historyIndex];
	}
#endif

	// if data fits and has even size, it's probably a POD type, store it verbatim
	// TODO: transfer offending blocks from clients to the server
	if (size == 1) {
		h->data = *(unsigned char*)p;
	} else if (size == sizeof(short)) {
		h->data = *(unsigned short*)p;
	} else if (size >= sizeof(unsigned)) {
		h->data = *(unsigned *)p;
	} else {
		h->data = -1;
	}

	// xor is dangerous in that every bit is independent of any other, this is bad
#ifdef SD_USE_SIMPLE_CHECKSUM
	unsigned i = 0;
	h->chk = 0;
	for (; i < (size & ~3); i += 4)
		h->chk ^= *(unsigned*) ((unsigned char*) p + i);
	for (; i < size; ++i)
		h->chk ^= *((unsigned char*) p + i);
#else
	// 33*flop + gs->frameNum is enough to prevent zero slurping
	// using CSyncChecker::g_checksum here will cause cascade false positives
	h->chk = CSyncChecker::HsiehHash((char*)p, size, 33*flop + gs->frameNum);
#endif

	if (++historyIndex == HISTORY_SIZE * BLOCK_SIZE)
		historyIndex = 0; // wrap around
	++flop;
}


/**
 * @brief output a backtrace to the log
 *
 * Writes the backtrace attached to history item # index to the log.
 * The backtrace is prefixed with prefix.
 */
void CSyncDebugger::Backtrace(int index, const char* prefix) const
{
	if (historybt) {
		for (unsigned i = 0; i < historybt[index].bt_size; ++i) {
			// the "{%p}" part is resolved to "functionname [filename:lineno]"
			// by the CLogger class.
#ifndef _WIN32
			logger.AddLine("%s#%u {%p}", prefix, i, historybt[index].bt[i]);
#else
			if (sizeof(void*) == 8)
				logger.AddLine("%s#%u {%llx}", prefix, i, (boost::uint64_t)historybt[index].bt[i]);
			else
				logger.AddLine("%s#%u {%x}", prefix, i, (boost::uint32_t)historybt[index].bt[i]);
#endif
		}
	}
}


/**
 * @brief get a checksum for a backtrace in the history
 *
 * @return a checksum for backtrace # index in the history.
 */
unsigned CSyncDebugger::GetBacktraceChecksum(int index) const
{
#ifdef SD_USE_SIMPLE_CHECKSUM
	unsigned checksum = 0;
	const unsigned* p = (const unsigned*) historybt[index].bt;
	for (unsigned i = 0; i < (sizeof(void*)/sizeof(unsigned)) * historybt[index].bt_size; ++i, ++p)
		checksum = 33 * checksum + *p;
	return checksum;
#else
	return CSyncChecker::HsiehHash((char *)historybt[index].bt, sizeof(void*) * historybt[index].bt_size, 0xf00dcafe);
#endif
}


/**
 * @brief serverside network receiver
 *
 * Plugin for the CGameServer network code in GameServer.cpp.
 * @return the number of bytes read from the network stream
 */
bool CSyncDebugger::ServerReceived(const unsigned char* inbuf)
{
	bool syncDebugPacket = false;
	switch (inbuf[0]) {
		case NETMSG_SD_CHKRESPONSE:
			if (*(short*)&inbuf[1] != HISTORY_SIZE * sizeof(unsigned) + 12) {
				logger.AddLine("Server: received checksum response of %d instead of %d bytes", *(short*)&inbuf[1], HISTORY_SIZE * 4 + 12);
			} else {
				int player = inbuf[3];
				if (!playerHandler->IsValidPlayer(player)) {
					logger.AddLine("Server: got invalid playernum %d in checksum response", player);
				} else {
					logger.AddLine("Server: got checksum response from %d", player);
					const unsigned* begin = (unsigned*)&inbuf[12];
					const unsigned* end = begin + HISTORY_SIZE;
					players[player].checksumResponses.resize(HISTORY_SIZE);
					std::copy(begin, end, players[player].checksumResponses.begin());
					players[player].remoteFlop = *(boost::uint64_t*)&inbuf[4];
					assert(!players[player].checksumResponses.empty());
					int i = 0;
					while (i < playerHandler->ActivePlayers() && !players[i].checksumResponses.empty()) ++i;
					if (i == playerHandler->ActivePlayers()) {
						ServerQueueBlockRequests();
						logger.AddLine("Server: checksum responses received; %d block requests queued", pendingBlocksToRequest.size());
					}
				}
			}
			syncDebugPacket = true;
			break;
		case NETMSG_SD_BLKRESPONSE:
			if (*(short*)&inbuf[1] != BLOCK_SIZE * sizeof(unsigned) + 4) {
				logger.AddLine("Server: received block response of %d instead of %d bytes", *(short*)&inbuf[1], BLOCK_SIZE * 4 + 4);
			} else {
				int player = inbuf[3];
				if (!playerHandler->IsValidPlayer(player)) {
					logger.AddLine("Server: got invalid playernum %d in block response", player);
				} else {
					const unsigned* begin = (unsigned*)&inbuf[4];
					const unsigned* end = begin + BLOCK_SIZE;
					unsigned size = players[player].remoteHistory.size();
					players[player].remoteHistory.resize(size + BLOCK_SIZE);
					std::copy(begin, end, players[player].remoteHistory.begin() + size);
					int i = 0;
					size += BLOCK_SIZE;
					while (i < playerHandler->ActivePlayers() && size == players[i].remoteHistory.size()) ++i;
					if (i == playerHandler->ActivePlayers()) {
						logger.AddLine("Server: block responses received");
						ServerReceivedBlockResponses();
					}
				}
			}
			syncDebugPacket = true;
			break;
		default:
			logger.AddLine("Server: unknown packet");
			break;
	}
	logger.FlushBuffer();
	return syncDebugPacket;
}


/**
 * @brief clientside network receiver
 *
 * Plugin for the CGame network code in Game.cpp.
 * @return the number of bytes read from the network stream
 */
bool CSyncDebugger::ClientReceived(const unsigned char* inbuf)
{
	bool syncDebugPacket = false;
	switch (inbuf[0]) {
		case NETMSG_SD_CHKREQUEST:
			if (gs->frameNum != *(int*)&inbuf[1]) {
				logger.AddLine("Client: received checksum request for frame %d instead of %d", *(int*)&inbuf[1], gs->frameNum);
			} else {
				disable_history = true; // no more additions to the history until we're done
				may_enable_history = false;
				ClientSendChecksumResponse();
				logger.AddLine("Client: checksum response sent");
			}
			syncDebugPacket = true;
			break;
		case NETMSG_SD_BLKREQUEST:
			if (*(unsigned short*)&inbuf[1] >= HISTORY_SIZE) {
				logger.AddLine("Client: invalid block number %d in block request", *(unsigned short*)&inbuf[1]);
			} else {
				ClientSendBlockResponse(*(unsigned short*)&inbuf[1]);
				logger.AddLine("Client: block response sent for block %d", *(unsigned short*)&inbuf[1]);
				// simple progress indication
				logOutput.Print("[SD] Client: %d / %d", *(unsigned short*)&inbuf[3], *(unsigned short*)&inbuf[5]);
			}
			syncDebugPacket = true;
			break;
		case NETMSG_SD_RESET:
			logger.CloseSession();
			logOutput.Print("[SD] Client: Done!");
// 			disable_history = false;
			may_enable_history = true;
			syncDebugPacket = true;
			break;
	}
	logger.FlushBuffer();
	return syncDebugPacket;
}


/**
 * @brief first step after desync
 *
 * Does nothing
 */
void CSyncDebugger::ServerTriggerSyncErrorHandling(int serverframenum)
{
	if (!disable_history) {
		//this will set disable_history = true once received so only one sync errors is handled at a time.
	}
}


/**
 * @brief second step after desync
 *
 * Called by client to send a response to a checksum request.
 */
void CSyncDebugger::ClientSendChecksumResponse()
{
	std::vector<unsigned> checksums;
	for (unsigned i = 0; i < HISTORY_SIZE; ++i) {
#ifdef SD_USE_SIMPLE_CHECKSUM
		unsigned checksum = 0;
		for (unsigned j = 0; j < BLOCK_SIZE; ++j) {
			if (historybt)
				checksum = 33 * checksum + historybt[BLOCK_SIZE * i + j].chk;
			else  checksum = 33 * checksum + history[BLOCK_SIZE * i + j].chk;
		}
#else
		unsigned checksum = 123456789;
		for (unsigned j = 0; j < BLOCK_SIZE; ++j) {
			if (historybt)
				checksum = CSyncChecker::HsiehHash((char*)&historybt[BLOCK_SIZE * i + j].chk, sizeof(historybt[BLOCK_SIZE * i + j].chk), checksum);
			else  checksum = CSyncChecker::HsiehHash((char*)&history[BLOCK_SIZE * i + j].chk, sizeof(history[BLOCK_SIZE * i + j].chk), checksum);
		}
#endif
		checksums.push_back(checksum);
	}
	net->Send(CBaseNetProtocol::Get().SendSdCheckresponse(gu->myPlayerNum, flop, checksums));
}


/**
 * @brief third step after desync
 *
 * Called by server after all checksum responses have been received.
 * Compares the checksumResponses and figures out which blocks are out of sync
 * (have different checksum). For these blocks requests are queued which will
 * be send next frames (one request at a time, see
 * CSyncDebugger::ServerHandlePendingBlockRequests()).
 */
void CSyncDebugger::ServerQueueBlockRequests()
{
	logger.AddLine("Server: queuing block requests");
	boost::uint64_t correctFlop = 0;
	for (int j = 0; j < playerHandler->ActivePlayers(); ++j) {
		if (correctFlop) {
			if (players[j].remoteFlop != correctFlop)
				logger.AddLine("Server: bad flop# %llu instead of %llu for player %d", players[j].remoteFlop, correctFlop, j);
		} else {
			correctFlop = players[j].remoteFlop;
		}
	}
	unsigned i = ((unsigned)(correctFlop % (HISTORY_SIZE * BLOCK_SIZE)) / BLOCK_SIZE) + 1;
	for (unsigned c = 0; c < HISTORY_SIZE; ++i, ++c) {
		unsigned correctChecksum = 0;
		if (i == HISTORY_SIZE) i = 0;
		for (int j = 0; j < playerHandler->ActivePlayers(); ++j) {
			if (correctChecksum && players[j].checksumResponses[i] != correctChecksum) {
				pendingBlocksToRequest.push_back(i);
				break;
			}
			correctChecksum = players[j].checksumResponses[i];
		}
	}
	if (!pendingBlocksToRequest.empty()) {
		logger.AddLine("Server: blocks: %u equal, %u not equal", HISTORY_SIZE - pendingBlocksToRequest.size(), pendingBlocksToRequest.size());
		requestedBlocks = pendingBlocksToRequest;
		// we know the first FPU bug occured in block # ii, so we send out a block request for it.
// 		serverNet->SendData<unsigned> (NETMSG_SD_BLKREQUEST, ii);
	} else {
		logger.AddLine("Server: huh, all blocks equal?!?");
		net->Send(CBaseNetProtocol::Get().SendSdReset());
	}
	//cleanup
	for (playerVec::iterator it = players.begin(); it != players.end(); ++it)
		it->checksumResponses.clear();
}


/**
 * @brief helper for the third step
 *
 * Must be called by the server in GameServer.cpp once every frame to handle
 * queued block requests (see CSyncDebugger::ServerQueueBlockRequests()).
 */
void CSyncDebugger::ServerHandlePendingBlockRequests()
{
	if (!pendingBlocksToRequest.empty() && !waitingForBlockResponse) {
		// last two shorts are for progress indication
		net->Send(CBaseNetProtocol::Get().SendSdBlockrequest(pendingBlocksToRequest.front(), requestedBlocks.size() - pendingBlocksToRequest.size() + 1, requestedBlocks.size()));
		waitingForBlockResponse = true;
	}
}


/**
 * @brief fourth step after desync
 *
 * Called by client to send a response to a block request.
 */
void CSyncDebugger::ClientSendBlockResponse(int block)
{
	std::vector<unsigned> checksums;
#ifdef TRACE_SYNC
	tracefile << "Sending block response for block " << block << "\n";
#endif
	for (unsigned i = 0; i < BLOCK_SIZE; ++i) {
		if (historybt)
			checksums.push_back(historybt[BLOCK_SIZE * block + i].chk);
		else  checksums.push_back(history[BLOCK_SIZE * block + i].chk);
#ifdef TRACE_SYNC
		if (historybt)
			tracefile << historybt[BLOCK_SIZE * block + i].chk << " " << historybt[BLOCK_SIZE * block + i].data << "\n";
		else  tracefile << history[BLOCK_SIZE * block + i].chk << " " << history[BLOCK_SIZE * block + i].data  << "\n";
#endif
	}
#ifdef TRACE_SYNC
	tracefile << "done\n";
#endif
	net->Send(CBaseNetProtocol::Get().SendSdBlockresponse(gu->myPlayerNum, checksums));
}


/**
 * @brief fifth step after desync
 *
 * Called each time a set of blockResponses (one for every client) is received.
 * If there are no more pendingBlocksToRequest, it triggers the sixth step,
 * ServerDumpStack().
 */
void CSyncDebugger::ServerReceivedBlockResponses()
{
	pendingBlocksToRequest.pop_front();
	waitingForBlockResponse = false;
	// analyse data and reset if this was the last block response
	if (pendingBlocksToRequest.empty())
		ServerDumpStack();
}


/**
 * @brief sixth step after desync
 *
 * Called by server once all blockResponses are received. It dumps a backtrace
 * to the logger for every checksum mismatch in the block which was out of
 * sync. The backtraces are passed to the logger in a fairly simple form
 * consisting basically only of hexadecimal addresses. The logger class
 * resolves those to function, filename & line number.
 */
void CSyncDebugger::ServerDumpStack()
{
	// first calculate start iterator...
	unsigned posInHistory = (unsigned)(players[0].remoteFlop % (HISTORY_SIZE * BLOCK_SIZE));
	logger.AddLine("Server: position in history: %u", posInHistory);
	unsigned blockNr = posInHistory / BLOCK_SIZE;
	unsigned virtualBlockNr = 0; // block nr in remoteHistory (which skips unchanged blocks)
	for (; virtualBlockNr < requestedBlocks.size() && requestedBlocks[virtualBlockNr] != blockNr; ++virtualBlockNr) {}
	unsigned virtualPosInHistory = (virtualBlockNr * BLOCK_SIZE) + (posInHistory % BLOCK_SIZE) + 1;
	unsigned virtualHistorySize = players[0].remoteHistory.size();
	if (virtualBlockNr >= requestedBlocks.size())
		virtualPosInHistory = virtualHistorySize;
	unsigned ndif = 0; // number of differences
	assert(virtualPosInHistory <= virtualHistorySize);

	// we make a pool of backtraces (to merge identical ones)
	unsigned curBacktrace = 0;
	std::map<unsigned, unsigned> checksumToIndex;
	std::map<unsigned, unsigned> indexToHistPos;

	// then loop from virtualPosInHistory to virtualHistorySize and from 0 to virtualPosInHistory.
	for (unsigned i = virtualPosInHistory, c = 0; c < virtualHistorySize; ++i, ++c) {
		unsigned correctChecksum = 0;
		if (i == virtualHistorySize) i = 0;
		bool err = false;
		for (int j = 0; j < playerHandler->ActivePlayers(); ++j) {
			if (correctChecksum && players[j].remoteHistory[i] != correctChecksum) {
				if (historybt) {
					virtualBlockNr = i / BLOCK_SIZE;
					blockNr = requestedBlocks[virtualBlockNr];
					unsigned histPos = blockNr * BLOCK_SIZE + i % BLOCK_SIZE;
					unsigned checksum = GetBacktraceChecksum(histPos);
					std::map<unsigned, unsigned>::iterator it = checksumToIndex.find(checksum);
					if (it == checksumToIndex.end()) {
						++curBacktrace;
						checksumToIndex[checksum] = curBacktrace;
						indexToHistPos[curBacktrace] = histPos;
					}
					logger.AddLine("Server: chk %08X instead of %08X frame %06u, (value=%08x %15.8e) backtrace %u in \"%s\"", players[j].remoteHistory[i], correctChecksum, historybt[histPos].frameNum, historybt[histPos].data, *(float*)&historybt[histPos].data, checksumToIndex[checksum], historybt[histPos].op);
				} else {
					logger.AddLine("Server: chk %08X instead of %08X", players[j].remoteHistory[i], correctChecksum);
				}
				err = true;
			} else {
				correctChecksum = players[j].remoteHistory[i];
			}
		}
		if (err) {
			++ndif;
		}
	}
	if (ndif)
		logger.AddLine("Server: chks: %d equal, %d not equal", virtualHistorySize - ndif, ndif);
	else
		// This is impossible (internal error).
		// Server first finds there are differing blocks, then all checksums equal??
		// Turns out this can happen if the checksum function is weak.
		logger.AddLine("Server: huh, all checksums equal?!? (INTERNAL ERROR)");

	//cleanup
	for (playerVec::iterator it = players.begin(); it != players.end(); ++it)
		it->remoteHistory.clear();

	if (historybt) {
		// output backtraces we collected earlier this function
		for (std::map<unsigned, unsigned>::iterator it = indexToHistPos.begin(); it != indexToHistPos.end(); ++it) {
			logger.AddLine("Server: === Backtrace %u ===", it->first);
			Backtrace(it->second, "Server: ");
		}
	}

	// and reset
	net->Send(CBaseNetProtocol::Get().SendSdReset());
	logger.AddLine("Server: Done!");
	logger.CloseSession();
	logOutput.Print("[SD] Server: Done!");
}

/**
 * @brief re-enable the history
 *
 * Restart the sync debugger lifecycle, so it can be used again (if the sync
 * errors are resolved somehow or you were just testing it using .fakedesync).
 *
 * Called after typing '.reset' in chat area.
 */
void CSyncDebugger::Reset()
{
	if (may_enable_history)
		disable_history = false;
}

#endif // SYNCDEBUG

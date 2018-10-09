/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifdef SYNCDEBUG

#include <cstring>

#include "SyncDebugger.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/Players/Player.h"
#include "Net/Protocol/BaseNetProtocol.h"
#include "Net/Protocol/NetProtocol.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/UnorderedMap.hpp"
#include "System/Log/ILog.h"

#include "HsiehHash.h"
#include "Logger.h"
#include "SyncTracer.h"

#ifndef WIN32
	/* for backtrace() function */
	#include <execinfo.h>
	#define HAVE_BACKTRACE
#elif defined __MINGW32__ || defined _MSC_VER
	/* from backtrace.c: */
	extern "C" int backtrace(void** array, int size);
	#define HAVE_BACKTRACE
#else
	#undef HAVE_BACKTRACE
#endif


#define LOGFILE_SERVER "syncdebug-server.log"
#define LOGFILE_CLIENT "syncdebug-client.log"


#define LOG_SECTION_SYNC_DEBUGGER "SD"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SYNC_DEBUGGER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SYNC_DEBUGGER



/**
 * @brief logging instance
 */
static CLogger logger;


CSyncDebugger* CSyncDebugger::GetInstance() {
	static CSyncDebugger instance;
	return &instance;
}


CSyncDebugger::CSyncDebugger()
	: history(NULL)
	, historybt(NULL)
	, historyIndex(0)
	, disableHistory(false)
	, mayEnableHistory(false)
	, flop(0)
	, waitingForBlockResponse(false)
{
}


CSyncDebugger::~CSyncDebugger()
{
	delete[] history;
	delete[] historybt;
}


void CSyncDebugger::Initialize(bool useBacktrace, unsigned numPlayers)
{
	delete[] history;
	history = NULL;
	delete[] historybt;
	historybt = NULL;

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
	disableHistory = false;
	mayEnableHistory = false;
	flop = 0;
	players.clear();
	players.resize(numPlayers);
	pendingBlocksToRequest.clear();
	waitingForBlockResponse = false;

	// init logger
	logger.SetFilename(useBacktrace ? LOGFILE_SERVER : LOGFILE_CLIENT);
	logger.AddLine("SyncDebugger initialized");
	logger.FlushBuffer();
}


void CSyncDebugger::Sync(const void* p, unsigned size, const char* op)
{
	if (!history && !historybt) {
		return;
	}

	HistItem* h = &history[historyIndex];

#ifdef HAVE_BACKTRACE
	if (historybt) {
		// HACK to skip the uppermost 2 or 3 (32 resp. 64 bit) frames without memcpy'ing the whole backtrace
		const int frameskip = (8 + sizeof(void*)) / sizeof(void*);
		historybt[historyIndex].bt_size = backtrace(historybt[historyIndex].bt - frameskip, MAX_STACK + frameskip) - frameskip;
		historybt[historyIndex].op = op;
		historybt[historyIndex].frameNum = gs->frameNum;
		h = &historybt[historyIndex];
	}
#endif

	if (size == 4) {
		// common case
		h->data = *(const unsigned*) p;
	}
	else {
		// > XOR seems dangerous in that every bit is independent of any other, this is bad.
		// This isn't the case here, however, because typically we checksum only 1-8 bytes
		// of data at a time, so most of it fits in the checksum anyway.
		// (see SyncedPrimitiveBase / SyncedPrimitive, the main client of this method)
		unsigned i = 0;
		h->data = 0;
		// whole dwords
		for (; i < (size & ~3); i += 4)
			h->data ^= *(const unsigned*) ((const unsigned char*) p + i);
		// remaining 0 to 3 bytes
		for (; i < size; ++i)
			h->data ^= *((const unsigned char*) p + i);
	}

	if (++historyIndex == HISTORY_SIZE * BLOCK_SIZE) {
		historyIndex = 0; // wrap around
	}
	++flop;
}


void CSyncDebugger::Backtrace(int index, const char* prefix) const
{
	if (historybt) {
		for (unsigned i = 0; i < historybt[index].bt_size; ++i) {
			// the "{%p}" part is resolved to "functionname [filename:lineno]"
			// by the CLogger class.
			if (historybt[index].bt[i] != 0) { //%p prints (nul), ignore it
				logger.AddLine("%s#%u {%p}", prefix, i, historybt[index].bt[i]);
			}
		}
	}
}


unsigned CSyncDebugger::GetBacktraceChecksum(int index) const
{
	return HsiehHash((char *)historybt[index].bt, sizeof(void*) * historybt[index].bt_size, 0xf00dcafe);
}


bool CSyncDebugger::ServerReceived(const unsigned char* inbuf)
{
	bool syncDebugPacket = false;
	switch (inbuf[0]) {
		case NETMSG_SD_CHKRESPONSE:
			if (*(short*)&inbuf[1] != HISTORY_SIZE * sizeof(unsigned) + 12) {
				logger.AddLine("Server: received checksum response of %d instead of %d bytes", *(short*)&inbuf[1], HISTORY_SIZE * 4 + 12);
			} else {
				int player = inbuf[3];
				if (!playerHandler.IsValidPlayer(player)) {
					logger.AddLine("Server: got invalid playernum %d in checksum response", player);
				} else {
					logger.AddLine("Server: got checksum response from %d", player);
					const unsigned* begin = (unsigned*)&inbuf[12];
					const unsigned* end = begin + HISTORY_SIZE;
					players[player].checksumResponses.resize(HISTORY_SIZE);
					std::copy(begin, end, players[player].checksumResponses.begin());
					players[player].remoteFlop = *(std::uint64_t*)&inbuf[4];
					assert(!players[player].checksumResponses.empty());
					int i = 0;
					while (i < playerHandler.ActivePlayers() &&
						(!players[i].checksumResponses.empty() ||
						!playerHandler.Player(i)->active)) ++i;
					if (i == playerHandler.ActivePlayers()) {
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
				if (!playerHandler.IsValidPlayer(player)) {
					logger.AddLine("Server: got invalid playernum %d in block response", player);
				} else {
					const unsigned* begin = (unsigned*)&inbuf[4];
					const unsigned* end = begin + BLOCK_SIZE;
					unsigned size = players[player].remoteHistory.size();
					players[player].remoteHistory.resize(size + BLOCK_SIZE);
					std::copy(begin, end, players[player].remoteHistory.begin() + size);
					int i = 0;
					size += BLOCK_SIZE;
					while (i < playerHandler.ActivePlayers() &&
						(size == players[i].remoteHistory.size() ||
						!playerHandler.Player(i)->active)) ++i;
					if (i == playerHandler.ActivePlayers()) {
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


bool CSyncDebugger::ClientReceived(const unsigned char* inbuf)
{
	bool syncDebugPacket = false;
	switch (inbuf[0]) {
		case NETMSG_SD_CHKREQUEST:
			if (gs->frameNum != *(int*)&inbuf[1]) {
				logger.AddLine("Client: received checksum request for frame %d instead of %d", *(int*)&inbuf[1], gs->frameNum);
			} else {
				disableHistory = true; // no more additions to the history until we're done
				mayEnableHistory = false;

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
				LOG("Client: %d / %d", *(unsigned short*)&inbuf[3], *(unsigned short*)&inbuf[5]);
			}
			syncDebugPacket = true;
			break;
		case NETMSG_SD_RESET:
			logger.CloseSession();
			LOG("Client: Done!");

// 			disableHistory = false;
			mayEnableHistory = true;
			syncDebugPacket = true;
			break;
	}
	logger.FlushBuffer();
	return syncDebugPacket;
}


void CSyncDebugger::ServerTriggerSyncErrorHandling(int serverframenum)
{
	if (!disableHistory) {
		//this will set disableHistory = true once received so only one sync errors is handled at a time.
	}
}


void CSyncDebugger::ClientSendChecksumResponse()
{
	std::vector<unsigned> checksums;
	for (unsigned i = 0; i < HISTORY_SIZE; ++i) {
		unsigned checksum = 123456789;
		for (unsigned j = 0; j < BLOCK_SIZE; ++j) {
			if (historybt) {
				checksum = HsiehHash((char*)&historybt[BLOCK_SIZE * i + j].data, sizeof(historybt[0].data), checksum);
			} else {
				checksum = HsiehHash((char*)&history[BLOCK_SIZE * i + j].data, sizeof(history[0].data), checksum);
			}
		}
		checksums.push_back(checksum);
	}
	clientNet->Send(CBaseNetProtocol::Get().SendSdCheckresponse(gu->myPlayerNum, flop, checksums));
}


void CSyncDebugger::ServerQueueBlockRequests()
{
	logger.AddLine("Server: queuing block requests");
	std::uint64_t correctFlop = 0;
	for (int j = 0; j < playerHandler.ActivePlayers(); ++j) {
		if (correctFlop) {
			if (players[j].remoteFlop != correctFlop)
				logger.AddLine(
#ifdef _WIN32
			"Server: bad flop# %I64u instead of %I64u for player %d",
#else
			"Server: bad flop# %llu instead of %llu for player %d",
#endif
				players[j].remoteFlop, correctFlop, j);
		} else {
			correctFlop = players[j].remoteFlop;
		}
	}
	unsigned i = ((unsigned)(correctFlop % (HISTORY_SIZE * BLOCK_SIZE)) / BLOCK_SIZE) + 1;
	for (unsigned c = 0; c < HISTORY_SIZE; ++i, ++c) {
		unsigned correctChecksum = 0;
		if (i == HISTORY_SIZE) i = 0;
		for (int j = 0; j < playerHandler.ActivePlayers(); ++j) {
			if (players[j].checksumResponses.empty())
				continue;
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
		clientNet->Send(CBaseNetProtocol::Get().SendSdReset());
	}
	//cleanup
	for (PlayerVec::iterator it = players.begin(); it != players.end(); ++it)
		it->checksumResponses.clear();
	logger.FlushBuffer();
}


void CSyncDebugger::ServerHandlePendingBlockRequests()
{
	if (!pendingBlocksToRequest.empty() && !waitingForBlockResponse) {
		// last two shorts are for progress indication
		clientNet->Send(CBaseNetProtocol::Get().SendSdBlockrequest(pendingBlocksToRequest.front(), requestedBlocks.size() - pendingBlocksToRequest.size() + 1, requestedBlocks.size()));
		waitingForBlockResponse = true;
	}
}


void CSyncDebugger::ClientSendBlockResponse(int block)
{
	std::vector<unsigned> checksums;
#ifdef TRACE_SYNC
	tracefile << "Sending block response for block " << block << "\n";
#endif
	for (unsigned i = 0; i < BLOCK_SIZE; ++i) {
		if (historybt) {
			checksums.push_back(historybt[BLOCK_SIZE * block + i].data);
		}
		else {
			checksums.push_back(history[BLOCK_SIZE * block + i].data);
		}
#ifdef TRACE_SYNC
		if (historybt) {
			tracefile << historybt[BLOCK_SIZE * block + i].data << " " << historybt[BLOCK_SIZE * block + i].data << "\n";
		}
		else {
			tracefile << history[BLOCK_SIZE * block + i].data << " " << history[BLOCK_SIZE * block + i].data  << "\n";
		}
#endif
	}
#ifdef TRACE_SYNC
	tracefile << "done\n";
#endif
	clientNet->Send(CBaseNetProtocol::Get().SendSdBlockresponse(gu->myPlayerNum, checksums));
}


void CSyncDebugger::ServerReceivedBlockResponses()
{
	pendingBlocksToRequest.pop_front();
	waitingForBlockResponse = false;
	// analyse data and reset if this was the last block response
	if (pendingBlocksToRequest.empty())
		ServerDumpStack();
}


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

	spring::unordered_map<unsigned, unsigned> checksumToIndex;
	spring::unordered_map<unsigned, unsigned> indexToHistPos;

	// then loop from virtualPosInHistory to virtualHistorySize and from 0 to virtualPosInHistory.
	for (unsigned i = virtualPosInHistory, c = 0; c < virtualHistorySize; ++i, ++c) {
		unsigned correctChecksum = 0;
		if (i == virtualHistorySize) i = 0;
		bool err = false;
		for (int j = 0; j < playerHandler.ActivePlayers(); ++j) {
			if (!playerHandler.Player(j)->active)
				continue;
			if (correctChecksum && players[j].remoteHistory[i] != correctChecksum) {
				if (historybt) {
					virtualBlockNr = i / BLOCK_SIZE;
					blockNr = requestedBlocks[virtualBlockNr];
					unsigned histPos = blockNr * BLOCK_SIZE + i % BLOCK_SIZE;
					unsigned checksum = GetBacktraceChecksum(histPos);
					const auto it = checksumToIndex.find(checksum);
					if (it == checksumToIndex.end()) {
						++curBacktrace;
						checksumToIndex[checksum] = curBacktrace;
						indexToHistPos[curBacktrace] = histPos;
					}
					logger.AddLine("Server: 0x%08X/%15.8e instead of 0x%08X/%15.8e, frame %06u, backtrace %u in \"%s\"",
							players[j].remoteHistory[i], *(float*)(char*)&players[j].remoteHistory[i],
							correctChecksum, *(float*)(char*)&correctChecksum,
							historybt[histPos].frameNum, checksumToIndex[checksum], historybt[histPos].op);
				} else {
					logger.AddLine("Server: 0x%08X/%15.8e instead of 0x%08X/%15.8e",
							players[j].remoteHistory[i], *(float*)(char*)&players[j].remoteHistory[i],
							correctChecksum, *(float*)(char*)&correctChecksum);
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
	for (PlayerVec::iterator it = players.begin(); it != players.end(); ++it)
		it->remoteHistory.clear();

	if (historybt) {
		// output backtraces we collected earlier this function
		for (auto it = indexToHistPos.cbegin(); it != indexToHistPos.cend(); ++it) {
			logger.AddLine("Server: === Backtrace %u ===", it->first);
			Backtrace(it->second, "Server: ");
		}
	}

	// and reset
	clientNet->Send(CBaseNetProtocol::Get().SendSdReset());
	logger.AddLine("Server: Done!");
	logger.CloseSession();
	LOG("Server: Done!");
}


void CSyncDebugger::Reset()
{
	if (mayEnableHistory)
		disableHistory = false;
}

#endif // SYNCDEBUG

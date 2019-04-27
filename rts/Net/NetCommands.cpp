/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cinttypes>

#include "Game/Game.h"
#include "GameServer.h"

#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/ClientData.h"
#include "Game/CommandMessage.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/ChatMessage.h"
#include "Game/WordCompletion.h"
#include "Game/IVideoCapturing.h"
#include "Game/InMapDraw.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/GameSetupDrawer.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaHandle.h"
#include "Net/Protocol/NetProtocol.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Net/UnpackPacket.h"
#include "System/Sound/ISound.h"

CONFIG(bool, LogClientData).defaultValue(false);

#define LOG_SECTION_NET "Net"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_NET)

static spring::unordered_map<int32_t, uint32_t> localSyncChecksums;


void CGame::AddTraffic(int playerID, int packetCode, int length)
{
	auto it = playerTraffic.find(playerID);

	if (it == playerTraffic.end()) {
		playerTraffic[playerID] = PlayerTrafficInfo();
		it = playerTraffic.find(playerID);
	}

	PlayerTrafficInfo& pti = it->second;
	pti.total += length;

	const auto cit = pti.packets.find(packetCode);

	if (cit == pti.packets.end()) {
		pti.packets[packetCode] = length;
	} else {
		cit->second += length;
	}
}

void CGame::SendClientProcUsage()
{
	static spring_time lastProcUsageUpdateTime = spring_gettime();

	if ((spring_gettime() - lastProcUsageUpdateTime).toMilliSecsf() >= 1000.0f) {
		lastProcUsageUpdateTime = spring_gettime();

		if (playing) {
			const float simProcUsage = (profiler.GetTimePercentage("Sim"));
			const float drawProcUsage = (profiler.GetTimePercentage("Draw") / std::max(1.0f, globalRendering->FPS)) * CGlobalUnsynced::minDrawFPS;
			const float totalProcUsage = simProcUsage + drawProcUsage;

			// take the minimum drawframes into account, too
			clientNet->Send(CBaseNetProtocol::Get().SendCPUUsage(totalProcUsage));
		} else {
			// the CPU-load percentage is undefined prior to SimFrame()
			clientNet->Send(CBaseNetProtocol::Get().SendCPUUsage(0.0f));
		}
	}
}


uint32_t CGame::GetNumQueuedSimFrameMessages(uint32_t maxFrames) const
{
	// read ahead to find number of NETMSG_XXXFRAMES we still have to process
	// this number is effectively a measure of current user network conditions
	std::shared_ptr<const netcode::RawPacket> packet;

	uint32_t numQueuedFrames = 0;
	uint32_t packetPeekIndex = 0;

	while ((packet = clientNet->Peek(packetPeekIndex))) {
		switch (packet->data[0]) {
			case NETMSG_PING: {
				const spring_time pktSendTime = spring_msecs(*reinterpret_cast<const float*>(&packet->data[3]));
				const spring_time pktRecvTime = spring_now();

				// LOG_L(L_INFO, "[Game::%s][NETMSG_PING] tag=%u dt=%fms", __func__, packet->data[2], pktRecvTime.toMilliSecsf() - pktSendTime.toMilliSecsf());

				eventHandler.Pong(packet->data[2], pktSendTime, pktRecvTime);
				clientNet->DeleteBufferPacketAt(packetPeekIndex);
			} break;
			case NETMSG_GAME_FRAME_PROGRESS: {
				// this special packet skips queue entirely, so gets processed here
				// it's meant to indicate current game progress for clients fast-forwarding to current point the game
				// NOTE: this event should be unsynced, since its time reference frame is not related to the current
				// progress of the game from the client's point of view
				eventHandler.GameProgress(*reinterpret_cast<int32_t*>(packet->data + 1));
				clientNet->DeleteBufferPacketAt(packetPeekIndex);
			} break;

			case NETMSG_NEWFRAME:
			case NETMSG_KEYFRAME:
				numQueuedFrames += (numQueuedFrames < maxFrames);
			default:
				packetPeekIndex += 1;
		}
	}

	return (numQueuedFrames);
}

void CGame::UpdateNumQueuedSimFrames()
{
	// on any *incoming* ping-response, just process NETMSG_{PING, GAME_FRAME_PROGRESS}
	// (even if host, self-ping processing time is useful to know for testing purposes)
	if (clientNet->GetNumWaitingPingPackets() > 0)
		GetNumQueuedSimFrameMessages(-1u);

	// if host, we have no buffer
	if (gameServer != nullptr)
		return;


	static spring_time lastUpdateTime = spring_gettime();

	const spring_time currTime = spring_gettime();
	const spring_time deltaTime = currTime - lastUpdateTime;

	// update consumption-rate faster at higher game speeds
	if (deltaTime.toMilliSecsf() < (500.0f / gs->speedFactor))
		return;


	// NOTE:
	//   unnecessary to scan entire queue *unless* joining a running game
	//   only reason in that case is to handle NETMSG_GAME_FRAME_PROGRESS
	const uint32_t numQueuedFrames = GetNumQueuedSimFrameMessages(-1u);

	if (globalConfig.useNetMessageSmoothingBuffer) {
		if (numQueuedFrames < lastNumQueuedSimFrames) {
			// conservative policy: take minimum of current and previous queue size
			// we *NEVER* want the queue to run completely dry (by not keeping a few
			// messages buffered) because this leads to micro-stutter which is WORSE
			// than trading latency for smoothness (by trailing some extra number of
			// simframes behind the server)
			lastNumQueuedSimFrames = numQueuedFrames;
		} else {
			// trust the past more than the future
			lastNumQueuedSimFrames = mix(lastNumQueuedSimFrames * 1.0f, numQueuedFrames * 1.0f, 0.1f);
		}

		// always stay a bit behind the actual server time
		// at higher speeds we need to keep more distance!
		// (because effect of network jitter is amplified)
		consumeSpeedMult = GAME_SPEED * gs->speedFactor + lastNumQueuedSimFrames - (2 * gs->speedFactor);
	} else {
		// Modified SPRING95 behaviour
		// Aim at staying 2 sim frames behind.
		consumeSpeedMult = GAME_SPEED * gs->speedFactor + (numQueuedFrames / 2) - 1;
	}

	// await one sim frame if queue is dry
	if (numQueuedFrames == 0)
		msgProcTimeLeft = -1000.0f * gs->speedFactor;

	lastUpdateTime = currTime;
}

void CGame::UpdateNetMessageProcessingTimeLeft()
{
	// compute new msgProcTimeLeft to "smooth" out SimFrame() calls
	if (gameServer == nullptr) {
		const spring_time currentReadNetTime = spring_gettime();
		const spring_time deltaReadNetTime = currentReadNetTime - lastReadNetTime;

		if (skipping) {
			msgProcTimeLeft = 10.0f;
		} else {
			// at <N> Hz we should consume one simframe message every (1000/N) ms
			//
			// <dt> since last call will typically be some small fraction of this
			// so we eat through the queue at a rate proportional to that fraction
			// (the amount of processing time is weighted by dt and also increases
			// when more messages are waiting)
			msgProcTimeLeft += (consumeSpeedMult * deltaReadNetTime.toMilliSecsf());
		}

		lastReadNetTime = currentReadNetTime;
	} else {
		// ensure ClientReadNet returns at least every 15 simframes
		// so CGame can process keyboard input, and render etc.
		msgProcTimeLeft = (GAME_SPEED / float(CGlobalUnsynced::minDrawFPS) * gs->wantedSpeedFactor) * 1000.0f;
	}
}

float CGame::GetNetMessageProcessingTimeLimit() const
{
	// balance the time spent in simulation & drawing (esp. when reconnecting)
	// use the following algo: i.e. with gu->reconnectSimDrawBalance = 0.2f
	//  -> try to spend minimum 20% of the time in drawing
	//  -> use remaining 80% for reconnecting
	// (maxSimFPS / minDrawFPS) is desired number of simframes per drawframe
	const float maxSimFPS    = (1.0f - CGlobalUnsynced::reconnectSimDrawBalance) * 1000.0f / std::max(0.01f, gu->avgSimFrameTime);
	const float minDrawFPS   =         CGlobalUnsynced::reconnectSimDrawBalance  * 1000.0f / std::max(0.01f, gu->avgDrawFrameTime);
	const float simDrawRatio = maxSimFPS / minDrawFPS;

	return Clamp(simDrawRatio * gu->avgSimFrameTime, 5.0f, 1000.0f / CGlobalUnsynced::minDrawFPS);
}

void CGame::ClientReadNet()
{
	// first look ahead so we can adapt consumeSpeedMult to network fluctuations
	// (smooths simframes across each full second, and balances the time spent in
	// sim & drawing)
	UpdateNumQueuedSimFrames();
	UpdateNetMessageProcessingTimeLeft();

	const spring_time msgProcEndTime = spring_gettime() + spring_msecs(GetNetMessageProcessingTimeLimit());

	const bool haveServerDemo = (gameServer != nullptr && gameServer->GetDemoReader() != nullptr);
	const bool haveClientDemo = (clientNet->GetDemoRecorder() != nullptr);

	// now really process the messages
	while (true) {
		if (msgProcTimeLeft <= 0.0f)
			break;
		if (spring_gettime() > msgProcEndTime)
			break;

		lastNetPacketProcessTime = spring_gettime();


		{
			// DemoFromDemo option only makes sense in combination with god-mode
			// however any sync-responses recorded in the original demo would be
			// written to the copy as-is by clientNet->GetData even though player
			// is locally desyncing the replayed game state via new orders
			//
			// when playing the copied demo this would cause desync warning spam,
			// so alter all source demo response packets before saving them again
			std::shared_ptr<const netcode::RawPacket> peekPacket = clientNet->Peek(0);

			if (peekPacket != nullptr && peekPacket->data[0] == NETMSG_SYNCRESPONSE) {
				if (haveServerDemo && haveClientDemo && gs->godMode) {
					assert(configHandler->GetBool("DemoFromDemo"));

					const  int32_t syncFrameNum = *reinterpret_cast<const int32_t*>(peekPacket->data + sizeof(uint8_t) + sizeof(uint8_t));
					const uint32_t syncCheckSum = localSyncChecksums[syncFrameNum];

					memcpy(peekPacket->data + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(int32_t), &syncCheckSum, sizeof(syncCheckSum));
				}
			}
		}


		// get netpacket from the queue
		std::shared_ptr<const netcode::RawPacket> packet = clientNet->GetData(gs->frameNum);

		if (packet == nullptr)
			break;

		lastReceivedNetPacketTime = spring_gettime();

		const uint8_t* inbuf = packet->data;
		const uint32_t dataLength = packet->length;
		const uint8_t packetCode = inbuf[0];

		switch (packetCode) {
			case NETMSG_QUIT: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string message;

					pckt >> message;

					LOG("%s", message.c_str());

					GameEnd({});
					AddTraffic(-1, packetCode, dataLength);
					clientNet->Close(true);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_QUIT] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_PLAYERLEFT: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_PLAYERLEFT] invalid player-number %i", __func__, playerNum);
					break;
				}

				playerHandler.PlayerLeft(playerNum, inbuf[2]);
				eventHandler.PlayerRemoved(playerNum, inbuf[2]);

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_STARTPLAYING: {
				const uint32_t timeToStart = *reinterpret_cast<const uint32_t*>(inbuf + 1);

				if (timeToStart > 0) {
					GameSetupDrawer::StartCountdown(timeToStart);
				} else {
					StartPlaying();
				}

				AddTraffic(-1, packetCode, dataLength);
			} break;

			case NETMSG_PLAYERSTAT: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_PLAYERSTAT] invalid player-number %i", __func__, playerNum);
					break;
				}

				CPlayer* player = playerHandler.Player(playerNum);
				player->currentStats = *reinterpret_cast<const PlayerStatistics*>(&inbuf[2]);

				CDemoRecorder* record = clientNet->GetDemoRecorder();

				if (record != nullptr)
					record->SetPlayerStats(playerNum, player->currentStats);

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_PAUSE: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_PAUSE] invalid player-number %i", __func__, playerNum);
					break;
				}

				gs->paused = !!inbuf[2];

				LOG("%s %s the game", playerHandler.Player(playerNum)->name.c_str(), (gs->paused ? "paused" : "unpaused"));

				eventHandler.GamePaused(playerNum, gs->paused);
				AddTraffic(playerNum, packetCode, dataLength);

				lastReadNetTime = spring_gettime();
			} break;

			case NETMSG_INTERNAL_SPEED: {
				sound->PitchAdjust(gs->speedFactor = *reinterpret_cast<const float*>(&inbuf[1]));
				AddTraffic(-1, packetCode, dataLength);
			} break;

			case NETMSG_USER_SPEED: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum) && playerNum != SERVER_PLAYER) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_USER_SPEED] invalid player-number %i", __func__, playerNum);
					break;
				}

				const char* pName = (playerNum == SERVER_PLAYER)? "server": playerHandler.Player(playerNum)->name.c_str();

				gs->wantedSpeedFactor = *reinterpret_cast<const float*>(&inbuf[2]);

				LOG("Speed set to %.1f [%s]", gs->wantedSpeedFactor, pName);
				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_CPU_USAGE: {
				LOG_L(L_WARNING, "[Game::%s][NETMSG_CPU_USAGE] clients should not get this", __func__);
				AddTraffic(-1, packetCode, dataLength);
			} break;

			case NETMSG_PLAYERINFO: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_PLAYERINFO] invalid player-number %i", __func__, playerNum);
					break;
				}

				CPlayer* p  = playerHandler.Player(playerNum);

				p->cpuUsage = *reinterpret_cast<const    float*>(&inbuf[2]);
				p->ping     = *reinterpret_cast<const uint32_t*>(&inbuf[6]);

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_PLAYERNAME: {
				try {
					netcode::UnpackPacket pckt(packet, 2);

					uint8_t playerID;
					pckt >> playerID;

					if (!playerHandler.IsValidPlayer(playerID))
						throw netcode::UnpackPacketException("Invalid player number");

					CPlayer* player = playerHandler.Player(playerID);
					pckt >> player->name;

					player->SetReadyToStart(gameSetup->startPosType != CGameSetup::StartPos_ChooseInGame);
					player->active = true;

					wordCompletion.AddWord(player->name, false, false, false); // required?
					AddTraffic(playerID, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_PLAYERNAME] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_CHAT: {
				try {
					const ChatMessage msg(packet);

					HandleChatMsg(msg);
					AddTraffic(msg.fromPlayer, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_CHAT] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_SYSTEMMSG: {
				try {
					netcode::UnpackPacket pckt(packet, 4);
					std::string sysMsg;

					pckt >> sysMsg;

					LOG("%s", sysMsg.c_str());
					AddTraffic(-1, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_SYSTEMMSG] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_STARTPOS: {
				const uint8_t playerID = inbuf[1];
				const uint8_t   teamID = inbuf[2];

				if (!playerHandler.IsValidPlayer(playerID) && playerID != SERVER_PLAYER) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_STARTPOS] invalid player-number %i", __func__, playerID);
					break;
				}
				if (!teamHandler.IsValidTeam(teamID)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_STARTPOS] invalid team-number %i", __func__, teamID);
					break;
				}

				const uint8_t rdyState = inbuf[3];

				float3 rawPickPos(*reinterpret_cast<const float*>(&inbuf[4]), *reinterpret_cast<const float*>(&inbuf[8]), *reinterpret_cast<const float*>(&inbuf[12]));
				float3 clampedPos(rawPickPos);

				CTeam* team = teamHandler.Team(teamID);
				team->ClampStartPosInStartBox(&clampedPos);

				if (eventHandler.AllowStartPosition(playerID, teamID, rdyState, clampedPos, rawPickPos)) {
					team->SetStartPos(clampedPos);

					if (playerID != SERVER_PLAYER)
						playerHandler.Player(playerID)->SetReadyToStart(rdyState != CPlayer::PLAYER_RDYSTATE_UPDATED);
				}

				AddTraffic(playerID, packetCode, dataLength);
			} break;

			case NETMSG_RANDSEED: {
				gsRNG.SetSeed(*((uint32_t*)&inbuf[1]), true);
				AddTraffic(-1, packetCode, dataLength);
			} break;

			case NETMSG_GAMEID: {
				const uint8_t* p = &inbuf[1];
				CDemoRecorder* record = clientNet->GetDemoRecorder();

				if (record != nullptr)
					record->SetGameID(p);

				memcpy(gameID, p, sizeof(gameID));
				LOG("GameID: "
						"%02x%02x%02x%02x%02x%02x%02x%02x"
						"%02x%02x%02x%02x%02x%02x%02x%02x",
						p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
						p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);

				AddTraffic(-1, packetCode, dataLength);
				eventHandler.GameID(gameID, sizeof(gameID));
			} break;

			case NETMSG_PATH_CHECKSUM: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_PATH_CHECKSUM] invalid player-number %i", __func__, playerNum);
					break;
				}
				if (pathManager->GetPathFinderType() == NOPFS_TYPE)
					break;

				const std::uint32_t playerCheckSum = *reinterpret_cast<const std::uint32_t*>(&inbuf[2]);
				const std::uint32_t localCheckSum = pathManager->GetPathCheckSum();

				const CPlayer* player = playerHandler.Player(playerNum);

				const char* pName = player->name.c_str();
				const char* pType = player->IsSpectator()? "spectator": "player";
				const char* fmtStrs[2] = {
					"[DESYNC WARNING] path-checksum for %s %d (%s) is 0; non-writable PathEstimator-cache?",
					"[DESYNC WARNING] path-checksum %08x for %s %d (%s) does not match local checksum %08x; stale PathEstimator-cache?",
				};

				// XXX maybe use a "Desync" section here?
				if (playerCheckSum == 0) {
					LOG_L(L_WARNING, fmtStrs[0], pType, playerNum, pName);
				} else {
					if (playerCheckSum != localCheckSum) {
						LOG_L(L_WARNING, fmtStrs[1], playerCheckSum, pType, playerNum, pName, localCheckSum);
					}
				}
			} break;

			case NETMSG_KEYFRAME: {
				if (!gameOver) {
					const int32_t serverFrameNum = *(int32_t*)(inbuf + 1);

					if (gs->frameNum != (serverFrameNum - 1)) {
						LOG_L(L_ERROR, "[Game::%s] keyframe difference: %i (client: %d, server: %d)", __func__, gs->frameNum - (serverFrameNum - 1), gs->frameNum, serverFrameNum);
						break;
					}

					// fall-through and run SimFrame() iff this message really came from the server
					clientNet->Send(CBaseNetProtocol::Get().SendKeyFrame(serverFrameNum));
				}
			}
			case NETMSG_NEWFRAME: {
				msgProcTimeLeft -= 1000.0f;
				lastSimFrameNetPacketTime = spring_gettime();

				SimFrame();

#ifdef SYNCCHECK
				// both NETMSG_SYNCRESPONSE and NETMSG_NEWFRAME are used for ping calculation by server
				ASSERT_SYNCED(gs->frameNum);
				ASSERT_SYNCED(CSyncChecker::GetChecksum());
				clientNet->Send(CBaseNetProtocol::Get().SendSyncResponse(gu->myPlayerNum, gs->frameNum, CSyncChecker::GetChecksum()));

				// buffer all checksums, so we can check sync later between demo & local
				if (haveServerDemo)
					localSyncChecksums[gs->frameNum] = CSyncChecker::GetChecksum();

				// reset checksum every 4096 frames =~ 2.5 minutes
				if ((gs->frameNum & 4095) == 0)
					CSyncChecker::NewFrame();
#endif
				AddTraffic(-1, packetCode, dataLength);
			} break;

			case NETMSG_SYNCRESPONSE: {
#if (defined(SYNCCHECK))
				if (haveServerDemo) {
					// NOTE:
					//   this packet is also sent during live games,
					//   during which we should just ignore it (the
					//   server does sync-checking for us)
					netcode::UnpackPacket pckt(packet, 1);

					uint8_t  playerNum; pckt >> playerNum;
					int32_t   frameNum; pckt >> frameNum;
					uint32_t  checkSum; pckt >> checkSum;

					const uint32_t ourCheckSum = localSyncChecksums[frameNum];

					// check if our checksum for this frame matches what
					// player <playerNum> sent to the server at the same
					// frame in the original game (in case of a demo)
					if (playerNum == gu->myPlayerNum)
						break;
					if (checkSum == ourCheckSum)
						break;

					const CPlayer* player = playerHandler.Player(playerNum);

					const char* pName = player->name.c_str();
					const char* pType = player->IsSpectator()? "spectator": "player";
					const char* fmtStr = "[DESYNC WARNING] checksum %x from demo %s %d (%s) does not match our checksum %x for frame-number %d";

					LOG_L(L_ERROR, fmtStr, checkSum, pType, playerNum, pName, ourCheckSum, frameNum);
				}
#endif
			} break;


			case NETMSG_COMMAND: {
				try {
					netcode::UnpackPacket pckt(packet, 1);

					uint16_t packetSize; pckt >> packetSize;
					uint8_t playerNum; pckt >> playerNum;

					if (!playerHandler.IsValidPlayer(playerNum))
						throw netcode::UnpackPacketException("Invalid player number");

					int32_t cmdID;
					int32_t cmdTimeOut;
					uint8_t cmdOptions;
					uint32_t numParams;

					pckt >> cmdID;
					pckt >> cmdTimeOut;
					pckt >> cmdOptions;
					pckt >> numParams;

					Command c(cmdID, cmdOptions);
					c.SetTimeOut(cmdTimeOut);

					for (uint32_t a = 0; a < numParams; ++a) {
						float param; pckt >> param;
						c.PushParam(param);
					}

					selectedUnitsHandler.NetOrder(c, playerNum);
					AddTraffic(playerNum, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_COMMAND] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_SELECT: {
				try {
					netcode::UnpackPacket pckt(packet, 1);
					std::vector<int32_t> selectedUnitIDs;

					uint16_t packetSize; pckt >> packetSize;
					uint8_t playerNum; pckt >> playerNum;

					const uint32_t numUnitIDs = (packetSize - 4) / sizeof(int16_t);

					if (!playerHandler.IsValidPlayer(playerNum))
						throw netcode::UnpackPacketException("Invalid player number");

					selectedUnitIDs.reserve(numUnitIDs);

					for (uint32_t a = 0; a < numUnitIDs; ++a) {
						int16_t unitID; pckt >> unitID;
						const CUnit* unit = unitHandler.GetUnit(unitID);

						if (unit == nullptr) {
							// unit was destroyed in simulation (without its ID being recycled)
							// after sending a command but before receiving it back, more likely
							// to happen in high-latency situations
							// LOG_L(L_WARNING, "[NETMSG_SELECT] invalid unitID (%i) from player %i", unitID, playerNum);
							continue;
						}

						// if in godMode, this is always true for any player
						if (playerHandler.Player(playerNum)->CanControlTeam(unit->team))
							selectedUnitIDs.push_back(unitID);
					}

					selectedUnitsHandler.NetSelect(selectedUnitIDs, playerNum);
					AddTraffic(playerNum, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_SELECT] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_AICOMMAND:
			case NETMSG_AICOMMAND_TRACKED: {
				try {
					netcode::UnpackPacket pckt(packet, 1);

					int16_t psize; pckt >> psize;
					uint8_t player; pckt >> player;
					uint8_t aiID; pckt >> aiID;
					int16_t unitID;

					if (!playerHandler.IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					pckt >> unitID;
					if (unitID < 0 || static_cast<size_t>(unitID) >= unitHandler.MaxUnits())
						throw netcode::UnpackPacketException("Invalid unit ID");

					int32_t cmdID;
					int32_t cmdTimeOut;
					uint8_t cmdOptions;
					uint32_t numParams;

					pckt >> cmdID;
					pckt >> cmdTimeOut;
					pckt >> cmdOptions;
					pckt >> numParams;

					Command c(cmdID, cmdOptions);
					c.SetTimeOut(cmdTimeOut);

					if (packetCode == NETMSG_AICOMMAND_TRACKED) {
						pckt >> cmdID;
						c.SetAICmdID(cmdID);
					}

					// insert the command parameters
					for (uint32_t a = 0; a < numParams; ++a) {
						float param;
						pckt >> param;
						c.PushParam(param);
					}

					// command originating from SkirmishAI or LuaUnsyncedCtrl
					selectedUnitsHandler.AINetOrder(unitID, player, c);
					AddTraffic(player, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_AICOMMAND*] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_AICOMMANDS: {
				try {
					netcode::UnpackPacket pckt(packet, 3);

					uint8_t player;
					uint8_t aiID;
					uint8_t pairwise;
					uint32_t sameCmdID;
					uint8_t sameCmdOpt;
					uint16_t sameCmdParamSize;

					int16_t unitCount;
					int16_t commandCount;

					pckt >> player;

					if (!playerHandler.IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					pckt >> aiID;
					pckt >> pairwise;
					pckt >> sameCmdID;
					pckt >> sameCmdOpt;
					pckt >> sameCmdParamSize;

					std::vector<int32_t> unitIDs;
					std::vector<Command> commands;

					// parse the unit list
					pckt >> unitCount;

					for (int16_t u = 0; u < unitCount; u++) {
						int16_t unitID;
						pckt >> unitID;
						unitIDs.push_back(unitID);
					}

					// parse the command list
					pckt >> commandCount;

					for (int16_t c = 0; c < commandCount; c++) {
						int32_t cmdID;
						uint8_t cmdOpt;
						uint16_t paramCount = 0;

						if ((cmdID = sameCmdID) == 0)
							pckt >> cmdID;
						if ((cmdOpt = sameCmdOpt) == 0xFF)
							pckt >> cmdOpt;

						Command cmd(cmdID, cmdOpt);

						if ((paramCount = sameCmdParamSize) == 0xFFFF)
							pckt >> paramCount;

						for (uint16_t p = 0; p < paramCount; p++) {
							float param;
							pckt >> param;
							cmd.PushParam(param);
						}
						commands.push_back(cmd);
					}

					// apply the commands
					if (pairwise) {
						for (int16_t x = 0; x < std::min(unitCount, commandCount); ++x) {
							selectedUnitsHandler.AINetOrder(unitIDs[x], player, commands[x]);
						}
					} else {
						for (int16_t c = 0; c < commandCount; c++) {
							for (int16_t u = 0; u < unitCount; u++) {
								selectedUnitsHandler.AINetOrder(unitIDs[u], player, commands[c]);
							}
						}
					}
					AddTraffic(player, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_AICOMMANDS] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_AISHARE: {
				try {
					netcode::UnpackPacket pckt(packet, 1);

					int16_t numBytes;
					uint8_t player;
					uint8_t aiID;
					uint8_t srcTeamID;
					uint8_t dstTeamID;

					float metalShare;
					float energyShare;

					pckt >> numBytes;
					pckt >> player;

					if (!playerHandler.IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					// total message length
					constexpr int32_t    fixedLen = (1 + sizeof(short) + 3 + (2 * sizeof(float)));
					const     int32_t variableLen = numBytes - fixedLen;
					const     int32_t  numUnitIDs = variableLen / sizeof(short); // each unitID is two bytes

					pckt >> aiID;
					pckt >> srcTeamID;
					pckt >> dstTeamID;
					pckt >> metalShare;
					pckt >> energyShare;

					CTeam* srcTeam = teamHandler.Team(srcTeamID);
					CTeam* dstTeam = teamHandler.Team(dstTeamID);

					if (metalShare > 0.0f) {
						if (eventHandler.AllowResourceTransfer(srcTeamID, dstTeamID, "m", metalShare)) {
							srcTeam->res.metal                       -= metalShare;
							srcTeam->resSent.metal                   += metalShare;
							srcTeam->GetCurrentStats().metalSent     += metalShare;
							dstTeam->res.metal                       += metalShare;
							dstTeam->resReceived.metal               += metalShare;
							dstTeam->GetCurrentStats().metalReceived += metalShare;
						}
					}
					if (energyShare > 0.0f) {
						if (eventHandler.AllowResourceTransfer(srcTeamID, dstTeamID, "e", energyShare)) {
							srcTeam->res.energy                       -= energyShare;
							srcTeam->resSent.energy                   += energyShare;
							srcTeam->GetCurrentStats().energySent     += energyShare;
							dstTeam->res.energy                       += energyShare;
							dstTeam->resReceived.energy               += energyShare;
							dstTeam->GetCurrentStats().energyReceived += energyShare;
						}
					}

					for (int32_t i = 0, j = fixedLen; i < numUnitIDs; i++, j += sizeof(short)) {
						int16_t unitID;
						pckt >> unitID;

						CUnit* u = unitHandler.GetUnit(unitID);

						if (u == nullptr)
							throw netcode::UnpackPacketException("Invalid unit ID");

						// ChangeTeam() handles the AllowUnitTransfer() LuaRule
						if (u->team == srcTeamID && !u->beingBuilt) {
							u->ChangeTeam(dstTeamID, CUnit::ChangeGiven);
						}
					}
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_AISHARE] exception \"%s\"", __func__, ex.what());
				}
			} break;


			case NETMSG_LOGMSG: {
				try {
					netcode::UnpackPacket unpack(packet, sizeof(uint8_t));

					std::uint16_t packetSize;
					std::uint8_t playerNum;
					std::uint8_t logLevel;
					std::string logString;

					unpack >> packetSize;
					if (packetSize != packet->length)
						throw netcode::UnpackPacketException("invalid size");

					unpack >> playerNum;
					if (!playerHandler.IsValidPlayer(playerNum))
						throw netcode::UnpackPacketException("invalid player number");

					unpack >> logLevel;
					unpack >> logString;

					const CPlayer* player = playerHandler.Player(playerNum);

					const char* fmtStr = "[Game::%s][LOGMSG] sender=\"%s\" string=\"%s\"";
					const char* logStr = logString.c_str();

					switch (logLevel) {
						case LOG_LEVEL_INFO   : { LOG_L(L_INFO   , fmtStr, __func__, player->name.c_str(), logStr); } break;
						case LOG_LEVEL_DEBUG  : { LOG_L(L_DEBUG  , fmtStr, __func__, player->name.c_str(), logStr); } break;
						case LOG_LEVEL_ERROR  : { LOG_L(L_ERROR  , fmtStr, __func__, player->name.c_str(), logStr); } break;
						case LOG_LEVEL_FATAL  : { LOG_L(L_FATAL  , fmtStr, __func__, player->name.c_str(), logStr); } break;
						case LOG_LEVEL_NOTICE : { LOG_L(L_NOTICE , fmtStr, __func__, player->name.c_str(), logStr); } break;
						case LOG_LEVEL_WARNING: { LOG_L(L_WARNING, fmtStr, __func__, player->name.c_str(), logStr); } break;
					}
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_LOGMSG] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_LUAMSG: {
				try {
					netcode::UnpackPacket unpack(packet, sizeof(uint8_t));

					std::uint16_t packetSize;
					std::uint8_t playerNum;
					std::uint16_t script;
					std::uint8_t mode;
					std::vector<std::uint8_t> data;

					unpack >> packetSize;
					if (packetSize != packet->length)
						throw netcode::UnpackPacketException("invalid packet-size");

					unpack >> playerNum;
					if (!playerHandler.IsValidPlayer(playerNum))
						throw netcode::UnpackPacketException("invalid player number");

					data.resize(packetSize - (1 + sizeof(packetSize) + sizeof(playerNum) + sizeof(script) + sizeof(mode)));

					unpack >> script;
					unpack >> mode;
					unpack >> data;

					CLuaHandle::HandleLuaMsg(playerNum, script, mode, data);
					AddTraffic(playerNum, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_LUAMSG] exception \"%s\"", __func__, ex.what());
				}
			} break;


			case NETMSG_SHARE: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_SHARE] invalid player-number %i", __func__, playerNum);
					break;
				}

				const int32_t srcTeamID = playerHandler.Player(playerNum)->team;
				const uint8_t dstTeamID = inbuf[2];

				CTeam* srcTeam = teamHandler.Team(srcTeamID);
				CTeam* dstTeam = teamHandler.Team(dstTeamID);

				const float metalShare  = Clamp(*reinterpret_cast<const float*>(&inbuf[4]), 0.0f, srcTeam->res.metal);
				const float energyShare = Clamp(*reinterpret_cast<const float*>(&inbuf[8]), 0.0f, srcTeam->res.energy);

				if (metalShare > 0.0f) {
					if (eventHandler.AllowResourceTransfer(srcTeamID, dstTeamID, "m", metalShare)) {
						srcTeam->res.metal                       -= metalShare;
						srcTeam->resSent.metal                   += metalShare;
						srcTeam->GetCurrentStats().metalSent     += metalShare;
						dstTeam->res.metal                       += metalShare;
						dstTeam->resReceived.metal               += metalShare;
						dstTeam->GetCurrentStats().metalReceived += metalShare;
					}
				}
				if (energyShare > 0.0f) {
					if (eventHandler.AllowResourceTransfer(srcTeamID, dstTeamID, "e", energyShare)) {
						srcTeam->res.energy                       -= energyShare;
						srcTeam->resSent.energy                   += energyShare;
						srcTeam->GetCurrentStats().energySent     += energyShare;
						dstTeam->res.energy                       += energyShare;
						dstTeam->resReceived.energy               += energyShare;
						dstTeam->GetCurrentStats().energyReceived += energyShare;
					}
				}

				if (static_cast<bool>(inbuf[3])) {
					// share units
					std::vector<int32_t >& netSelUnits = selectedUnitsHandler.netSelected[playerNum];

					for (const int32_t unitID: netSelUnits) {
						CUnit* unit = unitHandler.GetUnit(unitID);

						if (unit == nullptr)
							continue;
						if (unit->UnderFirstPersonControl())
							continue;
						// in godmode we can have units selected that are not ours
						if (unit->team != srcTeamID)
							continue;

						if (unit->isDead)
							continue;
						if (unit->IsCrashing())
							continue;

						unit->ChangeTeam(dstTeamID, CUnit::ChangeGiven);
					}

					netSelUnits.clear();
				}

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_SETSHARE: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_SETSHARE] invalid player-number %i", __func__, playerNum);
					break;
				}

				const uint8_t teamNum = inbuf[2];

				if (!teamHandler.IsValidTeam(teamNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_SETSHARE] invalid team-number %i", __func__, teamNum);
					break;
				}

				CTeam* team = teamHandler.Team(teamNum);

				const float metalShare = *reinterpret_cast<const float*>(&inbuf[3]);
				const float energyShare = *reinterpret_cast<const float*>(&inbuf[7]);

				if (eventHandler.AllowResourceLevel(teamNum, "m", metalShare))
					team->resShare.metal = metalShare;
				if (eventHandler.AllowResourceLevel(teamNum, "e", energyShare))
					team->resShare.energy = energyShare;

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_MAPDRAW: {
				const int32_t playerNum = inMapDrawer->GotNetMsg(packet);

				if (playerNum >= 0)
					AddTraffic(playerNum, packetCode, dataLength);

			} break;

			case NETMSG_TEAM: {
				const uint8_t playerNum = inbuf[1];
				const uint8_t teamAction = inbuf[2];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_TEAM] invalid player-number %i", __func__, playerNum);
					break;
				}

				CPlayer* player = playerHandler.Player(playerNum);

				switch (teamAction) {
					case TEAMMSG_GIVEAWAY: {
						const uint8_t toTeam = inbuf[3];
						const uint8_t giverTeam = inbuf[4];

						if (!teamHandler.IsValidTeam(toTeam) || !teamHandler.IsValidTeam(giverTeam)) {
							LOG_L(L_ERROR, "[Game::%s][TEAMMSG_GIVEWAY] invalid team-numbers {%i,%i}", __func__, toTeam, giverTeam);
							break;
						}

						const size_t numPlayersInGiverTeam      = playerHandler.NumActivePlayersInTeam(giverTeam);
						const size_t  numTotAIsInGiverTeam      = skirmishAIHandler.GetSkirmishAIsInTeam(giverTeam).size();
						const size_t  numControllersInGiverTeam = numPlayersInGiverTeam + numTotAIsInGiverTeam;

						bool giveAwayOk = false;

						if ((giveAwayOk = (giverTeam == player->team))) {
							// player is giving stuff from his own team
							if (numPlayersInGiverTeam == 1) {
								teamHandler.Team(giverTeam)->GiveEverythingTo(toTeam);
							} else {
								player->StartSpectating();
							}

							selectedUnitsHandler.ClearNetSelect(playerNum);
						} else {
							// player is giving stuff from one of his AI teams
							giveAwayOk = (numPlayersInGiverTeam == 0);
						}

						if (giveAwayOk && (numControllersInGiverTeam == 1)) {
							// team has no controller left now
							teamHandler.Team(giverTeam)->GiveEverythingTo(toTeam);
							teamHandler.Team(giverTeam)->SetLeader(-1);
						}

						CPlayer::UpdateControlledTeams();
					} break;
					case TEAMMSG_RESIGN: {
						if (!playing)
							break;

						player->StartSpectating();

						// update all teams of which the player is leader
						for (size_t t = 0; t < teamHandler.ActiveTeams(); ++t) {
							CTeam* team = teamHandler.Team(t);

							if (team->GetLeader() != playerNum)
								continue;

							const std::vector<int32_t>& teamPlayers = playerHandler.ActivePlayersInTeam(t);
							const std::vector<uint8_t>& teamAIs     = skirmishAIHandler.GetSkirmishAIsInTeam(t);

							if ((teamPlayers.size() + teamAIs.size()) == 0) {
								// no controllers left in team
								//team.active = false;
								team->SetLeader(-1);
							} else if (teamPlayers.empty()) {
								// no human player left in team
								team->SetLeader(skirmishAIHandler.GetSkirmishAI(teamAIs[0])->hostPlayer);
							} else {
								// still human controllers left in team
								team->SetLeader(teamPlayers[0]);
							}
						}

						LOG_L(L_DEBUG, "Player %i (%s) resigned and is now spectating!", playerNum, player->name.c_str());

						selectedUnitsHandler.ClearNetSelect(playerNum);
						CPlayer::UpdateControlledTeams();
					} break;

					case TEAMMSG_JOIN_TEAM: {
						const uint8_t newTeamNum = inbuf[3];

						if (!teamHandler.IsValidTeam(newTeamNum)) {
							LOG_L(L_ERROR, "[Game::%s][TEAMMSG_JOIN_TEAM] invalid team-number %i", __func__, newTeamNum);
							break;
						}

						teamHandler.Team(newTeamNum)->AddPlayer(playerNum);
					} break;
					case TEAMMSG_TEAM_DIED: {
						// silently drop since we can calculate this ourselves, although useful to store in replays
					} break;
					default: {
						LOG_L(L_ERROR, "[Game::%s][NETMSG_TEAM] unknown team-action %i from player %i", __func__, teamAction, playerNum);
					}
				}

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_GAMEOVER: {
				// silently drop since we can calculate this ourselves, although useful to store in replays
				AddTraffic(inbuf[1], packetCode, dataLength);
			} break;

			case NETMSG_TEAMSTAT: { /* LadderBot (dedicated client) only */ } break;
			case NETMSG_REQUEST_TEAMSTAT: { /* LadderBot (dedicated client) only */ } break;


			case NETMSG_AI_CREATED: {
				try {
					netcode::UnpackPacket pckt(packet, 2);
					std::string aiName;

					uint8_t playerNum;
					uint8_t     aiNum;
					uint8_t aiTeamNum;

					pckt >> playerNum;
					pckt >> aiNum;
					pckt >> aiTeamNum;
					pckt >> aiName;

					CTeam* aiTeam = teamHandler.Team(aiTeamNum);

					if (playerNum == gu->myPlayerNum) {
						// local player
						const SkirmishAIData& aiData = *(skirmishAIHandler.GetLocalSkirmishAIInCreation(aiTeamNum));

						if (skirmishAIHandler.IsActiveSkirmishAI(aiNum)) {
							#ifdef DEBUG
								// we will end up here for AIs defined in the start script
								const SkirmishAIData* curAIData = skirmishAIHandler.GetSkirmishAI(aiNum);
								assert((aiData.team == curAIData->team) && (aiData.name == curAIData->name) && (aiData.hostPlayer == curAIData->hostPlayer));
							#endif
						} else {
							// we will end up here for local AIs defined mid-game, eg. with /aicontrol
							wordCompletion.AddWord(aiData.name + " ", false, false, false);
							skirmishAIHandler.AddSkirmishAI(aiData, aiNum);
						}
					} else {
						SkirmishAIData aiData;
						aiData.hostPlayer = playerNum;
						aiData.team       = aiTeamNum;
						aiData.name       = aiName;
						aiData.shortName  = "n/a"; // determines validity for GetSkirmishAI

						wordCompletion.AddWord(aiData.name + " ", false, false, false);
						skirmishAIHandler.AddSkirmishAI(aiData, aiNum);
					}

					if (!aiTeam->HasLeader())
						aiTeam->SetLeader(playerNum);

					CPlayer::UpdateControlledTeams();
					eventHandler.PlayerChanged(playerNum);

					if (playerNum == gu->myPlayerNum) {
						LOG("[Game::%s] local skirmish AI being created for team %i ...", __func__, aiTeamNum);
						eoh->CreateSkirmishAI(aiNum);
					}
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_AI_CREATED] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_AI_STATE_CHANGED: {
				const uint8_t playerNum = inbuf[1];
				const uint8_t     aiNum = inbuf[2];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_AI_STATE_CHANGED] invalid player-number %i", __func__, playerNum);
					break;
				}

				SkirmishAIData* aiData           = skirmishAIHandler.GetSkirmishAI(aiNum);

				const ESkirmishAIStatus newState = (ESkirmishAIStatus) inbuf[3];
				const ESkirmishAIStatus oldState = aiData->status;

				const uint32_t aiTeamId          = aiData->team;

				const bool isLuaAI               = aiData->isLuaAI;
				const bool isLocal               = (aiData->hostPlayer == gu->myPlayerNum);

				const size_t numPlayersInAITeam  = playerHandler.NumActivePlayersInTeam(aiTeamId);
				const size_t numAIsInAITeam      = skirmishAIHandler.GetSkirmishAIsInTeam(aiTeamId).size();

				CTeam* aiTeam                    = teamHandler.Team(aiTeamId);

				aiData->status = newState;

				constexpr const char* types[] = {"remote", "local"};

				if (isLocal && !isLuaAI && ((newState == SKIRMAISTATE_DIEING) || (newState == SKIRMAISTATE_RELOADING))) {
					eoh->DestroySkirmishAI(aiNum);
					continue;
				}

				if (newState == SKIRMAISTATE_DEAD) {
					if (oldState == SKIRMAISTATE_RELOADING) {
						if (isLocal) {
							LOG("[Game::%s] %s skirmish AI \"%s\" being created for team %i", __func__, types[true], aiData->name.c_str(), aiTeamId);
							eoh->CreateSkirmishAI(aiNum);
						}
					} else {
						// this could be done in the above function as well, team has no controller left now
						if ((numPlayersInAITeam + numAIsInAITeam) == 1)
							aiTeam->SetLeader(-1);

						LOG("[Game::%s] %s skirmish AI \"%s\" (ID: %i) being removed from team %i", __func__, types[isLocal], aiData->name.c_str(), aiNum, aiTeamId);

						wordCompletion.RemoveWord(aiData->name + " ");
						skirmishAIHandler.RemoveSkirmishAI(aiNum);

						CPlayer::UpdateControlledTeams();
						eventHandler.PlayerChanged(playerNum);
					}

					continue;
				}

				if (newState == SKIRMAISTATE_ALIVE) {
					// short-name and version of the AI is unsynced data, only available on the AI host
					LOG(
						"[Game::%s] %s skirmish AI \"%s\" (ID: %i, Short-Name: \"%s\", Version: \"%s\") took over control of team %i",
						__func__, types[isLocal],
						isLocal? aiData->name.c_str(): "n/a", aiNum,
						isLocal? aiData->shortName.c_str(): "n/a",
						isLocal? aiData->version.c_str(): "n/a", aiTeamId
					);
				}
			} break;


			case NETMSG_ALLIANCE: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s] invalid player number %i in NETMSG_ALLIANCE", __func__, playerNum);
					break;
				}

				const bool allied = static_cast<bool>(inbuf[3]);

				const uint8_t whichAllyTeam = inbuf[2];
				const uint8_t fromAllyTeam = teamHandler.AllyTeam(playerHandler.Player(playerNum)->team);

				if (teamHandler.IsValidAllyTeam(whichAllyTeam) && fromAllyTeam != whichAllyTeam) {
					// FIXME NETMSG_ALLIANCE need to reset unit allyTeams
					// FIXME NETMSG_ALLIANCE need a call-in for AIs
					teamHandler.SetAlly(fromAllyTeam, whichAllyTeam, allied);

					// inform the players
					std::ostringstream msg;
					if (fromAllyTeam == gu->myAllyTeam) {
						msg << "Alliance: you have " << (allied ? "allied" : "unallied") << " allyteam " << whichAllyTeam << ".";
					} else if (whichAllyTeam == gu->myAllyTeam) {
						msg << "Alliance: allyteam " << whichAllyTeam << " has " << (allied ? "allied" : "unallied") <<  " with you.";
					} else {
						msg << "Alliance: allyteam " << whichAllyTeam << " has " << (allied ? "allied" : "unallied") <<  " with allyteam " << fromAllyTeam << ".";
					}
					LOG("%s", msg.str().c_str());

					// stop attacks against former foe
					if (allied) {
						for (CUnit* u: unitHandler.GetActiveUnits()) {
							if (teamHandler.Ally(u->allyteam, whichAllyTeam)) {
								u->StopAttackingAllyTeam(whichAllyTeam);
							}
						}
					}

					eventHandler.TeamChanged(playerHandler.Player(playerNum)->team);
				} else {
					LOG_L(L_WARNING, "Alliance: Player %i sent out wrong allyTeam index in alliance message", playerNum);
				}
			} break;

			case NETMSG_CCOMMAND: {
				try {
					CommandMessage msg(packet);

					ActionReceived(msg.GetAction(), msg.GetPlayerID());
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_CCOMMAND] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_DIRECT_CONTROL: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_DIRECT_CONTROL] invalid player-number %i", __func__, playerNum);
					break;
				}

				CPlayer* sender = playerHandler.Player(playerNum);

				if (sender->spectator || !sender->active)
					break;

				sender->StartControllingUnit();

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_DC_UPDATE: {
				const uint8_t playerNum = inbuf[1];

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "Invalid player number (%i) in NETMSG_DC_UPDATE", playerNum);
					break;
				}

				CPlayer* sender = playerHandler.Player(playerNum);
				sender->fpsController.RecvStateUpdate(inbuf);

				AddTraffic(playerNum, packetCode, dataLength);
			} break;

			case NETMSG_SETPLAYERNUM:
			case NETMSG_ATTEMPTCONNECT: {
				AddTraffic(-1, packetCode, dataLength);
			} break;

			// server sends this second to let us know about new clients that join midgame
			case NETMSG_CREATE_NEWPLAYER: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string name;

					uint8_t spectator;
					uint8_t team;
					uint8_t playerNum;

					// since the >> operator uses dest size to extract data from the packet, we need to use temp variables
					// of the same size of the packet, then convert to dest variable
					pckt >> playerNum;
					pckt >> spectator;
					pckt >> team;
					pckt >> name;

					CPlayer player;
					player.name = name;
					player.spectator = spectator;
					player.team = team;
					player.playerNum = playerNum;

					// add the new player
					playerHandler.AddPlayer(player);
					eventHandler.PlayerAdded(player.playerNum);

					LOG("[Game::%s] added new player %s with number %d to team %d", __func__, name.c_str(), player.playerNum, player.team);

					if (!player.spectator)
						eventHandler.TeamChanged(player.team);

					CDemoRecorder* record = clientNet->GetDemoRecorder();

					if (record != nullptr)
						record->AddNewPlayer(player.name, playerNum);

					AddTraffic(-1, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s][NETMSG_CREATE_NEWPLAYER] exception \"%s\"", __func__, ex.what());
				}
			} break;

			case NETMSG_CLIENTDATA: {
				if (!configHandler->GetBool("LogClientData"))
					break;

				constexpr uint8_t fixedSize = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t);

				netcode::UnpackPacket pckt(packet, 1);
				std::vector<std::uint8_t> blob;

				uint16_t totalSize;
				uint8_t playerNum;

				pckt >> totalSize;
				pckt >> playerNum;

				if (!playerHandler.IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "Invalid player number (%i) in NETMSG_CLIENTDATA", playerNum);
					break;
				}

				blob.resize(totalSize - fixedSize);
				pckt >> blob;

				const std::string clientData = ClientData::GetUncompressed(blob);

				if (clientData.empty()) {
					LOG_L(L_ERROR, "Corrupt Client Data received from player %d", playerNum);
					break;
				}

				CPlayer* sender = playerHandler.Player(playerNum);
				LOG("[Game::%s] Client Data for player %d (%s): \n%s", __func__, playerNum, sender->name.c_str(), clientData.c_str());

				AddTraffic(-1, packetCode, dataLength);
			} break;


			// if we received this packet here we are the local host player
			// (for which GetNumQueuedSimFrameMessages is not called where
			// it would normally be processed), so discard it
			case NETMSG_PING:
			case NETMSG_GAME_FRAME_PROGRESS: {
			} break;


			default: {
#ifdef SYNCDEBUG
				if (!CSyncDebugger::GetInstance()->ClientReceived(inbuf))
#endif
				{
					LOG_L(L_ERROR, "Unknown net msg received, packet code is %d."
							" A likely cause of this is network instability,"
							" which may happen in a WLAN, for example.",
							packetCode);
				}

				AddTraffic(-1, packetCode, dataLength);
			} break;
		}
	}
}

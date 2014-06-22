/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/Game.h"
#include "GameServer.h"

#ifdef _WIN32
#  include "winerror.h" // TODO someone on windows (MinGW? VS?) please check if this is required
#endif
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
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
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/TimeProfiler.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Net/UnpackPacket.h"
#include "System/Sound/ISound.h"

#include <boost/cstdint.hpp>

#define LOG_SECTION_NET "Net"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_NET)


static std::map<int, unsigned int> mySyncChecksums;


void CGame::AddTraffic(int playerID, int packetCode, int length)
{
	std::map<int, PlayerTrafficInfo>::iterator it = playerTraffic.find(playerID);
	if (it == playerTraffic.end()) {
		playerTraffic[playerID] = PlayerTrafficInfo();
		it = playerTraffic.find(playerID);
	}
	PlayerTrafficInfo& pti = it->second;
	pti.total += length;

	std::map<int, int>::iterator cit = pti.packets.find(packetCode);
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
			const float simProcUsage = (profiler.GetPercent("SimFrame"));
			const float drawProcUsage = (profiler.GetPercent("GameController::Draw") / std::max(1.0f, globalRendering->FPS)) * gu->minFPS;
			const float totalProcUsage = simProcUsage + drawProcUsage;

			// take the minimum drawframes into account, too
			net->Send(CBaseNetProtocol::Get().SendCPUUsage(totalProcUsage));
		} else {
			// the CPU-load percentage is undefined prior to SimFrame()
			net->Send(CBaseNetProtocol::Get().SendCPUUsage(0.0f));
		}
	}
}


unsigned int CGame::GetNumQueuedSimFrameMessages(unsigned int maxFrames) const
{
	// read ahead to find number of NETMSG_XXXFRAMES we still have to process
	// this number is effectively a measure of current user network conditions
	boost::shared_ptr<const netcode::RawPacket> packet;

	unsigned int numQueuedFrames = 0;
	unsigned int packetPeekIndex = 0;

	while ((packet = net->Peek(packetPeekIndex))) {
		switch (packet->data[0]) {
			case NETMSG_GAME_FRAME_PROGRESS: {
				// this special packet skips queue entirely, so gets processed here
				// it's meant to indicate current game progress for clients fast-forwarding to current point the game
				// NOTE: this event should be unsynced, since its time reference frame is not related to the current
				// progress of the game from the client's point of view
				//
				// send the event to lua call-in
				eventHandler.GameProgress(*(int*)(packet->data + 1));
				// pop it out of the net buffer
				net->DeleteBufferPacketAt(packetPeekIndex);
			} break;

			case NETMSG_NEWFRAME:
			case NETMSG_KEYFRAME:
				if (numQueuedFrames < maxFrames)
					++numQueuedFrames;
			default:
				++packetPeekIndex;
		}
	}

	return numQueuedFrames;
}

void CGame::UpdateNumQueuedSimFrames()
{
	if (gameServer != NULL)
		return;

	static spring_time lastUpdateTime = spring_gettime();

	const spring_time currTime = spring_gettime();
	const spring_time deltaTime = currTime - lastUpdateTime;

	if (globalConfig->useNetMessageSmoothingBuffer) {
		// update consumption-rate faster at higher game speeds
		if (deltaTime.toMilliSecsf() < (1000.0f / gs->speedFactor))
			return;

		// NOTE:
		//   unnecessary to scan entire queue *unless* joining a running game
		//   only reason in that case is to handle NETMSG_GAME_FRAME_PROGRESS
		//
		const unsigned int numQueuedFrames = GetNumQueuedSimFrameMessages(GAME_SPEED * gs->speedFactor * 5);
		// const unsigned int numQueuedFrames = GetNumQueuedSimFrameMessages(-1u);

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
		lastUpdateTime = currTime;
	} else {
		if (deltaTime.toMilliSecsf() < 1000.0f)
			return;

		// SPRING95
		// this uses no buffering which makes it too sensitive
		// to jitter (especially on lower-quality connections)
		consumeSpeedMult = GAME_SPEED * gs->speedFactor + (GetNumQueuedSimFrameMessages(-1u) / 2);
	}
}

void CGame::UpdateNetMessageProcessingTimeLeft()
{
	// compute new msgProcTimeLeft to "smooth" out SimFrame() calls
	if (gameServer == NULL) {
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
			//
			// don't let the limit get over 1000ms
			msgProcTimeLeft -= (1000.0f * float(msgProcTimeLeft > 1000.0f));
			msgProcTimeLeft += (consumeSpeedMult * deltaReadNetTime.toMilliSecsf());
		}

		lastReadNetTime = currentReadNetTime;
	} else {
		// ensure ClientReadNet returns at least every 15 simframes
		// so CGame can process keyboard input, and render etc.
		msgProcTimeLeft = (GAME_SPEED / float(gu->minFPS) * gs->wantedSpeedFactor) * 1000.0f;
	}
}

float CGame::GetNetMessageProcessingTimeLimit() const
{
	// balance the time spent in simulation & drawing (esp. when reconnecting)
	// use the following algo: i.e. with gu->reconnectSimDrawBalance = 0.2f
	//  -> try to spend minimum 20% of the time in drawing
	//  -> use remaining 80% for reconnecting
	// (maxSimFPS / minDrawFPS) is desired number of simframes per drawframe
	const float maxSimFPS    = (1.0f - gu->reconnectSimDrawBalance) * 1000.0f / std::max(0.01f, gu->avgSimFrameTime);
	const float minDrawFPS   =         gu->reconnectSimDrawBalance  * 1000.0f / std::max(0.01f, gu->avgDrawFrameTime);
	const float simDrawRatio = maxSimFPS / minDrawFPS;

	return Clamp(simDrawRatio * gu->avgSimFrameTime, 5.0f, 1000.0f / gu->minFPS);
}

void CGame::ClientReadNet()
{
	UpdateNetMessageProcessingTimeLeft();
	// look ahead so we can adapt consumeSpeedMult to network fluctuations
	UpdateNumQueuedSimFrames();

	const spring_time msgProcEndTime = spring_gettime() + spring_msecs(GetNetMessageProcessingTimeLimit());

	// really process the messages
	while (true) {
		// smooths simframes across the full second
		if (msgProcTimeLeft <= 0.0f)
			break;
		// balance the time spent in sim & drawing
		if (spring_gettime() > msgProcEndTime)
			break;

		lastNetPacketProcessTime = spring_gettime();

		// get netpacket from the queue
		boost::shared_ptr<const netcode::RawPacket> packet = net->GetData(gs->frameNum);

		if (!packet) {
			// LOG_SL(LOG_SECTION_NET, L_DEBUG, "Run out of netpackets!");
			break;
		}

		lastReceivedNetPacketTime = spring_gettime();

		const unsigned char* inbuf = packet->data;
		const unsigned dataLength = packet->length;
		const unsigned char packetCode = inbuf[0];

		switch (packetCode) {
			case NETMSG_QUIT: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string message;
					pckt >> message;

					LOG("%s", message.c_str());

					GameEnd(std::vector<unsigned char>());
					AddTraffic(-1, packetCode, dataLength);
					net->Close(true);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid QuitMessage: %s", ex.what());
				}
				break;
			}

			case NETMSG_PLAYERLEFT: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Got invalid player num (%i) in NETMSG_PLAYERLEFT", player);
					break;
				}
				playerHandler->PlayerLeft(player, inbuf[2]);
				eventHandler.PlayerRemoved(player, (int) inbuf[2]);

				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_STARTPLAYING: {
				unsigned timeToStart = *(unsigned*)(inbuf+1);
				if (timeToStart > 0) {
					GameSetupDrawer::StartCountdown(timeToStart);
				} else {
					StartPlaying();
				}
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERSTAT: {
				const unsigned char playerNum = inbuf[1];
				if (!playerHandler->IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "Got invalid player num %i in playerstat msg", playerNum);
					break;
				}

				CPlayer* player = playerHandler->Player(playerNum);
				player->currentStats = *reinterpret_cast<const PlayerStatistics*>(&inbuf[2]);

				if (gameOver) {
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL) {
						record->SetPlayerStats(playerNum, player->currentStats);
					}
				}
				AddTraffic(playerNum, packetCode, dataLength);
				break;
			}

			case NETMSG_PAUSE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Got invalid player num %i in pause msg", player);
					break;
				}
				gs->paused = !!inbuf[2];
				LOG("%s %s the game",
						playerHandler->Player(player)->name.c_str(),
						(gs->paused ? "paused" : "unpaused"));
				eventHandler.GamePaused(player, gs->paused);
				lastReadNetTime = spring_gettime();
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_INTERNAL_SPEED: {
				gs->speedFactor = *((float*) &inbuf[1]);
				sound->PitchAdjust(math::sqrt(gs->speedFactor));
				//LOG_L(L_DEBUG, "Internal speed set to %.2f", gs->speedFactor);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_USER_SPEED: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player) && player != SERVER_PLAYER) {
					LOG_L(L_ERROR, "Got invalid player num %i in user speed msg", player);
					break;
				}
				const char* pName = (player == SERVER_PLAYER)? "server": playerHandler->Player(player)->name.c_str();

				gs->wantedSpeedFactor = *((float*) &inbuf[2]);
				LOG("Speed set to %.1f [%s]", gs->wantedSpeedFactor, pName);
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_CPU_USAGE: {
				LOG_L(L_WARNING, "Game clients shouldn't get cpu usage msgs?");
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERINFO: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Got invalid player num %i in playerinfo msg", player);
					break;
				}
				playerHandler->Player(player)->cpuUsage = *(float*) &inbuf[2];
				playerHandler->Player(player)->ping = *(boost::uint32_t*) &inbuf[6];

				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERNAME: {
				try {
					netcode::UnpackPacket pckt(packet, 2);
					unsigned char playerID;
					pckt >> playerID;
					if (!playerHandler->IsValidPlayer(playerID))
						throw netcode::UnpackPacketException("Invalid player number");

					CPlayer* player = playerHandler->Player(playerID);
					pckt >> player->name;
					player->SetReadyToStart(gameSetup->startPosType != CGameSetup::StartPos_ChooseInGame);
					player->active = true;
					wordCompletion->AddWord(player->name, false, false, false); // required?
					AddTraffic(playerID, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Received an invalid net-message PlayerName: %s", ex.what());
				}
				break;
			}

			case NETMSG_CHAT: {
				try {
					ChatMessage msg(packet);

					HandleChatMsg(msg);
					AddTraffic(msg.fromPlayer, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid ChatMessage: %s", ex.what());
				}
				break;
			}

			case NETMSG_SYSTEMMSG: {
				try {
					netcode::UnpackPacket pckt(packet, 4);
					std::string sysMsg;
					pckt >> sysMsg;
					LOG("%s", sysMsg.c_str());
					AddTraffic(-1, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid SystemMessage: %s", ex.what());
				}
				break;
			}

			case NETMSG_STARTPOS: {
				const unsigned char playerID = inbuf[1];
				const unsigned int teamID = inbuf[2];
				const unsigned char rdyState = inbuf[3];

				if (!playerHandler->IsValidPlayer(playerID) && playerID != SERVER_PLAYER) {
					LOG_L(L_ERROR, "Got invalid player num %i in start pos msg", playerID);
					break;
				}

				if (!teamHandler->IsValidTeam(teamID)) {
					LOG_L(L_ERROR, "Got invalid team num %i in startpos msg", teamID);
				} else {
					float3 rawPickPos(*(float*) &inbuf[4], *(float*) &inbuf[8], *(float*) &inbuf[12]);
					float3 clampedPos(rawPickPos);

					CTeam* team = teamHandler->Team(teamID);
					team->ClampStartPosInStartBox(&clampedPos);

					if (eventHandler.AllowStartPosition(playerID, rdyState, clampedPos, rawPickPos)) {
						team->SetStartPos(clampedPos);

						if (playerID != SERVER_PLAYER) {
							playerHandler->Player(playerID)->SetReadyToStart(rdyState != CPlayer::PLAYER_RDYSTATE_UPDATED);
						}
					}
				}

				AddTraffic(playerID, packetCode, dataLength);
				break;
			}

			case NETMSG_RANDSEED: {
				gs->SetRandSeed(*((unsigned int*)&inbuf[1]), true);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_GAMEID: {
				const unsigned char* p = &inbuf[1];
				CDemoRecorder* record = net->GetDemoRecorder();
				if (record != NULL) {
					record->SetGameID(p);
				}
				memcpy(gameID, p, sizeof(gameID));
				LOG("GameID: "
						"%02x%02x%02x%02x%02x%02x%02x%02x"
						"%02x%02x%02x%02x%02x%02x%02x%02x",
						p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
						p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);
				AddTraffic(-1, packetCode, dataLength);
				eventHandler.GameID(gameID, sizeof(gameID));
				break;
			}

			case NETMSG_PATH_CHECKSUM: {
				const unsigned char playerNum = inbuf[1];
				if (!playerHandler->IsValidPlayer(playerNum)) {
					LOG_L(L_ERROR, "Got invalid player num %i in path checksum msg", playerNum);
					break;
				}

				const boost::uint32_t playerCheckSum = *(boost::uint32_t*) &inbuf[2];
				const boost::uint32_t localCheckSum = pathManager->GetPathCheckSum();
				const CPlayer* player = playerHandler->Player(playerNum);

				if (playerCheckSum == 0) {
					LOG_L(L_WARNING, // XXX maybe use a "Desync" section here?
							"[DESYNC WARNING] path-checksum for player %d (%s) is 0; non-writable PathEstimator-cache?",
							playerNum, player->name.c_str());
				} else {
					if (playerCheckSum != localCheckSum) {
						LOG_L(L_WARNING, // XXX maybe use a "Desync" section here?
								"[DESYNC WARNING] path-checksum %08x for player %d (%s)"
								" does not match local checksum %08x; stale PathEstimator-cache?",
								playerCheckSum, playerNum, player->name.c_str(), localCheckSum);
					}
				}
			} break;

			case NETMSG_KEYFRAME: {
				const int serverFrameNum = *(int*)(inbuf + 1);

				if (gs->frameNum != (serverFrameNum - 1)) {
					LOG_L(L_ERROR, "[%s] keyframe difference: %i (client: %d, server: %d)", __FUNCTION__, gs->frameNum - (serverFrameNum - 1), gs->frameNum, serverFrameNum);
					break;
				}

				// fall-through and run SimFrame() iff this message really came from the server
				net->Send(CBaseNetProtocol::Get().SendKeyFrame(serverFrameNum));
			}
			case NETMSG_NEWFRAME: {
				msgProcTimeLeft -= 1000.0f;
				lastSimFrameNetPacketTime = spring_gettime();

				SimFrame();

#ifdef SYNCCHECK
				// both NETMSG_SYNCRESPONSE and NETMSG_NEWFRAME are used for ping calculation by server
				ASSERT_SYNCED(gs->frameNum);
				ASSERT_SYNCED(CSyncChecker::GetChecksum());
				net->Send(CBaseNetProtocol::Get().SendSyncResponse(gu->myPlayerNum, gs->frameNum, CSyncChecker::GetChecksum()));

				if (gameServer != NULL && gameServer->GetDemoReader() != NULL) {
					// buffer all checksums, so we can check sync later between demo & local
					mySyncChecksums[gs->frameNum] = CSyncChecker::GetChecksum();
				}

				if ((gs->frameNum & 4095) == 0) {
					// reset checksum every 4096 frames =~ 2.5 minutes
					CSyncChecker::NewFrame();
				}
#endif
				AddTraffic(-1, packetCode, dataLength);

				if (videoCapturing->IsCapturing()) {
					return;
				}
			} break;

			case NETMSG_SYNCRESPONSE: {
#if (defined(SYNCCHECK))
				if (gameServer != NULL && gameServer->GetDemoReader() != NULL) {
					// NOTE:
					//   this packet is also sent during live games,
					//   during which we should just ignore it (the
					//   server does sync-checking for us)
					netcode::UnpackPacket pckt(packet, 1);

					unsigned char playerNum; pckt >> playerNum;
						  int  frameNum; pckt >> frameNum;
					unsigned  int  checkSum; pckt >> checkSum;

					const unsigned int ourCheckSum = mySyncChecksums[frameNum];

					const char* fmtStr =
						"[DESYNC_WARNING] checksum %x from player %d (%s)"
						" does not match our checksum %x for frame-number %d";
					const CPlayer* player = playerHandler->Player(playerNum);

					// check if our checksum for this frame matches what
					// player <playerNum> sent to the server at the same
					// frame in the original game (in case of a demo)
					if (playerNum == gu->myPlayerNum) { break; }
					if (checkSum == ourCheckSum) { break; }

					LOG_L(L_ERROR, fmtStr, checkSum, playerNum, player->name.c_str(), ourCheckSum, frameNum);
				}
#endif
			} break;


			case NETMSG_COMMAND: {
				try {
					netcode::UnpackPacket pckt(packet, 1);

					unsigned short packetSize; pckt >> packetSize;
					unsigned char playerNum; pckt >> playerNum;
					const unsigned int numParams = (packetSize - 9) / sizeof(float);

					if (!playerHandler->IsValidPlayer(playerNum))
						throw netcode::UnpackPacketException("Invalid player number");

					int cmdID;
					unsigned char cmdOpt;
					pckt >> cmdID;
					pckt >> cmdOpt;

					Command c(cmdID, cmdOpt);
					c.params.reserve(numParams);

					for (int a = 0; a < numParams; ++a) {
						float param; pckt >> param;
						c.PushParam(param);
					}

					selectedUnitsHandler.NetOrder(c, playerNum);
					AddTraffic(playerNum, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid Command: %s", ex.what());
				}

				break;
			}

			case NETMSG_SELECT: {
				try {
					netcode::UnpackPacket pckt(packet, 1);

					unsigned short packetSize; pckt >> packetSize;
					unsigned char playerNum; pckt >> playerNum;
					const unsigned int numUnitIDs = (packetSize - 4) / sizeof(short int);

					if (!playerHandler->IsValidPlayer(playerNum)) {
						throw netcode::UnpackPacketException("Invalid player number");
					}

					std::vector<int> selectedUnitIDs;
					selectedUnitIDs.reserve(numUnitIDs);

					for (int a = 0; a < numUnitIDs; ++a) {
						short int unitID; pckt >> unitID;
						const CUnit* unit = unitHandler->GetUnit(unitID);

						if (unit == NULL) {
							// unit was destroyed in simulation (without its ID being recycled)
							// after sending a command but before receiving it back, more likely
							// to happen in high-latency situations
							// LOG_L(L_WARNING, "[NETMSG_SELECT] invalid unitID (%i) from player %i", unitID, playerNum);
							continue;
						}

						// if in godMode, this is always true for any player
						if (playerHandler->Player(playerNum)->CanControlTeam(unit->team)) {
							selectedUnitIDs.push_back(unitID);
						}
					}

					selectedUnitsHandler.NetSelect(selectedUnitIDs, playerNum);
					AddTraffic(playerNum, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid Select: %s", ex.what());
				}

				break;
			}

			case NETMSG_AICOMMAND:
			case NETMSG_AICOMMAND_TRACKED: {
				try {
					netcode::UnpackPacket pckt(packet, 1);
					short int psize;
					pckt >> psize;
					unsigned char player;
					pckt >> player;
					unsigned char aiID;
					pckt >> aiID;

					if (!playerHandler->IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					short int unitid;
					pckt >> unitid;
					if (unitid < 0 || static_cast<size_t>(unitid) >= unitHandler->MaxUnits())
						throw netcode::UnpackPacketException("Invalid unit ID");

					int cmd_id;
					unsigned char cmd_opt;
					pckt >> cmd_id;
					pckt >> cmd_opt;

					Command c(cmd_id, cmd_opt);
					if (packetCode == NETMSG_AICOMMAND_TRACKED) {
						pckt >> c.aiCommandId;
					}

					// insert the command parameters
					for (int a = 0; a < ((psize - 11) / 4); ++a) {
						float param;
						pckt >> param;
						c.PushParam(param);
					}

					selectedUnitsHandler.AiOrder(unitid, c, player);
					AddTraffic(player, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid AICommand: %s", ex.what());
				}
				break;
			}

			case NETMSG_AICOMMANDS: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					unsigned char player;
					pckt >> player;
					if (!playerHandler->IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");
					unsigned char aiID;
					pckt >> aiID;
					unsigned char pairwise;
					pckt >> pairwise;
					unsigned int sameCmdID;
					pckt >> sameCmdID;
					unsigned char sameCmdOpt;
					pckt >> sameCmdOpt;
					unsigned short sameCmdParamSize;
					pckt >> sameCmdParamSize;
					// parse the unit list
					vector<int> unitIDs;
					short int unitCount;
					pckt >> unitCount;
					for (int u = 0; u < unitCount; u++) {
						short int unitID;
						pckt >> unitID;
						unitIDs.push_back(unitID);
					}
					// parse the command list
					vector<Command> commands;
					short int commandCount;
					pckt >> commandCount;
					for (int c = 0; c < commandCount; c++) {
						int cmd_id;
						unsigned char cmd_opt;
						if (sameCmdID == 0)
							pckt >> cmd_id;
						else
							cmd_id = sameCmdID;
						if (sameCmdOpt == 0xFF)
							pckt >> cmd_opt;
						else
							cmd_opt = sameCmdOpt;

						Command cmd(cmd_id, cmd_opt);
						short int paramCount;
						if (sameCmdParamSize == 0xFFFF)
							pckt >> paramCount;
						else
							paramCount = sameCmdParamSize;
						for (int p = 0; p < paramCount; p++) {
							float param;
							pckt >> param;
							cmd.PushParam(param);
						}
						commands.push_back(cmd);
					}
					// apply the commands
					if (pairwise) {
						for (int x = 0; x < std::min(unitCount, commandCount); ++x) {
							selectedUnitsHandler.AiOrder(unitIDs[x], commands[x], player);
						}
					}
					else {
						for (int c = 0; c < commandCount; c++) {
							for (int u = 0; u < unitCount; u++) {
								selectedUnitsHandler.AiOrder(unitIDs[u], commands[c], player);
							}
						}
					}
					AddTraffic(player, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid AICommands: %s", ex.what());
				}
				break;
			}

			case NETMSG_AISHARE: {
				try {
					netcode::UnpackPacket pckt(packet, 1);
					short int numBytes;
					pckt >> numBytes;
					unsigned char player;
					pckt >> player;
					if (!playerHandler->IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");
					unsigned char aiID;
					pckt >> aiID;

					// total message length
					const int fixedLen = (1 + sizeof(short) + 3 + (2 * sizeof(float)));
					const int variableLen = numBytes - fixedLen;
					const int numUnitIDs = variableLen / sizeof(short); // each unitID is two bytes
					unsigned char srcTeam;
					pckt >> srcTeam;
					unsigned char dstTeam;
					pckt >> dstTeam;
					float metalShare;
					pckt >> metalShare;
					float energyShare;
					pckt >> energyShare;

					if (metalShare > 0.0f) {
						if (eventHandler.AllowResourceTransfer(srcTeam, dstTeam, "m", metalShare)) {
							teamHandler->Team(srcTeam)->metal -= metalShare;
							teamHandler->Team(dstTeam)->metal += metalShare;
						}
					}
					if (energyShare > 0.0f) {
						if (eventHandler.AllowResourceTransfer(srcTeam, dstTeam, "e", energyShare)) {
							teamHandler->Team(srcTeam)->energy -= energyShare;
							teamHandler->Team(dstTeam)->energy += energyShare;
						}
					}

					for (int i = 0, j = fixedLen;  i < numUnitIDs;  i++, j += sizeof(short)) {
						short int unitID;
						pckt >> unitID;
						if (unitID >= unitHandler->MaxUnits() || unitID < 0)
							throw netcode::UnpackPacketException("Invalid unit ID");

						CUnit* u = unitHandler->units[unitID];
						// ChangeTeam() handles the AllowUnitTransfer() LuaRule
						if (u && u->team == srcTeam && !u->beingBuilt) {
							u->ChangeTeam(dstTeam, CUnit::ChangeGiven);
						}
					}
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid AIShare: %s", ex.what());
				}
				break;
			}

			case NETMSG_LUAMSG: {
				try {
					netcode::UnpackPacket unpack(packet, 1);
					boost::uint16_t size;
					unpack >> size;
					if (size != packet->length)
						throw netcode::UnpackPacketException("Invalid size");
					boost::uint8_t playerNum;
					unpack >> playerNum;
					if (!playerHandler->IsValidPlayer(playerNum))
						throw netcode::UnpackPacketException("Invalid player number");
					boost::uint16_t script;
					unpack >> script;
					boost::uint8_t mode;
					unpack >> mode;
					std::vector<boost::uint8_t> data(size - 7);
					unpack >> data;

					CLuaHandle::HandleLuaMsg(playerNum, script, mode, data);
					AddTraffic(playerNum, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid LuaMsg: %s", ex.what());
				}
				break;
			}

			case NETMSG_SHARE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Got invalid player num %i in share msg", player);
					break;
				}
				const int srcTeamID = playerHandler->Player(player)->team;
				const int dstTeamID = inbuf[2];
				const bool shareUnits = !!inbuf[3];
				CTeam* srcTeam = teamHandler->Team(srcTeamID);
				CTeam* dstTeam = teamHandler->Team(dstTeamID);
				const float metalShare  = Clamp(*(float*)&inbuf[4], 0.0f, (float)srcTeam->metal);
				const float energyShare = Clamp(*(float*)&inbuf[8], 0.0f, (float)srcTeam->energy);

				if (metalShare > 0.0f) {
					if (eventHandler.AllowResourceTransfer(srcTeamID, dstTeamID, "m", metalShare)) {
						srcTeam->metal                       -= metalShare;
						srcTeam->metalSent                   += metalShare;
						srcTeam->currentStats->metalSent     += metalShare;
						dstTeam->metal                       += metalShare;
						dstTeam->metalReceived               += metalShare;
						dstTeam->currentStats->metalReceived += metalShare;
					}
				}
				if (energyShare > 0.0f) {
					if (eventHandler.AllowResourceTransfer(srcTeamID, dstTeamID, "e", energyShare)) {
						srcTeam->energy                       -= energyShare;
						srcTeam->energySent                   += energyShare;
						srcTeam->currentStats->energySent     += energyShare;
						dstTeam->energy                       += energyShare;
						dstTeam->energyReceived               += energyShare;
						dstTeam->currentStats->energyReceived += energyShare;
					}
				}

				if (shareUnits) {
					vector<int>& netSelUnits = selectedUnitsHandler.netSelected[player];
					vector<int>::const_iterator ui;

					for (ui = netSelUnits.begin(); ui != netSelUnits.end(); ++ui) {
						CUnit* unit = unitHandler->GetUnit(*ui);

						if (unit == NULL)
							continue;
						if (unit->UnderFirstPersonControl())
							continue;
						// in godmode we can have units selected that are not ours
						if (unit->team != srcTeamID)
							continue;

						if (unit->isDead)
							continue;
						if (unit->beingBuilt)
							continue; // why?
						if (unit->IsStunned() || unit->IsCrashing())
							continue;

						unit->ChangeTeam(dstTeamID, CUnit::ChangeGiven);
					}

					netSelUnits.clear();
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SETSHARE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Got invalid player num %i in setshare msg", player);
					break;
				}
				const unsigned char team = inbuf[2];
				if (!teamHandler->IsValidTeam(team)) {
					LOG_L(L_ERROR, "Got invalid team num %i in setshare msg", team);
					break;
				}
				float metalShare=*(float*)&inbuf[3];
				float energyShare=*(float*)&inbuf[7];

				if (eventHandler.AllowResourceLevel(team, "m", metalShare)) {
					teamHandler->Team(team)->metalShare = metalShare;
				}
				if (eventHandler.AllowResourceLevel(team, "e", energyShare)) {
					teamHandler->Team(team)->energyShare = energyShare;
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_MAPDRAW: {
				int player = inMapDrawer->GotNetMsg(packet);
				if (player >= 0)
					AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_TEAM: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Got invalid player num %i in team msg", player);
					break;
				}

				const unsigned char action = inbuf[2];
				const int fromTeam = playerHandler->Player(player)->team;

				switch (action) {
					case TEAMMSG_GIVEAWAY: {
						const unsigned char toTeam = inbuf[3];
						const unsigned char giverTeam = inbuf[4];

						if (!teamHandler->IsValidTeam(toTeam) || !teamHandler->IsValidTeam(giverTeam)) {
							LOG_L(L_ERROR, "Got invalid team nums %i %i in team giveaway msg", toTeam, giverTeam);
							break;
						}

						const int numPlayersInGiverTeam        = playerHandler->ActivePlayersInTeam(giverTeam).size();
						const size_t numTotAIsInGiverTeam      = skirmishAIHandler.GetSkirmishAIsInTeam(giverTeam).size();
						const size_t numControllersInGiverTeam = numPlayersInGiverTeam + numTotAIsInGiverTeam;

						bool giveAwayOk = false;

						if (giverTeam == fromTeam) {
							// player is giving stuff from his own team
							giveAwayOk = true;

							if (numPlayersInGiverTeam == 1) {
								teamHandler->Team(giverTeam)->GiveEverythingTo(toTeam);
							} else {
								playerHandler->Player(player)->StartSpectating();
							}

							selectedUnitsHandler.ClearNetSelect(player);
						} else {
							// player is giving stuff from one of his AI teams
							if (numPlayersInGiverTeam == 0) {
								giveAwayOk = true;
							}
						}
						if (giveAwayOk && (numControllersInGiverTeam == 1)) {
							// team has no controller left now
							teamHandler->Team(giverTeam)->GiveEverythingTo(toTeam);
							teamHandler->Team(giverTeam)->SetLeader(-1);
						}
						CPlayer::UpdateControlledTeams();
						break;
					}
					case TEAMMSG_RESIGN: {
						if (!playing)
							break;

						playerHandler->Player(player)->StartSpectating();

						// update all teams of which the player is leader
						for (size_t t = 0; t < teamHandler->ActiveTeams(); ++t) {
							CTeam* team = teamHandler->Team(t);

							if (team->GetLeader() != player)
								continue;

							const std::vector<int> &teamPlayers = playerHandler->ActivePlayersInTeam(t);
							const std::vector<unsigned char> &teamAIs  = skirmishAIHandler.GetSkirmishAIsInTeam(t);

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
						LOG_L(L_DEBUG, "Player %i (%s) resigned and is now spectating!",
								player,
								playerHandler->Player(player)->name.c_str());
						selectedUnitsHandler.ClearNetSelect(player);
						CPlayer::UpdateControlledTeams();
						break;
					}
					case TEAMMSG_JOIN_TEAM: {
						const unsigned char newTeam = inbuf[3];
						if (!teamHandler->IsValidTeam(newTeam)) {
							LOG_L(L_ERROR, "Got invalid team num %i in team join msg", newTeam);
							break;
						}

						teamHandler->Team(newTeam)->AddPlayer(player);
						break;
					}
					case TEAMMSG_TEAM_DIED: {
						// silently drop since we can calculate this ourself, altho it's useful info to store in replays
						break;
					}
					default: {
						LOG_L(L_ERROR, "Unknown action in NETMSG_TEAM (%i) from player %i", action, player);
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_GAMEOVER: {
				const unsigned char player = inbuf[1];
				// silently drop since we can calculate this ourself, altho it's useful info to store in replays
				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_TEAMSTAT: { /* LadderBot (dedicated client) only */ } break;
			case NETMSG_REQUEST_TEAMSTAT: { /* LadderBot (dedicated client) only */ } break;

			case NETMSG_AI_CREATED: {
				try {
					netcode::UnpackPacket pckt(packet, 2);
					unsigned char playerId;
					pckt >> playerId;
					unsigned char skirmishAIId;
					pckt >> skirmishAIId;
					unsigned char aiTeamId;
					pckt >> aiTeamId;
					std::string aiName;
					pckt >> aiName;
					CTeam* tai = teamHandler->Team(aiTeamId);
					const unsigned isLocal = (playerId == gu->myPlayerNum);

					if (isLocal) {
						const SkirmishAIData& aiData = *(skirmishAIHandler.GetLocalSkirmishAIInCreation(aiTeamId));
						if (skirmishAIHandler.IsActiveSkirmishAI(skirmishAIId)) {
							#ifdef DEBUG
								// we will end up here for AIs defined in the start script
								const SkirmishAIData* curAIData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
								assert((aiData.team == curAIData->team) && (aiData.name == curAIData->name) && (aiData.hostPlayer == curAIData->hostPlayer));
							#endif
						} else {
							// we will end up here for local AIs defined mid-game,
							// eg. with /aicontrol
							const std::string aiName = aiData.name + " "; // aiData would be invalid after the next line
							skirmishAIHandler.AddSkirmishAI(aiData, skirmishAIId);
							wordCompletion->AddWord(aiName, false, false, false);
						}
					} else {
						SkirmishAIData aiData;
						aiData.team       = aiTeamId;
						aiData.name       = aiName;
						aiData.hostPlayer = playerId;
						skirmishAIHandler.AddSkirmishAI(aiData, skirmishAIId);
						wordCompletion->AddWord(aiData.name + " ", false, false, false);
					}

					if (!tai->HasLeader()) {
						tai->SetLeader(playerId);
					}
					CPlayer::UpdateControlledTeams();
					eventHandler.PlayerChanged(playerId);
					if (isLocal) {
						LOG("Skirmish AI being created for team %i ...", aiTeamId);
						eoh->CreateSkirmishAI(skirmishAIId);
					}
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid AICreated: %s", ex.what());
				}
				break;
			}
			case NETMSG_AI_STATE_CHANGED: {
				const unsigned char playerId     = inbuf[1];
				if (!playerHandler->IsValidPlayer(playerId)) {
					LOG_L(L_ERROR, "Got invalid player num %i in ai state changed msg", playerId);
					break;
				}

				const unsigned char skirmishAIId  = inbuf[2];
				const ESkirmishAIStatus newState = (ESkirmishAIStatus) inbuf[3];
				SkirmishAIData* aiData           = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
				const ESkirmishAIStatus oldState = aiData->status;
				const unsigned aiTeamId          = aiData->team;
				const bool isLuaAI               = aiData->isLuaAI;
				const unsigned isLocal           = (aiData->hostPlayer == gu->myPlayerNum);
				const size_t numPlayersInAITeam  = playerHandler->ActivePlayersInTeam(aiTeamId).size();
				const size_t numAIsInAITeam      = skirmishAIHandler.GetSkirmishAIsInTeam(aiTeamId).size();
				CTeam* tai                       = teamHandler->Team(aiTeamId);

				aiData->status = newState;

				if (isLocal && !isLuaAI && ((newState == SKIRMAISTATE_DIEING) || (newState == SKIRMAISTATE_RELOADING))) {
					eoh->DestroySkirmishAI(skirmishAIId);
				} else if (newState == SKIRMAISTATE_DEAD) {
					if (oldState == SKIRMAISTATE_RELOADING) {
						if (isLocal) {
							LOG("Skirmish AI \"%s\" being reloaded for team %i ...",
									aiData->name.c_str(), aiTeamId);
							eoh->CreateSkirmishAI(skirmishAIId);
						}
					} else {
						const std::string aiInstanceName = aiData->name;
						wordCompletion->RemoveWord(aiData->name + " ");
						skirmishAIHandler.RemoveSkirmishAI(skirmishAIId);
						aiData = NULL; // not valid anymore after RemoveSkirmishAI()
						// this could be done in the above function as well
						if ((numPlayersInAITeam + numAIsInAITeam) == 1) {
							// team has no controller left now
							tai->SetLeader(-1);
						}
						CPlayer::UpdateControlledTeams();
						eventHandler.PlayerChanged(playerId);
						LOG("Skirmish AI \"%s\" (ID:%i), which controlled team %i is now dead",
								aiInstanceName.c_str(), skirmishAIId, aiTeamId);
					}
				} else if (newState == SKIRMAISTATE_ALIVE) {
					if (isLocal) {
						// short-name and version of the AI is unsynced data
						// -> only available on the AI host
						LOG("Skirmish AI \"%s\" (ID:%i, Short-Name:\"%s\", "
								"Version:\"%s\") took over control of team %i",
								aiData->name.c_str(), skirmishAIId,
								aiData->shortName.c_str(),
								aiData->version.c_str(), aiTeamId);
					} else {
						LOG("Skirmish AI \"%s\" (ID:%i) took over control of team %i",
								aiData->name.c_str(), skirmishAIId, aiTeamId);
					}
				}
				break;
			}
			case NETMSG_ALLIANCE: {
				const unsigned char player = inbuf[1];

				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "[Game::%s] invalid player number %i in NETMSG_ALLIANCE", __FUNCTION__, player);
					break;
				}

				const bool allied = static_cast<bool>(inbuf[3]);
				const unsigned char whichAllyTeam = inbuf[2];
				const unsigned char fromAllyTeam = teamHandler->AllyTeam(playerHandler->Player(player)->team);

				if (teamHandler->IsValidAllyTeam(whichAllyTeam) && fromAllyTeam != whichAllyTeam) {
					// FIXME NETMSG_ALLIANCE need to reset unit allyTeams
					// FIXME NETMSG_ALLIANCE need a call-in for AIs
					teamHandler->SetAlly(fromAllyTeam, whichAllyTeam, allied);

					// inform the players
					std::ostringstream msg;
					if (fromAllyTeam == gu->myAllyTeam) {
						msg << "Alliance: you have " << (allied ? "allied" : "unallied")
							<< " allyteam " << whichAllyTeam << ".";
					} else if (whichAllyTeam == gu->myAllyTeam) {
						msg << "Alliance: allyteam " << whichAllyTeam << " has "
							<< (allied ? "allied" : "unallied") <<  " with you.";
					} else {
						msg << "Alliance: allyteam " << whichAllyTeam << " has "
							<< (allied ? "allied" : "unallied")
							<<  " with allyteam " << fromAllyTeam << ".";
					}
					LOG("%s", msg.str().c_str());

					// stop attacks against former foe
					if (allied) {
						const auto& units = unitHandler->activeUnits;

						for (auto it = units.begin(); it != units.end(); ++it) {
							if (teamHandler->Ally((*it)->allyteam, whichAllyTeam)) {
								(*it)->StopAttackingAllyTeam(whichAllyTeam);
							}
						}
					}
					eventHandler.TeamChanged(playerHandler->Player(player)->team);
				} else {
					LOG_L(L_WARNING, "Alliance: Player %i sent out wrong allyTeam index in alliance message", player);
				}
				break;
			}
			case NETMSG_CCOMMAND: {
				try {
					CommandMessage msg(packet);

					ActionReceived(msg.GetAction(), msg.GetPlayerID());
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "Got invalid CommandMessage: %s", ex.what());
				}
				break;
			}

			case NETMSG_DIRECT_CONTROL: {
				const unsigned char player = inbuf[1];

				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Invalid player number (%i) in NETMSG_DIRECT_CONTROL", player);
					break;
				}

				CPlayer* sender = playerHandler->Player(player);
				if (sender->spectator || !sender->active) {
					break;
				}

				sender->StartControllingUnit();

				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_DC_UPDATE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					LOG_L(L_ERROR, "Invalid player number (%i) in NETMSG_DC_UPDATE", player);
					break;
				}

				CPlayer* sender = playerHandler->Player(player);
				sender->fpsController.RecvStateUpdate(inbuf);

				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_SETPLAYERNUM:
			case NETMSG_ATTEMPTCONNECT: {
				AddTraffic(-1, packetCode, dataLength);
				break;
			}
			case NETMSG_CREATE_NEWPLAYER: { // server sends this second to let us know about new clients that join midgame
				try {
					netcode::UnpackPacket pckt(packet, 3);
					unsigned char spectator, team, playerNum;
					std::string name;

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
					// TODO NETMSG_CREATE_NEWPLAYER perhaps add a lua hook; hook should be able to reassign the player to a team and/or create a new team/allyteam
					playerHandler->AddPlayer(player);
					eventHandler.PlayerAdded(player.playerNum);

					LOG("[Game::%s] added new player %s with number %d to team %d", __FUNCTION__, name.c_str(), player.playerNum, player.team);

					if (!player.spectator) {
						eventHandler.TeamChanged(player.team);
					}

					AddTraffic(-1, packetCode, dataLength);
				} catch (const netcode::UnpackPacketException& ex) {
					LOG_L(L_ERROR, "[Game::%s] invalid NETMSG_CREATE_NEWPLAYER: %s", __FUNCTION__, ex.what());
				}
				break;
			}

			// if we received this packet here we are the host player
			// (meaning the message was not processed), so discard it
			case NETMSG_GAME_FRAME_PROGRESS: {
				break;
			}
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
				break;
			}
		}
	}
}

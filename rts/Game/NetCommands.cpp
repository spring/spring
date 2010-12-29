/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "Game.h"
#include "CameraHandler.h"
#include "GameServer.h"
#include "CommandMessage.h"
#include "GameSetup.h"
#include "SelectedUnits.h"
#include "PlayerHandler.h"
#include "ChatMessage.h"
#include "TimeProfiler.h"
#include "WordCompletion.h"
#include "IVideoCapturing.h"
#include "Game/UI/UnitTracker.h"
#ifdef _WIN32
#  include "winerror.h"
#endif
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/InMapDraw.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "UI/GameSetupDrawer.h"
#include "UI/LuaUI.h"
#include "UI/MouseHandler.h"
#include "System/EventHandler.h"
#include "System/FPUCheck.h"
#include "System/myMath.h"
#include "System/NetProtocol.h"
#include "System/LoadSave/DemoRecorder.h"
#include "System/Net/UnpackPacket.h"
#include "System/Sound/ISound.h"

#include <boost/cstdint.hpp>

void CGame::ClientReadNet()
{
	if (gu->gameTime - lastCpuUsageTime >= 1) {
		lastCpuUsageTime = gu->gameTime;

		if (playing) {
			net->Send(CBaseNetProtocol::Get().SendCPUUsage(profiler.GetPercent("CPU load")));
#if defined(USE_GML) && GML_ENABLE_SIM
			net->Send(CBaseNetProtocol::Get().SendLuaDrawTime(gu->myPlayerNum, luaDrawTime));
#endif
		} else {
			// the CPU-load percentage is undefined prior to SimFrame()
			net->Send(CBaseNetProtocol::Get().SendCPUUsage(0.0f));
		}
	}

	boost::shared_ptr<const netcode::RawPacket> packet;

	// compute new timeLeft to "smooth" out SimFrame() calls
	if (!gameServer) {
		const unsigned int currentFrame = SDL_GetTicks();

		if (timeLeft > 1.0f)
			timeLeft -= 1.0f;
		timeLeft += consumeSpeed * ((float)(currentFrame - lastframe) / 1000.f);
		if (skipping)
			timeLeft = 0.01f;
		lastframe = currentFrame;

		// read ahead to calculate the number of NETMSG_NEWFRAMES
		// we still have to process (in variable "que")
		int que = 0; // Number of NETMSG_NEWFRAMEs waiting to be processed.
		unsigned ahead = 0;
		while ((packet = net->Peek(ahead))) {
			if (packet->data[0] == NETMSG_NEWFRAME || packet->data[0] == NETMSG_KEYFRAME)
				++que;
			++ahead;
		}

		if(que < leastQue)
			leastQue = que;
	}
	else
	{
		// make sure ClientReadNet returns at least every 15 game frames
		// so CGame can process keyboard input, and render etc.
		timeLeft = (float)MAX_CONSECUTIVE_SIMFRAMES * gs->userSpeedFactor;
	}

	// always render at least 2FPS (will otherwise be highly unresponsive when catching up after a reconnection)
	unsigned procstarttime = SDL_GetTicks();
	// really process the messages
	while (timeLeft > 0.0f && (SDL_GetTicks() - procstarttime) < 500 && (packet = net->GetData(gs->frameNum)))
	{
		const unsigned char* inbuf = packet->data;
		const unsigned dataLength = packet->length;
		const unsigned char packetCode = inbuf[0];

		switch (packetCode) {
			case NETMSG_QUIT: {
				try {
					netcode::UnpackPacket pckt(packet, 3);
					std::string message;
					pckt >> message;
					logOutput.Print(message);
					if (!gameOver) {
						GameEnd(std::vector<unsigned char>());
					}
					AddTraffic(-1, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid QuitMessage: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_PLAYERLEFT: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num (%i) in NETMSG_PLAYERLEFT", player);
					break;
				}
				playerHandler->PlayerLeft(player, inbuf[2]);
				eventHandler.PlayerRemoved(player, (int) inbuf[2]);

				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_MEMDUMP: {
				MakeMemDump();
#ifdef TRACE_SYNC
				tracefile.Commit();
#endif
				AddTraffic(-1, packetCode, dataLength);
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

			case NETMSG_SENDPLAYERSTAT: {
				//logOutput.Print("Game over");
			// Warning: using CPlayer::Statistics here may cause endianness problems
			// once net->SendData is endian aware!
				net->Send(CBaseNetProtocol::Get().SendPlayerStat(gu->myPlayerNum, playerHandler->Player(gu->myPlayerNum)->currentStats));
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERSTAT: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num %i in playerstat msg",player);
					break;
				}
				playerHandler->Player(player)->currentStats = *(CPlayer::Statistics*)&inbuf[2];
				if (gameOver) {
					CDemoRecorder* record = net->GetDemoRecorder();
					if (record != NULL) {
						record->SetPlayerStats(player, playerHandler->Player(player)->currentStats);
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_PAUSE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num %i in pause msg",player);
					break;
				}
				gs->paused=!!inbuf[2];
				logOutput.Print(gs->paused ? "%s paused the game" : "%s unpaused the game" ,
											playerHandler->Player(player)->name.c_str());
				eventHandler.GamePaused(player, gs->paused);
				lastframe = SDL_GetTicks();
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_INTERNAL_SPEED: {
				gs->speedFactor = *((float*) &inbuf[1]);
				sound->PitchAdjust(sqrt(gs->speedFactor));
				//	logOutput.Print("Internal speed set to %.2f",gs->speedFactor);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_USER_SPEED: {
				gs->userSpeedFactor = *((float*) &inbuf[2]);

				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player) && player != SERVER_PLAYER) {
					logOutput.Print("Got invalid player num %i in user speed msg", player);
					break;
				}
				const char* pName = (player == SERVER_PLAYER)? "server": playerHandler->Player(player)->name.c_str();

				logOutput.Print("Speed set to %.1f [%s]", gs->userSpeedFactor, pName);
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_CPU_USAGE: {
				logOutput.Print("Game clients shouldn't get cpu usage msgs?");
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PLAYERINFO: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num %i in playerinfo msg", player);
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
					unsigned char player;
					pckt >> player;
					if (!playerHandler->IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					pckt >> playerHandler->Player(player)->name;
					playerHandler->Player(player)->readyToStart=(gameSetup->startPosType != CGameSetup::StartPos_ChooseInGame);
					playerHandler->Player(player)->active=true;
					wordCompletion->AddWord(playerHandler->Player(player)->name, false, false, false); // required?
					AddTraffic(player, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid PlayerName: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_CHAT: {
				try {
					ChatMessage msg(packet);

					HandleChatMsg(msg);
					AddTraffic(msg.fromPlayer, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid ChatMessage: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_SYSTEMMSG:{
				try {
					netcode::UnpackPacket pckt(packet, 4);
					string s;
					pckt >> s;
					logOutput.Print(s);
					AddTraffic(-1, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid SystemMessage: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_STARTPOS:{
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player) && player != SERVER_PLAYER) {
					logOutput.Print("Got invalid player num %i in start pos msg", player);
					break;
				}
				const int team = inbuf[2];
				if (!teamHandler->IsValidTeam(team)) {
					logOutput.Print("Got invalid team num %i in startpos msg", team);
				} else {
					float3 pos(*(float*)&inbuf[4],
					           *(float*)&inbuf[8],
					           *(float*)&inbuf[12]);
					if (!luaRules || luaRules->AllowStartPosition(player, pos)) {
						teamHandler->Team(team)->StartposMessage(pos);
						if (inbuf[3] != 2 && player != SERVER_PLAYER)
						{
							playerHandler->Player(player)->readyToStart = !!inbuf[3];
						}
						if (pos.y != -500) // no marker marker when no pos set yet
						{
							char label[128];
							SNPRINTF(label, sizeof(label), "Start %i", team);
							inMapDrawer->LocalPoint(pos, label, player);
							// FIXME - erase old pos ?
						}
					}
				}
				AddTraffic(player, packetCode, dataLength);
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
				logOutput.Print(
				  "GameID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				  p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
				  p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);
				AddTraffic(-1, packetCode, dataLength);
				break;
			}

			case NETMSG_PATH_CHECKSUM: {
				const unsigned char playerNum = inbuf[1];
				if (!playerHandler->IsValidPlayer(playerNum)) {
					logOutput.Print("Got invalid player num %i in path checksum msg", playerNum);
					break;
				}

				const boost::uint32_t playerCheckSum = *(boost::uint32_t*) &inbuf[2];
				const boost::uint32_t localCheckSum = pathManager->GetPathCheckSum();
				const CPlayer* player = playerHandler->Player(playerNum);

				if (playerCheckSum == 0) {
					logOutput.Print(
						"[DESYNC WARNING] path-checksum for player %d (%s) is 0; non-writable cache?",
						playerNum, player->name.c_str()
					);
				} else {
					if (playerCheckSum != localCheckSum) {
						logOutput.Print(
							"[DESYNC WARNING] path-checksum %08x for player %d (%s) does not match local checksum %08x",
							playerCheckSum, playerNum, player->name.c_str(), localCheckSum
						);
					}
				}
			} break;

			case NETMSG_KEYFRAME: {
				int serverframenum = *(int*)(inbuf+1);
				net->Send(CBaseNetProtocol::Get().SendKeyFrame(serverframenum));
				if (gs->frameNum == (serverframenum - 1)) {
				} else {
					// error
					LogObject() << "Error: Keyframe difference: " << gs->frameNum - (serverframenum - 1);
				}
				/* Fall through */
			}
			case NETMSG_NEWFRAME: {
				timeLeft -= 1.0f;
				SimFrame();
				// both NETMSG_SYNCRESPONSE and NETMSG_NEWFRAME are used for ping calculation by server
#ifdef SYNCCHECK
				net->Send(CBaseNetProtocol::Get().SendSyncResponse(gs->frameNum, CSyncChecker::GetChecksum()));
				if ((gs->frameNum & 4095) == 0) {// reset checksum every ~2.5 minute gametime
					CSyncChecker::NewFrame();
				}
#endif
				AddTraffic(-1, packetCode, dataLength);

				if (videoCapturing->IsCapturing()) {
					return;
				}
				break;
			}

			case NETMSG_COMMAND: {
				try {
					netcode::UnpackPacket pckt(packet, 1);
					short int psize;
					pckt >> psize;
					unsigned char player;
					pckt >> player;
					if (!playerHandler->IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					Command c;
					pckt >> c.id;
					pckt >> c.options;
					for(int a = 0; a < ((psize-9)/4); ++a) {
						float param;
						pckt >> param;
						c.params.push_back(param);
					}
					selectedUnits.NetOrder(c,player);
					AddTraffic(player, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid Command: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_SELECT: {
				try {
					netcode::UnpackPacket pckt(packet, 1);
					short int psize;
					pckt >> psize;
					unsigned char player;
					pckt >> player;
					if (!playerHandler->IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					vector<int> selected;
					for (int a = 0; a < ((psize-4)/2); ++a) {
						short int unitid;
						pckt >> unitid;

						if(unitid < 0 || static_cast<size_t>(unitid) >= uh->MaxUnits())
							throw netcode::UnpackPacketException("Invalid unit ID");

						if ((uh->units[unitid] &&
							(uh->units[unitid]->team == playerHandler->Player(player)->team)) ||
							gs->godMode) {
								selected.push_back(unitid);
						}
					}
					selectedUnits.NetSelect(selected, player);

					AddTraffic(player, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid Select: %s", e.err.c_str());
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

					if (!playerHandler->IsValidPlayer(player))
						throw netcode::UnpackPacketException("Invalid player number");

					short int unitid;
					pckt >> unitid;
					if (unitid < 0 || static_cast<size_t>(unitid) >= uh->MaxUnits())
						throw netcode::UnpackPacketException("Invalid unit ID");

					Command c;
					pckt >> c.id;
					pckt >> c.options;
					if (packetCode == NETMSG_AICOMMAND_TRACKED) {
						pckt >> c.aiCommandId;
					}

					// insert the command parameters
					for (int a = 0; a < ((psize - 11) / 4); ++a) {
						float param;
						pckt >> param;
						c.params.push_back(param);
					}

					selectedUnits.AiOrder(unitid, c, player);
					AddTraffic(player, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid AICommand: %s", e.err.c_str());
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
						Command cmd;
						pckt >> cmd.id;
						pckt >> cmd.options;
						short int paramCount;
						pckt >> paramCount;
						for (int p = 0; p < paramCount; p++) {
							float param;
							pckt >> param;
							cmd.params.push_back(param);
						}
						commands.push_back(cmd);
					}
					// apply the commands
					for (int c = 0; c < commandCount; c++) {
						for (int u = 0; u < unitCount; u++) {
							selectedUnits.AiOrder(unitIDs[u], commands[c], player);
						}
					}
					AddTraffic(player, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid AICommands: %s", e.err.c_str());
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
						if (!luaRules || luaRules->AllowResourceTransfer(srcTeam, dstTeam, "m", metalShare)) {
							teamHandler->Team(srcTeam)->metal -= metalShare;
							teamHandler->Team(dstTeam)->metal += metalShare;
						}
					}
					if (energyShare > 0.0f) {
						if (!luaRules || luaRules->AllowResourceTransfer(srcTeam, dstTeam, "e", energyShare)) {
							teamHandler->Team(srcTeam)->energy -= energyShare;
							teamHandler->Team(dstTeam)->energy += energyShare;
						}
					}

					for (int i = 0, j = fixedLen;  i < numUnitIDs;  i++, j += sizeof(short)) {
						short int unitID;
						pckt >> unitID;
						if(unitID >= uh->MaxUnits() || unitID < 0)
							throw netcode::UnpackPacketException("Invalid unit ID");

						CUnit* u = uh->units[unitID];
						// ChangeTeam() handles the AllowUnitTransfer() LuaRule
						if (u && u->team == srcTeam && !u->beingBuilt) {
							u->ChangeTeam(dstTeam, CUnit::ChangeGiven);
						}
					}
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid AIShare: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_LUAMSG: {
				try {
					netcode::UnpackPacket unpack(packet, 1);
					boost::uint16_t size;
					unpack >> size;
					if(size != packet->length)
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
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid LuaMsg: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_SHARE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num %i in share msg", player);
					break;
				}
				int teamID1 = playerHandler->Player(player)->team;
				int teamID2 = inbuf[2];
				bool shareUnits = !!inbuf[3];
				CTeam* team1 = teamHandler->Team(teamID1);
				CTeam* team2 = teamHandler->Team(teamID2);
				float metalShare  = std::min(*(float*)&inbuf[4], (float)team1->metal);
				float energyShare = std::min(*(float*)&inbuf[8], (float)team1->energy);

				if (metalShare != 0.0f) {
					metalShare = std::min(metalShare, team1->metal);
					if (!luaRules || luaRules->AllowResourceTransfer(teamID1, teamID2, "m", metalShare)) {
						team1->metal                       -= metalShare;
						team1->metalSent                   += metalShare;
						team1->currentStats->metalSent     += metalShare;
						team2->metal                       += metalShare;
						team2->metalReceived               += metalShare;
						team2->currentStats->metalReceived += metalShare;
					}
				}
				if (energyShare != 0.0f) {
					energyShare = std::min(energyShare, team1->energy);
					if (!luaRules || luaRules->AllowResourceTransfer(teamID1, teamID2, "e", energyShare)) {
						team1->energy                       -= energyShare;
						team1->energySent                   += energyShare;
						team1->currentStats->energySent     += energyShare;
						team2->energy                       += energyShare;
						team2->energyReceived               += energyShare;
						team2->currentStats->energyReceived += energyShare;
					}
				}

				if (shareUnits) {
					vector<int>& netSelUnits = selectedUnits.netSelected[player];
					vector<int>::const_iterator ui;
					for (ui = netSelUnits.begin(); ui != netSelUnits.end(); ++ui){
						CUnit* unit = uh->units[*ui];
						if (unit && unit->team == teamID1 && !unit->beingBuilt) {
							if (!unit->directControl)
								unit->ChangeTeam(teamID2, CUnit::ChangeGiven);
						}
					}
					netSelUnits.clear();
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_SETSHARE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num %i in setshare msg", player);
					break;
				}
				const unsigned char team = inbuf[2];
				if (!teamHandler->IsValidTeam(team)) {
					logOutput.Print("Got invalid team num %i in setshare msg", team);
					break;
				}
				float metalShare=*(float*)&inbuf[3];
				float energyShare=*(float*)&inbuf[7];

				if (!luaRules || luaRules->AllowResourceLevel(team, "m", metalShare)) {
					teamHandler->Team(team)->metalShare = metalShare;
				}
				if (!luaRules || luaRules->AllowResourceLevel(team, "e", energyShare)) {
					teamHandler->Team(team)->energyShare = energyShare;
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_MAPDRAW: {
				int player = inMapDrawer->GotNetMsg(packet);
				if(player >= 0)
					AddTraffic(player, packetCode, dataLength);
				break;
			}
			case NETMSG_TEAM: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num %i in team msg", player);
					break;
				}

				const unsigned char action = inbuf[2];
				const int fromTeam = playerHandler->Player(player)->team;

				switch (action)
				{
					case TEAMMSG_GIVEAWAY: {
						const unsigned char toTeam = inbuf[3];
						const unsigned char fromTeam_g = inbuf[4];
						if (!teamHandler->IsValidTeam(toTeam) || !teamHandler->IsValidTeam(fromTeam_g)) {
							logOutput.Print("Got invalid team nums %i %i in team giveaway msg", toTeam, fromTeam_g);
							break;
						}

						const int numPlayersInTeam_g        = playerHandler->ActivePlayersInTeam(fromTeam_g).size();
						const size_t numTotAIsInTeam_g      = skirmishAIHandler.GetSkirmishAIsInTeam(fromTeam_g).size();
						const size_t numControllersInTeam_g = numPlayersInTeam_g + numTotAIsInTeam_g;
						const bool isOwnTeam_g              = (fromTeam_g == fromTeam);

						bool giveAwayOk = false;
						if (isOwnTeam_g) {
							// player is giving stuff from his own team
							giveAwayOk = true;
							if (numPlayersInTeam_g == 1) {
								teamHandler->Team(fromTeam_g)->GiveEverythingTo(toTeam);
							} else {
								playerHandler->Player(player)->StartSpectating();
							}
							selectedUnits.ClearNetSelect(player);
						} else {
							// player is giving stuff from one of his AI teams
							if (numPlayersInTeam_g == 0) {
								giveAwayOk = true;
							}
						}
						if (giveAwayOk && (numControllersInTeam_g == 1)) {
							// team has no controller left now
							teamHandler->Team(fromTeam_g)->GiveEverythingTo(toTeam);
							teamHandler->Team(fromTeam_g)->leader = -1;
						}
						CPlayer::UpdateControlledTeams();
						break;
					}
					case TEAMMSG_RESIGN: {
						playerHandler->Player(player)->StartSpectating();
						if (player == gu->myPlayerNum) {
							selectedUnits.ClearSelected();
							unitTracker.Disable();
							CLuaUI::UpdateTeams();
						}
						// actualize all teams of which the player is leader
						for (size_t t = 0; t < teamHandler->ActiveTeams(); ++t) {
							CTeam* team = teamHandler->Team(t);
							if (team->leader == player) {
								const std::vector<int> &teamPlayers = playerHandler->ActivePlayersInTeam(t);
								const std::vector<size_t> &teamAIs  = skirmishAIHandler.GetSkirmishAIsInTeam(t);
								if ((teamPlayers.size() + teamAIs.size()) == 0) {
									// no controllers left in team
									//team.active = false;
									team->leader = -1;
								} else if (teamPlayers.empty()) {
									// no human player left in team
									team->leader = skirmishAIHandler.GetSkirmishAI(teamAIs[0])->hostPlayer;
								} else {
									// still human controllers left in team
									team->leader = teamPlayers[0];
								}
							}
						}
						logOutput.Print("Player %i resigned and is now spectating!", player);
						selectedUnits.ClearNetSelect(player);
						CPlayer::UpdateControlledTeams();
						break;
					}
					case TEAMMSG_JOIN_TEAM: {
						const unsigned char newTeam = inbuf[3];
						if (!teamHandler->IsValidTeam(newTeam)) {
							logOutput.Print("Got invalid team num %i in team join msg", newTeam);
							break;
						}

						playerHandler->Player(player)->team      = newTeam;
						playerHandler->Player(player)->spectator = false;
						if (player == gu->myPlayerNum) {
							gu->myPlayingTeam = gu->myTeam = newTeam;
							gu->myPlayingAllyTeam = gu->myAllyTeam = teamHandler->AllyTeam(gu->myTeam);
							gu->spectating           = false;
							gu->spectatingFullView   = false;
							gu->spectatingFullSelect = false;
							selectedUnits.ClearSelected();
							unitTracker.Disable();
							CLuaUI::UpdateTeams();
						}
						if (teamHandler->Team(newTeam)->leader == -1) {
							teamHandler->Team(newTeam)->leader = player;
						}
						CPlayer::UpdateControlledTeams();
						eventHandler.PlayerChanged(player);
						break;
					}
					case TEAMMSG_TEAM_DIED: {
						// silently drop since we can calculate this ourself, altho it's useful info to store in replays
						break;
					}
					default: {
						logOutput.Print("Unknown action in NETMSG_TEAM (%i) from player %i", action, player);
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
					unsigned int skirmishAIId;
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

					if (tai->leader == -1) {
						tai->leader = playerId;
					}
					CPlayer::UpdateControlledTeams();
					eventHandler.PlayerChanged(playerId);
					if (isLocal) {
						logOutput.Print("Skirmish AI being created for team %i ...", aiTeamId);
						eoh->CreateSkirmishAI(skirmishAIId);
					}
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid AICreated: %s", e.err.c_str());
				}
				break;
			}
			case NETMSG_AI_STATE_CHANGED: {
				const unsigned char playerId     = inbuf[1];
				if (!playerHandler->IsValidPlayer(playerId)) {
					logOutput.Print("Got invalid player num %i in ai state changed msg", playerId);
					break;
				}

				const unsigned int skirmishAIId  = *((unsigned int*)&inbuf[2]); // 4 bytes
				const ESkirmishAIStatus newState = (ESkirmishAIStatus) inbuf[6];
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
							logOutput.Print("Skirmish AI \"%s\" being reloaded for team %i ...", aiData->name.c_str(), aiTeamId);
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
							tai->leader = -1;
						}
						CPlayer::UpdateControlledTeams();
						eventHandler.PlayerChanged(playerId);
						logOutput.Print("Skirmish AI \"%s\" (ID:%i), which controlled team %i is now dead", aiInstanceName.c_str(), skirmishAIId, aiTeamId);
					}
				} else if (newState == SKIRMAISTATE_ALIVE) {
					if (isLocal) {
						// short-name and version of the AI is unsynced data
						// -> only available on the AI host
						logOutput.Print("Skirmish AI \"%s\" (ID:%i, Short-Name:\"%s\", Version:\"%s\") took over control of team %i",
								aiData->name.c_str(), skirmishAIId, aiData->shortName.c_str(), aiData->version.c_str(), aiTeamId);
					} else {
						logOutput.Print("Skirmish AI \"%s\" (ID:%i) took over control of team %i",
								aiData->name.c_str(), skirmishAIId, aiTeamId);
					}
				}
				break;
			}
			case NETMSG_ALLIANCE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Got invalid player num %i in alliance msg", player);
					break;
				}

				const unsigned char whichAllyTeam = inbuf[2];
				const bool allied = static_cast<bool>(inbuf[3]);
				const unsigned char fromAllyTeam = teamHandler->AllyTeam(playerHandler->Player(player)->team);
				if (teamHandler->IsValidAllyTeam(whichAllyTeam) && fromAllyTeam != whichAllyTeam) {
					// FIXME - need to reset unit allyTeams
					//       - need a call-in for AIs
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
					logOutput.Print(msg.str());

					// stop attacks against former foe
					if (allied) {
						for (std::list<CUnit*>::iterator it = uh->activeUnits.begin();
								it != uh->activeUnits.end();
								++it) {
							if (teamHandler->Ally((*it)->allyteam, whichAllyTeam)) {
								(*it)->StopAttackingAllyTeam(whichAllyTeam);
							}
						}
					}
					eventHandler.TeamChanged(playerHandler->Player(player)->team);
				} else {
					logOutput.Print("Alliance: Player %i sent out wrong allyTeam index in alliance message", player);
				}
				break;
			}
			case NETMSG_CCOMMAND: {
				try {
					CommandMessage msg(packet);

					ActionReceived(msg.action, msg.player);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid CommandMessage: %s", e.err.c_str());
				}
				break;
			}

			case NETMSG_DIRECT_CONTROL: {
				const unsigned char player = inbuf[1];

				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Invalid player number (%i) in NETMSG_DIRECT_CONTROL", player);
					break;
				}

				CPlayer* sender = playerHandler->Player(player);
				if (sender->spectator || !sender->active) {
					break;
				}

				CUnit* ctrlUnit = (sender->dccs).playerControlledUnit;
				if (ctrlUnit) {
					// player released control
					sender->StopControllingUnit();
				} else {
					// player took control
					if (
						!selectedUnits.netSelected[player].empty() &&
						uh->units[selectedUnits.netSelected[player][0]] != NULL &&
						!uh->units[selectedUnits.netSelected[player][0]]->weapons.empty()
					) {
						CUnit* unit = uh->units[selectedUnits.netSelected[player][0]];

						if (unit->directControl && unit->directControl->myController) {
							if (player == gu->myPlayerNum) {
								logOutput.Print(
									"player %d is already controlling unit %d, try later",
									unit->directControl->myController->playerNum, unit->id
								);
							}
						}
						else if (!luaRules || luaRules->AllowDirectUnitControl(player, unit)) {
							unit->directControl = &sender->myControl;
							(sender->dccs).playerControlledUnit = unit;

							if (player == gu->myPlayerNum) {
								gu->directControl = unit;
								mouse->wasLocked = mouse->locked;
								if (!mouse->locked) {
									mouse->locked = true;
									mouse->HideMouse();
								}
								camHandler->PushMode();
								camHandler->SetCameraMode(0);
								selectedUnits.ClearSelected();
							}
						}
					}
				}
				AddTraffic(player, packetCode, dataLength);
				break;
			}

			case NETMSG_DC_UPDATE: {
				const unsigned char player = inbuf[1];
				if (!playerHandler->IsValidPlayer(player)) {
					logOutput.Print("Invalid player number (%i) in NETMSG_DC_UPDATE", player);
					break;
				}

				DirectControlStruct* dc = &playerHandler->Player(player)->myControl;
				DirectControlClientState& dccs = playerHandler->Player(player)->dccs;
				CUnit* unit = dccs.playerControlledUnit;

				dc->forward    = !!(inbuf[2] & (1 << 0));
				dc->back       = !!(inbuf[2] & (1 << 1));
				dc->left       = !!(inbuf[2] & (1 << 2));
				dc->right      = !!(inbuf[2] & (1 << 3));
				dc->mouse1     = !!(inbuf[2] & (1 << 4));
				bool newMouse2 = !!(inbuf[2] & (1 << 5));

				if (!dc->mouse2 && newMouse2 && unit) {
					unit->AttackUnit(0, true);
				}
				dc->mouse2 = newMouse2;

				short int h = *((short int*) &inbuf[3]);
				short int p = *((short int*) &inbuf[5]);
				dc->viewDir = GetVectorFromHAndPExact(h, p);

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
					playerHandler->AddPlayer(player);
					eventHandler.PlayerAdded(player.playerNum);
					logOutput.Print("Added new player: %s", name.c_str());
					//TODO: perhaps add a lua hook, hook should be able to reassign the player to a team and/or create a new team/allyteam
					AddTraffic(-1, packetCode, dataLength);
				} catch (netcode::UnpackPacketException &e) {
					logOutput.Print("Got invalid New player message: %s", e.err.c_str());
				}
				break;
			}
			default: {
#ifdef SYNCDEBUG
				if (!CSyncDebugger::GetInstance()->ClientReceived(inbuf))
#endif
				{
					logOutput.Print("Unknown net msg received, packet code is %d."
							" A likely cause of this is network instability,"
							" which may happen in a WLAN, for example.",
							packetCode);
				}
				AddTraffic(-1, packetCode, dataLength);
				break;
			}
		}
	}

	return;
}


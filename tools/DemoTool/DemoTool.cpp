/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <map>
#include <iostream>
#include <gflags/gflags.h>
#include <iomanip> //hex

#include "StringSerializer.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "System/LoadSave/DemoReader.h"
#include "System/Net/RawPacket.h"
#include "Sim/Units/CommandAI/Command.h"

/*
Usage:
Start with the full! path to the demofile as the only argument

Please note that not all NETMSG's are implemented, expand if needed.

When compiling for windows with MinGW, make sure to use the
-Wl,-subsystem,console flag when linking, as otherwise there will be
no console output (you still could use this.exe > z.tzt though).
*/

	DEFINE_string(demofile,     "",    "Path to demo file");
	DEFINE_bool  (dump,         false, "Only dump networc traffic saved in demo");
	DEFINE_bool  (stats,        false, "Print all game, player and team stats");
	DEFINE_bool  (header,       false, "Print demoheader content");
	DEFINE_bool  (playerstats,  false, "Print playerstats");
	DEFINE_bool  (teamstats,    false, "Print teamstats");
	DEFINE_int32 (team,         -1,    "Select team");
	DEFINE_string(teamsstatcsv, "",    "Write teamstats in a csv file");


void TrafficDump(CDemoReader& reader, bool trafficStats);
void WriteTeamstatHistory(CDemoReader& reader, unsigned team, const std::string& file);

int main (int argc, char* argv[])
{
	std::string filename;

	gflags::SetUsageMessage(std::string("Usage: ") + argv[0] + " [options] path_to_demo.sdfz");
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	if (!FLAGS_demofile.empty()) {
		filename = FLAGS_demofile;
	} else if (argc >= 2) {
		filename = argv[1];
	} else {
		std::cout << "No demofile given" << std::endl;
		gflags::ShowUsageWithFlags(argv[0]);
	}

	CDemoReader reader(filename, 0.0f);
	reader.LoadStats();
	if (FLAGS_dump)
	{
		TrafficDump(reader, true);
		return 0;
	}
	if (!FLAGS_teamsstatcsv.empty())
	{
		if (FLAGS_team < 0)
		{
			std::cout << "teamsstatcsv requires a team to select" << std::endl;
			exit(1);
		}
		WriteTeamstatHistory(reader, (unsigned) FLAGS_team, FLAGS_teamsstatcsv);
	}

	if (FLAGS_header || FLAGS_stats)
	{
		wstringstream buf;
		buf << reader.GetFileHeader();
		std::wcout << buf.str();
	}
	if (FLAGS_playerstats || FLAGS_stats)
	{
		const std::vector<PlayerStatistics> statvec = reader.GetPlayerStats();
		for (unsigned i = 0; i < statvec.size(); ++i)
		{
			std::wcout << L"-- Player statistics for player " << i << L" --" << std::endl;
			wstringstream buf;
			buf << statvec[i];
			std::wcout << buf.str();
		}
	}
	if (FLAGS_teamstats || FLAGS_stats)
	{
		const DemoFileHeader header = reader.GetFileHeader();
		const std::vector< std::vector<TeamStatistics> > statvec = reader.GetTeamStats();
		for (unsigned teamNum = 0; teamNum < statvec.size(); ++teamNum)
		{
			int time = 0;
			for (unsigned i = 0; i < statvec[teamNum].size(); ++i)
			{
				std::wcout << L"-- Team statistics for player " << teamNum << L", game second " << time << L" --" << std::endl;
				wstringstream buf;
				buf << statvec[teamNum][i];
				time += header.teamStatPeriod;
				std::wcout << buf.str();
			}
		}
		const std::vector<unsigned char>& winningAllyTeams = reader.GetWinningAllyTeams();
		std::wcout << L"Winning Allyteams:";
		wstringstream buf;
		for (unsigned char allyteam: winningAllyTeams){
			buf << " " << (unsigned) allyteam;
		}
		std::wcout << buf.str();
	}
	return 0;
}


static std::map<int, std::string> cmdIdToName;

void InitCommandNames()
{
#define REGISTER_CMD(cmdDefine) \
		cmdIdToName[cmdDefine] = #cmdDefine;

	REGISTER_CMD(CMD_STOP)
	REGISTER_CMD(CMD_INSERT)
	REGISTER_CMD(CMD_REMOVE)
	REGISTER_CMD(CMD_WAIT)
	REGISTER_CMD(CMD_TIMEWAIT)
	REGISTER_CMD(CMD_DEATHWAIT)
	REGISTER_CMD(CMD_SQUADWAIT)
	REGISTER_CMD(CMD_GATHERWAIT)
	REGISTER_CMD(CMD_MOVE)
	REGISTER_CMD(CMD_PATROL)
	REGISTER_CMD(CMD_FIGHT)
	REGISTER_CMD(CMD_ATTACK)
	REGISTER_CMD(CMD_AREA_ATTACK)
	REGISTER_CMD(CMD_GUARD)
	REGISTER_CMD(CMD_AISELECT)
	REGISTER_CMD(CMD_GROUPSELECT)
	REGISTER_CMD(CMD_GROUPADD)
	REGISTER_CMD(CMD_GROUPCLEAR)
	REGISTER_CMD(CMD_REPAIR)
	REGISTER_CMD(CMD_FIRE_STATE)
	REGISTER_CMD(CMD_MOVE_STATE)
	REGISTER_CMD(CMD_SETBASE)
	REGISTER_CMD(CMD_INTERNAL)
	REGISTER_CMD(CMD_SELFD)
	REGISTER_CMD(CMD_LOAD_UNITS)
	REGISTER_CMD(CMD_LOAD_ONTO)
	REGISTER_CMD(CMD_UNLOAD_UNITS)
	REGISTER_CMD(CMD_UNLOAD_UNIT)
	REGISTER_CMD(CMD_ONOFF)
	REGISTER_CMD(CMD_RECLAIM)
	REGISTER_CMD(CMD_CLOAK)
	REGISTER_CMD(CMD_STOCKPILE)
	REGISTER_CMD(CMD_MANUALFIRE)
	REGISTER_CMD(CMD_RESTORE)
	REGISTER_CMD(CMD_REPEAT)
	REGISTER_CMD(CMD_TRAJECTORY)
	REGISTER_CMD(CMD_RESURRECT)
	REGISTER_CMD(CMD_CAPTURE)
	REGISTER_CMD(CMD_AUTOREPAIRLEVEL)
	REGISTER_CMD(CMD_IDLEMODE)
	REGISTER_CMD(CMD_FAILED)

#undef  REGISTER_CMD
}

const std::string& GetCommandName(int commandId)
{
	static const std::string CMD_NAME_BUILD_UNIT = "<BUILD_UNIT>";
	static const std::string CMD_NAME_UNKNOWN = "<UNKNOWN>";

	if (commandId < 0) {
		return CMD_NAME_BUILD_UNIT;
	}

	const std::map<int, std::string>::const_iterator cmd = cmdIdToName.find(commandId);
	if (cmd != cmdIdToName.end()) {
		return cmd->second;
	}

	return CMD_NAME_UNKNOWN;
}

void PrintBinary(const unsigned char* const buf, int len)
{
	for(int i=0; i<len; i++) {
		std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)buf[i];
	}
	std::cout << std::dec; //reset to decimal
}

void TrafficDump(CDemoReader& reader, bool trafficStats)
{
	InitCommandNames();
	std::vector<unsigned> trafficCounter(NETMSG_LAST, 0);
	int frame = -1;
	int cmdId = 0;
	while (!reader.ReachedEnd())
	{
		netcode::RawPacket* packet;
		packet = reader.GetData(3.402823466e+38f);
		if (packet == NULL)
			continue;
		assert(packet->data[0]<NETMSG_LAST);
		trafficCounter[packet->data[0]] += packet->length;
		const unsigned char* buffer = packet->data;
		char buf[16]; // FIXME: cba to look up how to format numbers with iostreams
		sprintf(buf, "%06d ", frame);
		const int cmd = (unsigned char)buffer[0];
		if (cmd == NETMSG_GAME_FRAME_PROGRESS) { //ignore as its unsynced (TODO: why is this recorded in demo?)
			delete packet;
			continue;
		}
		std::cout << buf;
		switch (cmd)
		{
			case NETMSG_AICOMMAND:
				std::cout << "AICOMMAND: Playernum: " << (unsigned)buffer[3];
				std::cout << " Length: " << (unsigned)packet->length;
				std::cout << " AI id: " << (unsigned)buffer[4];
				std::cout << " UnitId: " << *((short*)(buffer + 5));
				cmdId = *((int*)(buffer + 7));
				std::cout << " CommandId: " << GetCommandName(cmdId) << "(" << cmdId << ")";
				std::cout << " Options: " << (unsigned)buffer[11];
				std::cout << " Parameters:";
				for (unsigned short i = 12; i < packet->length; i += sizeof(float)) {
					std::cout << " " << *((float*)(buffer + i));
				}
				std::cout << std::endl;
				break;
			case NETMSG_AICOMMANDS: {
				std::cout << "AICOMMANDS: Playernum: " << (unsigned)buffer[3];
				std::cout << " Length: " << (unsigned)packet->length;
				std::cout << " AI id: " << (unsigned)buffer[4];
				std::cout << " Pair: " << (unsigned)buffer[5];
				unsigned int sameid = *((unsigned int*)(buffer + 6));
				std::cout << " SameID: " << sameid;
				unsigned int sameopt = (unsigned)buffer[10];
				std::cout << " SameOpt: " << sameopt;
				unsigned short samesize = *((unsigned short*)(buffer + 11));
				std::cout << " SameSize: " << samesize;
				short uidc = *((short*)(buffer + 13));
				std::cout << " UnitIDCount: " << uidc;
				for (unsigned int i = 0; i < uidc; ++i) {
					std::cout << " " << *((short*)(buffer + 15 + i * 2));
				}
				short cidc = *((short*)(buffer + 15 + uidc * 2));
				int startp = 15 + uidc * 2 + 2;
				std::cout << " CmdIDCount: " << cidc;
				for (unsigned int i = 0; i < cidc; ++i) {
					if (sameid == 0) {
						std::cout << " " << *((unsigned int*)(buffer + startp));
						startp += 4;
					}
					if (sameopt == 0xFF) {
						std::cout << " " << (unsigned)buffer[startp];
						startp += 1;
					}
					if (sameopt == 0xFFFF) {
						std::cout << " " << *((unsigned short*)(buffer + startp));
						startp += 2;
					}
				}
				std::cout << std::endl;
				break;
			}
			case NETMSG_PLAYERNAME:
				std::cout << "PLAYERNAME: Playernum: " << (unsigned)buffer[2] << " Name: " << buffer+3 << std::endl;
				break;
			case NETMSG_SETPLAYERNUM:
				std::cout << "SETPLAYERNUM: Playernum: " << (unsigned)buffer[1] << std::endl;
				break;
			case NETMSG_QUIT:
				std::cout << "QUIT" << std::endl;
				break;
			case NETMSG_STARTPLAYING:
				std::cout << "STARTPLAYING" << std::endl;
				break;
			case NETMSG_STARTPOS:
				std::cout << "STARTPOS: Playernum: " << (unsigned)buffer[1] << " Team: " << (unsigned)buffer[2] << " Readyness: " << (unsigned)buffer[3] << std::endl;
				break;
			case NETMSG_SYSTEMMSG:
				std::cout << "SYSTEMMSG: Player: " << (unsigned)buffer[3] << " Msg: " << (char*)(buffer+4) << std::endl;
				break;
			case NETMSG_CHAT:
				std::cout << "CHAT: Player: " << (unsigned)buffer[2] << " Msg: " << (char*)(buffer+4) << std::endl;
				break;
			case NETMSG_KEYFRAME:
				std::cout << "KEYFRAME: " << *(int*)(buffer+1) << std::endl;
				++frame;
				if (*(int*)(buffer+1) != frame) {
					std::cout << "keyframe mismatch!" << std::endl;
				}
				break;
			case NETMSG_NEWFRAME:
				std::cout << "NEWFRAME" << std::endl;
				++frame;
				break;
			case NETMSG_PLAYERINFO:
				std::cout << "NETMSG_PLAYERINFO: Player:" << (int)buffer[1] << " Ping: " << *(uint16_t*)&buffer[6] << std::endl;
				break;
			case NETMSG_LUAMSG:
				{
				std::cout << "LUAMSG length:" << packet->length << " Player:" << (unsigned)buffer[3] << " Script: " << *(uint16_t*)&buffer[4] << " Mode: " << (unsigned)buffer[6] << " Msg: ";
				PrintBinary(&packet->data[7], packet->length - 7);
				std::cout << std::endl;
				break;
				}
			case NETMSG_TEAM:
				std::cout << "TEAM Playernum: " << (int)buffer[1] << " Action:";
				switch (buffer[2]) {
					case TEAMMSG_GIVEAWAY: std::cout << "GIVEAWAY"; break;
					case TEAMMSG_RESIGN: std::cout << "RESIGN"; break;
					case TEAMMSG_TEAM_DIED: std::cout << "TEAM_DIED"; break;
					case TEAMMSG_JOIN_TEAM: std::cout << "JOIN_TEAM"; break;
					default: std::cout << (int)buffer[2];
				}
				std::cout << " Parameter:" << (int)buffer[3] << std::endl;
				break;
			case NETMSG_COMMAND:
				std::cout << "COMMAND Playernum: " << (int)buffer[3];
				std::cout << " Size: " << *(unsigned short*)(buffer+1);
				cmdId = *((int*)(buffer + 4));
				std::cout << " CommandId: " << GetCommandName(cmdId) << "(" << cmdId << ")";
				std::cout << " Options: " << (unsigned)buffer[8];
				std::cout << " Parameters:";
				for (unsigned short i = 9; i < packet->length; i += sizeof(float)) {
					std::cout << " " << *((float*)(buffer + i));
				}
				std::cout << std::endl;
				if (*(unsigned short*)(buffer+1) != packet->length)
					std::cout << "      packet length error: expected: " <<  *(unsigned short*)(buffer+1) << " got: " << packet->length << std::endl;
				break;
			case NETMSG_SELECT:
				std::cout << "NETMGS_SELECT: Playernum: " << (unsigned)buffer[3];
				std::cout << " Length: " << (unsigned)packet->length;
				std::cout << " Unit IDs:";
				for (unsigned short i = 4; i < packet->length; i += 2) {
					std::cout << " " << *((short*)(buffer + i));
				}
				std::cout << std::endl;
				break;
			case NETMSG_GAMEOVER:
				std::cout << "NETMSG_GAMEOVER";
				std::cout << " Length: " << (unsigned)packet->length;
				std::cout << " Player: " << (unsigned)buffer[2];
				std::cout << " Winning ids:";
				for (unsigned short i = 3; i < packet->length; i += 1) {
					std::cout << " " << (unsigned) *((char*)(buffer + i));
				}
				std::cout << std::endl;
				break;
			case NETMSG_MAPDRAW:
				std::cout << "NETMSG_MAPDRAW Player:" << (int)buffer[2];
				switch (buffer[3]) {
					case MAPDRAW_POINT:
						std::cout << " POINT x:" << *(int16_t*)&buffer[4] << " z:" << *(int16_t*)&buffer[6];
						if (packet->length > 10) {
							std::cout << " Msg: " << (char*)buffer+9;
						}
						break;
					case MAPDRAW_LINE:
						std::cout << " LINE x1:" << *(int16_t*)&buffer[4] << " z1:" << *(int16_t*)&buffer[6] << " x2:" << *(int16_t*)&buffer[8] << " z2:" << *(int16_t*)&buffer[10];
						break;
					case MAPDRAW_ERASE:
						std::cout << " ERASE x:" << *(int16_t*)&buffer[4] << " z:" << *(int16_t*)&buffer[6];
						break;
				}
				std::cout << std::endl;
				break;
			case NETMSG_PATH_CHECKSUM:
				std::cout << "NETMSG_PATH_CHECKSUM" << std::endl;
				break;
			case NETMSG_INTERNAL_SPEED:
				std::cout << "NETMSG_INTERNAL_SPEED" << std::endl;
				break;
			case NETMSG_PLAYERLEFT:
				std::cout << "NETMSG_PLAYERLEFT" << std::endl;
				break;
			case NETMSG_GAMEDATA:
				std::cout << "NETMSG_GAMEDATA" << std::endl;
				break;
			case NETMSG_CREATE_NEWPLAYER:
				// uchar myPlayerNum, uchar spectator, uchar teamNum, std::string playerName
				std::cout << "NETMSG_CREATE_NEWPLAYER: Playernum: " << (unsigned)buffer[3];
				std::cout << " Spectator: " << (unsigned)buffer[4];
				std::cout << " Team: " << (unsigned) buffer[5];
				std::cout << " PlayerName: " << (char*) (buffer + 6);
				std::cout << std::endl;
				break;
			case NETMSG_GAMEID:
				std::cout << "NETMSG_GAMEID: ";
				PrintBinary(&packet->data[1], packet->length - 1);
				std::cout << std::endl;
				break;
			case NETMSG_RANDSEED:
				std::cout << "NETMSG_RANDSEED: ";
				PrintBinary(&packet->data[1], packet->length - 1);
				std::cout << std::endl;
				break;
			case NETMSG_SHARE:
				std::cout << "NETMSG_SHARE: Playernum: " << (unsigned)buffer[1];
				std::cout << " Team: " << (unsigned)buffer[2];
				std::cout << " ShareUnits: " << (unsigned)buffer[3];
				std::cout << " Metal: " << *(float*)(buffer + 4);
				std::cout << " Energy: " << *(float*)(buffer + 8);
				std::cout << std::endl;
				break;
			case NETMSG_CCOMMAND:
				std::cout << "NETMSG_CCOMMAND: " << std::endl;
				break;
			case NETMSG_PAUSE:
				std::cout << "NETMSG_PAUSE: Player " << (unsigned)buffer[1] << " paused: " << (unsigned)buffer[2] << std::endl;
				break;
			case NETMSG_SYNCRESPONSE:
				//uchar myPlayerNum; int frameNum; uint checksum;
				std::cout << "NETMSG_SYNCRESPONSE: Playernum: "<< (unsigned)buffer[1];
				std::cout << " Framenum: " << *(int*)(buffer+2);
				std::cout << " Checksum: " << (unsigned)buffer[6];
				std::cout << std::endl;
				break;
			case NETMSG_DIRECT_CONTROL:
				std::cout << "NETMSG_DIRECT_CONTROL: " << std::endl;
				break;
			case NETMSG_SETSHARE:
				std::cout << "NETMSG_SETSHARE: " << std::endl;
				break;
			case NETMSG_CLIENTDATA: {
				const uint16_t playerNum = (int)buffer[3];
				const uint16_t totalSize = *(unsigned short*)(buffer+1);
				std::cout << "NETMSG_CLIENTDATA: Player " << (unsigned)playerNum << " totalSize: " <<totalSize << std::endl;
				break;
			}
			case NETMSG_AI_STATE_CHANGED:
				std::cout << "NETMSG_AI_STATE_CHANGED: " << std::endl;
				break;
			case NETMSG_PLAYERSTAT:
				std::cout << "NETMSG_PLAYERSTAT: " << std::endl;
				break;
			default:
				std::cout << "MSG: " << cmd << std::endl;
		}
		delete packet;
	}

	// how many times did each message appear
	for (unsigned i = 0; i != trafficCounter.size(); ++i)
	{
		if (trafficStats && trafficCounter[i] > 0)
			std::cout << "Msg " << i << ": " << trafficCounter[i] << std::endl;
	}
}

template<typename T>
void PrintSep(std::ofstream& file, T value)
{
	file << value << ";";
}

void WriteTeamstatHistory(CDemoReader& reader, unsigned team, const std::string& file)
{
	const DemoFileHeader header = reader.GetFileHeader();
	const std::vector< std::vector<TeamStatistics> >& statvec = reader.GetTeamStats();
	if (team < statvec.size())
	{
		int time = 0;
		std::ofstream out(file.c_str());
		out << "Team Statistics for " << team << std::endl;
		out << "Time[sec];MetalUsed;EnergyUsed;MetalProduced;EnergyProduced;MetalExcess;EnergyExcess;"
		    << "EnergyReceived;MetalSent;EnergySent;DamageDealt;DamageReceived;"
		    << "UnitsProduced;UnitsDied;UnitsReceived;UnitsSent;nitsCaptured;"
		    << "UnitsOutCaptured;UnitsKilled" << std::endl;
		for (unsigned i = 0; i < statvec[team].size(); ++i)
		{
			PrintSep(out, time);
			PrintSep(out, statvec[team][i].metalUsed);
			PrintSep(out, statvec[team][i].energyUsed);
			PrintSep(out, statvec[team][i].metalProduced);
			PrintSep(out, statvec[team][i].energyProduced);
			PrintSep(out, statvec[team][i].metalExcess);
			PrintSep(out, statvec[team][i].energyExcess);
			PrintSep(out, statvec[team][i].metalReceived);
			PrintSep(out, statvec[team][i].energyReceived);
			PrintSep(out, statvec[team][i].metalSent);
			PrintSep(out, statvec[team][i].energySent);
			PrintSep(out, statvec[team][i].damageDealt);
			PrintSep(out, statvec[team][i].damageReceived);
			PrintSep(out, statvec[team][i].unitsProduced);
			PrintSep(out, statvec[team][i].unitsDied);
			PrintSep(out, statvec[team][i].unitsReceived);
			PrintSep(out, statvec[team][i].unitsSent);
			PrintSep(out, statvec[team][i].unitsCaptured);
			PrintSep(out, statvec[team][i].unitsOutCaptured);
			PrintSep(out, statvec[team][i].unitsKilled);
			out << std::endl;
			time += header.teamStatPeriod;
		}
	}
	else
	{
		std::wcout << L"Invalid teamnumber" << std::endl;
		exit(1);
	}
};

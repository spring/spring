#include <string>
#include <iostream>
#include <boost/program_options.hpp>

#include "DemoReader.h"
#include "BaseNetProtocol.h"
#include "Net/RawPacket.h"
#include "StringSerializer.h"

using namespace std;
using namespace netcode;
namespace po = boost::program_options;

/*
Usage:
Start with the full! path to the demofile as the only argument

Please note that not all NETMSG's are implemented, expand if needed.

When compiling for windows with MinGW, make sure to use the
-Wl,-subsystem,console flag when linking, as otherwise there will be
no console output (you still could use this.exe > z.tzt though).
*/

void TrafficDump(CDemoReader& reader, bool trafficStats);
void WriteTeamstatHistory(CDemoReader& reader, unsigned team, const std::string& file);

int main (int argc, char* argv[])
{
	string filename;
	po::variables_map vm;
	{
		po::options_description all;
		all.add_options()("demofile,f", po::value<std::string>(), "Path to demo file");
		po::positional_options_description p;
		p.add("demofile", 1);
		all.add_options()("help,h", "This one");
		all.add_options()("dump,d", "Only dump networc traffic saved in demo");
		all.add_options()("stats,s", "Print all game, player and team stats");
		all.add_options()("header,H", "Print demoheader content");
		all.add_options()("playerstats,p", "Print playerstats");
		all.add_options()("teamstats,t", "Print teamstats");
		all.add_options()("team", po::value<unsigned>(), "Select team");
		all.add_options()("teamsstatcsv", po::value<std::string>(), "Write teamstats in a csv file");

		po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
		po::notify(vm);
		
		if (vm.count("help"))
		{
			cout << "demotool Usage: " << endl;
			all.print(cout);
			return 0;
		}
		if (vm.count("demofile"))
			filename = vm["demofile"].as<std::string>();
		else
		{
			cout << "No demofile given" << endl;
			all.print(cout);
			return 1;
		}
	}
	const bool printStats = vm.count("stats");
	CDemoReader reader(filename, 0.0f);
	reader.LoadStats();
	if (vm.count("dump"))
	{
		TrafficDump(reader, true);
		return 0;
	}
	if (vm.count("teamsstatcsv"))
	{
		const std::string outfile = vm["teamsstatcsv"].as<std::string>();
		if (!vm.count("team"))
		{
			cout << "teamsstatcsv requires a team to select" << endl;
			exit(1);
		}
		unsigned team = vm["team"].as<unsigned>();
		WriteTeamstatHistory(reader, team, outfile);
	}
	
	if (vm.count("header") || printStats)
	{
		wstringstream buf;
		buf << reader.GetFileHeader();
		wcout << buf.str();
	}
	if (vm.count("playerstats") || printStats)
	{
		const std::vector<PlayerStatistics> statvec = reader.GetPlayerStats();
		for (unsigned i = 0; i < statvec.size(); ++i)
		{
			wcout << L"-- Player statistics for player " << i << L" --" << endl;
			wstringstream buf;
			buf << statvec[i];
			wcout << buf.str();
		}
	}
	if (vm.count("teamstats") || printStats)
	{
		const DemoFileHeader header = reader.GetFileHeader();
		const std::vector< std::vector<TeamStatistics> > statvec = reader.GetTeamStats();
		for (unsigned teamNum = 0; teamNum < statvec.size(); ++teamNum)
		{
			int time = 0;
			for (unsigned i = 0; i < statvec[teamNum].size(); ++i)
			{
				wcout << L"-- Team statistics for player " << teamNum << L", game second " << time << L" --" << endl;
				wstringstream buf;
				buf << statvec[teamNum][i];
				time += header.teamStatPeriod;
				wcout << buf.str();
			}
		}
	}
	return 0;
}


void TrafficDump(CDemoReader& reader, bool trafficStats)
{
	std::vector<unsigned> trafficCounter(55, 0);
	int frame = 0;
	while (!reader.ReachedEnd())
	{
		RawPacket* packet;
		packet = reader.GetData(3.40282347e+38f);
		if (packet == 0)
			continue;
		trafficCounter[packet->data[0]] += packet->length;
		const unsigned char* buffer = packet->data;
		char buf[16]; // FIXME: cba to look up how to format numbers with iostreams
		sprintf(buf, "%06d ", frame);
		cout << buf;
		switch ((unsigned char)buffer[0])
		{
			case NETMSG_PLAYERNAME:
				cout << "PLAYERNAME: Playernum: " << (unsigned)buffer[2] << " Name: " << buffer+3 << endl;
				break;
			case NETMSG_SETPLAYERNUM:
				cout << "SETPLAYERNUM: Playernum: " << (unsigned)buffer[1] << endl;
				break;
			case NETMSG_QUIT:
				cout << "QUIT" << endl;
				break;
			case NETMSG_STARTPLAYING:
				cout << "STARTPLAYING" << endl;
				break;
			case NETMSG_STARTPOS:
				cout << "STARTPOS: Playernum: " << (unsigned)buffer[1] << " Team: " << (unsigned)buffer[2] << " Readyness: " << (unsigned)buffer[3] << endl;
				break;
			case NETMSG_SYSTEMMSG:
				cout << "SYSTEMMSG: Player: " << (unsigned)buffer[3] << " Msg: " << (char*)(buffer+4) << endl;
				break;
			case NETMSG_CHAT:
				cout << "CHAT: Player: " << (unsigned)buffer[2] << " Msg: " << (char*)(buffer+4) << endl;
				break;
			case NETMSG_KEYFRAME:
				cout << "KEYFRAME: " << *(int*)(buffer+1) << endl;
				++frame;
				if (*(int*)(buffer+1) != frame) {
					cout << "keyframe mismatch!" << endl;
				}
				break;
			case NETMSG_NEWFRAME:
				cout << "NEWFRAME" << endl;
				++frame;
				break;
			case NETMSG_PLAYERINFO:
				cout << "NETMSG_PLAYERINFO: Player:" << (int)buffer[1] << " Ping: " << *(uint16_t*)&buffer[6] << endl;
				break;
			case NETMSG_LUAMSG:
				cout << "LUAMSG length:" << packet->length << endl;
				break;
			case NETMSG_TEAM:
				cout << "TEAM Playernum:" << (int)buffer[1] << " Action:";
				switch (buffer[2]) {
					case TEAMMSG_GIVEAWAY: cout << "GIVEAWAY"; break;
					case TEAMMSG_RESIGN: cout << "RESIGN"; break;
					case TEAMMSG_TEAM_DIED: cout << "TEAM_DIED"; break;
					case TEAMMSG_JOIN_TEAM: cout << "JOIN_TEAM"; break;
					default: cout << (int)buffer[2];
				}
				cout << " Parameter:" << (int)buffer[3] << endl;
				break;
			case NETMSG_COMMAND:
				cout << "COMMAND Playernum:" << (int)buffer[3] << " Size: " << *(unsigned short*)(buffer+1) << endl;
				if (*(unsigned short*)(buffer+1) != packet->length)
					cout << "      packet length error: expected: " <<  *(unsigned short*)(buffer+1) << " got: " << packet->length << endl;
				break;
			default:
				cout << "MSG: " << (unsigned)buffer[0] << endl;
		}
		delete packet;
	}

	for (unsigned i = 0; i != trafficCounter.size(); ++i)
	{
		if (trafficStats && trafficCounter[i] > 0)
			cout << "Msg " << i << ": " << trafficCounter[i] << endl;
	}
};

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
		out << "Team Statistics for " << team << endl;
		out << "Time[sec];MetalUsed;EnergyUsed;MetalProduced;EnergyProduced;MetalExcess;EnergyExcess;"
		    << "EnergyReceived;MetalSent;EnergySent;DamageDealt;DamageReceived;"
		    << "UnitsProduced;UnitsDied;UnitsReceived;UnitsSent;nitsCaptured;"
		    << "UnitsOutCaptured;UnitsKilled" << endl;
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
			out << endl;
			time += header.teamStatPeriod;
		}
	}
	else
	{
		wcout << L"Invalid teamnumber" << endl;
		exit(1);
	}
};

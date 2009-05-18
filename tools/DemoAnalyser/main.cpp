#include <string>
#include <iostream>
#include <limits>

#include "DemoReader.h"
#include "FileSystem/FileSystem.h"
#include "ConfigHandler.h"
#include "Game/GameSetup.h"
#include "BaseNetProtocol.h"
#include "Net/RawPacket.h"

using namespace std;
using namespace netcode;

/*
Usage:
Start with the full! path to the demofile as the only argument

Please note that not all NETMSG's are implemented, expand if needed.
*/

int main (int argc, char* argv[])
{
	if (argc <= 1)
	{
		cout << "Missing parameter (full path to demo file)\n";
		return 0;
	}

	ConfigHandler::Instantiate("");
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);

	CDemoReader reader(string(argv[1]), 0.0f);
	DemoFileHeader header = reader.GetFileHeader();
	std::vector<unsigned> trafficCounter(55, 0);
	while (!reader.ReachedEnd())
	{
		RawPacket* packet;
		packet = reader.GetData(3.40282347e+38f);
		if (packet == 0)
			continue;
		trafficCounter[packet->data[0]] += packet->length;
		const unsigned char* buffer = packet->data;
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
				cout << "SYSTEMMSG: Player: " << (unsigned)buffer[2] << " Msg: " << (char*)(buffer+3) << endl;
				break;
			case NETMSG_CHAT:
				cout << "CHAT: Player: " << (unsigned)buffer[2] << " Msg: " << (char*)(buffer+4) << endl;
				break;
			case NETMSG_KEYFRAME:
				cout << "KEYFRAME: " << *(int*)(buffer+1) << endl;
				break;
			case NETMSG_NEWFRAME:
				cout << "NEWFRAME" << endl;
				break;
			case NETMSG_LUAMSG:
				cout << "LUAMSG length:" << packet->length << endl;
				break;
			default:
				cout << "MSG: " << (unsigned)buffer[0] << endl;
		}
		delete packet;
	}
	FileSystemHandler::Cleanup();

	for (unsigned i = 0; i != trafficCounter.size(); ++i)
	{
		if (trafficCounter[i] > 0)
			cout << "Msg " << i << ": " << trafficCounter[i] << endl;
	}
	return 0;
}

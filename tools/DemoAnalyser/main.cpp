#include <string>
#include <iostream>
#include <limits>

#include "../../rts/System/DemoReader.h"
#include "System/Platform/FileSystem.h"
#include "Game/GameSetupData.h"
#include "BaseNetProtocol.h"
#include "../../rts/System/Net/RawPacket.h"

CGameSetupData* gameSetup = 0;
using namespace std;

/*
Usage:
Start with the full! path to the demofile as the only argument

Please note that not all NETMSG's are implemented, expand if needed.
*/

int main (int argc, char* argv[])
{
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);

	CDemoReader reader(string(argv[1]), 0.0f);
	DemoFileHeader header = reader.GetFileHeader();
	
	unsigned unknownMessages = 0;
	while (!reader.ReachedEnd())
	{
		RawPacket* packet;
		packet = reader.GetData(3.40282347e+38f);
		if (packet == 0)
			continue;
		const unsigned char* buffer = packet->data;
		switch ((unsigned char)buffer[0])
		{
			case NETMSG_ATTEMPTCONNECT:
				cout << "ATTEMPTCONNECT: Playernum: " << (unsigned)buffer[1] << " Version: " << (unsigned)buffer[2] << endl;
				break;
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
				cout << "CHAT: Player: " << (unsigned)buffer[2] << " Msg: " << (char*)(buffer+3) << endl;
				break;
			default:
				unknownMessages++;
		}
		delete packet;
	}
	cout << endl << "There had been " << unknownMessages << " unknown NETMSGs" << endl;
	FileSystemHandler::Cleanup();
	return 0;
}

#include "Game/GameServer.h"
#include "GameSetupData.h"
#include "System/Platform/FileSystem.h"

CGameSetupData* gameSetup = 0;

int main(int argc, char *argv[])
{
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);
	
	CGameServer* temp = new CGameServer(8452, "Altored_Divide.smf", "Balanced Annihilation V5.91", "Cmds 1000 res");
	sleep(60);
	delete temp;
	
	FileSystemHandler::Cleanup();
	
	return 0;
}

#include "Game/GameServer.h"
#include "GameSetupData.h"
#include "System/Platform/FileSystem.h"


int main(int argc, char *argv[])
{
	FileSystemHandler::Cleanup();
	FileSystemHandler::Initialize(false);
	
	CGameSetupData* gameSetup = 0;
	
	CGameServer* temp = new CGameServer(8452, "Altored_Divide.smf", "Balanced Annihilation V6.0", "Cmds 1000 res", gameSetup);
	while (!temp->HasFinished())
		sleep(1);
	delete temp;
	
	FileSystemHandler::Cleanup();
	
	return 0;
}

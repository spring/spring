#include "Game/GameServer.h"
#include "GameSetupData.h"

CGameSetupData* gameSetup = 0;

int main(int argc, char *argv[])
{
	CGameServer* temp = new CGameServer(8452, "Altored_Divide.smf", "Balanced Annihilation V5.91", "Cmds 1000 res");
	while (1)
	{
		sleep(1);
	}
	delete temp;
	return 0;
}

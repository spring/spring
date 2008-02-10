#include "GameSetupData.h"

CGameSetupData::CGameSetupData()
{
	gameSetupText=0;
	startPosType=StartPos_Fixed;
	numDemoPlayers=0;
	hostDemo=false;
	autohostport = 0;
}

CGameSetupData::~CGameSetupData()
{
	if (gameSetupText)
		delete[] gameSetupText;
}

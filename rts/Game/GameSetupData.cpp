#include "GameSetupData.h"

CGameSetupData::CGameSetupData()
{
	gameSetupText=0;
	startPosType=StartPos_Fixed;
	numDemoPlayers=0;
	hostDemo=false;
}

CGameSetupData::~CGameSetupData()
{
	if (gameSetupText)
		delete[] gameSetupText;
}

#include "GameSetupData.h"

CGameSetupData::CGameSetupData()
{
	readyTime=0;
	gameSetupText=0;
	startPosType=StartPos_Fixed;
	numDemoPlayers=0;
	hostDemo=false;
	forceReady=false;
}

CGameSetupData::~CGameSetupData()
{
	if (gameSetupText)
		delete[] gameSetupText;
}

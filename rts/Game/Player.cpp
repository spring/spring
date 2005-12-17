#include "StdAfx.h"
// Player.cpp: implementation of the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

#include "Player.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	currentStats=new Statistics;
	memset(currentStats,0,sizeof(Statistics));
	POP_CODE_MODE;

	active=false;
	playerName="Player";
	spectator=false;
	team=0;
	readyToStart=false;
	cpuUsage=0;
	ping=0;


#ifdef DIRECT_CONTROL_ALLOWED
	playerControlledUnit=0;

	myControl.forward=0;
	myControl.back=0;
	myControl.left=0;
	myControl.right=0;

	myControl.mouse1=0;
	myControl.mouse2=0;
	myControl.viewDir=float3(0,0,1);
	myControl.targetPos=float3(0,0,1);
	myControl.targetDist=1000;
	myControl.target=0;
	myControl.myController=this;
#endif
}

CPlayer::~CPlayer()
{
	delete currentStats;
}

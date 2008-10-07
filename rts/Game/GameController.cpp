#include "StdAfx.h"

#include "mmgr.h"

#include "GameController.h"


CGameController::CGameController(void)
{
	userWriting = false;

	writingPos = 0;

	ignoreNextChar = false;

	ignoreChar = 0;
}



CGameController::~CGameController(void)
{

}



bool CGameController::Draw(void)
{
	return true;
}
/*
bool CGameController::Draw2(void)
{
	return true;
}*/
/*bool CGameController::DrawMT(void)
{
	return true;
}*/

bool CGameController::Update(void)
{
	return true;
}



int CGameController::KeyPressed(unsigned short k,bool isRepeat)
{
	return 0;
}



int CGameController::KeyReleased(unsigned short k)
{
	return 0;
}


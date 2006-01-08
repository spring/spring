#include "StdAfx.h"

#include "GameController.h"

#include "mmgr.h"



CGameController::CGameController(void)

{

	userWriting=false;

	ignoreNextChar=false;

	ignoreChar=0;

}



CGameController::~CGameController(void)

{

}



bool CGameController::Draw(void)

{

	return true;

}



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


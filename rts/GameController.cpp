#include "stdafx.h"

#include ".\gamecontroller.h"

//#include "mmgr.h"



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



int CGameController::KeyPressed(unsigned char k,bool isRepeat)

{

	return 0;

}



int CGameController::KeyReleased(unsigned char k)

{

	return 0;

}


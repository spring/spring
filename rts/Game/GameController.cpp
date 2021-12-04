/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameController.h"

CGameController* activeController = nullptr;

CGameController::~CGameController()
{
	if (activeController != this)
		return;

	activeController = nullptr;
}


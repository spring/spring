/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "GameController.h"
#include "Platform/Clipboard.h"


CGameController* activeController = NULL;


CGameController::CGameController()
{
	userWriting = false;
	writingPos = 0;
	ignoreNextChar = false;
	ignoreChar = 0;
}


CGameController::~CGameController()
{
	if (activeController == this)
		activeController = NULL;
}


bool CGameController::Draw()
{
	return true;
}


bool CGameController::Update()
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


void CGameController::PasteClipboard()
{
	CClipboard clipboard;
	const std::string text = clipboard.GetContents();
	userInput.insert(writingPos, text);
	writingPos += text.length();
}

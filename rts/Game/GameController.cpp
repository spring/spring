/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GameController.h"
#include "System/Platform/Clipboard.h"


CGameController* activeController = NULL;


CGameController::CGameController()
	: userWriting(false)
	, writingPos(0)
	, ignoreNextChar(false)
{
}


CGameController::~CGameController()
{
	if (activeController == this) {
		activeController = NULL;
	}
}


bool CGameController::Draw()
{
	return true;
}


bool CGameController::Update()
{
	return true;
}


int CGameController::KeyPressed(int key, bool isRepeat)
{
	return 0;
}


int CGameController::KeyReleased(int key)
{
	return 0;
}


int CGameController::TextInput(const std::string& utf8Text)
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

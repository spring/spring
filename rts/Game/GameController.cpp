/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GameController.h"
#include "System/Platform/Clipboard.h"


CGameController* activeController = nullptr;

CGameController::CGameController()
	: userWriting(false)
	, writingPos(0)
	, ignoreNextChar(false)
{
}

CGameController::~CGameController()
{
	if (activeController == this) {
		activeController = nullptr;
	}
}


void CGameController::PasteClipboard()
{
	CClipboard clipboard;
	const std::string text = clipboard.GetContents();
	userInput.insert(writingPos, text);
	writingPos += text.length();
}

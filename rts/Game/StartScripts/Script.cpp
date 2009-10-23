// Script.cpp: implementation of the CScript class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "Script.h"
#include "ScriptHandler.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScript::CScript(const std::string& name)
{
	CScriptHandler::Instance().AddScript(name,this);
	wantCameraControl=false;
	onlySinglePlayer=false;
	loadGame=false;
}

CScript::~CScript()
{

}

void CScript::Update()
{
}

void CScript::SetCamera()
{

}

void CScript::GotChatMsg(const std::string& msg, int player)
{
}

void CScript::ScriptSelected()
{
}

void CScript::GameStart()
{
}

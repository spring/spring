// Script.cpp: implementation of the CScript class.
//
//////////////////////////////////////////////////////////////////////

#include "System/StdAfx.h"
#include "Script.h"
#include "ScriptHandler.h"
//#include "System/mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScript::CScript(const std::string& name)
{
	CScriptHandler::Instance()->AddScript(name,this);
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

std::string CScript::GetMapName(void)
{
	return "";
}

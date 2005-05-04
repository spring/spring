// IGroupAI.h: interface for the IGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IGROUPAI_H__7A933264_A3D8_4969_9003_3122E2512161__INCLUDED_)
#define AFX_IGROUPAI_H__7A933264_A3D8_4969_9003_3122E2512161__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "command.h"
class IGroupAiCallback;

#define AI_INTERFACE_VERSION 1

class IGroupAI  
{
public:
	virtual void InitAi(IGroupAiCallback* callback)=0;
	virtual bool AddUnit(int unit)=0;										//group should return false if it doenst want the unit for some reason
	virtual void RemoveUnit(int unit)=0;								//no way to refuse giving up a unit

	virtual void GiveCommand(Command* c)=0;							//the group is given a command by the player
	virtual const vector<CommandDescription>& GetPossibleCommands()=0;		//the ai tells the interface what commands it can take (note that it returns a reference so it must keep the vector in memory itself)
	virtual int GetDefaultCmd(int unitid)=0;															//the default command for the ai given that the mouse pointer hovers above unit unitid (or no unit if unitid=0)

	virtual void CommandFinished(int unit,int type)=0;										//a specific unit has finished a specific command, might be a good idea to give new orders to it

	virtual void Update()=0;																							//called once a frame (30 times a second)
};

#endif // !defined(AFX_IGROUPAI_H__7A933264_A3D8_4969_9003_3122E2512161__INCLUDED_)


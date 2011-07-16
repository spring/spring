/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include <assert.h>

#include "System/mmgr.h"

#include "Console.h" 

#include "System/LogOutput.h"
#include "Action.h"

void CommandReceiver::RegisterAction(const std::string& name)
{
	Console::Instance().AddCommandReceiver(name, this);
}

Console& Console::Instance()
{
	static Console myInstance;
	return myInstance;
}

void Console::AddCommandReceiver(const std::string& name, CommandReceiver* rec)
{
	if (commandMap.find(name) != commandMap.end()) {
		logOutput.Print("Overwriting command: %s", name.c_str());
	}
	commandMap[name] = rec;
}

bool Console::ExecuteAction(const Action& action)
{
	if (action.command == "commands")
	{
		logOutput.Print("Registered commands:");
		std::map<const std::string, CommandReceiver*>::const_iterator cri;
		for (cri = commandMap.begin(); cri != commandMap.end(); ++cri)
		{
			logOutput.Print(cri->first);
		}
		return true;
	}

	std::map<const std::string, CommandReceiver*>::iterator cri = commandMap.find(action.command);
	if (cri == commandMap.end()) {
		return false;
	} else {
		cri->second->PushAction(action);
		return true;
	}
}


Console::Console()
{
}

Console::~Console()
{
}


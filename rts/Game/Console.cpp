/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Console.h" 

#include "System/Log/ILog.h"
#include "Action.h"

#include <cassert>


void CommandReceiver::RegisterAction(const std::string& name)
{
	commandConsole.AddCommandReceiver(name, this);
}


CommandConsole& CommandConsole::Instance()
{
	// commandMap gets cleared by CGame, so this is fine wrt. reloading
	static CommandConsole myInstance;
	return myInstance;
}

void CommandConsole::AddCommandReceiver(const std::string& name, CommandReceiver* rec)
{
	if (commandMap.find(name) != commandMap.end()) {
		LOG_L(L_WARNING, "Overwriting command: %s", name.c_str());
	}

	commandMap[name] = rec;
}

bool CommandConsole::ExecuteAction(const Action& action)
{
	if (action.command == "commands") {
		LOG("Registered commands:");

		for (auto cri = commandMap.cbegin(); cri != commandMap.cend(); ++cri) {
			LOG("%s", cri->first.c_str());
		}

		return true;
	}

	const auto cri = commandMap.find(action.command);

	if (cri == commandMap.end())
		return false;

	cri->second->PushAction(action);
	return true;
}


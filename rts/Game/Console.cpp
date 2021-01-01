/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Console.h"

#include "System/Log/ILog.h"
#include "Action.h"

#include <cassert>
#include <algorithm>

CommandConsole gameCommandConsole;


void CommandReceiver::RegisterAction(const std::string& name) { gameCommandConsole.AddCommandReceiver(name, this); }
void CommandReceiver::SortRegisteredActions() { gameCommandConsole.SortCommandMap(); }


void CommandConsole::SortCommandMap()
{
	const auto cmpPred = [](const CmdPair& a, const CmdPair& b) { return (a.first <  b.first); };
	const auto dupPred = [](const CmdPair& a, const CmdPair& b) { return (a.first == b.first); };

	std::sort(commandMap.begin(), commandMap.end(), cmpPred);

	const auto end = commandMap.end();
	const auto iter = std::unique(commandMap.begin(), end, dupPred);

	commandMap.erase(iter, end);
}

bool CommandConsole::ExecuteAction(const Action& action)
{
	if (action.command == "commands") {
		LOG("Registered commands:");

		for (const auto& pair: commandMap) {
			LOG("%s", pair.first.c_str());
		}

		return true;
	}

	const auto pred = [](const CmdPair& a, const CmdPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(commandMap.begin(), commandMap.end(), CmdPair{action.command, nullptr}, pred);

	if (iter == commandMap.end() || iter->first != action.command)
		return false;

	iter->second->PushAction(action);
	return true;
}


#include "Console.h" 

#include "LogOutput.h"
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
	if (commandMap.find(name) != commandMap.end())
		logOutput.Print("Overwriting command: %s", name.c_str());
	commandMap[name] = rec;
}

bool Console::ExecuteAction(const Action& action)
{
	if (action.command == "commands")
	{
		logOutput.Print("Registered commands:");
		for (std::map<const std::string, CommandReceiver*>::iterator it = commandMap.begin(); it != commandMap.end(); ++it)
		{
			logOutput.Print(it->first);
		}
		return true;
	}
	
	std::map<const std::string, CommandReceiver*>::iterator it = commandMap.find(action.command);
	if (it == commandMap.end())
		return false;
	else
	{
		it->second->PushAction(action);
		return true;
	}
}

Console::Console()
{
}

Console::~Console()
{
}


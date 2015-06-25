/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <map>
#include <string>

class Action;

// TODO convert existing CommandReceiver's to use IUnsyncedActionExecutor's instead
/**
 * @brief this class can recieve commands (actions)
 * @deprecated Use IUnsyncedActionExecutor instead
 */
class CommandReceiver
{
public:
	CommandReceiver() {}
	virtual ~CommandReceiver() {}
	
	/**
	 * @brief callback function for all registered commands
	 */
	virtual void PushAction(const Action& action) = 0;

protected:
	/**
	 * @brief register a command
	 *
	 * PushAction will be called if this command is received by the console
	 */
	void RegisterAction(const std::string& name);
};


/**
 * @brief handles and forwards commands
 */
class CommandConsole
{
public:
	static CommandConsole& Instance();
	
	/**
	 * @brief register a command
	 * @param name the name of the command (e.g. "cheat")
	 * @param rec the CommandReceiver who want to recieve the command
	 */
	void AddCommandReceiver(const std::string& name, CommandReceiver* rec);
	
	/**
	 * @brief Execute an action
	 */
	bool ExecuteAction(const Action&);

	void ResetState() { commandMap.clear(); }

private:
	std::map<const std::string, CommandReceiver*> commandMap;
};

#define commandConsole (CommandConsole::Instance())
#endif // CONSOLE_H


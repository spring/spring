/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <map>
#include <string>

class Action;

/**
@brief this class can recieve commands (actions)
 */
class CommandReceiver
{
public:
	CommandReceiver() {};
	virtual ~CommandReceiver() {};
	
	/**
	@brief callback function for all registered commands
	*/
	virtual void PushAction(const Action&) = 0;

protected:
	/**
	@brief register a command

	PushAction will be called if this command is received by the console
	*/
	void RegisterAction(const std::string& name);
};

/**
@brief handles and forward commands
*/
class Console
{
public:
	static Console& Instance();
	
	/**
	@brief register a command
	@param name the name of the command (e.g. "cheat")
	@param rec the CommandReceiver who want to recieve the command
	*/
	void AddCommandReceiver(const std::string& name, CommandReceiver* rec);
	
	/**
	@brief Execute an action
	*/
	bool ExecuteAction(const Action&);

private:
	Console();
	~Console();
	std::map<const std::string, CommandReceiver*> commandMap;
};

#endif

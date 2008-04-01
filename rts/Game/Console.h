#ifndef CONSOLE_H
#define CONSOLE_H

#include <map>
#include <string>

class Action;

/**
@brief this class can recieve commands (actions)
 */
class CommandReciever
{
public:
	CommandReciever() {};
	virtual ~CommandReciever() {};
	
	/**
	@brief callback function for all registered commands
	*/
	virtual void PushAction(const Action&) = 0;

protected:
	/**
	@brief register a command

	PushAction will be called if this command is recieved by the console
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
	@param rec the CommandReciever who want to recieve the command
	*/
	void AddCommandReciever(const std::string& name, CommandReciever* rec);
	
	/**
	@brief Execute an action
	*/
	bool ExecuteAction(const Action&);

private:
	Console();
	~Console();
	std::map<const std::string, CommandReciever*> commandMap;
};

#endif

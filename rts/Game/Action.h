/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ACTION_H
#define ACTION_H

#include <string>

class Action
{
public:
	Action() {}
	Action(const std::string& line);

	std::string command;   ///< first word, lowercase
	std::string extra;     ///< everything but the first word
	std::string rawline;   ///< includes the command, case preserved
	std::string boundWith; ///< the string that defined the binding keyset
};

class IActionExecutor
{
protected:
	IActionExecutor(const std::string& command, bool cheatRequired = false)
		: command(command)
		, cheatRequired(cheatRequired)
	{}

public:
	virtual ~IActionExecutor() {}

	/**
	 * Returns the command string that is unique for this executor.
	 */
	const std::string& GetCommand() const { return command; }

	/**
	 * Returns the command string that is unique for this executor.
	 */
	bool IsCheatRequired() const { return cheatRequired; }

	/**
	 * Executes one instance of an action of this type.
	 * Does a few checks internally, and then calls Execute(args).
	 */
	void ExecuteAction(const Action& action, int playerID) const;

protected:
	/**
	 * Executes one instance of an action of this type.
	 */
	virtual void Execute(const std::string& args, int playerID) const = 0;

private:
	std::string command;
	bool cheatRequired;
};

#endif // ACTION_H

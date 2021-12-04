/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONSOLE_HISTORY_H
#define CONSOLE_HISTORY_H

#include <string>
#include <list>

class CConsoleHistory
{
public:
	CConsoleHistory();
	~CConsoleHistory();

	void ResetPosition();
	bool AddLine(const std::string& msg);
	std::string NextLine(const std::string& current);
	std::string PrevLine(const std::string& current);

protected:
	bool AddLineRaw(const std::string& msg);

	std::list<std::string> lines;
	std::list<std::string>::const_iterator pos;
	static const unsigned int MAX_LINES;
};

#endif /* CONSOLE_HISTORY_H */

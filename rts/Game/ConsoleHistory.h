/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONSOLE_HISTORY_H
#define CONSOLE_HISTORY_H

#include <string>
#include <deque>

class CConsoleHistory
{
public:
	CConsoleHistory() { Init(); }

	void Init();
	void ResetPosition();
	bool AddLine(const std::string& msg);

	std::string NextLine(const std::string& current);
	std::string PrevLine(const std::string& current);

protected:
	bool AddLineRaw(const std::string& msg);

	std::deque<std::string> lines;
	std::size_t pos = 0;

	static constexpr unsigned int MAX_LINES = 256;
};

extern CConsoleHistory gameConsoleHistory;

#endif


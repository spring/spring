#ifndef CONSOLE_HISTORY_H
#define CONSOLE_HISTORY_H
// ConsoleHistory.h: interface for the CConsoleHistory class.
//
//////////////////////////////////////////////////////////////////////

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

protected:
	std::list<std::string> lines;
	std::list<std::string>::const_iterator pos;
	static unsigned int MaxLines;
};

#endif /* CONSOLE_HISTORY_H */

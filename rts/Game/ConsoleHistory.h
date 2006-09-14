#ifndef CONSOLE_HISTORY_H
#define CONSOLE_HISTORY_H
// ConsoleHistory.h: interface for the CConsoleHistory class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <list>

using namespace std;

class CConsoleHistory 
{
public:
	CConsoleHistory();
	~CConsoleHistory();
	void ResetPosition();
	bool AddLine(const string& msg);
	string NextLine(const string& current);
	string PrevLine(const string& current);
	
protected:
	bool AddLineRaw(const string& msg);

protected:
	list<string> lines;
	list<string>::const_iterator pos;
	static unsigned int MaxLines;
};

#endif /* CONSOLE_HISTORY_H */

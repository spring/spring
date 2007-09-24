#ifndef UNITSYNC_LOGGING_H
#define UNITSYNC_LOGGING_H

#include <cstdio>

class CLogOutput {
	std::FILE* file;
public:
//	void AddLine(const char *, ...);
	CLogOutput();
	~CLogOutput();
	void Print(const string& text);
	void Print(const char* fmt, ...);
};

extern CLogOutput logOutput;

#endif

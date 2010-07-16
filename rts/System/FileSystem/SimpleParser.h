/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __SIMPLE_PARSER_H__
#define __SIMPLE_PARSER_H__

#include <string>
#include <vector>

#include "FileHandler.h"

class CSimpleParser
{
public:
	static std::vector<std::string> Tokenize(const std::string& line, int minWords = 0);

	CSimpleParser(CFileHandler& fh);

	int GetLineNumber() const;
	std::string GetLine();
	std::string GetCleanLine();

private:
	CFileHandler& file;
	int lineNumber;
	bool inComment;
};

#endif // __SIMPLE_PARSER_H__


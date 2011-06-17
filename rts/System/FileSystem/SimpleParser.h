/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SIMPLE_PARSER_H_
#define _SIMPLE_PARSER_H_

#include <string>
#include <vector>

class CFileHandler;


/**
 * Allows to tokenize a string or a text-files content based on white-space.
 */
class CSimpleParser
{
public:
	/** Splits a string based on white-space. */
	static std::vector<std::string> Tokenize(const std::string& line, int minWords = 0);

	CSimpleParser(CFileHandler& fh);

	/** Returns the current line number. */
	int GetLineNumber() const;
	/** Returns the next line (without newlines). */
	std::string GetLine();
	/** Returns the next non-blank line (without newlines or comments). */
	std::string GetCleanLine();

private:
	CFileHandler& file;
	int lineNumber;
	bool inComment;
};

#endif // _SIMPLE_PARSER_H_


/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SIMPLE_PARSER_H_
#define _SIMPLE_PARSER_H_

#include <string>
#include <vector>

class CFileHandler;


/**
 * Allows one to tokenize a string or a text-files content based on white-space.
 */
class CSimpleParser
{
public:
	/** Splits a string based on white-space. */
	static std::vector<std::string> Tokenize(const std::string& line, int minWords = 0);
	/** Splits a string based on a set of delimitters. */
	static std::vector<std::string> Split(const std::string& str, const std::string& delimitters);

	CSimpleParser(CFileHandler& fh);
	CSimpleParser(const std::string& filecontent);

	/** Returns the current line number. */
	int GetLineNumber() const;
	/** Returns the next line (without newlines). */
	std::string GetLine();
	/** Returns the next non-blank line (without newlines or comments). */
	std::string GetCleanLine();

	bool Eof() const {
		return (curPos >= file.size());
	}

private:
	std::string file;
	std::string::size_type curPos;
	int lineNumber;
//	bool inComment;
};

#endif // _SIMPLE_PARSER_H_


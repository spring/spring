#ifndef __SIMPLE_PARSER_H__
#define __SIMPLE_PARSER_H__
// SimpleParser.h: interface for the SimpleParser namespace.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <vector>
using namespace std;

#include "System/FileSystem/FileHandler.h"


namespace SimpleParser {

	// call this before using GetLine() or GetCleanLine()
	void Init();

	// returns the current line number
	int GetLineNumber();

	// returns next line (without newlines)
	string GetLine(CFileHandler& fh);
	
	// returns next non-blank line (without newlines or comments)
	string GetCleanLine(CFileHandler& fh);
	
	// splits a string based on white space
	vector<string> Tokenize(const string& line, int minWords = 0);
};


#endif // __SIMPLE_PARSER_H__

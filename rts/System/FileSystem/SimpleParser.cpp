#include "StdAfx.h"
// SimpleParser.cpp: implementation of the SimpleParser class.
//
//////////////////////////////////////////////////////////////////////
#include <sstream>
#include "mmgr.h"

#include "SimpleParser.h"

using namespace std;


/******************************************************************************/

/** call this before using GetLine() or GetCleanLine() */
CSimpleParser::CSimpleParser(CFileHandler& fh) : file(fh)
{
	lineNumber = 0;
	inComment = false; // /* text */ comments are not implemented
}


/** returns the current line number */
int CSimpleParser::GetLineNumber()
{
	return lineNumber;
}


/** returns next line (without newlines) */
string CSimpleParser::GetLine()
{
	lineNumber++;
	stringstream s;
	while (!file.Eof()) {
		char a = '\n'; // break if this is not overwritten
		file.Read(&a, 1);
		if (a == '\n') { break; }
		if (a != '\r') { s << a; }
	}
	return s.str();
}


/** returns next non-blank line (without newlines or comments) */
string CSimpleParser::GetCleanLine()
{
	string::size_type pos;
	while (true) {
		if (file.Eof()) {
			return ""; // end of file
		}
		string line = GetLine();

		pos = line.find_first_not_of(" \t");
		if (pos == string::npos) {
			continue; // blank line
		}

		pos = line.find("//");
		if (pos != string::npos) {
			line.erase(pos);
			pos = line.find_first_not_of(" \t");
			if (pos == string::npos) {
				continue; // blank line (after removing comments)
			}
		}

		return line;
	}
}


/** splits a string based on white space */
vector<string> CSimpleParser::Tokenize(const string& line, int minWords)
{
	vector<string> words;
	string::size_type start;
	string::size_type end = 0;
	while (true) {
		start = line.find_first_not_of(" \t", end);
		if (start == string::npos) {
			break;
		}
		string word;
		if ((minWords > 0) && ((int)words.size() >= minWords)) {
			word = line.substr(start);
			// strip trailing whitespace
			string::size_type pos = word.find_last_not_of(" \t");
			if (pos != (word.size() - 1)) {
				word.resize(pos + 1);
			}
			end = string::npos;
		}
		else {
			end = line.find_first_of(" \t", start);
			if (end == string::npos) {
				word = line.substr(start);
			} else {
				word = line.substr(start, end - start);
			}
		}
		words.push_back(word);
		if (end == string::npos) {
			break;
		}
	}

	return words;
}


/******************************************************************************/

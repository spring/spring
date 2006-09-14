#include "StdAfx.h"
// SimpleParser.cxx: implementation of the SimpleParser namespace.
//
//////////////////////////////////////////////////////////////////////
#include "SimpleParser.h"


/******************************************************************************/

static int lineNumber = 0;
static bool inComment = false; // /* text */ comments are not implemented


/******************************************************************************/

void SimpleParser::Init()
{
	lineNumber = 0;
	inComment = false;
}


int SimpleParser::GetLineNumber()
{
	return lineNumber;
}


// returns next line (without newlines)
string SimpleParser::GetLine(CFileHandler& fh)
{
	lineNumber++;
	char a;
	string s = "";
	while (true) {
		a = fh.Peek();
		if (a == EOF)  { break; }
		fh.Read(&a, 1);
		if (a == '\n') { break; }
		if (a != '\r') { s += a; }
	}
	return s;
}


// returns next non-blank line (without newlines or comments)
string SimpleParser::GetCleanLine(CFileHandler& fh)
{
	string::size_type pos;
	while (true) {
		if (fh.Eof()) {
			return ""; // end of file
		}
		string line = GetLine(fh);

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


vector<string> SimpleParser::Tokenize(const string& line, int minWords)
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
		if ((minWords > 0) && (words.size() >= minWords)) {
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

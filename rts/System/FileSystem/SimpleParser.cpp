/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SimpleParser.h"
#include "FileHandler.h"

#include <sstream>


CSimpleParser::CSimpleParser(CFileHandler& fh)
	: curPos(0)
	, lineNumber(0)
//	, inComment(false) // /* text */ comments are not implemented
{
	fh.LoadStringData(file);
}

CSimpleParser::CSimpleParser(const std::string& filecontent)
	: file(filecontent)
	, curPos(0)
	, lineNumber(0)
//	, inComment(false) // /* text */ comments are not implemented
{
}



int CSimpleParser::GetLineNumber() const
{
	return lineNumber;
}


std::string CSimpleParser::GetLine()
{
	lineNumber++;
	std::stringstream s;
	while (curPos < file.size()) {
		char& a = file[curPos++];
		if (a == '\n') { break; }
		if (a != '\r') { s << a; }
	}
	return s.str();
}


std::string CSimpleParser::GetCleanLine()
{
	std::string::size_type pos;
	while (true) {
		if (curPos >= file.size()) {
			return ""; // end of file
		}
		std::string line = GetLine();

		pos = line.find_first_not_of(" \t");
		if (pos == std::string::npos) {
			continue; // blank line
		}

		pos = line.find("//");
		if (pos != std::string::npos) {
			line.erase(pos);
			pos = line.find_first_not_of(" \t");
			if (pos == std::string::npos) {
				continue; // blank line (after removing comments)
			}
		}

		return line;
	}
}


std::vector<std::string> CSimpleParser::Tokenize(const std::string& line, int minWords)
{
	std::vector<std::string> words;
	std::string::size_type start;
	std::string::size_type end = 0;
	while (true) {
		start = line.find_first_not_of(" \t", end);
		if (start == std::string::npos) {
			break;
		}
		std::string word;
		if ((minWords > 0) && ((int)words.size() >= minWords)) {
			word = line.substr(start);
			// strip trailing whitespace
			std::string::size_type pos = word.find_last_not_of(" \t");
			if (pos != (word.size() - 1)) {
				word.resize(pos + 1);
			}
			end = std::string::npos;
		}
		else {
			end = line.find_first_of(" \t", start);
			if (end == std::string::npos) {
				word = line.substr(start);
			} else {
				word = line.substr(start, end - start);
			}
		}
		words.push_back(word);
		if (end == std::string::npos) {
			break;
		}
	}

	return words;
}


std::vector<std::string> CSimpleParser::Split(const std::string& str, const std::string& delimitters) {

	std::vector<std::string> tokens;

	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimitters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos     = str.find_first_of(delimitters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters. Note the "not_of"
		lastPos = str.find_first_not_of(delimitters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimitters, lastPos);
	}

	return tokens;
}

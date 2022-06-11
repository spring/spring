/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <vector>
#include <algorithm>


#include "Action.h"
#include "System/FileSystem/SimpleParser.h"


static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}


Action::Action(const std::string& l)
	: rawline(l)
{
	const std::vector<std::string> &words = CSimpleParser::Tokenize(l, 1);
	if (!words.empty()) {
		command.resize(words[0].length());
		std::transform(words[0].begin(), words[0].end(), command.begin(), (int (*)(int))tolower);
	}
	if (words.size() > 1) {
		extra = words[1].substr(0, words[1].find("//", 0));
		rtrim(extra);
	}

	line = command + extra;
}

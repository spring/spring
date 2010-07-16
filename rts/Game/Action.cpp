/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <vector>
#include <algorithm>

#include "mmgr.h"

#include "Action.h"
#include "FileSystem/SimpleParser.h"

Action::Action(const std::string& line)
{
	rawline = line;
	const std::vector<std::string> words = CSimpleParser::Tokenize(line, 1);
	if (words.size() > 0) {
		command.resize(words[0].length());
		std::transform(words[0].begin(), words[0].end(), command.begin(), (int (*)(int))tolower);
	}
	if (words.size() > 1) {
		extra = words[1];
	}
}

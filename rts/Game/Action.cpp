#include <vector>
#include <algorithm>

#include "mmgr.h"

#include "Action.h"
#include "System/FileSystem/SimpleParser.h"

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

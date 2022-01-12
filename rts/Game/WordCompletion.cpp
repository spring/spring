/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//  TODO: replace 'WordProperties' with regex/glob expressions
//        ex: '/give [0-9]* [_0-9a-zA-Z]+'
//        or even create a domain specific language
//        ex: '/give <ammount> {unitname} <team-ID>'
//        user-input: '/give 10 armcom 1'


#include "WordCompletion.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"

#include <stdexcept>
#include <algorithm>

CWordCompletion wordCompletion;

void CWordCompletion::Init()
{
	words.clear();
	words.reserve(256);

	// mini-map (Game/UI/MiniMap.cpp)
	AddWordRaw("fullproxy ", false, false, true);
	AddWordRaw("drawcommands ", false, false, true);
	AddWordRaw("drawprojectiles ", false, false, true);
	AddWordRaw("icons ", false, false, true);
	AddWordRaw("unitexp ", false, false, true);
	AddWordRaw("unitsize ", false, false, true);
	AddWordRaw("simplecolors ", false, false, true);
	AddWordRaw("geometry ", false, false, true);
	AddWordRaw("minimize ", false, false, true);
	AddWordRaw("maximize ", false, false, true);
	AddWordRaw("maxspect ", false, false, true);

	// commands not yet handled by *ActionExecutor's
	AddWordRaw("/nopause ", true, false, false);
	AddWordRaw("/setmaxspeed ", true, false, false);
	AddWordRaw("/setminspeed ", true, false, false);
	AddWordRaw("/kick ", true, false, false);
	AddWordRaw("/kickbynum ", true, false, false);
	AddWordRaw("/mutebynum ", true, false, false);
}


void CWordCompletion::Sort() {
	std::sort(words.begin(), words.end(), [](const WordEntry& a, const WordEntry& b) { return (a.first < b.first); });
}

void CWordCompletion::Filter() {
	words.erase(std::unique(words.begin(), words.end(), [](const WordEntry& a, const WordEntry& b) { return (a.first == b.first); }), words.end());
}


bool CWordCompletion::AddWord(const std::string& word, bool startOfLine, bool unitName, bool miniMap)
{
	if (word.empty())
		return false;

	// assumes <words> is already sorted
	const auto pred = [](const WordEntry& a, const WordEntry& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(words.begin(), words.end(), WordEntry{word, {}}, pred);

	if (iter != words.end() && iter->first == word)
		return false;

	words.emplace_back(word, WordProperties(startOfLine, unitName, miniMap));

	for (size_t i = words.size() - 1; i > 0; i--) {
		if (words[i - 1].first < words[i].first)
			break;

		std::swap(words[i - 1], words[i]);
	}

	return true;
}

bool CWordCompletion::AddWordRaw(const std::string& word, bool startOfLine, bool unitName, bool miniMap)
{
	if (word.empty())
		return false;

	// assumes <words> is still unsorted
	words.emplace_back(word, WordProperties(startOfLine, unitName, miniMap));
	return true;
}

bool CWordCompletion::RemoveWord(const std::string& word)
{
	const auto pred = [](const WordEntry& a, const WordEntry& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(words.begin(), words.end(), WordEntry{word, {}}, pred);

	if (iter == words.end() || iter->first != word)
		return false;

	words.erase(iter);
	return true;
}


std::vector<std::string> CWordCompletion::Complete(std::string& msg) const
{
	std::vector<std::string> partials;

	const bool unitName = (msg.find("/give ") == 0);
	const bool miniMap = (msg.find("/minimap ") == 0);

	// strip "a:" and "s:" prefixes
	std::string prefix;
	std::string rawmsg;

	if ((msg.find_first_of("aAsS") == 0) && (msg[1] == ':')) {
		prefix = msg.substr(0, 2);
		rawmsg = msg.substr(2);
	} else {
		rawmsg = msg;
	}


	const std::string::size_type spacePos = rawmsg.find_last_of(' ');
	const bool startOfLine = (spacePos == std::string::npos);
	const std::string::size_type startPos = (spacePos + 1) * (1 - startOfLine);

	// the word fragment
	const std::string fragment(rawmsg, startPos, std::string::npos);

	if (fragment.empty())
		return partials;

	// pick a decent spot to start scanning
	const auto pred = [](const WordEntry& a, const WordEntry& b) { return (a.first < b.first); };
	const auto start = std::lower_bound(words.begin(), words.end(), WordEntry{fragment, {}}, pred);

	// make a list of valid possible matches
	for (auto it = start; it != words.end(); ++it) {
		const int cmp = it->first.compare(0, fragment.size(), fragment);

		if (cmp < 0) continue;
		if (cmp > 0) break;

		if ((!it->second.startOfLine || startOfLine) &&
		    (!it->second.unitName    || unitName)    &&
		    (!it->second.miniMap     || miniMap)) {
			partials.push_back(it->first);
		}
	}

	if (partials.empty())
		return partials; // no possible matches

	if (partials.size() == 1) {
		msg = prefix + rawmsg.substr(0, startPos) + partials[0];
		partials.clear();
		return partials; // single match
	}

	// add as much of the match as possible
	const std::string& firstStr = partials[0];
	const std::string& lastStr = partials[partials.size() - 1];

	unsigned int i = 0;
	unsigned int least = std::min(firstStr.size(), lastStr.size());

	for (i = 0; i < least; i++) {
		if (firstStr[i] != lastStr[i])
			break;
	}

	msg = prefix + rawmsg.substr(0, startPos) + partials[0].substr(0, i);

	return partials;
}

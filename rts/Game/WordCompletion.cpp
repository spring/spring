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
	words.reserve(64);

	WordProperties sl(true, false, false);

	// key bindings (Game/UI/KeyBindings.cpp)
	words.emplace_back("/bind ", sl);
	words.emplace_back("/unbind ", sl);
	words.emplace_back("/unbindall ", sl);
	words.emplace_back("/unbindaction ", sl);
	words.emplace_back("/unbindkeyset ", sl);
	words.emplace_back("/keyload ", sl);
	words.emplace_back("/keyreload ", sl);
	words.emplace_back("/keysave ", sl);
	words.emplace_back("/keysyms ", sl);
	words.emplace_back("/keycodes ", sl);
	words.emplace_back("/keyprint ", sl);
	words.emplace_back("/keydebug ", sl);
	words.emplace_back("/fakemeta ", sl);

	// camera handler (Game/CameraHandler.cpp)
	words.emplace_back("/viewtaflip ", sl);

	// mini-map (Game/UI/MiniMap.cpp)
	WordProperties mm(false, false, true);
	words.emplace_back("fullproxy ", mm);
	words.emplace_back("drawcommands ", mm);
	words.emplace_back("drawprojectiles ", mm);
	words.emplace_back("icons ", mm);
	words.emplace_back("unitexp ", mm);
	words.emplace_back("unitsize ", mm);
	words.emplace_back("simplecolors ", mm);
	words.emplace_back("geometry ", mm);
	words.emplace_back("minimize ", mm);
	words.emplace_back("maximize ", mm);
	words.emplace_back("maxspect ", mm);

	// remote commands (Game/GameServer.cpp)
	// TODO those commans are registered in Console, get the list from there
	// This is best done with a new command class (eg. ConsoleCommand)
	// deprecated idea, use *ActionExecutor's instead
	words.emplace_back("/atm", sl);
	words.emplace_back("/devlua ", sl);
	words.emplace_back("/editdefs ", sl);
	words.emplace_back("/give ", sl);
	words.emplace_back("/luagaia ", sl);
	words.emplace_back("/luarules ", sl);
	words.emplace_back("/nocost", sl);
	words.emplace_back("/nospectatorchat ", sl);
	words.emplace_back("/reloadcob ", sl);
	words.emplace_back("/reloadcegs ", sl);
	words.emplace_back("/save ", sl);
	words.emplace_back("/skip ", sl);
	words.emplace_back("/skip +", sl);
	words.emplace_back("/skip f", sl);
	words.emplace_back("/skip f+", sl);
	words.emplace_back("/spectator ", sl);
	words.emplace_back("/take ", sl);
	words.emplace_back("/team ", sl);

	words.emplace_back("/cheat ", sl);
	words.emplace_back("/nohelp ", sl);
	words.emplace_back("/nopause ", sl);
	words.emplace_back("/setmaxspeed ", sl);
	words.emplace_back("/setminspeed ", sl);
	words.emplace_back("/kick ", sl);
	words.emplace_back("/kickbynum ", sl);
	words.emplace_back("/mute ", sl);
	words.emplace_back("/mutebynum ", sl);
}

void CWordCompletion::Sort() {
	std::sort(words.begin(), words.end(), [](const WordEntry& a, const WordEntry& b) { return (a.first < b.first); });
}

void CWordCompletion::AddWord(const std::string& word, bool startOfLine, bool unitName, bool miniMap, bool resort)
{
	if (word.empty())
		return;

	const auto pred = [](const WordEntry& a, const WordEntry& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(words.begin(), words.end(), WordEntry{word, {}}, pred);

	if (iter != words.end() && iter->first == word) {
		LOG_SL("WordCompletion", L_DEBUG, "Tried to add already present word: %s", word.c_str());
		return;
	}

	words.emplace_back(word, WordProperties(startOfLine, unitName, miniMap));

	if (!resort)
		return;

	Sort();
}

void CWordCompletion::RemoveWord(const std::string& word)
{
	const auto pred = [](const WordEntry& a, const WordEntry& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(words.begin(), words.end(), WordEntry{word, {}}, pred);

	if (iter == words.end() || iter->first != word)
		return;

	words.erase(iter);
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

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//  TODO: replace 'WordProperties' with regex/glob expressions
//        ex: '/give [0-9]* [_0-9a-zA-Z]+'
//        or even create a domain specific language
//        ex: '/give <ammount> {unitname} <team-ID>'
//        user-input: '/give 10 armcom 1'


#include "WordCompletion.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#include <stdexcept>
#include <algorithm>

CWordCompletion* CWordCompletion::singleton = NULL;

void CWordCompletion::CreateInstance() {
	if (singleton == NULL) {
		singleton = new CWordCompletion();
	} else {
		throw std::logic_error("WordCompletion singleton is already initialized");
	}
}

void CWordCompletion::DestroyInstance() {
	if (singleton != NULL) {
		SafeDelete(singleton);
	} else {
		throw std::logic_error("WordCompletion singleton was not initialized or is already destroyed");
	}
}



CWordCompletion::CWordCompletion()
{
	Reset();
}

void CWordCompletion::Reset()
{
	words.clear();
	WordProperties sl(true, false, false);

	// key bindings (Game/UI/KeyBindings.cpp)
	words["/bind "] = sl;
	words["/unbind "] = sl;
	words["/unbindall "] = sl;
	words["/unbindaction "] = sl;
	words["/unbindkeyset "] = sl;
	words["/keyload "] = sl;
	words["/keyreload "] = sl;
	words["/keysave "] = sl;
	words["/keysyms "] = sl;
	words["/keycodes "] = sl;
	words["/keyprint "] = sl;
	words["/keydebug "] = sl;
	words["/fakemeta "] = sl;

	// camera handler (Game/CameraHandler.cpp)
	words["/viewtaflip "] = sl;

	// mini-map (Game/UI/MiniMap.cpp)
	WordProperties mm(false, false, true);
	words["fullproxy "] = mm;
	words["drawcommands "] = mm;
	words["drawprojectiles "] = mm;
	words["icons "] = mm;
	words["unitexp "] = mm;
	words["unitsize "] = mm;
	words["simplecolors "] = mm;
	words["geometry "] = mm;
	words["minimize "] = mm;
	words["maximize "] = mm;
	words["maxspect "] = mm;

	// remote commands (Game/GameServer.cpp)
	// TODO those commans are registered in Console, get the list from there
	// This is best done with a new command class (eg. ConsoleCommand)
	// deprecated idea, use *ActionExecutor's instead
	words["/atm"] = sl;
	words["/devlua "] = sl;
	words["/editdefs "] = sl;
	words["/give "] = sl;
	words["/luagaia "] = sl;
	words["/luarules "] = sl;
	words["/nocost"] = sl;
	words["/nospectatorchat "] = sl;
	words["/reloadcob "] = sl;
	words["/reloadcegs "] = sl;
	words["/save "] = sl;
	words["/skip "] = sl;
	words["/skip +"] = sl;
	words["/skip f"] = sl;
	words["/skip f+"] = sl;
	words["/spectator "] = sl;
	words["/take "] = sl;
	words["/team "] = sl;

	words["/cheat "] = sl;
	words["/nohelp "] = sl;
	words["/nopause "] = sl;
	words["/setmaxspeed "] = sl;
	words["/setminspeed "] = sl;
	words["/kick "] = sl;
	words["/kickbynum "] = sl;
	words["/mute "] = sl;
	words["/mutebynum "] = sl;
}


void CWordCompletion::AddWord(const std::string& word, bool startOfLine,
		bool unitName, bool miniMap)
{
	if (!word.empty()) {
		if (words.find(word) != words.end()) {
			LOG_SL("WordCompletion", L_DEBUG,
					"Tried to add already present word: %s", word.c_str());
			return;
		}
		words[word] = WordProperties(startOfLine, unitName, miniMap);
	}
}

void CWordCompletion::RemoveWord(const std::string& word)
{
	words.erase(word);
}


std::vector<std::string> CWordCompletion::Complete(std::string& msg) const
{
	std::vector<std::string> partials;

	const bool unitName = (msg.find("/give ") == 0);
	const bool miniMap = (msg.find("/minimap ") == 0);

	// strip "a:" and "s:" prefixes
	std::string prefix, rawmsg;
	if ((msg.find_first_of("aAsS") == 0) && (msg[1] == ':')) {
		prefix = msg.substr(0, 2);
		rawmsg = msg.substr(2);
	} else {
		rawmsg = msg;
	}

	// the word fragment
	std::string::size_type startPos = rawmsg.find_last_of(' ');
	const bool startOfLine = (startPos == std::string::npos);
	startPos = startOfLine ? 0 : startPos + 1;
	const std::string fragment(rawmsg, startPos, std::string::npos);
	if (fragment.empty()) {
		return partials;
	}
	const std::string::size_type fragLen = fragment.size();

	// pick a decent spot to start scanning
	std::map<std::string, WordProperties>::const_iterator start;
	start = words.lower_bound(fragment);

	// make a list of valid possible matches
	std::map<std::string, WordProperties>::const_iterator it;
	for (it = start; it != words.end(); ++it) {
		const int cmp = it->first.compare(0, fragLen, fragment);
		if (cmp < 0) continue;
		if (cmp > 0) break;
		if ((!it->second.startOfLine || startOfLine) &&
		    (!it->second.unitName    || unitName)    &&
		    (!it->second.miniMap     || miniMap)) {
			partials.push_back(it->first);
		}
	}

	if (partials.empty()) {
		return partials; // no possible matches
	}

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
		if (firstStr[i] != lastStr[i]) {
			break;
		}
	}
	msg = prefix + rawmsg.substr(0, startPos) + partials[0].substr(0, i);

	return partials;
}

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//  TODO: replace 'WordProperties' with regex/glob expressions
//        ex: '.give [0-9]* armcom'

#include "StdAfx.h"
#include "mmgr.h"

#include "WordCompletion.h"

CWordCompletion::CWordCompletion()
{
	Reset();
	return;
}


CWordCompletion::~CWordCompletion()
{
	return;
}


void CWordCompletion::Reset()
{
	words.clear();
	WordProperties sl(true, false, false);

	// local commands
	words["/aikill "] = sl;
	words["/aireload "] = sl;
	words["/aicontrol "] = sl;
	words["/ailist"] = sl;
	words["/advshading "] = sl;
	words["/hardwarecursor "] = sl;
	words["/clock"] = sl;
	words["/cmdcolors "] = sl;
	words["/ctrlpanel "] = sl;
	words["/distdraw "] = sl;
	words["/disticon "] = sl;
	words["/debugdrawai"] = sl;
	words["/debuginfo sound"] = sl;
	words["/debuginfo profiling"] = sl;
	words["/incguiopacity "] = sl;
	words["/decguiopacity "] = sl;
	words["/echo "] = sl;
	words["/font "] = sl;
	words["/gameinfo"] = sl;
	words["/gathermode"] = sl;
	words["/grounddecals "] = sl;
	words["/info "] = sl;
	words["/luaui "] = sl;
	words["/maxparticles "] = sl;
	words["/maxnanoparticles "] = sl;
	words["/minimap "] = sl;
	words["/say "] = sl;
	words["/shadows "] = sl;
	words["/specfullview "] = sl;
	words["/water "] = sl;
	words["/bind "] = sl;
	words["/unbind "] = sl;
	words["/unbindall"] = sl;
	words["/unbindaction "] = sl;
	words["/unbindkeyset "] = sl;
	words["/keyload"] = sl;
	words["/keyreload"] = sl;
	words["/keysave"] = sl;
	words["/keysyms"] = sl;
	words["/keycodes"] = sl;
	words["/keyprint"] = sl;
	words["/keydebug"] = sl;
	words["/fakemeta "] = sl;
	words["/volume "] = sl;
	words["/unitreplyvolume "] = sl;
	words["/viewtaflip "] = sl;
	words["/vsync "] = sl;
	words["/wiremap "] = sl;
	words["/airmesh "] = sl;

	// minimap sub-commands
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

	// remote commands
	// TODO those commans are registered in Console, get the list from there
	// This is best done with a new command class (eg. ConsoleCommand)
	words["/atm"] = sl;
	words["/cheat "] = sl;
	words["/devlua "] = sl;
	words["/editdefs "] = sl;
	words["/give "] = sl;
	words["/kick "] = sl;
	words["/kickbynum "] = sl;
	words["/luagaia "] = sl;
	words["/luarules "] = sl;
	words["/nocost"] = sl;
	words["/nohelp "] = sl;
	words["/nopause "] = sl;
	words["/nospectatorchat "] = sl;
	words["/reloadcob "] = sl;
	words["/reloadcegs "] = sl;
	words["/save "] = sl;
	words["/setmaxspeed "] = sl;
	words["/setminspeed "] = sl;
	words["/skip "] = sl;
	words["/skip +"] = sl;
	words["/skip f"] = sl;
	words["/skip f+"] = sl;
	words["/spectator"] = sl;
	words["/take"] = sl;
	words["/team "] = sl;
	// words[".crash"] = sl; // don't make it too easy

	return;
}


void CWordCompletion::AddWord(const std::string& word,
                              bool startOfLine, bool unitName, bool minimap)
{
	if (!word.empty()) {
		words[word] = WordProperties(startOfLine, unitName, minimap);
	}
	return;
}

void CWordCompletion::RemoveWord(const std::string& word)
{
	words.erase(word);
}


std::vector<std::string> CWordCompletion::Complete(std::string& msg) const
{
	std::vector<std::string> partials;

	const bool unitName = (msg.find("/give ") == 0);
	const bool minimap = (msg.find("/minimap ") == 0);

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
		    (!it->second.minimap     || minimap)) {
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

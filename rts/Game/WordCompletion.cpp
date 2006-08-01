// WordCompletion.cpp: implementation of the CWordCompletion class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
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
	WordProperties wp(true, false);
	words[".atm"] = wp;
	words[".bind "] = wp;
	words[".cheat"] = wp;
	words[".clock"] = wp;
	words[".cmd "] = wp;
	//  words[".crash"] = wp; // don't make it too easy
	words[".give "] = wp;
	words[".info"] = wp;
	words[".kick "] = wp;
	words[".nocost"] = wp;
	words[".nopause"] = wp;
	words[".nospectatorchat"] = wp;
	words[".setmaxspeed"] = wp;
	words[".setminspeed"] = wp;
	words[".spectator"] = wp;
	words[".take"] = wp;
	words[".team "] = wp;
	return;
}


void CWordCompletion::AddWord(const string& word, bool startOfLine, bool unitName)
{
	if (!word.empty()) {
		words[word] = WordProperties(startOfLine, unitName);
	}
	return;
}


vector<string> CWordCompletion::Complete(string& msg) const
{
	vector<string> partials;

	const bool unitName = (msg.find(".give ") == 0);
	  
	// strip "a:" and "s:" prefixes
	string prefix, rawmsg;
	if ((msg.find_first_of("aAsS") == 0) && (msg[1] == ':')) {
		prefix = msg.substr(0, 2);
		rawmsg = msg.substr(2);
	} else {
		rawmsg = msg;
	}

	// the word fragment
	string::size_type startPos = rawmsg.find_last_of(' ');
	const bool startOfLine = (startPos == string::npos);
	startPos = startOfLine ? 0 : startPos + 1;
	const string fragment(rawmsg, startPos, string::npos);
	if (fragment.empty()) {
		return partials;
	}
	const string::size_type fragLen = fragment.size();

	// pick a decent spot to start scanning
	map<string,WordProperties>::const_iterator start;
	start = words.lower_bound(fragment);

	// make a list of valid possible matches
	map<string,WordProperties>::const_iterator it;
	for (it = start; it != words.end(); it++) {
		const int cmp = it->first.compare(0, fragLen, fragment);
		if (cmp < 0) continue;
		if (cmp > 0) break;
		if ((!it->second.startOfLine || startOfLine) &&
			(!it->second.unitName || unitName)) {
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
	const string& firstStr = partials[0];
	const string& lastStr = partials[partials.size() - 1];
	unsigned int i = 0;
	unsigned int least = min(firstStr.size(), lastStr.size());
	for (i = 0; i < least; i++) {
		if (firstStr[i] != lastStr[i]) {
		break;
		}
	}
	msg = prefix + rawmsg.substr(0, startPos) + partials[0].substr(0, i);
	  
	return partials;
}

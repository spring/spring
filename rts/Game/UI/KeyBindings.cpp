#include "StdAfx.h"
// CKeyBindings.cpp: implementation of the CKeyBindings class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <cctype>
#include "KeyBindings.h"
#include "KeyCodes.h"
#include "SDL_keysym.h"
#include "System/Platform/errorhandler.h"
#include "System/FileSystem/FileHandler.h"


CKeyBindings* keyBindings = NULL;


static const struct DefaultBinding {
	const char* key;
	const char* action;
	const char* context;
}
defaultBindings[] = {

	{ "esc",       "quit", "" },
	{ "Shift+esc", "quit", "" },

	{ "b",     "debug",                "" },
	{ "o",     "singlestep",           "" },
	{ "c",     "controlunit",          "" },
	{ "h",     "sharedialog",          "" },
	{ "tab",   "toggleoverview",       "" },
	{ "l",     "togglelos",            "" },
	{ ";",     "toggleradarandjammer", "" },

	{ "j",         "mousestate", "" },
	{ "backspace", "mousestate", "" },

	{ "enter",       "chat",     "" },
	{ "Alt+enter",   "chatally", "" },
	{ "Shift+enter", "chatspec", "" },
	{ "Ctrl+m",      "chatall",  "" }, // CTRL+ENTER returns CTRL+m

	{ "home", "increaseViewRadius", "" },
	{ "end",  "decreaseViewRadius", "" },

	{ "insert",  "speedup" , "" },
	{ "delete",  "slowdown", "" },
	{ "=",       "speedup" , "" },
	{ "+",       "speedup" , "" },
	{ "Shift++", "speedup" , "" },
	{ "-",       "slowdown", "" },

	{ ">", "incguiopacity", "" },
	{ "<", "decguiopacity", "" },

	{ "[", "incbuildfacing", "" },
	{ "]", "decbuildfacing", "" },

	{ "Any+z", "mouse4", "" },
	{ "Any+x", "mouse5", "" },

	{ "Any+0", "group0", "" },
	{ "Any+1", "group1", "" },
	{ "Any+2", "group2", "" },
	{ "Any+3", "group3", "" },
	{ "Any+4", "group4", "" },
	{ "Any+5", "group5", "" },
	{ "Any+6", "group6", "" },
	{ "Any+7", "group7", "" },
	{ "Any+8", "group8", "" },
	{ "Any+9", "group9", "" },

	{ "s",       "stop",         "command" },
	{ "w",       "wait",         "command" },
	{ "m",       "move",         "command" },
	{ "a",       "attack",       "command" },
	{ "Alt+a",   "attack",       "command" },
	{ "f",       "fight",        "command" },
	{ "p",       "patrol",       "command" },
	{ "g",       "guard",        "command" },
	{ "d",       "dgun",         "command" },
	{ "e",       "reclaim",      "command" },
	{ "r",       "repair",       "command" },
	{ "Ctrl+c",  "capture",      "command" },
	{ "Ctrl+d",  "selfd",        "command" },
	{ "l",       "loadunits",    "command" },
	{ "u",       "unloadunits",  "command" },
	{ "k",       "cloak",        "command" },
	{ "x",       "onoff",        "command" },
	{ "q",       "groupadd",     "command" },
	{ "q",       "groupselect",  "command" },
	{ "Shift+q", "groupclear",   "command" },
	{ "Ctrl+q",  "aiselect",     "command" },

	// FIXME
	{ "numpad7", "prevmenu",     "command" },
	{ "numpad8", "repeat",       "command" },
	{ "numpad9", "nextmenu",     "command" },
	{ "numpad4", "firestate",    "command" },
	{ "numpad1", "movestate",    "command" },
	{ "numpad0", "trajectory",   "command" },
	{ "numpad3", "showcommands", "command" },

	{ "t",       "track",      "" },
	{ "Ctrl+t",  "trackoff",   "" },
	{ "Shift+t", "trackmode",  "" },

	{ "Ctrl+f1",  "viewfps",   "" },
	{ "Ctrl+f2",  "viewta",    "" },
	{ "Ctrl+f3",  "viewtw",    "" },
	{ "Ctrl+f4",  "viewrot",   "" },
	
	{ "f1",  "showElevation",  "" },
	{ "f2",  "ShowPathMap",    "" },
	{ "f3",  "LastMsgPos",     "" },
	{ "f4",  "ShowMetalMap",   "" },
	{ "f5",  "hideinterface",  "" },
	{ "f6",  "NoSound",        "" },
	{ "f7",  "dynamicSky",     "" },
	{ "f8",  "savegame",       "" },
	{ "f9",  "showhealthbars", "" },
	{ "f10", "createvideo",    "" },
	{ "f11", "screenshot",     "" },
	{ "f12", "screenshot",     "" },

	{ "Any+S+~",  "drawlabel", "" },
	{    "Any+`", "drawinmap", "" },
	{ "Up+Any+`", "drawinmap", "" },

	{    "Any+up",       "moveforward", "" },
	{ "Up+Any+up",       "moveforward", "" },
	{    "Any+down",     "moveback",    "" },
	{ "Up+Any+down",     "moveback",    "" },
	{    "Any+right",    "moveright",   "" },
	{ "Up+Any+right",    "moveright",   "" },
	{    "Any+left",     "moveleft",    "" },
	{ "Up+Any+left",     "moveleft",    "" },
	{    "Any+pageup",   "moveup",      "" },
	{ "Up+Any+pageup",   "moveup",      "" },
	{    "Any+pagedown", "movedown",    "" },
	{ "Up+Any+pagedown", "movedown",    "" },

	{    "Any+ctrl",     "moveslow",    "" },
	{ "Up+Any+ctrl",     "moveslow",    "" },
	{    "Any+shift",    "movefast",    "" },
	{ "Up+Any+shift",    "movefast",    "" }
};


/******************************************************************************/
//
// Parsing Routines
//

static int lineNumber = 0;
static bool inComment = false;


// returns next line (without newlines)
static string GetLine(CFileHandler& fh)
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
static string GetCleanLine(CFileHandler& fh)
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


static vector<string> Tokenize(const string& line)
{
	vector<string> words;
	string::size_type start;
	string::size_type end = 0;
	while (true) {
		start = line.find_first_not_of(" \t", end);
		if (start == string::npos) {
			break;
		}
		end = line.find_first_of(" \t", start);
		string word;
		if (end == string::npos) {
			word = line.substr(start);
		} else {
			word = line.substr(start, end - start);
		}
		words.push_back(word);
		if (end == string::npos) {
			break;
		}
	}
	return words;
}


static bool CheckTokens(vector<string>& words, int min, int fullSize)
{
	if (words.size() < min) {
		handleerror(0, "Missing command parameters", words[0].c_str(), 0);
		return false;
	}
	// fill in the blanks
	for (unsigned int i = words.size(); i < fullSize; i++) {
		words.push_back("");
	}
	return true;
}


/******************************************************************************/
//
// KeyBindings
//

CKeyBindings::CKeyBindings(const string& filename) : debug(false)
{
	Load(filename);
}


CKeyBindings::~CKeyBindings()
{
}


/******************************************************************************/

string CKeyBindings::GetAction(const CKeySet& ks, const string& context) const
{
	if (debug) {
		printf("GetAction: %s (%i) <%s>",
	         ks.GetString().c_str(), ks.Key(), context.c_str());
	}

	ContextKeyMap::const_iterator cit = bindings.find(context);
	if (cit == bindings.end()) {
		return "";
	}
	const KeyMap& keymap = cit->second;
	
	KeyMap::const_iterator kit = keymap.find(ks);
	if (kit == keymap.end()) {
		if (debug) {
			printf("\n");
		}
		return "";
	}
	if (debug) {
		printf(" -> %s (%i): %s\n",
		       kit->first.GetString().c_str(), kit->first.Key(), kit->second.c_str());
	}
	return kit->second;
}


/******************************************************************************/

bool CKeyBindings::Bind(const string& keystr,
                        const string& action,
                        const string& context)
{
	string lowerKey = StringToLower(keystr);
	CKeySet ks;
	if (!ks.Parse(lowerKey)) {
		printf("Could not parse key: %s\n", keystr.c_str());
	}

	string a;
	if (action.find("select:") != 0) {
		a = StringToLower(action);
	} else {
		a = action;
	}

	bindings[context][ks] = a;

	return true;
}


bool CKeyBindings::UnBindKeyset(const string& keystr, const string& context)
{
	ContextKeyMap::iterator cit = bindings.find(context);
	if (cit == bindings.end()) {
		return false;
	}
	KeyMap& keymap = cit->second;
	
	string lowerKey = StringToLower(keystr);
	CKeySet ks;
	if (!ks.Parse(lowerKey)) {
		printf("Could not parse key: %s\n", keystr.c_str());
	}
	bool success = false;
	KeyMap::iterator kit = bindings[context].find(ks);
	while (kit != keymap.end()) {
		keymap.erase(kit);
		if (keymap.empty()) {
			bindings.erase(cit);
			return true;
		}
		kit = keymap.find(ks);
		success = true;
	}
	return success;
}


bool CKeyBindings::UnBindAction(const string& action, const string& context)
{
	ContextKeyMap::iterator cit = bindings.find(context);
	if (cit == bindings.end()) {
		return false;
	}
	KeyMap& keymap = cit->second;
	
	string a;
	if (action.find("select:") != 0) {
		a = StringToLower(action);
	} else {
		a = action;
	}

	bool success = false;
	KeyMap::iterator kit = keymap.begin();
	while (kit != keymap.end()) {
		if (kit->second != a) {
			kit++;
		} else {
			KeyMap::iterator it_next = kit;
			it_next++;
			keymap.erase(kit);
			if (keymap.empty()) {
				bindings.erase(cit);
				return true;
			}
			kit = it_next;
			success = true;
		}
	}
	return success;
}


bool CKeyBindings::UnBindContext(const string& context)
{
	ContextKeyMap::iterator cit = bindings.find(context);
	if (cit != bindings.end()) {
		bindings.erase(cit);
		return true;
	}
	return false;
}


/******************************************************************************/

void CKeyBindings::LoadDefaults()
{
	const int count = sizeof(defaultBindings) / sizeof(defaultBindings[0]);
	for (int i = 0; i < count; ++i) {
		Bind(defaultBindings[i].key,
		     defaultBindings[i].action,
		     defaultBindings[i].context);
	}
}


bool CKeyBindings::Load(const string& filename)
{
	inComment = false;
	lineNumber = 0;
	
	CFileHandler ifs(filename);
	
	LoadDefaults();
	
	while (true) {
		const string line = GetCleanLine(ifs);
		if (line.empty()) {
			break;
		}
		vector<string> words = Tokenize(line);
		if (words.size() <= 0) {
			continue;
		}
		const string command = StringToLower(words[0]);

		if (command == "bind") {
			if (CheckTokens(words, 3, 4)) {
				Bind(words[1], words[2], words[3]);
			}
		}
		else if (command == "unbindall") {
			bindings.clear();
		}
		else if (command == "unbindaction") {
			if (CheckTokens(words, 2, 3)) {
				UnBindAction(words[1], words[2]);
			}
		}
		else if (command == "unbindkeyset") {
			if (CheckTokens(words, 2, 3)) {
				UnBindKeyset(words[1], words[2]);
			}
		}
		else if (command == "unbindcontext") {
			if (CheckTokens(words, 2, 2)) {
				UnBindContext(words[1]);
			}
		}
		else { 
			handleerror(0, "Could not parse control file", line.c_str(), 0);
			return false;
		}
	} // End While
	return true;
}


bool CKeyBindings::Save(const string& filename) const
{
	FILE* out = fopen(filename.c_str(), "wt");
	if (out == NULL) {
		return false;
	}

	// FIXME -- sorted by keycode? merge...
	ContextKeyMap::const_iterator cit;
	for (cit = bindings.begin(); cit != bindings.end(); ++cit) {
		const KeyMap& keymap = cit->second;
		KeyMap::const_iterator kit;
		for (kit = keymap.begin(); kit != keymap.end(); ++kit) {
			if (cit->first.empty()) {
				fprintf(out, "bind %18s  %s\n",
								kit->first.GetString().c_str(), kit->second.c_str());
			} else {
				fprintf(out, "bind %18s  %-20s %s\n",
								kit->first.GetString().c_str(), kit->second.c_str(),
								cit->first.c_str());
			}
		}
	}
	fclose(out);
	return true;
}


void CKeyBindings::Print() const
{
	ContextKeyMap::const_iterator cit;
	for (cit = bindings.begin(); cit != bindings.end(); ++cit) {
		printf("KeyContext: %s\n", cit->first.c_str());
		const KeyMap& keymap = cit->second;
		KeyMap::const_iterator kit;
		for (kit = keymap.begin(); kit != keymap.end(); ++kit) {
			printf("  Binding: %18s (%i)  to  %-20s <%s>\n",
						 kit->first.GetString().c_str(), kit->first.Key(),
						 kit->second.c_str(), cit->first.c_str());
		}
	}
}

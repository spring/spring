/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cstdio>
#include <cctype>
#include <cstring>

#include "KeyBindings.h"
#include "KeyBindingsLog.h"
#include "SDL_keysym.h"
#include "KeyCodes.h"
#include "KeySet.h"
#include "SelectionKeyHandler.h"
#include "KeyAutoBinder.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Log/ILog.h"
#include "System/Log/DefaultFilter.h"
#include "System/Util.h"

#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_KEY_BINDINGS


CKeyBindings* keyBindings = NULL;


static const struct DefaultBinding {
	const char* key;
	const char* action;
}
defaultBindings[] = {

	{        "esc", "quitmessage"    },
	{        "Shift+esc", "quitmenu"    },
	{ "Ctrl+Shift+esc", "quitforce"    },
	{  "Any+pause", "pause"       },

	{ "c", "controlunit"          },
	{ "Any+h", "sharedialog"          },
	{ "Any+l", "togglelos"            },
	{ "Any+;", "toggleradarandjammer" },

	{ "Any+tab", "toggleoverview" },

	{ "Any+j",         "mouse2" },
	{ "backspace", "mousestate" },
	{ "Shift+backspace", "togglecammode" },
	{ "Ctrl+backspace", "togglecammode" },

	{ "Any+i", "gameinfo" },

	{   "Any+enter", "chat"           },
	{  "Ctrl+enter", "chatall"        },
	{  "Ctrl+enter", "chatswitchall"  },
	{   "Alt+enter", "chatally"       },
	{   "Alt+enter", "chatswitchally" },
	{ "Shift+enter", "chatspec"       },
	{ "Shift+enter", "chatswitchspec" },

	{       "Any+tab", "edit_complete"  },
	{ "Any+backspace", "edit_backspace" },
	{    "Any+delete", "edit_delete"    },
	{      "Any+home", "edit_home"      },
	{      "Alt+left", "edit_home"      },
	{       "Any+end", "edit_end"       },
	{     "Alt+right", "edit_end"       },
	{        "Any+up", "edit_prev_line" },
	{      "Any+down", "edit_next_line" },
	{      "Any+left", "edit_prev_char" },
	{     "Any+right", "edit_next_char" },
	{     "Ctrl+left", "edit_prev_word" },
	{    "Ctrl+right", "edit_next_word" },
	{     "Any+enter", "edit_return"    },
	{    "Any+escape", "edit_escape"    },

	{ "Ctrl+v", "pastetext" },

	{ "Any+home", "increaseViewRadius" },
	{ "Any+end",  "decreaseViewRadius" },

	{ "Ctrl+insert", "hotbind"   },
	{ "Ctrl+delete", "hotunbind" },

	{ "Alt+insert",  "speedup"  },
	{ "Alt+delete",  "slowdown" },
	{ "Alt+=",       "speedup"  },
	{ "Alt++",       "speedup"  },
	{ "Alt+-",       "slowdown" },
	{ "Alt+numpad+", "speedup"  },
	{ "Alt+numpad-", "slowdown" },

	{       ",", "prevmenu" },
	{       ".", "nextmenu" },
	{ "Shift+,", "decguiopacity" },
	{ "Shift+.", "incguiopacity" },

	{      "1", "specteam 0"  },
	{      "2", "specteam 1"  },
	{      "3", "specteam 2"  },
	{      "4", "specteam 3"  },
	{      "5", "specteam 4"  },
	{      "6", "specteam 5"  },
	{      "7", "specteam 6"  },
	{      "8", "specteam 7"  },
	{      "9", "specteam 8"  },
	{      "0", "specteam 9"  },
	{ "Ctrl+1", "specteam 10" },
	{ "Ctrl+2", "specteam 11" },
	{ "Ctrl+3", "specteam 12" },
	{ "Ctrl+4", "specteam 13" },
	{ "Ctrl+5", "specteam 14" },
	{ "Ctrl+6", "specteam 15" },
	{ "Ctrl+7", "specteam 16" },
	{ "Ctrl+8", "specteam 17" },
	{ "Ctrl+9", "specteam 18" },
	{ "Ctrl+0", "specteam 19" },

	{ "Any+0", "group0" },
	{ "Any+1", "group1" },
	{ "Any+2", "group2" },
	{ "Any+3", "group3" },
	{ "Any+4", "group4" },
	{ "Any+5", "group5" },
	{ "Any+6", "group6" },
	{ "Any+7", "group7" },
	{ "Any+8", "group8" },
	{ "Any+9", "group9" },

	{       "[", "buildfacing inc"  },
	{ "Shift+[", "buildfacing inc"  },
	{       "]", "buildfacing dec"  },
	{ "Shift+]", "buildfacing dec"  },
	{   "Any+z", "buildspacing inc" },
	{   "Any+x", "buildspacing dec" },

	{            "a", "attack"       },
	{      "Shift+a", "attack"       },
	{        "Alt+a", "areaattack"   },
	{  "Alt+Shift+a", "areaattack"   },
	{        "Alt+b", "debug"        },
	{        "Alt+v", "debugcolvol"  },
	{        "Alt+p", "debugpath"    },
	{            "d", "manualfire"   },
	{      "Shift+d", "manualfire"   },
	{       "Ctrl+d", "selfd"        },
	{ "Ctrl+Shift+d", "selfd queued" },
	{            "e", "reclaim"      },
	{      "Shift+e", "reclaim"      },
	{            "f", "fight"        },
	{      "Shift+f", "fight"        },
	{        "Alt+f", "forcestart"   },
	{            "g", "guard"        },
	{      "Shift+g", "guard"        },
	{            "k", "cloak"        },
	{      "Shift+k", "cloak"        },
	{            "l", "loadunits"    },
	{      "Shift+l", "loadunits"    },
	{            "m", "move"         },
	{      "Shift+m", "move"         },
	{        "Alt+o", "singlestep"   },
	{            "p", "patrol"       },
	{      "Shift+p", "patrol"       },
	{            "q", "groupselect"  },
	{            "q", "groupadd"     },
	{       "Ctrl+q", "aiselect"     },
	{      "Shift+q", "groupclear"   },
	{            "r", "repair"       },
	{      "Shift+r", "repair"       },
	{            "s", "stop"         },
	{      "Shift+s", "stop"         },
	{            "u", "unloadunits"  },
	{      "Shift+u", "unloadunits"  },
	{            "w", "wait"         },
	{      "Shift+w", "wait queued"  },
	{            "x", "onoff"        },
	{      "Shift+x", "onoff"        },


	{  "Ctrl+t", "trackmode" },
	{   "Any+t", "track" },

	{ "Ctrl+f1", "viewfps"  },
	{ "Ctrl+f2", "viewta"   },
	{ "Ctrl+f3", "viewtw"   },
	{ "Ctrl+f4", "viewrot"  },
	{ "Ctrl+f5", "viewfree" },

	{ "Any+f1",  "ShowElevation"         },
	{ "Any+f2",  "ShowPathTraversability"},
	{ "Any+f3",  "LastMsgPos"            },
	{ "Any+f4",  "ShowMetalMap"          },
	{ "Any+f5",  "HideInterface"         },
	{ "Any+f6",  "NoSound"               },
	{ "Any+f7",  "DynamicSky"            },

	{ "Ctrl+Shift+f8", "savegame" },
	{ "Any+f9",  "showhealthbars" }, // MT only
	{ "Ctrl+Shift+f10", "createvideo" },
	{ "Any+f11", "screenshot"     },
	{ "Any+f12", "screenshot"     },

	// NOTE: Up bindings are currently converted to press bindings
	//       (see KeySet.cpp / DISALLOW_RELEASE_BINDINGS)

	{    "Any+`",    "drawinmap"  },
	{ "Up+Any+`",    "drawinmap"  },
	{    "Any+\\",   "drawinmap"  },
	{ "Up+Any+\\",   "drawinmap"  },
	{    "Any+0xa7", "drawinmap"  },
	{ "Up+Any+0xa7", "drawinmap"  },

	{    "Any+up",       "moveforward"  },
	{ "Up+Any+up",       "moveforward"  },
	{    "Any+down",     "moveback"     },
	{ "Up+Any+down",     "moveback"     },
	{    "Any+right",    "moveright"    },
	{ "Up+Any+right",    "moveright"    },
	{    "Any+left",     "moveleft"     },
	{ "Up+Any+left",     "moveleft"     },
	{    "Any+pageup",   "moveup"       },
	{ "Up+Any+pageup",   "moveup"       },
	{    "Any+pagedown", "movedown"     },
	{ "Up+Any+pagedown", "movedown"     },

	{    "Any+ctrl",     "moveslow"     },
	{ "Up+Any+ctrl",     "moveslow"     },
	{    "Any+shift",    "movefast"     },
	{ "Up+Any+shift",    "movefast"     },


	// selection keys
	{ "Ctrl+a",    "select AllMap++_ClearSelection_SelectAll+"                                         },
	{ "Ctrl+b",    "select AllMap+_Builder_Idle+_ClearSelection_SelectOne+"                            },
	{ "Ctrl+c",    "select AllMap+_ManualFireUnit+_ClearSelection_SelectOne+"                          },
	{ "Ctrl+r",    "select AllMap+_Radar+_ClearSelection_SelectAll+"                                   },
	{ "Ctrl+v",    "select AllMap+_Not_Builder_Not_Commander_InPrevSel_Not_InHotkeyGroup+_SelectAll+"  },
	{ "Ctrl+w",    "select AllMap+_Not_Aircraft_Weapons+_ClearSelection_SelectAll+"                    },
	{ "Ctrl+x",    "select AllMap+_InPrevSel_Not_InHotkeyGroup+_SelectAll+"                            },
	{ "Ctrl+z",    "select AllMap+_InPrevSel+_ClearSelection_SelectAll+"                               }
};


/******************************************************************************/
//
// CKeyBindings
//

CKeyBindings::CKeyBindings()
{
	fakeMetaKey = -1;
	userCommand = true;

	statefulCommands.insert("drawinmap");
	statefulCommands.insert("moveforward");
	statefulCommands.insert("moveback");
	statefulCommands.insert("moveright");
	statefulCommands.insert("moveleft");
	statefulCommands.insert("moveup");
	statefulCommands.insert("movedown");
	statefulCommands.insert("moveslow");
	statefulCommands.insert("movefast");

	RegisterAction("bind");
	RegisterAction("unbind");
	RegisterAction("unbindall");
	RegisterAction("unbindkeyset");
	RegisterAction("unbindaction");
	RegisterAction("keydebug");
	RegisterAction("fakemeta");
	RegisterAction("keyload");
	RegisterAction("keyreload");
	RegisterAction("keysave");
	RegisterAction("keyprint");
	RegisterAction("keysyms");
	RegisterAction("keycodes");
}


CKeyBindings::~CKeyBindings()
{
}


/******************************************************************************/

const CKeyBindings::ActionList&
	CKeyBindings::GetActionList(const CKeySet& ks) const
{
	static const ActionList empty;
	const ActionList* alPtr = NULL;

	if (ks.AnyMod()) {
		KeyMap::const_iterator it = bindings.find(ks);
		if (it == bindings.end()) {
			alPtr = &empty;
		} else {
			alPtr = &(it->second);
		}
	}
	else {
		// have to check for an AnyMod keyset as well as the normal one
		CKeySet anyMod = ks;
		anyMod.SetAnyBit();
		KeyMap::const_iterator nit = bindings.find(ks);
		KeyMap::const_iterator ait = bindings.find(anyMod);
		const bool haveNormal = (nit != bindings.end());
		const bool haveAnyMod = (ait != bindings.end());
		if (!haveNormal && !haveAnyMod) {
			alPtr = &empty;
		}
		else if (haveNormal && !haveAnyMod) {
			alPtr = &(nit->second);
		}
		else if (!haveNormal && haveAnyMod) {
			alPtr = &(ait->second);
		}
		else {
			// combine the two lists (normal first)
			static ActionList merged;
			merged = nit->second;
			const ActionList& aal = ait->second;
			for (int i = 0; i < (int)aal.size(); ++i) {
				merged.push_back(aal[i]);
			}
			alPtr = &merged;
		}
	}

	if (LOG_IS_ENABLED(L_DEBUG)) {
		const bool isEmpty = (alPtr == &empty);
		LOG_L(L_DEBUG, "GetAction: %s (0x%03X)%s",
				ks.GetString(false).c_str(), ks.Key(),
				(isEmpty ? "  EMPTY" : ""));
		if (!isEmpty) {
			const ActionList& al = *alPtr;
			for (size_t i = 0; i < al.size(); ++i) {
				LOG_L(L_DEBUG, "  %s  \"%s\"",
						al[i].command.c_str(), al[i].rawline.c_str());
			}
		}
	}

	return *alPtr;
}


const CKeyBindings::HotkeyList&
	CKeyBindings::GetHotkeys(const string& action) const
{
	ActionMap::const_iterator it = hotkeys.find(action);
	if (it == hotkeys.end()) {
		static HotkeyList empty;
		return empty;
	}
	const HotkeyList& hl = it->second;
	return hl;
}


/******************************************************************************/

bool CKeyBindings::Bind(const string& keystr, const string& line)
{
	CKeySet ks;
	if (!ParseKeySet(keystr, ks)) {
		LOG_L(L_WARNING, "Bind: could not parse key: %s", keystr.c_str());
		return false;
	}
	Action action(line);
	action.boundWith = keystr;
	if (action.command.empty()) {
		LOG_L(L_WARNING, "Bind: empty action: %s", line.c_str());
		return false;
	}

	// Try to be safe, force AnyMod mode for stateful commands
	if (statefulCommands.find(action.command) != statefulCommands.end()) {
		ks.SetAnyBit();
	}

	KeyMap::iterator it = bindings.find(ks);
	if (it == bindings.end()) {
		// new entry, push it
		ActionList& al = bindings[ks];
		al.push_back(action);
	} else {
		if (it->first != ks) {
			// not a match, push it
			ActionList& al = it->second;
			al.push_back(action);
		}
		else {
			// an exact keyset match, check the command
			ActionList& al = it->second;
			int i;
			for (i = 0; i < (int)al.size(); i++) {
				if (action.command == al[i].command) {
					break;
				}
			}
			if (i == (int)al.size()) {
				// not a match, push it
				al.push_back(action);
			}
		}
	}

	return true;
}


bool CKeyBindings::UnBind(const string& keystr, const string& command)
{
	CKeySet ks;
	if (!ParseKeySet(keystr, ks)) {
		LOG_L(L_WARNING, "UnBind: could not parse key: %s", keystr.c_str());
		return false;
	}
	bool success = false;

	KeyMap::iterator it = bindings.find(ks);
	if (it != bindings.end()) {
		ActionList& al = it->second;
		success = RemoveCommandFromList(al, command);
		if (al.empty()) {
			bindings.erase(it);
		}
	}
	return success;
}


bool CKeyBindings::UnBindKeyset(const string& keystr)
{
	CKeySet ks;
	if (!ParseKeySet(keystr, ks)) {
		LOG_L(L_WARNING, "UnBindKeyset: could not parse key: %s", keystr.c_str());
		return false;
	}
	bool success = false;

	KeyMap::iterator it = bindings.find(ks);
	if (it != bindings.end()) {
		bindings.erase(it);
		success = true;
	}
	return success;
}


bool CKeyBindings::UnBindAction(const string& command)
{
	bool success = false;

	KeyMap::iterator it = bindings.begin();
	while (it != bindings.end()) {
		ActionList& al = it->second;
		if (RemoveCommandFromList(al, command)) {
			success = true;
		}
		if (al.empty()) {
			KeyMap::iterator it_next = it;
			++it_next;
			bindings.erase(it);
			it = it_next;
		} else {
			++it;
		}
	}
	return success;
}


bool CKeyBindings::SetFakeMetaKey(const string& keystr)
{
	CKeySet ks;
	if (StringToLower(keystr) == "none") {
		fakeMetaKey = -1;
		return true;
	}
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "SetFakeMetaKey: could not parse key: %s", keystr.c_str());
		return false;
	}
	fakeMetaKey = ks.Key();
	return true;
}

bool CKeyBindings::AddKeySymbol(const string& keysym, const string& code)
{
	CKeySet ks;
	if (!ks.Parse(code)) {
		LOG_L(L_WARNING, "AddKeySymbol: could not parse key: %s", code.c_str());
		return false;
	}
	if (!keyCodes->AddKeySymbol(keysym, ks.Key())) {
		LOG_L(L_WARNING, "AddKeySymbol: could not add: %s", keysym.c_str());
		return false;
	}
	return true;
}


bool CKeyBindings::AddNamedKeySet(const string& name, const string& keystr)
{
	CKeySet ks;
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "AddNamedKeySet: could not parse keyset: %s", keystr.c_str());
		return false;
	}
	if ((ks.Key() < 0) || !CKeyCodes::IsValidLabel(name)) {
		LOG_L(L_WARNING, "AddNamedKeySet: bad custom keyset name: %s", name.c_str());
		return false;
	}
	namedKeySets[name] = ks;
	return true;
}


bool CKeyBindings::RemoveCommandFromList(ActionList& al, const string& command)
{
	bool success = false;
	for (int i = 0; i < (int)al.size(); ++i) {
		if (al[i].command == command) {
			success = true;
			for (int j = (i + 1); j < (int)al.size(); ++j) {
				al[j - 1] = al[j];
			}
			al.resize(al.size() - 1);
		}
	}
	return success;
}


bool CKeyBindings::ParseKeySet(const string& keystr, CKeySet& ks) const
{
	if (keystr[0] != NamedKeySetChar) {
		return ks.Parse(keystr);
	}
	else {
		const string keysetName = keystr.substr(1);
		NamedKeySetMap::const_iterator it = namedKeySets.find(keysetName);
		if (it !=  namedKeySets.end()) {
			ks = it->second;
		} else {
			return false;
		}
	}
	return true;
}


/******************************************************************************/

void CKeyBindings::LoadDefaults()
{
	SetFakeMetaKey("space");
	const int count = sizeof(defaultBindings) / sizeof(defaultBindings[0]);
	for (int i = 0; i < count; ++i) {
		Bind(defaultBindings[i].key, defaultBindings[i].action);
	}
}

void CKeyBindings::PushAction(const Action& action)
{
	if (action.command == "keyload") {
		Load("uikeys.txt");
	}
	else if (action.command == "keyreload") {
		ExecuteCommand("unbindall");
		ExecuteCommand("unbind enter chat");
		Load("uikeys.txt");
	}
	else if (action.command == "keysave") {
		if (Save("uikeys.tmp")) {  // tmp, not txt
			LOG("Saved uikeys.tmp");
		} else {
			LOG_L(L_WARNING, "Could not save uikeys.tmp");
		}
	}
	else if (action.command == "keyprint") {
		Print();
	}
	else if (action.command == "keysyms") {
		keyCodes->PrintNameToCode(); // move to CKeyCodes?
	}
	else if (action.command == "keycodes") {
		keyCodes->PrintCodeToName(); // move to CKeyCodes?
	}
	else {
		ExecuteCommand(action.rawline);
	}
}

bool CKeyBindings::ExecuteCommand(const string& line)
{
	const vector<string> words = CSimpleParser::Tokenize(line, 2);

	if (words.empty()) {
		return false;
	}
	const string command = StringToLower(words[0]);

	if (command == "keydebug") {
		const int curLogLevel = log_filter_section_getMinLevel(LOG_SECTION_KEY_BINDINGS);
		bool debug = (curLogLevel == LOG_LEVEL_DEBUG);
		if (words.size() == 1) {
			// toggle
			debug = !debug;
		} else if (words.size() >= 2) {
			// set
			debug = atoi(words[1].c_str());
		}
		if (debug && !LOG_IS_ENABLED_STATIC(L_DEBUG)) {
			LOG_L(L_WARNING,
					"You have to run a DEBUG build to be able to log L_DEBUG messages");
		}
		log_filter_section_setMinLevel(LOG_SECTION_KEY_BINDINGS,
				(debug ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO));
	}
	else if ((command == "fakemeta") && (words.size() > 1)) {
		if (!SetFakeMetaKey(words[1])) { return false; }
	}
	else if ((command == "keyset") && (words.size() > 2)) {
		if (!AddNamedKeySet(words[1], words[2])) { return false; }
	}
	else if ((command == "keysym") && (words.size() > 2)) {
		if (!AddKeySymbol(words[1], words[2])) { return false; }
	}
	else if ((command == "bind") && (words.size() > 2)) {
		if (!Bind(words[1], words[2])) { return false; }
	}
	else if ((command == "unbind") && (words.size() > 2)) {
		if (!UnBind(words[1], words[2])) { return false; }
	}
	else if ((command == "unbindaction") && (words.size() > 1)) {
		if (!UnBindAction(words[1])) { return false; }
	}
	else if ((command == "unbindkeyset") && (words.size() > 1)) {
		if (!UnBindKeyset(words[1])) { return false; }
	}
	else if (command == "unbindall") {
		bindings.clear();
		keyCodes->Reset();
		namedKeySets.clear();
		typeBindings.clear();
		Bind("enter", "chat"); // bare minimum
	}
	else {
		return false;
	}

	if (userCommand) {
		Sanitize();
	}

	return false;
}


bool CKeyBindings::Load(const string& filename)
{
//	inComment = false;
	CFileHandler ifs(filename);
	CSimpleParser parser(ifs);

	userCommand = false; // temporarily disable Sanitize() calls

	LoadDefaults();

	while (true) {
		const string line = parser.GetCleanLine();
		if (line.empty()) {
			break;
		}
		if (!ExecuteCommand(line)) {
			ParseTypeBind(parser, line);
		}
	}

	Sanitize();

	userCommand = true; // re-enable Sanitize() calls

	return true;
}


bool CKeyBindings::ParseTypeBind(CSimpleParser& parser, const string& line)
{
	BuildTypeBinding btb;

	const vector<string> words = parser.Tokenize(line, 2);
	if ((words.size() == 3) &&
			(words[2] == "{") && (StringToLower(words[0]) == "bindbuildtype")) {
		btb.keystr = words[1];
	} else {
		return false;
	}

	while (true) {
		const string line = parser.GetCleanLine();
		if (line.empty()) {
			return false;
		}

		const vector<string> words = parser.Tokenize(line, 1);
		if ((words.size() == 1) && (words[0] == "}")) {
			break;
		}

		const string command = StringToLower(words[0]);

		if ((command == "req") || (command == "require")) {
			if (words.size() > 1) {
				btb.reqs.push_back(words[1]);
			}
		}
		else if (command == "sort") {
			if (words.size() > 1) {
				btb.sorts.push_back(words[1]);
			}
		}
		else if (command == "chords") {
			// split them up, tack them on  (in order)
			const vector<string> chords = parser.Tokenize(line, 0);
			for (int i = 1; i < (int)chords.size(); i++) {
				btb.chords.push_back(chords[i]);
			}
		}
	}

	typeBindings.push_back(btb);

	CKeyAutoBinder autoBinder;
	if (!autoBinder.BindBuildType(btb.keystr,
	                              btb.reqs, btb.sorts, btb.chords)) {
		return false;
	}

	return true;
}


void CKeyBindings::Sanitize()
{
	// FIXME -- do something extra here?
	//          (seems as though removing the Up states fixed most of the problems)
	BuildHotkeyMap();
}


void CKeyBindings::BuildHotkeyMap()
{
	hotkeys.clear();

	KeyMap::const_iterator kit;
	for (kit = bindings.begin(); kit != bindings.end(); ++kit) {
		const CKeySet ks = kit->first;
		const string keystr = ks.GetString(true);
		const ActionList& al = kit->second;
		for (int i = 0; i < (int)al.size(); ++i) {
			HotkeyList& hl = hotkeys[al[i].command + ((al[i].extra == "") ? "" : " " + al[i].extra)];
			int j;
			for (j = 0; j < (int)hl.size(); ++j) {
				if (hl[j] == keystr) {
					break;
				}
			}
			if (j == (int)hl.size()) {
				hl.push_back(keystr);
			}
		}
	}
}


/******************************************************************************/

void CKeyBindings::Print() const
{
	FileSave(stdout);
}


bool CKeyBindings::Save(const string& filename) const
{
	FILE* out = fopen(filename.c_str(), "wt");
	if (out == NULL) {
		return false;
	}

	const bool success = FileSave(out);

	fclose(out);

	return success;
}


bool CKeyBindings::FileSave(FILE* out) const
{
	if (out == NULL) {
		return false;
	}

	// clear the defaults
	fprintf(out, "\n");
	fprintf(out, "unbindall          // clear the defaults\n");
	fprintf(out, "unbind enter chat  // clear the defaults\n");
	fprintf(out, "\n");

	// save the user defined key symbols
	keyCodes->SaveUserKeySymbols(out);

	// save the fake meta key (if it has been defined)
	if (fakeMetaKey >= 0) {
		fprintf(out, "fakemeta  %s\n\n", keyCodes->GetName(fakeMetaKey).c_str());
	}

	// save the named keysets
	NamedKeySetMap::const_iterator ks_it;
	for (ks_it = namedKeySets.begin(); ks_it != namedKeySets.end(); ++ks_it) {
		fprintf(out, "keyset  %-15s  %s\n",
		        ks_it->first.c_str(), ks_it->second.GetString(false).c_str());
	}
	if (!namedKeySets.empty()) {
		fprintf(out, "\n");
	}

	// save the bindings
	KeyMap::const_iterator it;
	for (it = bindings.begin(); it != bindings.end(); ++it) {
		const ActionList& al = it->second;
		for (int i = 0; i < (int)al.size(); ++i) {
			const Action& action = al[i];
			string comment;
			if (unitDefHandler && (action.command.find("buildunit_") == 0)) {
				const string unitName = action.command.substr(10);
				const UnitDef* unitDef = unitDefHandler->GetUnitDefByName(unitName);
				if (unitDef) {
					comment = "  // " + unitDef->humanName + " - " + unitDef->tooltip;
				}
			}
			if (comment.empty()) {
				fprintf(out, "bind %18s  %s\n",
				        action.boundWith.c_str(),
				        action.rawline.c_str());
			} else {
				fprintf(out, "bind %18s  %-20s%s\n",
				        action.boundWith.c_str(),
				        action.rawline.c_str(), comment.c_str());
			}
		}
	}
	return true;
}


/******************************************************************************/


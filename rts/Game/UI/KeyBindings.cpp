/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>

#include "KeyBindings.h"
#include "KeyCodes.h"
#include "KeySet.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"


#define LOG_SECTION_KEY_BINDINGS "KeyBindings"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_KEY_BINDINGS)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_KEY_BINDINGS


CONFIG(int, KeyChainTimeout).defaultValue(750).minimumValue(0).description("Timeout in milliseconds waiting for a key chain shortcut.");


CKeyBindings keyBindings;


struct DefaultBinding {
	const char* key;
	const char* action;
};

static const DefaultBinding defaultBindings[] = {
	{            "esc", "quitmessage" },
	{      "Shift+esc", "quitmenu"    },
	{ "Ctrl+Shift+esc", "quitforce"   },
	{  "Alt+Shift+esc", "reloadforce" },
	{      "Any+pause", "pause"       },

	{ "c", "controlunit"      },
	{ "Any+h", "sharedialog"  },
	{ "Any+i", "gameinfo"     },

	{ "Any+j",         "mouse2" },
	{ "backspace", "mousestate" },
	{ "Shift+backspace", "togglecammode" },
	{  "Ctrl+backspace", "togglecammode" },
	{         "Any+tab", "toggleoverview" },

	{               "Any+enter", "chat"           },
	// leave this unbound, takes as many keypresses as exiting ally/spec modes
	// { "Alt+ctrl+z,Alt+ctrl+z", "chatswitchall"  },
	{ "Alt+ctrl+a,Alt+ctrl+a", "chatswitchally" },
	{ "Alt+ctrl+s,Alt+ctrl+s", "chatswitchspec" },

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
	{ "Ctrl+f3", "viewspring" },
	{ "Ctrl+f4", "viewrot"  },
	{ "Ctrl+f5", "viewfree" },

	{ "Any+f1",  "ShowElevation"         },
	{ "Any+f2",  "ShowPathTraversability"},
	{ "Any+f3",  "LastMsgPos"            },
	{ "Any+f4",  "ShowMetalMap"          },
	{ "Any+f5",  "HideInterface"         },
	{ "Any+f6",  "MuteSound"             },
	{ "Any+l",   "togglelos"             },

	{ "Ctrl+Shift+f8",  "savegame" },
	{ "Ctrl+Shift+f10", "createvideo" },
	{ "Any+f11", "screenshot"     },
	{ "Any+f12", "screenshot"     },
	{ "Alt+enter", "fullscreen"  },

	// NOTE: Up bindings are currently converted to press bindings
	//       (see KeySet.cpp / DISALLOW_RELEASE_BINDINGS)

	{ "Any+`,Any+`",    "drawlabel" },
	{ "Any+\\,Any+\\",  "drawlabel" },
	{ "Any+~,Any+~",    "drawlabel" },
	{ "Any+§,Any+§",    "drawlabel" },
	{ "Any+^,Any+^",    "drawlabel" },

	{    "Any+`",    "drawinmap"  },
	{ "Up+Any+`",    "drawinmap"  },
	{    "Any+\\",   "drawinmap"  },
	{ "Up+Any+\\",   "drawinmap"  },
	{    "Any+~",    "drawinmap"  },
	{ "Up+Any+~",    "drawinmap"  },
	{    "Any+§",    "drawinmap"  },
	{ "Up+Any+§",    "drawinmap"  },
	{    "Any+^",    "drawinmap"  },
	{ "Up+Any+^",    "drawinmap"  },

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

void CKeyBindings::Init()
{
	fakeMetaKey = -1;
	keyChainTimeout = 750;

	buildHotkeyMap = true;
	debugEnabled = false;


	bindings.reserve(32);
	hotkeys.reserve(32);

	statefulCommands.reserve(16);
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
	RegisterAction("unbindaction");
	RegisterAction("unbindkeyset");
	RegisterAction("fakemeta");
	RegisterAction("keydebug");
	RegisterAction("keyload");
	RegisterAction("keyreload");
	RegisterAction("keysave");
	RegisterAction("keysyms");
	RegisterAction("keycodes");
	RegisterAction("keyprint");
	SortRegisteredActions();

	configHandler->NotifyOnChange(this, {"KeyChainTimeout"});
}

void CKeyBindings::Kill()
{
	bindings.clear();
	hotkeys.clear();
	statefulCommands.clear();

	configHandler->RemoveObserver(this);
}


/******************************************************************************/

const CKeyBindings::ActionList& CKeyBindings::GetActionList(const CKeySet& ks) const
{
	static const ActionList empty;
	static ActionList merged; //FIXME switch to thread_local (?)

	const ActionList* alPtr = &empty;

	if (ks.AnyMod()) {
		const auto it = bindings.find(ks);

		if (it != bindings.end())
			alPtr = &(it->second);

	} else {
		// have to check for an AnyMod keyset as well as the normal one
		CKeySet anyMod = ks;
		anyMod.SetAnyBit();

		const auto nit = bindings.find(ks);
		const auto ait = bindings.find(anyMod);

		const bool haveNormal = (nit != bindings.end());
		const bool haveAnyMod = (ait != bindings.end());

		if (haveNormal && !haveAnyMod) {
			alPtr = &(nit->second);
		}
		else if (!haveNormal && haveAnyMod) {
			alPtr = &(ait->second);
		}
		else if (haveNormal && haveAnyMod) {
			// combine the two lists (normal first)
			merged = nit->second;
			merged.insert(merged.end(), ait->second.begin(), ait->second.end());
			alPtr = &merged;
		}
	}

	if (debugEnabled) {
		LOG("GetActions: hex=0x%02X acii=\"%s\":", ks.Key(), ks.GetString(false).c_str());

		if (alPtr != &empty) {
			int i = 1;
			for (const auto& a: *alPtr) {
				LOG("   %i. action=\"%s\"  rawline=\"%s\"  shortcut=\"%s\"", i++, a.command.c_str(), a.rawline.c_str(), a.boundWith.c_str());
			}
		} else {
			LOG("   EMPTY");
		}
	}

	return *alPtr;
}


const CKeyBindings::ActionList& CKeyBindings::GetActionList(const CKeyChain& kc) const
{
	static ActionList out; //FIXME switch to thread_local when all buildbots are using >=gcc4.7
	out.clear();

	if (kc.empty())
		return out;

	const CKeyBindings::ActionList& al = GetActionList(kc.back());
	for (const Action& action: al) {
		if (kc.fit(action.keyChain))
			out.push_back(action);
	}
	return out;
}


const CKeyBindings::HotkeyList& CKeyBindings::GetHotkeys(const std::string& action) const
{
	const auto it = hotkeys.find(action);
	if (it == hotkeys.end()) {
		static HotkeyList empty;
		return empty;
	}
	return it->second;
}


/******************************************************************************/

static bool ParseSingleChain(const std::string& keystr, CKeyChain* kc)
{
	kc->clear();
	CKeySet ks;

	// note: this will fail if keystr contains spaces
	std::stringstream ss(keystr);

	while (ss.good()) {
		char kcstr[256];
		ss.getline(kcstr, 256, ',');
		std::string kstr(kcstr);

		if (!ks.Parse(kstr, false))
			return false;

		kc->emplace_back(ks);
	}

	return true;
}

static bool ParseKeyChain(std::string keystr, CKeyChain* kc, const size_t pos = std::string::npos)
{
	// recursive function to allow "," as separator-char & as shortcut
	// -> when parsing fails, this functions replaces one by one all "," by their hexcode
	//    and tries then to reparse it
	// -> i.e. ",,," will at the end parsed as "0x2c,0x2c"

	const size_t cpos = keystr.rfind(',', pos);

	if (ParseSingleChain(keystr, kc))
		return true;

	if (cpos == std::string::npos)
		return false;

	// if cpos is 0, cpos - 1 will equal std::string::npos (size_t::max)
	const size_t nextpos = cpos - 1;

	if ((nextpos != std::string::npos) && ParseKeyChain(keystr, kc, nextpos))
		return true;

	keystr.replace(cpos, 1, IntToString(keyCodes.GetCode(","), "%#x"));
	return ParseKeyChain(keystr, kc, cpos);
}


bool CKeyBindings::Bind(const std::string& keystr, const std::string& line)
{
	Action action(line);
	action.boundWith = keystr;
	if (action.command.empty()) {
		LOG_L(L_WARNING, "Bind: empty action: %s", line.c_str());
		return false;
	}

	if (!ParseKeyChain(keystr, &action.keyChain) || action.keyChain.empty()) {
		LOG_L(L_WARNING, "Bind: could not parse key: %s", keystr.c_str());
		return false;
	}
	CKeySet& ks = action.keyChain.back();

	// Try to be safe, force AnyMod mode for stateful commands
	if (statefulCommands.find(action.command) != statefulCommands.end())
		ks.SetAnyBit();

	const auto it = bindings.find(ks);

	if (it == bindings.end()) {
		// create new keyset entry and push it command
		ActionList& al = bindings[ks];
		al.push_back(action);
	} else {
		ActionList& al = it->second;
		assert(it->first == ks);

		// check if the command is already found to the given keyset
		bool found = false;
		for (const Action& act: al) {
			if (act.command == action.command) {
				found = true;
				break;
			}
		}
		if (!found) {
			// not yet bound, push it
			al.push_back(action);
		}
	}

	return true;
}


bool CKeyBindings::UnBind(const std::string& keystr, const std::string& command)
{
	CKeySet ks;
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "UnBind: could not parse key: %s", keystr.c_str());
		return false;
	}

	const auto it = bindings.find(ks);

	if (it == bindings.end())
		return false;

	ActionList& al = it->second;
	const bool success = RemoveCommandFromList(al, command);

	if (al.empty())
		bindings.erase(it);

	return success;
}


bool CKeyBindings::UnBindKeyset(const std::string& keystr)
{
	CKeySet ks;
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "UnBindKeyset: could not parse key: %s", keystr.c_str());
		return false;
	}

	const auto it = bindings.find(ks);

	if (it == bindings.end())
		return false;

	bindings.erase(it);
	return true;
}


bool CKeyBindings::UnBindAction(const std::string& command)
{
	bool success = false;

	auto it = bindings.begin();

	while (it != bindings.end()) {
		ActionList& al = it->second;

		if (RemoveCommandFromList(al, command))
			success = true;

		if (al.empty()) {
			it = bindings.erase(it);
		} else {
			++it;
		}
	}

	return success;
}


bool CKeyBindings::SetFakeMetaKey(const std::string& keystr)
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


bool CKeyBindings::AddKeySymbol(const std::string& keysym, const std::string& code)
{
	CKeySet ks;
	if (!ks.Parse(code)) {
		LOG_L(L_WARNING, "AddKeySymbol: could not parse key: %s", code.c_str());
		return false;
	}
	if (!keyCodes.AddKeySymbol(keysym, ks.Key())) {
		LOG_L(L_WARNING, "AddKeySymbol: could not add: %s", keysym.c_str());
		return false;
	}
	return true;
}


bool CKeyBindings::RemoveCommandFromList(ActionList& al, const std::string& command)
{
	bool success = false;

	auto it = al.begin();

	while (it != al.end()) {
		if (it->command == command) {
			it = al.erase(it);
			success = true;
		} else {
			++it;
		}
	}

	return success;
}


void CKeyBindings::ConfigNotify(const std::string& key, const std::string& value)
{
	keyChainTimeout = atoi(value.c_str());
}


/******************************************************************************/

void CKeyBindings::LoadDefaults()
{
	SetFakeMetaKey("space");

	for (const auto& b: defaultBindings) {
		Bind(b.key, b.action);
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
		keyCodes.PrintNameToCode(); //TODO move to CKeyCodes?
	}
	else if (action.command == "keycodes") {
		keyCodes.PrintCodeToName(); //TODO move to CKeyCodes?
	}
	else {
		ExecuteCommand(action.rawline);
	}
}

bool CKeyBindings::ExecuteCommand(const std::string& line)
{
	const std::vector<std::string> words = CSimpleParser::Tokenize(line, 2);

	if (words.empty())
		return false;

	const std::string command = StringToLower(words[0]);

	if (command == "keydebug") {
		if (words.size() == 1) {
			// toggle
			debugEnabled = !debugEnabled;
		} else if (words.size() >= 2) {
			// set
			debugEnabled = atoi(words[1].c_str());
		}
	}
	else if ((command == "fakemeta") && (words.size() > 1)) {
		if (!SetFakeMetaKey(words[1])) { return false; }
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
		keyCodes.Reset();
		Bind("enter", "chat"); // bare minimum
	}
	else {
		return false;
	}

	if (buildHotkeyMap)
		BuildHotkeyMap();

	return false;
}


bool CKeyBindings::Load(const std::string& filename)
{
	CFileHandler ifs(filename);
	CSimpleParser parser(ifs);
	buildHotkeyMap = false; // temporarily disable BuildHotkeyMap() calls
	LoadDefaults();

	while (!parser.Eof()) {
		ExecuteCommand(parser.GetCleanLine());
	}

	BuildHotkeyMap();
	buildHotkeyMap = true; // re-enable BuildHotkeyMap() calls
	return true;
}


void CKeyBindings::BuildHotkeyMap()
{
	// create reverse map of bindings ([action] -> key shortcuts)
	hotkeys.clear();

	for (const auto& p: bindings) {
		for (const Action& action: p.second) {
			HotkeyList& hl = hotkeys[action.command + (action.extra.empty() ? "" : " " + action.extra)];
			hl.insert(action.boundWith);
		}
	}
}


/******************************************************************************/

void CKeyBindings::Print() const
{
	FileSave(stdout);
}


bool CKeyBindings::Save(const std::string& filename) const
{
	FILE* out = fopen(filename.c_str(), "wt");
	if (out == nullptr)
		return false;

	const bool success = FileSave(out);
	fclose(out);
	return success;
}


bool CKeyBindings::FileSave(FILE* out) const
{
	if (out == nullptr)
		return false;

	// clear the defaults
	fprintf(out, "\n");
	fprintf(out, "unbindall          // clear the defaults\n");
	fprintf(out, "unbind enter chat  // clear the defaults\n");
	fprintf(out, "\n");

	// save the user defined key symbols
	keyCodes.SaveUserKeySymbols(out);

	// save the fake meta key (if it has been defined)
	if (fakeMetaKey >= 0)
		fprintf(out, "fakemeta  %s\n\n", keyCodes.GetName(fakeMetaKey).c_str());

	// save the bindings
	for (const auto& p: bindings) {
		const ActionList& al = p.second;

		for (const Action& action: al) {
			std::string comment;

			if (unitDefHandler && (action.command.find("buildunit_") == 0)) {
				const std::string unitName = action.command.substr(10);
				const UnitDef* unitDef = unitDefHandler->GetUnitDefByName(unitName);

				if (unitDef != nullptr)
					comment = "  // " + unitDef->humanName + " - " + unitDef->tooltip;
			}

			if (comment.empty()) {
				fprintf(out, "bind %18s  %s\n", action.boundWith.c_str(), action.rawline.c_str());
			} else {
				fprintf(out, "bind %18s  %-20s%s\n", action.boundWith.c_str(), action.rawline.c_str(), comment.c_str());
			}
		}
	}

	return true;
}


/******************************************************************************/


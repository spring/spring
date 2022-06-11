/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>

#include "KeyBindings.h"
#include "KeyCodes.h"
#include "ScanCodes.h"
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


static const CKeyBindings::ActionComparison compareActionByTriggerOrder = [](const Action& a, const Action& b) {
	bool selfAnyMod = a.keyChain.back().AnyMod();
	bool actionAnyMod = b.keyChain.back().AnyMod();

	if (selfAnyMod == actionAnyMod)
		return a.bindingIndex < b.bindingIndex;
	else
		return actionAnyMod;
};


static const CKeyBindings::ActionComparison compareActionByBindingOrder = [](const Action& a, const Action& b) {
  return (a.bindingIndex < b.bindingIndex);
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
	{ "Any+f7",  "DynamicSky"            },
	{ "Any+l",   "togglelos"             },

	{ "Ctrl+Shift+f8",  "savegame" },
	{ "Ctrl+Shift+f10", "createvideo" },
	{ "Any+f11", "screenshot"     },
	{ "Any+f12", "screenshot"     },
	{ "Alt+enter", "fullscreen"  },

	{ "Any+`,Any+`",    "drawlabel" },
	{ "Any+\\,Any+\\",  "drawlabel" },
	{ "Any+~,Any+~",    "drawlabel" },
	{ "Any+§,Any+§",    "drawlabel" },
	{ "Any+^,Any+^",    "drawlabel" },

	{    "Any+`",    "drawinmap"  },
	{    "Any+\\",   "drawinmap"  },
	{    "Any+~",    "drawinmap"  },
	{    "Any+§",    "drawinmap"  },
	{    "Any+^",    "drawinmap"  },

	{    "Any+up",       "moveforward"  },
	{    "Any+down",     "moveback"     },
	{    "Any+right",    "moveright"    },
	{    "Any+left",     "moveleft"     },
	{    "Any+pageup",   "moveup"       },
	{    "Any+pagedown", "movedown"     },

	{    "Any+ctrl",     "moveslow"     },
	{    "Any+shift",    "movefast"     },

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

	codeBindings.reserve(32);
	scanBindings.reserve(32);
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
	codeBindings.clear();
	scanBindings.clear();
	hotkeys.clear();
	statefulCommands.clear();

	configHandler->RemoveObserver(this);
}


/******************************************************************************/

void FilterByKeychain(const ActionList & in, const CKeyChain & kc, ActionList & out)
{
	for (const Action& action: in)
		if (kc.fit(action.keyChain))
			out.push_back(action);
}


void MergeActionListsByTrigger(const ActionList& actionListA, const ActionList& actionListB, ActionList & out)
{
	// When we are retrieving actionlists for a given keyboard state we need to
	// remove duplicate actions that might arise from binding the same action
	// to both key and scancodes
	//
	// If the user bound same action to both keys and scancodes we want to not
	// include action duplicates keeping the action with lower bindingIndex.
	//
	// A duplicate is an action for which theres a correspondent and only one
	// identical action present, but bound to a different key.
	// This is guaranteed by the logic in ::Bind, which also imples it will
	// invariably be the opposite of the action keytype (scancode<->keycode)
	// Duplicates will not exist in a single list but would exist if both lists
	// were merged without care.
	//
	// For each list we have a list of ordered actions that is partitioned in two sections:
	// [k, k, ..., Any+k, Any+k,...]

	// We assume that both lists are already sorted by binding id.
	// We assume that each list does not contain, itself, duplicates.
	// We assume that the two lists are either both non-Any lists, or both Any lists.
	//
	// Their merged result is appended to out.
	if (actionListA.empty()) {
		out.insert(std::end(out), std::begin(actionListB), std::end(actionListB));
		return;
	}

	// Only items from list A
	size_t aBeginId = out.size();
	out.insert(std::end(out), std::begin(actionListA), std::end(actionListA));
	size_t aEndId = out.size();

	if (actionListB.empty()) return;

	// Only add items from list B
	// - If there are duplicates with >= bindingId, don't add them
	// - If there are duplicates with < bindingId, remove the A item
	for (const auto & aB : actionListB) {
		bool toAdd = true;
		for (size_t a = aBeginId; a < aEndId; ++a) {
			// B is a duplicate...
			if (aB.line == out[a].line) {
				// ...with a higher id, so do not add it.
				if (aB.bindingIndex >= out[a].bindingIndex) {
					toAdd = false;
				// ...with a lower id, so remove its A equivalent.
				} else {
					out.erase(std::next(std::begin(out), a));
					--aEndId;
				}
				break;
			}
		}
		if (toAdd) out.push_back(aB);
	}

	// Merge the two parts.
	std::inplace_merge(std::next(std::begin(out), aBeginId), std::next(std::begin(out), aEndId), std::end(out), compareActionByTriggerOrder);
}


const ActionList & CKeyBindings::GetActionList(const CKeySet& ks, bool forceAny) const
{
	static ActionList empty;

	if (ks.Key() < 0)
		return empty;

	const auto & bindings = ks.IsKeyCode() ? codeBindings : scanBindings;

	CKeySet toUse = ks;
	if (forceAny) toUse.SetAnyBit();

	const auto it = bindings.find(toUse);
	if (it != bindings.end())
		return (it->second);

	return empty;
}


ActionList CKeyBindings::GetActionList(const CKeyChain& kc) const
{
	ActionList out;

	if (kc.empty())
		return out;

	const auto & al = GetActionList(kc.back(), false);
	FilterByKeychain(al, kc, out);

	if (!kc.back().AnyMod()) {
		const auto & al = GetActionList(kc.back(), true);
		FilterByKeychain(al, kc, out);
	}

	return out;
}


ActionList CKeyBindings::GetActionList(const CKeyChain& kc, const CKeyChain& sc) const
{
	// Recover the actionLists we need to merge.
	ActionList merged;

	// First get non-Any lists.
	ActionList kList, sList;
	if (!kc.back().AnyMod()) FilterByKeychain(GetActionList(kc.back(), false), kc, kList);
	if (!sc.back().AnyMod()) FilterByKeychain(GetActionList(sc.back(), false), sc, sList);

	MergeActionListsByTrigger(kList, sList, merged);

	// Now do Any
	kList.clear();
	sList.clear();
	FilterByKeychain(GetActionList(kc.back(), true), kc, kList);
	FilterByKeychain(GetActionList(sc.back(), true), sc, sList);

	MergeActionListsByTrigger(kList, sList, merged);

	if (debugEnabled) {
		LOG("GetActions: codeChain=\"%s\" scanChain=\"%s\":", kc.GetString().c_str(), sc.GetString().c_str());

		DebugActionList(merged);
	}

	return merged;
}


ActionList CKeyBindings::GetActionList(int keyCode, int scanCode) const
{
	return GetActionList(keyCode, scanCode, CKeySet::GetCurrentModifiers());
}


ActionList CKeyBindings::GetActionList(int keyCode, int scanCode, unsigned char modifiers) const
{
	CKeyChain codeChain;
	CKeyChain scanChain;

	codeChain.emplace_back(CKeySet(keyCode, modifiers, CKeySet::KSKeyCode));
	scanChain.emplace_back(CKeySet(scanCode, modifiers, CKeySet::KSScanCode));

	return GetActionList(codeChain, scanChain);
}


ActionList CKeyBindings::GetActionList() const
{
	ActionList merged;

	// If hotkey map is built hotkey size is often equal to action count, + 1 for recently bound action
	if (!hotkeys.empty())
		merged.reserve(hotkeys.size() + 1);

	for (const auto& p: codeBindings) {
		const ActionList& al = p.second;

		merged.insert(merged.end(), al.begin(), al.end());
	}

	for (const auto& p: scanBindings) {
		const ActionList& al = p.second;

		merged.insert(merged.end(), al.begin(), al.end());
	}

	std::sort(merged.begin(), merged.end(), compareActionByBindingOrder);

	return merged;
}


void CKeyBindings::DebugActionList(const ActionList& actionList) const {
	LOG("Action List:");
	if (actionList.empty()) {
		LOG("   EMPTY");
	} else {
		int i = 1;
		for (const auto& a: actionList) {
			LOG("   %i.  action=\"%s\"  rawline=\"%s\"  shortcut=\"%s\"  index=\"%i\"", i++, a.command.c_str(), a.rawline.c_str(), a.boundWith.c_str(), a.bindingIndex);
		}
	}
};


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


void CKeyBindings::AddActionToKeyMap(KeyMap& bindings, Action& action)
{
	CKeySet& ks = action.keyChain.back();

	const auto it = bindings.find(ks);

	if (it == bindings.end()) {
		// create new keyset entry and push it command
		ActionList& al = bindings[ks];
		action.bindingIndex = ++bindingsCount;
		al.push_back(action);
	} else {
		ActionList& al = it->second;
		assert(it->first == ks);

		auto it = std::find_if(al.begin(), al.end(), [&action](Action a) {
			return action.line == a.line;
		});

		// check if the command is already bound to the given keyset
		if (it == std::end(al)) {
			// not yet bound, push it
			action.bindingIndex = ++bindingsCount;
			al.push_back(action);
		}
	}
}


bool CKeyBindings::Bind(const std::string& keystr, const std::string& line)
{
	if (debugEnabled)
		LOG("[CKeyBindings::Bind] index=%i keystr=%s line=%s", bindingsCount + 1, keystr.c_str(), line.c_str());

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

	KeyMap& bindings = ks.IsKeyCode() ? codeBindings : scanBindings;
	AddActionToKeyMap(bindings, action);

	return true;
}


bool CKeyBindings::UnBind(const std::string& keystr, const std::string& command)
{
	CKeySet ks;
	if (!ks.Parse(keystr)) {
		LOG_L(L_WARNING, "UnBind: could not parse key: %s", keystr.c_str());
		return false;
	}

	KeyMap& bindings = ks.IsKeyCode() ? codeBindings : scanBindings;
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

	KeyMap& bindings = ks.IsKeyCode() ? codeBindings : scanBindings;

	const auto it = bindings.find(ks);

	if (it == bindings.end())
		return false;

	bindings.erase(it);
	return true;
}


bool CKeyBindings::RemoveActionFromKeyMap(const std::string& command, KeyMap& bindings)
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


bool CKeyBindings::UnBindAction(const std::string& command)
{
	return RemoveActionFromKeyMap(command, codeBindings) || RemoveActionFromKeyMap(command, scanBindings);
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
	if (!ks.IsKeyCode()) {
		LOG_L(L_WARNING, "SetFakeMetaKey: can't assign to scancode: %s", keystr.c_str());
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
	if (!ks.GetKeys().AddKeySymbol(keysym, ks.Key())) {
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
		codeBindings.clear();
		scanBindings.clear();
		keyCodes.Reset();
		scanCodes.Reset();
		bindingsCount = 0;
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

	for (const auto& action: GetActionList()) {
		HotkeyList& hl = hotkeys[action.command + (action.extra.empty() ? "" : " " + action.extra)];
		hl.insert(action.boundWith);
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
	scanCodes.SaveUserKeySymbols(out);

	// save the fake meta key (if it has been defined)
	if (fakeMetaKey >= 0)
		fprintf(out, "fakemeta  %s\n\n", keyCodes.GetName(fakeMetaKey).c_str());

	for (const Action& action: GetActionList()) {
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

	return true;
}


/******************************************************************************/

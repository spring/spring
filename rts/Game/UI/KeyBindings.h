/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <stdio.h>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "KeySet.h"
#include "Game/Console.h"
#include "Game/Action.h"
#include "System/Misc/SpringTime.h"

class CUnit;
class CSimpleParser;


class CKeyBindings : public CommandReceiver
{
	public:
		CKeyBindings();
		~CKeyBindings();

		bool Load(const std::string& filename);
		bool Save(const std::string& filename) const;
		void Print() const;

		typedef std::vector<Action> ActionList;
		typedef std::vector<std::string> HotkeyList;

		const ActionList& GetActionList(const CKeySet& ks);
		const HotkeyList& GetHotkeys(const std::string& action) const;

		virtual void PushAction(const Action&);
		bool ExecuteCommand(const std::string& line);

		int GetFakeMetaKey() const { return fakeMetaKey; }

		void CreateMode(const std::string& modeName);
		void RemoveMode(const std::string& modeName);
		void SwitchMode(const std::string& modeName);
		void SustainOrResetMode() {
		  if (keyModeSustain) {
		    SustainMode();
		  } else {
		    ResetMode();
		  }
		}
		void ResetMode() { SwitchMode(""); }
		void SustainMode();
		static bool IsModeSwitchAction (const std::string& cmd);
		bool IsModeExpired() {
		  spring_time currentTime = spring_gettime();
		  return ((currentTime.toMilliSecsi() - lastModeUseTime.toMilliSecsi()) > keyModeResetTime);
		}

    // Receive configuration notifications (for modeResetMillis)
    void ConfigNotify(const std::string& key, const std::string& value);

	public:
		static const char NamedKeySetChar = '&';
		static const char ModeKeySetChar = '@';

	protected:
		void LoadDefaults();
		void Sanitize();
		void BuildHotkeyMap();

		bool Bind(const std::string& keystring, const std::string& action);
		bool UnBind(const std::string& keystring);
		bool UnBind(const std::string& keystring, const std::string& action);
		bool UnBindKeyset(const std::string& keystr);
		bool UnBindAction(const std::string& action);
		bool SetFakeMetaKey(const std::string& keystring);
		bool AddKeySymbol(const std::string& keysym, const std::string& code);
		bool AddNamedKeySet(const std::string& name, const std::string& keyset);
		bool ParseTypeBind(CSimpleParser& parser, const std::string& line);

		bool ParseModeAndKeySet(const std::string& keystr, CKeySet& ks, std::string& mode) const;
		bool RemoveCommandFromList(ActionList& al, const std::string& command);

		bool FileSave(FILE* file) const;

	protected:
		typedef std::map<CKeySet, ActionList> KeyMap; // keyset to action
		KeyMap bindings;

		typedef std::map<std::string, HotkeyList> ActionMap; // action to keyset
		ActionMap hotkeys;

		typedef std::map<std::string, CKeySet> NamedKeySetMap; // user defined keysets
		NamedKeySetMap namedKeySets;

		typedef std::map<std::string, KeyMap* > ModeMap;
    ModeMap modes;

		KeyMap* GetModeBindings(const std::string& modeName, bool create = false);
    void CheckAndCleanModesByName (const std::string& modeName);

    KeyMap* currentBindings;           // current keymap -- usually &bindings but can point at something in modes[]
    spring_time   lastModeUseTime;     // time of last mode switch or use
    int           keyModeResetTime;    // delay (in ms) after which the current key bindings revert to the default "root" bindings.
    bool          keyModeSustain;      // If true, pressing a key in a mode will sustain the mode for another <keyModeResetTime> millis.
                                       // If false, pressing a key in a mode will end that mode and reactivate the default bindings.

		struct BuildTypeBinding {
			std::string keystr;         // principal keyset
			std::vector<std::string> reqs;   // requirements
			std::vector<std::string> sorts;  // sorting criteria
			std::vector<std::string> chords; // enumerated keyset chords
		};
		std::vector<BuildTypeBinding> typeBindings;

		// commands that use both Up and Down key presses
		std::set<std::string> statefulCommands;

		int fakeMetaKey;
		bool userCommand;
	private:
		bool debugEnabled;
};


extern CKeyBindings* keyBindings;


#endif /* KEYBINDINGS_H */

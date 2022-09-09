/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <string>
#include <vector>

#include "KeySet.h"
#include "Game/Console.h"
#include "Game/Action.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"


class CKeyBindings : public CommandReceiver
{
	public:
		typedef std::vector<std::string> HotkeyList;
		typedef std::function<bool (Action, Action)> ActionComparison;

	protected:
		struct KeySetHash {
			uint64_t operator ()(const CKeySet& ks) const {
				return (((unsigned long long)ks.Key() * 6364136223846793005ull + ks.Mod() * 9600629759793949339ull) % 15726070495360670683ull);
			}
		};

		typedef spring::unsynced_map<CKeySet, ActionList, KeySetHash> KeyMap; // keyset to action
		typedef spring::unsynced_map<std::string, HotkeyList> ActionMap; // action to keyset

	public:
		void Init();
		void Kill();

		bool Load(const std::string& filename);
		bool Save(const std::string& filename) const;
		void Print() const;

		ActionList GetActionList() const;
		ActionList GetActionList(int keyCode, int scanCode) const;
		ActionList GetActionList(int keyCode, int scanCode, unsigned char modifiers) const;
		ActionList GetActionList(const CKeyChain& kc) const;
		ActionList GetActionList(const CKeyChain& kc, const CKeyChain& sc) const;
		const HotkeyList& GetHotkeys(const std::string& action) const;

		virtual void PushAction(const Action&);
		bool ExecuteCommand(const std::string& line);

		// Receive configuration notifications (for KeyChainTimeout)
		void ConfigNotify(const std::string& key, const std::string& value);

		int GetFakeMetaKey() const { return fakeMetaKey; }
		int GetKeyChainTimeout() const { return keyChainTimeout; }

	protected:
		void LoadDefaults();
		void BuildHotkeyMap();
		void DebugActionList(const ActionList& actionList) const;

		void AddActionToKeyMap(KeyMap& bindings, Action& action);
		static bool RemoveActionFromKeyMap(const std::string& command, KeyMap& bindings);

		bool Bind(const std::string& keystring, const std::string& action);
		bool UnBind(const std::string& keystring, const std::string& action);
		bool UnBindKeyset(const std::string& keystr);
		bool UnBindAction(const std::string& action);
		bool SetFakeMetaKey(const std::string& keystring);
		bool AddKeySymbol(const std::string& keysym, const std::string& code);

		static bool RemoveCommandFromList(ActionList& al, const std::string& command);

		bool FileSave(FILE* file) const;

  protected:
		const ActionList & GetActionList(const CKeySet& ks, bool forceAny) const;

		KeyMap codeBindings;
		KeyMap scanBindings;
		ActionMap hotkeys;
		int bindingsCount;

		// commands that use both Up and Down key presses
		spring::unsynced_set<std::string> statefulCommands;

	private:
		int fakeMetaKey = -1;
		int keyChainTimeout = 750;

		bool buildHotkeyMap = true;
		bool debugEnabled = false;
};


extern CKeyBindings keyBindings;


#endif /* KEYBINDINGS_H */

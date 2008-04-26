#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H
// KeyBindings.h: interface for the CKeyBindings class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "KeySet.h"
#include "Game/Console.h"
#include "Game/Action.h"

class CUnit;
class CFileHandler;
class CSimpleParser;


class CKeyBindings : public CommandReciever
{
	public:
		CKeyBindings();
		~CKeyBindings();

		bool Load(const std::string& filename);
		bool Save(const std::string& filename) const;
		void Print() const;

		typedef std::vector<Action> ActionList;
		typedef std::vector<std::string> HotkeyList;

		const ActionList& GetActionList(const CKeySet& ks) const;
		const HotkeyList& GetHotkeys(const std::string& action) const;

		virtual void PushAction(const Action&);
		bool Command(const std::string& line);

		int GetFakeMetaKey() const { return fakeMetaKey; }

		int GetDebug() const { return debug; }

	public:
		static const char NamedKeySetChar = '&';

	protected:
		void LoadDefaults();
		void Sanitize();
		void BuildHotkeyMap();

		bool Bind(const std::string& keystring, const std::string& action);
		bool UnBind(const std::string& keystring, const std::string& action);
		bool UnBindKeyset(const std::string& keystr);
		bool UnBindAction(const std::string& action);
		bool SetFakeMetaKey(const std::string& keystring);
		bool AddKeySymbol(const std::string& keysym, const std::string& code);
		bool AddNamedKeySet(const std::string& name, const std::string& keyset);
		bool ParseTypeBind(CSimpleParser& parser, const std::string& line);

		bool ParseKeySet(const std::string& keystr, CKeySet& ks) const;
		bool RemoveCommandFromList(ActionList& al, const std::string& command);

		bool FileSave(FILE* file) const;

	protected:
		typedef std::map<CKeySet, ActionList> KeyMap; // keyset to action
		KeyMap bindings;

		typedef std::map<std::string, HotkeyList> ActionMap; // action to keyset
		ActionMap hotkeys;

		typedef std::map<std::string, CKeySet> NamedKeySetMap; // user defined keysets
		NamedKeySetMap namedKeySets;

		struct BuildTypeBinding {
			std::string keystr;         // principal keyset
			std::vector<std::string> reqs;   // requirements
			std::vector<std::string> sorts;  // sorting criteria
			std::vector<std::string> chords; // enumerated keyset chords
		};
		std::vector<BuildTypeBinding> typeBindings;

		// commands that use both Up and Down key presses
		std::set<std::string> statefulCommands;

		int debug;
		int fakeMetaKey;
		bool userCommand;
};


extern CKeyBindings* keyBindings;


#endif /* KEYBINDINGS_H */

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

using namespace std;

class CUnit;
class CFileHandler;
class CSimpleParser;


class CKeyBindings : public CommandReciever
{
	public:
		CKeyBindings();
		~CKeyBindings();

		bool Load(const string& filename);
		bool Save(const string& filename) const;
		void Print() const;

		typedef vector<Action> ActionList;
		typedef vector<string> HotkeyList;

		const ActionList& GetActionList(const CKeySet& ks) const;
		const HotkeyList& GetHotkeys(const string& action) const;

		virtual void PushAction(const Action&);
		bool Command(const string& line);

		int GetFakeMetaKey() const { return fakeMetaKey; }

		int GetDebug() const { return debug; }

	public:
		static const char NamedKeySetChar = '&';

	protected:
		void LoadDefaults();
		void Sanitize();
		void BuildHotkeyMap();

		bool Bind(const string& keystring, const string& action);
		bool UnBind(const string& keystring, const string& action);
		bool UnBindKeyset(const string& keystr);
		bool UnBindAction(const string& action);
		bool SetFakeMetaKey(const string& keystring);
		bool AddKeySymbol(const string& keysym, const string& code);
		bool AddNamedKeySet(const string& name, const string& keyset);
		bool ParseTypeBind(CSimpleParser& parser, const string& line);

		bool ParseKeySet(const string& keystr, CKeySet& ks) const;
		bool RemoveCommandFromList(ActionList& al, const string& command);

		bool FileSave(FILE* file) const;

	protected:
		typedef map<CKeySet, ActionList> KeyMap; // keyset to action
		KeyMap bindings;

		typedef map<string, HotkeyList> ActionMap; // action to keyset
		ActionMap hotkeys;

		typedef map<string, CKeySet> NamedKeySetMap; // user defined keysets
		NamedKeySetMap namedKeySets;

		struct BuildTypeBinding {
			string keystr;         // principal keyset
			vector<string> reqs;   // requirements
			vector<string> sorts;  // sorting criteria
			vector<string> chords; // enumerated keyset chords
		};
		vector<BuildTypeBinding> typeBindings;

		// commands that use both Up and Down key presses
		set<string> statefulCommands;

		int debug;
		int fakeMetaKey;
		bool userCommand;
};


extern CKeyBindings* keyBindings;


#endif /* KEYBINDINGS_H */

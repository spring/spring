#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H
// KeyBindings.h: interface for the CKeyBindings class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <stdio.h>
#include <string>
#include <vector>
#include <set>
#include <map>
using namespace std;
#include "KeySet.h"


class CUnit;


class CKeyBindings
{
	public:
		CKeyBindings();
		~CKeyBindings();
		
		bool Load(const string& filename);
		bool Save(const string& filename) const;
		void Print() const;

		struct Action {
			Action() {}
			Action(const string& line);
			string command;   // first word, lowercase
			string extra;     // everything but the first word
			string rawline;   // includes the command, case preserved
			string boundWith; // the string that defined the bound key
		};
		typedef vector<Action> ActionList;
		typedef vector<string> HotkeyList;
		
		const ActionList& GetActionList(const CKeySet& ks) const;
		const HotkeyList& GetHotkeys(const string& action) const;
		
		bool Command(const string& line);
		
		int GetFakeMetaKey() const { return fakeMetaKey; }
		
		int GetDebug() const { return debug; }
		void SetDebug(int dbg) { debug = dbg; }

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

		bool ParseKeySet(const string& keystr, CKeySet& ks) const;
		bool RemoveCommandFromList(ActionList& al, const string& command);

		bool FileSave(FILE* file) const;
		void OutputDebug(const char* msg) const;

	protected:
		typedef map<CKeySet, ActionList> KeyMap; // keyset to action
		KeyMap bindings;

		typedef map<string, HotkeyList> ActionMap; // action to keyset
		ActionMap hotkeys;
		
		typedef map<string, CKeySet> NamedKeySetMap; // user defined keysets
		NamedKeySetMap namedKeySets;

		// commands that use both Up and Down key presses		
		set<string> statefulCommands;

		int debug;
		int fakeMetaKey;
		bool userCommand;
};


extern CKeyBindings* keyBindings;


#endif /* KEYBINDINGS_H */

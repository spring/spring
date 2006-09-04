#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H
// KeyBindings.h: interface for the CKeyBindings class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

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
			string rawline; // includes the command, case preserved
			string command; // first word, lowercase
			string extra;   // everything but the first word
		};
		typedef vector<Action> ActionList;
		typedef vector<string> HotkeyList;
		
		const ActionList& GetActionList(const CKeySet& ks) const;
		const HotkeyList& GetHotkeys(const string& action) const;
		
		bool Command(const string& line);
		
		int GetFakeMetaKey() const { return fakeMetaKey; }
		
		int GetDebug() const { return debug; }
		void SetDebug(int dbg) { debug = dbg; }
		
	protected:
		void LoadDefaults();
		void Sanitize();
		void BuildHotkeyMap();
		bool Bind(const string& keystring, const string& action);
		bool UnBind(const string& keystring, const string& action);
		bool UnBindKeyset(const string& keystr);
		bool UnBindAction(const string& action);
		bool SetFakeMetaKey(const string& keystring);
		bool RemoveCommandFromList(ActionList& al, const string& command);
		void OutputDebug(const char* msg) const;

	protected:
		typedef map<CKeySet, ActionList> KeyMap; // keyset to action
		KeyMap bindings;

		typedef map<string, HotkeyList> ActionMap; // action to keyset
		ActionMap hotkeys;

		// commands that use both Up and Down key presses		
		set<string> statefulCommands;

		int debug;
		int fakeMetaKey;
		bool userCommand;
};


extern CKeyBindings* keyBindings;


#endif /* KEYBINDINGS_H */

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
		CKeyBindings(const string& filename);
		virtual ~CKeyBindings();
		
		bool Load(const string& filename);
		bool Save(const string& filename) const;
		void Print() const;

		string GetAction(const CKeySet& ks, const string& context) const; // FIXME
		
		bool Bind(const string& keystring, const string& action, const string& context);
		bool UnBindKeyset(const string& keystr, const string& context);
		bool UnBindAction(const string& action, const string& context);
		bool UnBindContext(const string& context);
		
	public:
		bool debug;
		
	protected:
		void LoadDefaults();

	protected:
		typedef map<CKeySet, string> KeyMap;
		typedef map<string, KeyMap>  ContextKeyMap;
		ContextKeyMap bindings;
};

extern CKeyBindings* keyBindings;


#endif /* KEYBINDINGS_H */

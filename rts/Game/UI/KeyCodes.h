#ifndef KEYCODES_H
#define KEYCODES_H
// KeyCodes.h: interface for the CKeyCodes class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <stdio.h>
#include <string>
#include <map>

using namespace std;


class CKeyCodes {
	public:
		CKeyCodes();
		void Reset();

		int GetCode(const string& name) const;

		string GetName(int code) const;
		string GetDefaultName(int code) const;
		
		bool AddKeySymbol(const string& name, int code);
		
		bool IsModifier(int code) const;
		
		void PrintNameToCode() const;
		void PrintCodeToName() const;
		
		void SaveUserKeySymbols(FILE* file) const;
		
	public:
		static bool IsValidLabel(const string& label);

	protected:
		void AddPair(const string& name, int code);
		
	protected:
		map<string, int> nameToCode;
		map<int, string> codeToName;
		map<string, int> defaultNameToCode;
		map<int, string> defaultCodeToName;
};

extern CKeyCodes* keyCodes;


#endif /* KEYCODES_H */

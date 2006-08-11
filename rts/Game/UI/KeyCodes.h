#ifndef KEYCODES_H
#define KEYCODES_H
// KeyCodes.h: interface for the CKeyCodes class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <map>

using namespace std;


class CKeyCodes {
	public:
		CKeyCodes();

		int GetCode(const string& name) const;
		string GetName(int code) const;
		
		bool IsModifier(int code) const;
		
		void PrintNameToCode() const;
		void PrintCodeToName() const;

	protected:
		void AddPair(const string& name, int code);
		
	protected:
		map<string, int> nameToCode;
		map<int, string> codeToName;
};

extern CKeyCodes* keyCodes;


#endif /* KEYCODES_H */

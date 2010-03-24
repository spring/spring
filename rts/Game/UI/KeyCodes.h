/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYCODES_H
#define KEYCODES_H

#include <stdio.h>
#include <string>
#include <map>

class CKeyCodes {
	public:
		CKeyCodes();
		void Reset();

		int GetCode(const std::string& name) const;

		std::string GetName(int code) const;
		std::string GetDefaultName(int code) const;

		bool AddKeySymbol(const std::string& name, int code);

		bool IsModifier(int code) const;

		void PrintNameToCode() const;
		void PrintCodeToName() const;

		void SaveUserKeySymbols(FILE* file) const;

	public:
		static bool IsValidLabel(const std::string& label);

	protected:
		void AddPair(const std::string& name, int code);

	protected:
		std::map<std::string, int> nameToCode;
		std::map<int, std::string> codeToName;
		std::map<std::string, int> defaultNameToCode;
		std::map<int, std::string> defaultCodeToName;
};

extern CKeyCodes* keyCodes;

#endif /* KEYCODES_H */

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYCODES_H
#define KEYCODES_H

#include <cstdio>
#include <string>
#include <vector>

class CKeyCodes {
public:
	void Reset();

	int GetCode(const std::string& name) const;

	std::string GetName(int code) const;
	std::string GetDefaultName(int code) const;

	bool AddKeySymbol(const std::string& name, int code);

	static bool IsModifier(int code);
	       bool IsPrintable(int code) const;

	void PrintNameToCode() const;
	void PrintCodeToName() const;

	void SaveUserKeySymbols(FILE* file) const;

public:
	static bool IsValidLabel(const std::string& label);

protected:
	void AddPair(const std::string& name, const int code, const bool printable = false);

protected:
	typedef std::pair<std::string, int> NameCodePair;
	typedef std::pair<int, std::string> CodeNamePair;

	std::vector<NameCodePair> nameToCode;
	std::vector<CodeNamePair> codeToName;
	std::vector<NameCodePair> defaultNameToCode;
	std::vector<CodeNamePair> defaultCodeToName;
	std::vector<         int> printableCodes;
};

extern CKeyCodes keyCodes;

#endif /* KEYCODES_H */

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>
#include <SDL_keycode.h>

#include "KeyCodes.h"
#include "System/Log/ILog.h"
#include "System/Platform/SDL1_keysym.h"
#include "System/StringUtil.h"

CKeyCodes keyCodes;


int CKeyCodes::GetCode(const std::string& name) const
{
	const auto pred = [](const NameCodePair& a, const NameCodePair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(nameToCode.begin(), nameToCode.end(), NameCodePair{name, 0}, pred);

	if (iter == nameToCode.end() || iter->first != name)
		return -1;

	return iter->second;
}


std::string CKeyCodes::GetName(int code) const
{
	const auto pred = [](const CodeNamePair& a, const CodeNamePair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(codeToName.begin(), codeToName.end(), CodeNamePair{0, ""}, pred);

	if (iter == codeToName.end() || iter->first != code)
		return IntToString(code, "0x%03X");

	return iter->second;
}

std::string CKeyCodes::GetDefaultName(int code) const
{
	const auto pred = [](const CodeNamePair& a, const CodeNamePair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(defaultCodeToName.begin(), defaultCodeToName.end(), CodeNamePair{0, ""}, pred);

	if (iter == defaultCodeToName.end() || iter->first != code)
		return IntToString(code, "0x%03X");

	return iter->second;
}


bool CKeyCodes::AddKeySymbol(const std::string& name, int code)
{
	if ((code < 0) || !IsValidLabel(name))
		return false;

	const std::string keysym = StringToLower(name);

	// do not allow existing keysyms to be renamed
	const auto namePred = [](const NameCodePair& a, const NameCodePair& b) { return (a.first < b.first); };
	const auto nameIter = std::lower_bound(nameToCode.begin(), nameToCode.end(), NameCodePair{keysym, 0}, namePred);

	if (nameIter != nameToCode.end() && nameIter->first == name)
		return false;

	nameToCode.emplace_back(keysym, code);
	// assumes that the user would rather see their own names
	codeToName.emplace_back(code, keysym);

	// swap into position
	for (size_t i = nameToCode.size() - 1; i > 0; i--) {
		if (nameToCode[i - 1].first < nameToCode[i].first)
			break;

		std::swap(nameToCode[i - 1], nameToCode[i]);
	}
	for (size_t i = codeToName.size() - 1; i > 0; i--) {
		if (codeToName[i - 1].first < codeToName[i].first)
			break;

		std::swap(codeToName[i - 1], codeToName[i]);
	}

	return true;
}


bool CKeyCodes::IsValidLabel(const std::string& label)
{
	if (label.empty())
		return false;

	if (!isalpha(label[0]))
		return false;

	// if any character is not alpha-numeric *and* not space, reject label as invalid
	return (std::find_if(label.begin(), label.end(), [](char c) { return (!isalnum(c) && (c != '_')); }) == label.end());
}

bool CKeyCodes::IsModifier(int code)
{
	switch (code) {
		case SDLK_LALT:
		case SDLK_LCTRL:
		case SDLK_LGUI:
		case SDLK_LSHIFT:
		case SDLK_RALT:
		case SDLK_RCTRL:
		case SDLK_RGUI:
		case SDLK_RSHIFT:
			return true;
	}
	return false;
}


bool CKeyCodes::IsPrintable(int code) const
{
	const auto pred = [](int a, int b) { return (a < b); };
	const auto iter = std::lower_bound(printableCodes.begin(), printableCodes.end(), code, pred);

	return (iter != printableCodes.end() && *iter == code);
}


void CKeyCodes::AddPair(const std::string& name, const int code, const bool printable)
{
	nameToCode.emplace_back(name, code);
	codeToName.emplace_back(code, name);

	if (!printable)
		return;

	printableCodes.push_back(code);
}


void CKeyCodes::Reset()
{
	nameToCode.clear();
	nameToCode.reserve(64);
	codeToName.clear();
	codeToName.reserve(64);

	printableCodes.clear();
	printableCodes.reserve(64);
	
	AddPair("backspace", SDLK_BACKSPACE);
	AddPair("tab",       SDLK_TAB);
	AddPair("clear",     SDLK_CLEAR);
	AddPair("enter",     SDLK_RETURN); //FIXME
	AddPair("return",    SDLK_RETURN);
	AddPair("pause",     SDLK_PAUSE);
	AddPair("esc",       SDLK_ESCAPE);
	AddPair("escape",    SDLK_ESCAPE);
	AddPair("space",     SDLK_SPACE, true);
	AddPair("delete",    SDLK_DELETE);

	// ASCII mapped keysyms
	for (unsigned char i = ' '; i <= 'z'; ++i) {
		if (isupper(i))
			continue;

		AddPair(std::string(1, i), i, true);
	}

	AddPair("§", 0xA7, true);
	AddPair("~", SDLK_BACKQUOTE, true);
	AddPair("tilde", SDLK_BACKQUOTE, true);
	AddPair("backquote", SDLK_BACKQUOTE, true);

	// Numeric keypad
	AddPair("numpad0", SDLK_KP_0, true);
	AddPair("numpad1", SDLK_KP_1, true);
	AddPair("numpad2", SDLK_KP_2, true);
	AddPair("numpad3", SDLK_KP_3, true);
	AddPair("numpad4", SDLK_KP_4, true);
	AddPair("numpad5", SDLK_KP_5, true);
	AddPair("numpad6", SDLK_KP_6, true);
	AddPair("numpad7", SDLK_KP_7, true);
	AddPair("numpad8", SDLK_KP_8, true);
	AddPair("numpad9", SDLK_KP_9, true);
	AddPair("numpad.", SDLK_KP_PERIOD, true);
	AddPair("numpad/", SDLK_KP_DIVIDE, true);
	AddPair("numpad*", SDLK_KP_MULTIPLY, true);
	AddPair("numpad-", SDLK_KP_MINUS, true);
	AddPair("numpad+", SDLK_KP_PLUS, true);
	AddPair("numpad=", SDLK_KP_EQUALS, true);
	AddPair("numpad_enter", SDLK_KP_ENTER);

	// Arrows + Home/End pad
	AddPair("up",       SDLK_UP);
	AddPair("down",     SDLK_DOWN);
	AddPair("right",    SDLK_RIGHT);
	AddPair("left",     SDLK_LEFT);
	AddPair("insert",   SDLK_INSERT);
	AddPair("home",     SDLK_HOME);
	AddPair("end",      SDLK_END);
	AddPair("pageup",   SDLK_PAGEUP);
	AddPair("pagedown", SDLK_PAGEDOWN);

	// Function keys
	AddPair("f1",  SDLK_F1);
	AddPair("f2",  SDLK_F2);
	AddPair("f3",  SDLK_F3);
	AddPair("f4",  SDLK_F4);
	AddPair("f5",  SDLK_F5);
	AddPair("f6",  SDLK_F6);
	AddPair("f7",  SDLK_F7);
	AddPair("f8",  SDLK_F8);
	AddPair("f9",  SDLK_F9);
	AddPair("f10", SDLK_F10);
	AddPair("f11", SDLK_F11);
	AddPair("f12", SDLK_F12);
	AddPair("f13", SDLK_F13);
	AddPair("f14", SDLK_F14);
	AddPair("f15", SDLK_F15);

	// Key state modifier keys
	//AddPair("numlock", SDLK_NUMLOCK);
	//AddPair("capslock", SDLK_CAPSLOCK);
	//AddPair("scrollock", SDLK_SCROLLOCK);
	AddPair("shift", SDLK_LSHIFT);
	AddPair("ctrl",  SDLK_LCTRL);
	AddPair("alt",   SDLK_LALT);
	AddPair("meta",  SDLK_LGUI);
	// these can not be used correctly anyway (without special support in other parts of Spring code...)
	//AddPair("super", SDLK_LSUPER);    // Left "Windows" key
	//AddPair("mode", SDLK_MODE);       // "Alt Gr" key
	//AddPair("compose", SDLK_COMPOSE); // Multi-key compose key

	// Miscellaneous function keys
	AddPair("help", SDLK_HELP);
	AddPair("printscreen", SDLK_PRINTSCREEN);
	AddPair("print", SDLK_PRINTSCREEN);
	//AddPair("sysreq", SDLK_SYSREQ);
	//AddPair("break", SDLK_BREAK);
	//AddPair("menu", SDLK_MENU);
	//AddPair("power", SDLK_POWER);     // Power Macintosh power key
	//AddPair("euro", SDLK_EURO);       // Some european keyboards
	//AddPair("undo", SDLK_UNDO);       // Atari keyboard has Undo

	std::sort(nameToCode.begin(), nameToCode.end(), [](const NameCodePair& a, const NameCodePair& b) { return (a.first < b.first); });
	std::sort(codeToName.begin(), codeToName.end(), [](const CodeNamePair& a, const CodeNamePair& b) { return (a.first < b.first); });
	std::sort(printableCodes.begin(), printableCodes.end());

	nameToCode.erase(std::unique(nameToCode.begin(), nameToCode.end(), [](const NameCodePair& a, const NameCodePair& b) { return (a.first == b.first); }), nameToCode.end());
	codeToName.erase(std::unique(codeToName.begin(), codeToName.end(), [](const CodeNamePair& a, const CodeNamePair& b) { return (a.first == b.first); }), codeToName.end());
	printableCodes.erase(std::unique(printableCodes.begin(), printableCodes.end()), printableCodes.end());

	// remember our defaults
	defaultNameToCode.clear();
	defaultNameToCode.resize(nameToCode.size());
	defaultCodeToName.clear();
	defaultCodeToName.resize(codeToName.size());

	std::copy(nameToCode.begin(), nameToCode.end(), defaultNameToCode.begin());
	std::copy(codeToName.begin(), codeToName.end(), defaultCodeToName.begin());
}


void CKeyCodes::PrintNameToCode() const
{
	for (const auto& p: nameToCode) {
		LOG("KEYNAME: %13s = 0x%03X (SDL1 = 0x%03X)", p.first.c_str(), p.second, SDL21_keysyms(p.second));
	}
}

void CKeyCodes::PrintCodeToName() const
{
	for (const auto& p: codeToName) {
		LOG("KEYCODE: 0x%03X = '%s' (SDL1 = 0x%03X)", p.first, p.second.c_str(), SDL21_keysyms(p.first));
	}
}


void CKeyCodes::SaveUserKeySymbols(FILE* file) const
{
	bool output = false;

	for (const auto& p: nameToCode) {
		const auto defSymPred = [](const NameCodePair& a, const NameCodePair& b) { return (a.first < b.first); };
		const auto defSymIter = std::lower_bound(defaultNameToCode.begin(), defaultNameToCode.end(), NameCodePair{p.first, 0}, defSymPred);

		if (defSymIter != defaultNameToCode.end() && defSymIter->first == p.first)
			continue;

		// this keysym is not standard
		const std::string& extSymName = p.first;
		const std::string& defSymName = GetDefaultName(p.second);
		fprintf(file, "keysym  %-10s  %s\n", extSymName.c_str(), defSymName.c_str());
		output = true;
	}

	if (!output)
		return;

	fprintf(file, "\n");
}

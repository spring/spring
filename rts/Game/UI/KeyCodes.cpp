/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>


#include "KeyCodes.h"
#include "SDL_keycode.h"
#include "System/Log/ILog.h"
#include "System/Platform/SDL1_keysym.h"
#include "System/Util.h"

CKeyCodes* keyCodes = NULL;


int CKeyCodes::GetCode(const std::string& name) const
{
	const auto it = nameToCode.find(name);
	if (it == nameToCode.end()) {
		return -1;
	}
	return it->second;
}


std::string CKeyCodes::GetName(int code) const
{
	const auto it = codeToName.find(code);
	if (it == codeToName.end()) {
		return IntToString(code, "0x%03X");
	}
	return it->second;
}


std::string CKeyCodes::GetDefaultName(int code) const
{
	const auto it = defaultCodeToName.find(code);
	if (it == defaultCodeToName.end()) {
		return IntToString(code, "0x%03X");
	}
	return it->second;
}


bool CKeyCodes::AddKeySymbol(const std::string& name, int code)
{
	if ((code < 0) || !IsValidLabel(name)) {
		return false;
	}

	const std::string keysym = StringToLower(name);

	// do not allow existing keysyms to be renamed
	const auto name_it = nameToCode.find(keysym);
	if (name_it != nameToCode.end()) {
		return false;
	}
	nameToCode[keysym] = code;

	// assumes that the user would rather see their own names
	codeToName[code] = keysym;
	return true;
}


bool CKeyCodes::IsValidLabel(const std::string& label)
{
	if (label.empty()) {
		return false;
	}
	if (!isalpha(label[0])) {
		return false;
	}
	for (const char& c: label) {
		if (!isalnum(c) && (c != '_')) {
			return false;
		}
	}
	return true;
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


bool CKeyCodes::IsPrintable(int code)
{
	return (printableCodes.find(code) != printableCodes.end());
}



void CKeyCodes::AddPair(const std::string& name, const int code, const bool printable)
{
	if (nameToCode.find(name) == nameToCode.end()) {
		nameToCode[name] = code;
	}
	if (codeToName.find(code) == codeToName.end()) {
		codeToName[code] = name;
	}
	if (printable)
		printableCodes.insert(code);
}


CKeyCodes::CKeyCodes()
{
	Reset();
}


void CKeyCodes::Reset()
{
	nameToCode.clear();
	codeToName.clear();
	
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
		if (!isupper(i)) {
			AddPair(std::string(1, i), i, true);
		}
	}

	AddPair("§", 0xA7, true);
	AddPair("~", SDLK_BACKQUOTE, true);
	AddPair("tilde", SDLK_BACKQUOTE, true);
	AddPair("backquote", SDLK_BACKQUOTE, true);

	/* Numeric keypad */
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

	/* Arrows + Home/End pad */
	AddPair("up",       SDLK_UP);
	AddPair("down",     SDLK_DOWN);
	AddPair("right",    SDLK_RIGHT);
	AddPair("left",     SDLK_LEFT);
	AddPair("insert",   SDLK_INSERT);
	AddPair("home",     SDLK_HOME);
	AddPair("end",      SDLK_END);
	AddPair("pageup",   SDLK_PAGEUP);
	AddPair("pagedown", SDLK_PAGEDOWN);

	/* Function keys */
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

	/* Key state modifier keys */
	//AddPair("numlock", SDLK_NUMLOCK);
	//AddPair("capslock", SDLK_CAPSLOCK);
	//AddPair("scrollock", SDLK_SCROLLOCK);
	AddPair("shift", SDLK_LSHIFT);
	AddPair("ctrl",  SDLK_LCTRL);
	AddPair("alt",   SDLK_LALT);
	AddPair("meta",  SDLK_LGUI);
	// I doubt these can be used correctly anyway (without special support in other parts of the spring code...)
	//AddPair("super", SDLK_LSUPER);    /* Left "Windows" key */
	//AddPair("mode", SDLK_MODE);       /* "Alt Gr" key */
	//AddPair("compose", SDLK_COMPOSE); /* Multi-key compose key */

	/* Miscellaneous function keys */
	AddPair("help", SDLK_HELP);
	AddPair("printscreen", SDLK_PRINTSCREEN);
	AddPair("print", SDLK_PRINTSCREEN);
	//AddPair("sysreq", SDLK_SYSREQ);
	//AddPair("break", SDLK_BREAK);
	//AddPair("menu", SDLK_MENU);
	//AddPair("power", SDLK_POWER);     /* Power Macintosh power key */
	//AddPair("euro", SDLK_EURO);       /* Some european keyboards */
	//AddPair("undo", SDLK_UNDO);       /* Atari keyboard has Undo */

	//XXX Do they work? > NO
	/*AddPair("joyx", 400);
	AddPair("joyy", 401);
	AddPair("joyz", 402);
	AddPair("joyw", 403);
	AddPair("joy0", 300);
	AddPair("joy1", 301);
	AddPair("joy2", 302);
	AddPair("joy3", 303);
	AddPair("joy4", 304);
	AddPair("joy5", 305);
	AddPair("joy6", 306);
	AddPair("joy7", 307);
	AddPair("joyup",    320);
	AddPair("joydown",  321);
	AddPair("joyleft",  322);
	AddPair("joyright", 323);*/

	// remember our defaults
	defaultNameToCode = nameToCode;
	defaultCodeToName = codeToName;
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
		const std::string& keysym = p.first;
		const auto def_it = defaultNameToCode.find(keysym);
		if (def_it == defaultNameToCode.end()) {
			// this keysym is not standard
			const int code = p.second;
			std::string name = GetDefaultName(code);
			fprintf(file, "keysym  %-10s  %s\n", keysym.c_str(), name.c_str());
			output = true;
		}
	}

	if (output) {
		fprintf(file, "\n");
	}
	
	return;
}

#include "StdAfx.h"
// KeyCodes.cpp: implementation of the CKeyCodes class.
//
//////////////////////////////////////////////////////////////////////
#include <cctype>

#include "mmgr.h"

#include "KeyCodes.h"
#include "SDL_keysym.h"
#include "System/LogOutput.h"


CKeyCodes* keyCodes = NULL;


int CKeyCodes::GetCode(const std::string& name) const
{
	std::map<std::string, int>::const_iterator it = nameToCode.find(name);
	if (it == nameToCode.end()) {
		return -1;
	}
	return it->second;
}


std::string CKeyCodes::GetName(int code) const
{
	std::map<int, std::string>::const_iterator it = codeToName.find(code);
	if (it == codeToName.end()) {
		char buf[64];
		SNPRINTF(buf, sizeof(buf), "0x%03X", code);
		return buf;
	}
	return it->second;
}


std::string CKeyCodes::GetDefaultName(int code) const
{
	std::map<int, std::string>::const_iterator it = defaultCodeToName.find(code);
	if (it == defaultCodeToName.end()) {
		char buf[64];
		SNPRINTF(buf, sizeof(buf), "0x%03X", code);
		return buf;
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
	std::map<std::string, int>::const_iterator name_it = nameToCode.find(keysym);
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
	for (int i = 0; i < (int)label.size(); i++) {
		const char c = label[i];
		if (!isalnum(c) && (c != '_')) {
			return false;
		}
	}
	return true;
}


bool CKeyCodes::IsModifier(int code) const
{
	switch (code) {
		case SDLK_LALT:
		case SDLK_LCTRL:
		case SDLK_LMETA:
		case SDLK_LSHIFT:
			return true;
	}
	return false;
}


void CKeyCodes::AddPair(const std::string& name, int code)
{
	if (nameToCode.find(name) == nameToCode.end()) {
		nameToCode[name] = code;
	}
	if (codeToName.find(code) == codeToName.end()) {
		codeToName[code] = name;
	}
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
	AddPair("enter",     SDLK_RETURN);
	AddPair("return",    SDLK_RETURN);
	AddPair("pause",     SDLK_PAUSE);
	AddPair("esc",       SDLK_ESCAPE);
	AddPair("escape",    SDLK_ESCAPE);
	AddPair("space",     SDLK_SPACE);
	AddPair("delete",    SDLK_DELETE);

	// ASCII mapped keysyms
	for (unsigned char i = ' '; i <= '~'; ++i) {
		if (!isupper(i)) {
			AddPair(std::string(1, i), i);
		}
	}

	nameToCode["\xA7"] = 220;

	/* Numeric keypad */
	AddPair("numpad0", SDLK_KP0);
	AddPair("numpad1", SDLK_KP1);
	AddPair("numpad2", SDLK_KP2);
	AddPair("numpad3", SDLK_KP3);
	AddPair("numpad4", SDLK_KP4);
	AddPair("numpad5", SDLK_KP5);
	AddPair("numpad6", SDLK_KP6);
	AddPair("numpad7", SDLK_KP7);
	AddPair("numpad8", SDLK_KP8);
	AddPair("numpad9", SDLK_KP9);
	AddPair("numpad.", SDLK_KP_PERIOD);
	AddPair("numpad/", SDLK_KP_DIVIDE);
	AddPair("numpad*", SDLK_KP_MULTIPLY);
	AddPair("numpad-", SDLK_KP_MINUS);
	AddPair("numpad+", SDLK_KP_PLUS);
	AddPair("numpad=", SDLK_KP_EQUALS);
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
	AddPair("meta",  SDLK_LMETA);
	// I doubt these can be used correctly anyway (without special support in other parts of the spring code...)
	//AddPair("super", SDLK_LSUPER);    /* Left "Windows" key */
	//AddPair("mode", SDLK_MODE);       /* "Alt Gr" key */
	//AddPair("compose", SDLK_COMPOSE); /* Multi-key compose key */

	/* Miscellaneous function keys */
	//AddPair("help", SDLK_HELP);
	AddPair("printscreen", SDLK_PRINT);
	//AddPair("sysreq", SDLK_SYSREQ);
	//AddPair("break", SDLK_BREAK);
	//AddPair("menu", SDLK_MENU);
	// These conflict with "joy*" nameToCode.
	//AddPair("power", SDLK_POWER);     /* Power Macintosh power key */
	//AddPair("euro", SDLK_EURO);       /* Some european keyboards */
	//AddPair("undo", SDLK_UNDO);       /* Atari keyboard has Undo */

	// Are these required for someone??
	//AddPair("'<'", 226);
	//AddPair("','", 188);
	//AddPair("'.'", 190);

	AddPair("joyx", 400);
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
	AddPair("joyright", 323);

	// remember our defaults	
  defaultNameToCode = nameToCode;
  defaultCodeToName = codeToName;
}


void CKeyCodes::PrintNameToCode() const
{
	std::map<std::string, int>::const_iterator it;
	for (it = nameToCode.begin(); it != nameToCode.end(); ++it) {
		logOutput.Print("KEYNAME: %13s = 0x%03X\n", it->first.c_str(), it->second);
	}
}


void CKeyCodes::PrintCodeToName() const
{
	std::map<int, std::string>::const_iterator it;
	for (it = codeToName.begin(); it != codeToName.end(); ++it) {
		logOutput.Print("KEYCODE: 0x%03X = '%s'\n", it->first, it->second.c_str());
	}
}


void CKeyCodes::SaveUserKeySymbols(FILE* file) const
{
	bool output = false;
	std::map<std::string, int>::const_iterator user_it;
	for (user_it = nameToCode.begin(); user_it != nameToCode.end(); ++user_it) {
		std::map<std::string, int>::const_iterator def_it;
		const std::string& keysym = user_it->first;
		def_it = defaultNameToCode.find(keysym);
		if (def_it == defaultNameToCode.end()) {
			// this keysym is not standard
			const int code = user_it->second;
			std::string name = GetDefaultName(code);
			if (name.empty()) {
				char buf[16];
				SNPRINTF(buf, 16, "0x%03X", code);
				name = buf;
			}
			fprintf(file, "keysym  %-10s  %s\n", keysym.c_str(), name.c_str());
			output = true;
		}
	}

	if (output) {
		fprintf(file, "\n");
	}
	
	return;
}

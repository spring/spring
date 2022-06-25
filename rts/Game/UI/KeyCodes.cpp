/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>
#include <SDL_keycode.h>

#include "KeyCodes.h"
#include "System/Log/ILog.h"
#include "System/Platform/SDL1_keysym.h"
#include "System/StringUtil.h"

CKeyCodes keyCodes;


int CKeyCodes::GetNormalizedSymbol(int sym)
{
	if (sym <= SDLK_DELETE)
		return (tolower(sym));

	switch (sym) {
		case SDLK_RSHIFT: { return SDLK_LSHIFT; } break;
		case SDLK_RCTRL : { return SDLK_LCTRL ; } break;
		case SDLK_RGUI  : { return SDLK_LGUI  ; } break;
		case SDLK_RALT  : { return SDLK_LALT  ; } break;
		default         : {                     } break;
	}

	return sym;
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
	for (int i = 0; i < 12; i++) {
		AddPair("f" + IntToString(i + 1 ), SDLK_F1  + i);
		AddPair("f" + IntToString(i + 13), SDLK_F13 + i);
	}

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

	std::sort(nameToCode.begin(), nameToCode.end(), namePred);
	std::sort(codeToName.begin(), codeToName.end(), codePred);
	std::sort(printableCodes.begin(), printableCodes.end());

	nameToCode.erase(std::unique(nameToCode.begin(), nameToCode.end(), [](const auto& a, const auto& b) { return (a.first == b.first); }), nameToCode.end());
	codeToName.erase(std::unique(codeToName.begin(), codeToName.end(), [](const auto& a, const auto& b) { return (a.first == b.first); }), codeToName.end());
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

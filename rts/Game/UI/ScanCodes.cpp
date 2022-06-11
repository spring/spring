/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <SDL_scancode.h>

#include "ScanCodes.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"

CScanCodes scanCodes;


int CScanCodes::GetNormalizedSymbol(int sym)
{
	switch (sym) {
		case SDL_SCANCODE_RSHIFT: { return SDL_SCANCODE_LSHIFT; } break;
		case SDL_SCANCODE_RCTRL : { return SDL_SCANCODE_LCTRL ; } break;
		case SDL_SCANCODE_RGUI  : { return SDL_SCANCODE_LGUI  ; } break;
		case SDL_SCANCODE_RALT  : { return SDL_SCANCODE_LALT  ; } break;
		default                 : {                             } break;
	}

	return sym;
}


bool CScanCodes::IsModifier(int code)
{
	switch (code) {
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_LGUI:
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RALT:
		case SDL_SCANCODE_RCTRL:
		case SDL_SCANCODE_RGUI:
		case SDL_SCANCODE_RSHIFT:
			return true;
	}
	return false;
}


void CScanCodes::Reset()
{
	nameToCode.clear();
	nameToCode.reserve(64);
	codeToName.clear();
	codeToName.reserve(64);

	printableCodes.clear();
	printableCodes.reserve(64);

	AddPair("sc_backspace", SDL_SCANCODE_BACKSPACE);
	AddPair("sc_tab",       SDL_SCANCODE_TAB);
	AddPair("sc_clear",     SDL_SCANCODE_CLEAR);
	AddPair("sc_return",    SDL_SCANCODE_RETURN);
	AddPair("sc_pause",     SDL_SCANCODE_PAUSE);
	AddPair("sc_esc",       SDL_SCANCODE_ESCAPE);
	AddPair("sc_escape",    SDL_SCANCODE_ESCAPE);
	AddPair("sc_space",     SDL_SCANCODE_SPACE, true);
	AddPair("sc_delete",    SDL_SCANCODE_DELETE);

	AddPair("sc_a",    SDL_SCANCODE_A);
	AddPair("sc_b",    SDL_SCANCODE_B);
	AddPair("sc_c",    SDL_SCANCODE_C);
	AddPair("sc_d",    SDL_SCANCODE_D);
	AddPair("sc_e",    SDL_SCANCODE_E);
	AddPair("sc_f",    SDL_SCANCODE_F);
	AddPair("sc_g",    SDL_SCANCODE_G);
	AddPair("sc_h",    SDL_SCANCODE_H);
	AddPair("sc_i",    SDL_SCANCODE_I);
	AddPair("sc_j",    SDL_SCANCODE_J);
	AddPair("sc_k",    SDL_SCANCODE_K);
	AddPair("sc_l",    SDL_SCANCODE_L);
	AddPair("sc_m",    SDL_SCANCODE_M);
	AddPair("sc_n",    SDL_SCANCODE_N);
	AddPair("sc_o",    SDL_SCANCODE_O);
	AddPair("sc_p",    SDL_SCANCODE_P);
	AddPair("sc_q",    SDL_SCANCODE_Q);
	AddPair("sc_r",    SDL_SCANCODE_R);
	AddPair("sc_s",    SDL_SCANCODE_S);
	AddPair("sc_t",    SDL_SCANCODE_T);
	AddPair("sc_u",    SDL_SCANCODE_U);
	AddPair("sc_v",    SDL_SCANCODE_V);
	AddPair("sc_w",    SDL_SCANCODE_W);
	AddPair("sc_x",    SDL_SCANCODE_X);
	AddPair("sc_y",    SDL_SCANCODE_Y);
	AddPair("sc_z",    SDL_SCANCODE_Z);
	AddPair("sc_0",    SDL_SCANCODE_0);
	AddPair("sc_1",    SDL_SCANCODE_1);
	AddPair("sc_2",    SDL_SCANCODE_2);
	AddPair("sc_3",    SDL_SCANCODE_3);
	AddPair("sc_4",    SDL_SCANCODE_4);
	AddPair("sc_5",    SDL_SCANCODE_5);
	AddPair("sc_6",    SDL_SCANCODE_6);
	AddPair("sc_7",    SDL_SCANCODE_7);
	AddPair("sc_8",    SDL_SCANCODE_8);
	AddPair("sc_9",    SDL_SCANCODE_9);

	AddPair("sc_comma", SDL_SCANCODE_COMMA, true);

	// (Located in the top left corner (on both ANSI and ISO keyboards).
	//
	// Produces GRAVE ACCENT and TILDE in a US Windows layout and in US and UK Mac
	// layouts on ANSI keyboards, GRAVE ACCENT and NOT SIGN in a UK Windows
	// layout, SECTION SIGN and PLUS-MINUS SIGN in US and UK Mac layouts on ISO
	// keyboards, SECTION SIGN and DEGREE SIGN in a Swiss German layout
	// (Mac: only on ISO keyboards), CIRCUMFLEX ACCENT and DEGREE SIGN in a
	// German layout (Mac: only on ISO keyboards), SUPERSCRIPT TWO and TILDE in
	// a French Windows layout, COMMERCIAL AT and NUMBER SIGN in a French Mac
	// layout on ISO keyboards, and LESS-THAN SIGN and GREATER-THAN SIGN in a
	// Swiss German, German, or French Mac layout on ANSI keyboards.)
	AddPair("sc_`", SDL_SCANCODE_GRAVE, true);
	AddPair("sc_backquote", SDL_SCANCODE_GRAVE, true);

	// (Located at the lower left of the return key on ISO keyboards and at the
	// right end of the QWERTY row on ANSI keyboards.
	//
	// Produces REVERSE SOLIDUS (backslash) and VERTICAL LINE in a US layout,
	// REVERSE SOLIDUS and VERTICAL LINE in a UK Mac layout, NUMBER SIGN and
	// TILDE in a UK Windows layout, DOLLAR SIGN and POUND SIGN in a Swiss
	// German layout, NUMBER SIGN and APOSTROPHE in a German layout, GRAVE ACCENT
	// and POUND SIGN in a French Mac layout, and ASTERISK and MICRO SIGN in a
	// French Windows layout.)
	AddPair("sc_\\", SDL_SCANCODE_BACKSLASH, true);
	AddPair("sc_backslash", SDL_SCANCODE_BACKSLASH, true);

	// This is the additional key that ISO keyboards have over ANSI ones, located
	// between left shift and Y.
	//
	// Produces GRAVE ACCENT and TILDE in a US or UK Mac layout, REVERSE SOLIDUS
	// (backslash) and VERTICAL LINE in a US or UK Windows layout, and LESS-THAN
	// SIGN and GREATER-THAN SIGN in a Swiss German, German, or French layout.)
	//
	// Note from Spring developer: Games should not rely on this scancode for
	// core features.
	//
	// However, even if the feature is obscure, games can add functionality for
	// when the user has a keyboard that emits the key or users can bind to it
	// without relying on the keycode.
	AddPair("sc_nonusbacklash", SDL_SCANCODE_NONUSBACKSLASH, true);

	// Numeric keypad
	AddPair("sc_numpad0", SDL_SCANCODE_KP_0, true);
	AddPair("sc_numpad1", SDL_SCANCODE_KP_1, true);
	AddPair("sc_numpad2", SDL_SCANCODE_KP_2, true);
	AddPair("sc_numpad3", SDL_SCANCODE_KP_3, true);
	AddPair("sc_numpad4", SDL_SCANCODE_KP_4, true);
	AddPair("sc_numpad5", SDL_SCANCODE_KP_5, true);
	AddPair("sc_numpad6", SDL_SCANCODE_KP_6, true);
	AddPair("sc_numpad7", SDL_SCANCODE_KP_7, true);
	AddPair("sc_numpad8", SDL_SCANCODE_KP_8, true);
	AddPair("sc_numpad9", SDL_SCANCODE_KP_9, true);
	AddPair("sc_numpad.", SDL_SCANCODE_KP_PERIOD, true);
	AddPair("sc_numpad/", SDL_SCANCODE_KP_DIVIDE, true);
	AddPair("sc_numpad*", SDL_SCANCODE_KP_MULTIPLY, true);
	AddPair("sc_numpad-", SDL_SCANCODE_KP_MINUS, true);
	AddPair("sc_numpad+", SDL_SCANCODE_KP_PLUS, true);
	AddPair("sc_numpad=", SDL_SCANCODE_KP_EQUALS, true);
	AddPair("sc_numpad_enter", SDL_SCANCODE_KP_ENTER);

	// Arrows + Home/End pad
	AddPair("sc_up",       SDL_SCANCODE_UP);
	AddPair("sc_down",     SDL_SCANCODE_DOWN);
	AddPair("sc_right",    SDL_SCANCODE_RIGHT);
	AddPair("sc_left",     SDL_SCANCODE_LEFT);
	AddPair("sc_insert",   SDL_SCANCODE_INSERT);
	AddPair("sc_home",     SDL_SCANCODE_HOME);
	AddPair("sc_end",      SDL_SCANCODE_END);
	AddPair("sc_pageup",   SDL_SCANCODE_PAGEUP);
	AddPair("sc_pagedown", SDL_SCANCODE_PAGEDOWN);

	// Function keys
	AddPair("sc_f1",  SDL_SCANCODE_F1);
	AddPair("sc_f2",  SDL_SCANCODE_F2);
	AddPair("sc_f3",  SDL_SCANCODE_F3);
	AddPair("sc_f4",  SDL_SCANCODE_F4);
	AddPair("sc_f5",  SDL_SCANCODE_F5);
	AddPair("sc_f6",  SDL_SCANCODE_F6);
	AddPair("sc_f7",  SDL_SCANCODE_F7);
	AddPair("sc_f8",  SDL_SCANCODE_F8);
	AddPair("sc_f9",  SDL_SCANCODE_F9);
	AddPair("sc_f10", SDL_SCANCODE_F10);
	AddPair("sc_f11", SDL_SCANCODE_F11);
	AddPair("sc_f12", SDL_SCANCODE_F12);
	AddPair("sc_f13", SDL_SCANCODE_F13);
	AddPair("sc_f14", SDL_SCANCODE_F14);
	AddPair("sc_f15", SDL_SCANCODE_F15);

	// Key state modifier keys
	//AddPair("sc_numlock", SDL_SCANCODE_NUMLOCKCLEAR);
	//AddPair("sc_capslock", SDL_SCANCODE_CAPSLOCK);
	//AddPair("sc_scrollock", SDL_SCANCODE_SCROLLLOCK);
	AddPair("sc_shift", SDL_SCANCODE_LSHIFT);
	AddPair("sc_ctrl",  SDL_SCANCODE_LCTRL);
	AddPair("sc_alt",   SDL_SCANCODE_LALT);
	AddPair("sc_meta",  SDL_SCANCODE_LGUI);

	// Miscellaneous function keys
	AddPair("sc_help", SDL_SCANCODE_HELP);
	AddPair("sc_printscreen", SDL_SCANCODE_PRINTSCREEN);
	AddPair("sc_print", SDL_SCANCODE_PRINTSCREEN);
	//AddPair("sc_sysreq", SDL_SCANCODE_SYSREQ);
	//AddPair("sc_menu", SDL_SCANCODE_MENU);
	//AddPair("sc_power", SDL_SCANCODE_POWER);     // Power Macintosh power key
	//AddPair("sc_undo", SDL_SCANCODE_UNDO);       // Atari keyboard has Undo

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


void CScanCodes::PrintNameToCode() const
{
	for (const auto& p: nameToCode) {
		LOG("SCANNAME: %s = %d", p.first.c_str(), p.second);
	}
}


void CScanCodes::PrintCodeToName() const
{
	for (const auto& p: codeToName) {
		LOG("SCANCODE: %d = '%s'", p.first, p.second.c_str());
	}
}

extern CScanCodes scanCodes;

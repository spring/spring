/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SDL1_keysym.h"
#include <SDL_keycode.h>
#include <unordered_map>


template<typename First, typename Second>
class unordered_bimap {
public:
	typedef std::unordered_map<First, Second> first_type;
	typedef std::unordered_map<Second, First> second_type;

	first_type const& first() const { return first_; }
	second_type const& second() const { return second_; }

	unordered_bimap(const std::initializer_list<std::pair<const First, Second>> list)
		: first_(list)
	{
		second_.reserve(list.size());
		for (const auto& pair: list) {
			if (second_.find(pair.second) == second_.end()) {
				second_[pair.second] = pair.first;
			}
		}
	}

private:
	const first_type first_;
	      second_type second_;
};


static const unordered_bimap<int, int> SDL_keysym_bimap = {
	{SDLK_UNKNOWN, 0},

	{SDLK_RETURN, 13},
	{SDLK_ESCAPE, 27},
	{SDLK_BACKSPACE, 8},
	{SDLK_TAB, 9},
	{SDLK_SPACE, 32},
	{SDLK_EXCLAIM, 33},
	{SDLK_QUOTEDBL, 34},
	{SDLK_HASH, 35},
	//{SDLK_PERCENT, 0},
	{SDLK_DOLLAR, 36},
	{SDLK_AMPERSAND, 38},
	{SDLK_QUOTE, 39},
	{SDLK_LEFTPAREN, 40},
	{SDLK_RIGHTPAREN, 41},
	{SDLK_ASTERISK, 42},
	{SDLK_PLUS, 43},
	{SDLK_COMMA, 44},
	{SDLK_MINUS, 45},
	{SDLK_PERIOD, 46},
	{SDLK_SLASH, 47},
	{SDLK_0, 48},
	{SDLK_1, 49},
	{SDLK_2, 50},
	{SDLK_3, 51},
	{SDLK_4, 52},
	{SDLK_5, 53},
	{SDLK_6, 54},
	{SDLK_7, 55},
	{SDLK_8, 56},
	{SDLK_9, 57},
	{SDLK_COLON, 58},
	{SDLK_SEMICOLON, 59},
	{SDLK_LESS, 60},
	{SDLK_EQUALS, 61},
	{SDLK_GREATER, 62},
	{SDLK_QUESTION, 63},
	{SDLK_AT, 64},
	{SDLK_LEFTBRACKET, 91},
	{SDLK_BACKSLASH, 92},
	{SDLK_RIGHTBRACKET, 93},
	{SDLK_CARET, 94},
	{SDLK_UNDERSCORE, 95},
	{SDLK_BACKQUOTE, 96},
	{SDLK_a, 97},
	{SDLK_b, 98},
	{SDLK_c, 99},
	{SDLK_d, 100},
	{SDLK_e, 101},
	{SDLK_f, 102},
	{SDLK_g, 103},
	{SDLK_h, 104},
	{SDLK_i, 105},
	{SDLK_j, 106},
	{SDLK_k, 107},
	{SDLK_l, 108},
	{SDLK_m, 109},
	{SDLK_n, 110},
	{SDLK_o, 111},
	{SDLK_p, 112},
	{SDLK_q, 113},
	{SDLK_r, 114},
	{SDLK_s, 115},
	{SDLK_t, 116},
	{SDLK_u, 117},
	{SDLK_v, 118},
	{SDLK_w, 119},
	{SDLK_x, 120},
	{SDLK_y, 121},
	{SDLK_z, 122},

	{SDLK_CAPSLOCK, 301},

	{SDLK_F1, 282},
	{SDLK_F2, 283},
	{SDLK_F3, 284},
	{SDLK_F4, 285},
	{SDLK_F5, 286},
	{SDLK_F6, 287},
	{SDLK_F7, 288},
	{SDLK_F8, 289},
	{SDLK_F9, 290},
	{SDLK_F10, 291},
	{SDLK_F11, 292},
	{SDLK_F12, 293},

	{SDLK_PRINTSCREEN, 316},
	{SDLK_SCROLLLOCK, 302},
	{SDLK_PAUSE, 19},
	{SDLK_INSERT, 277},
	{SDLK_HOME, 278},
	{SDLK_PAGEUP, 280},
	{SDLK_DELETE, 127},
	{SDLK_END, 279},
	{SDLK_PAGEDOWN, 281},
	{SDLK_RIGHT, 275},
	{SDLK_LEFT, 276},
	{SDLK_DOWN, 274},
	{SDLK_UP, 273},

	{SDLK_NUMLOCKCLEAR, 300},
	{SDLK_KP_DIVIDE, 267},
	{SDLK_KP_MULTIPLY, 268},
	{SDLK_KP_MINUS, 269},
	{SDLK_KP_PLUS, 270},
	{SDLK_KP_ENTER, 271},
	{SDLK_KP_1, 257},
	{SDLK_KP_2, 258},
	{SDLK_KP_3, 259},
	{SDLK_KP_4, 260},
	{SDLK_KP_5, 261},
	{SDLK_KP_6, 262},
	{SDLK_KP_7, 263},
	{SDLK_KP_8, 264},
	{SDLK_KP_9, 265},
	{SDLK_KP_0, 256},
	{SDLK_KP_PERIOD, 266},

//	{SDLK_APPLICATION, 0},
	{SDLK_POWER, 320},
	{SDLK_KP_EQUALS, 272},
	{SDLK_F13, 294},
	{SDLK_F14, 295},
	{SDLK_F15, 296},
//	{SDLK_F16, 0},
//	{SDLK_F17, 0},
//	{SDLK_F18, 0},
//	{SDLK_F19, 0},
//	{SDLK_F20, 0},
//	{SDLK_F21, 0},
//	{SDLK_F22, 0},
//	{SDLK_F23, 0},
//	{SDLK_F24, 0},
//	{SDLK_EXECUTE, 0},
	{SDLK_HELP, 315},
	{SDLK_MENU, 319},
//	{SDLK_SELECT, 0},
//	{SDLK_STOP, 0},
//	{SDLK_AGAIN, 0},
	{SDLK_UNDO, 322},
//	{SDLK_CUT, 0},
//	{SDLK_COPY, 0},
//	{SDLK_PASTE, 0},
//	{SDLK_FIND, 0},
//	{SDLK_MUTE, 0},
//	{SDLK_VOLUMEUP, 0},
//	{SDLK_VOLUMEDOWN, 0},
	{SDLK_KP_COMMA, 44}, //using SDLK_COMMA
//	{SDLK_KP_EQUALSAS400, 0},

//	{SDLK_ALTERASE, 0},
	{SDLK_SYSREQ, 317},
//	{SDLK_CANCEL, 0},
	{SDLK_CLEAR, 12},
//	{SDLK_PRIOR, 0},
//	{SDLK_RETURN2, 0},
//	{SDLK_SEPARATOR, 0},
//	{SDLK_OUT, 0},
//	{SDLK_OPER, 0},
//	{SDLK_CLEARAGAIN, 0},
//	{SDLK_CRSEL, 0},
//	{SDLK_EXSEL, 0},

//	{SDLK_KP_00, 0},
//	{SDLK_KP_000, 0},
//	{SDLK_THOUSANDSSEPARATOR, 0},
//	{SDLK_DECIMALSEPARATOR, 0},
//	{SDLK_CURRENCYUNIT, 0},
//	{SDLK_CURRENCYSUBUNIT, 0},
//	{SDLK_KP_LEFTPAREN, 0},
//	{SDLK_KP_RIGHTPAREN, 0},
//	{SDLK_KP_LEFTBRACE, 0},
//	{SDLK_KP_RIGHTBRACE, 0},
//	{SDLK_KP_TAB, 0},
//	{SDLK_KP_BACKSPACE, 0},
//	{SDLK_KP_A, 0},
//	{SDLK_KP_B, 0},
//	{SDLK_KP_C, 0},
//	{SDLK_KP_D, 0},
//	{SDLK_KP_E, 0},
//	{SDLK_KP_F, 0},
//	{SDLK_KP_XOR, 0},
//	{SDLK_KP_POWER, 0},
//	{SDLK_KP_PERCENT, 0},
//	{SDLK_KP_LESS, 0},
//	{SDLK_KP_GREATER, 0},
//	{SDLK_KP_AMPERSAND, 0},
//	{SDLK_KP_DBLAMPERSAND, 0},
//	{SDLK_KP_VERTICALBAR, 0},
//	{SDLK_KP_DBLVERTICALBAR, 0},
//	{SDLK_KP_COLON, 0},
//	{SDLK_KP_HASH, 0},
//	{SDLK_KP_SPACE, 0},
//	{SDLK_KP_AT, 0},
//	{SDLK_KP_EXCLAM, 0},
//	{SDLK_KP_MEMSTORE, 0},
//	{SDLK_KP_MEMRECALL, 0},
//	{SDLK_KP_MEMCLEAR, 0},
//	{SDLK_KP_MEMADD, 0},
//	{SDLK_KP_MEMSUBTRACT, 0},
//	{SDLK_KP_MEMMULTIPLY, 0},
//	{SDLK_KP_MEMDIVIDE, 0},
//	{SDLK_KP_PLUSMINUS, 0},
//	{SDLK_KP_CLEAR, 0},
//	{SDLK_KP_CLEARENTRY, 0},
//	{SDLK_KP_BINARY, 0},
//	{SDLK_KP_OCTAL, 0},
//	{SDLK_KP_DECIMAL, 0},
//	{SDLK_KP_HEXADECIMAL, 0},

	{SDLK_LCTRL, 306},
	{SDLK_LSHIFT, 304},
	{SDLK_LALT, 308},
	{SDLK_LGUI, 310},
	{SDLK_RCTRL, 305},
	{SDLK_RSHIFT, 303},
	{SDLK_RALT, 307},
	{SDLK_RGUI, 309},

	{SDLK_MODE, 313},

//	{SDLK_AUDIONEXT, 0},
//	{SDLK_AUDIOPREV, 0},
//	{SDLK_AUDIOSTOP, 0},
//	{SDLK_AUDIOPLAY, 0},
//	{SDLK_AUDIOMUTE, 0},
//	{SDLK_MEDIASELECT, 0},
//	{SDLK_WWW, 0},
//	{SDLK_MAIL, 0},
//	{SDLK_CALCULATOR, 0},
//	{SDLK_COMPUTER, 0},
//	{SDLK_AC_SEARCH, 0},
//	{SDLK_AC_HOME, 0},
//	{SDLK_AC_BACK, 0},
//	{SDLK_AC_FORWARD, 0},
//	{SDLK_AC_STOP, 0},
//	{SDLK_AC_REFRESH, 0},
//	{SDLK_AC_BOOKMARKS, 0},

//	{SDLK_BRIGHTNESSDOWN, 0},
//	{SDLK_BRIGHTNESSUP, 0},
//	{SDLK_DISPLAYSWITCH, 0},
//	{SDLK_KBDILLUMTOGGLE, 0},
//	{SDLK_KBDILLUMDOWN, 0},
//	{SDLK_KBDILLUMUP, 0},
//	{SDLK_EJECT, 0},
//	{SDLK_SLEEP, 0},
};


int SDL21_keysyms(const int SDL2_keycode)
{
	auto it = SDL_keysym_bimap.first().find(SDL2_keycode);
	if (it != SDL_keysym_bimap.first().end())
		return it->second;
	return 0;
}


int SDL12_keysyms(const int SDL1_keycode)
{
	auto it = SDL_keysym_bimap.second().find(SDL1_keycode);
	if (it != SDL_keysym_bimap.second().end())
		return it->second;
	return 0;
}


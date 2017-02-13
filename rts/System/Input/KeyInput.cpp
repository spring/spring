/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_events.h>
#include <SDL_stdinc.h>

#include "KeyInput.h"

#include "System/Log/ILog.h"
/**
* @brief keys
*
* Array of possible keys, and which are being pressed
*/
static std::map<int, bool> keys;
static SDL_Keymod keyMods;


namespace KeyInput {
	bool IsKeyPressed(int idx) {
		auto it = keys.find(idx);
		if (it != keys.end())
			return (it->second != 0);
		return false;
	}

	void SetKeyModState(int mod, bool pressed) {
		if (pressed) {
			keyMods = SDL_Keymod(keyMods | mod);
		} else {
			keyMods = SDL_Keymod(keyMods & ~mod);
		}
	}

	bool GetKeyModState(int mod) {
		return (keyMods & mod);
	}

	/**
	* Tests SDL keystates and sets values in key array
	*/
	void Update(int currKeycode, int fakeMetaKey)
	{
		int numKeys = 0;
		auto state = SDL_GetKeyboardState(&numKeys);
		for (int i = 0; i < numKeys; ++i) {
			auto scancode = (SDL_Scancode)i;
			auto keycode  = SDL_GetKeyFromScancode(scancode);
			keys[keycode] = (state[scancode] != 0);
		}

		keyMods = SDL_GetModState();
		SetKeyModState(KMOD_GUI, keys[fakeMetaKey]);

		keys[SDLK_LALT]   = GetKeyModState(KMOD_ALT);
		keys[SDLK_LCTRL]  = GetKeyModState(KMOD_CTRL);
		keys[SDLK_LGUI]   = GetKeyModState(KMOD_GUI);
		keys[SDLK_LSHIFT] = GetKeyModState(KMOD_SHIFT);
	}

	const std::map<int,bool>& GetPressedKeys()
	{
		return keys;
	}

	int GetNormalizedKeySymbol(int sym)
	{
		if (sym <= SDLK_DELETE) {
			sym = tolower(sym);
		}
		else if (sym == SDLK_RSHIFT) { sym = SDLK_LSHIFT; }
		else if (sym == SDLK_RCTRL)  { sym = SDLK_LCTRL;  }
		else if (sym == SDLK_RGUI)   { sym = SDLK_LGUI;   }
		else if (sym == SDLK_RALT)   { sym = SDLK_LALT;   }

		return sym;
	}

	void ReleaseAllKeys()
	{
		for (auto key: keys) {
			auto keycode  = (SDL_Keycode)key.first;
			auto scancode = SDL_GetScancodeFromKey(keycode);

			if (keycode == SDLK_NUMLOCKCLEAR || keycode == SDLK_CAPSLOCK || keycode == SDLK_SCROLLLOCK)
				continue;

			if (!KeyInput::IsKeyPressed(keycode))
				continue;

			SDL_Event event;
			event.type = event.key.type = SDL_KEYUP;
			event.key.state = SDL_RELEASED;
			event.key.keysym.sym = keycode;
			event.key.keysym.mod = 0;
			event.key.keysym.scancode = scancode;
			SDL_PushEvent(&event);
		}
	}
} // namespace KeyInput

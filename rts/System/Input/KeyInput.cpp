/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <functional>
#include <cassert>
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

namespace KeyInput {
	static       std::vector<Key> keyVec;
	static const std::function<bool(const Key&, const Key&)> keyCmp = [](const Key& a, const Key& b) { return (a.first < b.first); };

	static SDL_Keymod keyMods;


	bool IsKeyPressed(int keyCode) {
		const auto& pred = keyCmp;
		const auto  iter = std::lower_bound(keyVec.begin(), keyVec.end(), Key{keyCode, false}, pred);

		return (iter != keyVec.end() && iter->first == keyCode && iter->second);
	}

	void SetKeyPressed(int keyCode, bool isPressed) {
		const auto& pred = keyCmp;
		const auto  iter = std::lower_bound(keyVec.begin(), keyVec.end(), Key{keyCode, false}, pred);

		// not reachable for default modifiers
		if (iter == keyVec.end())
			return;

		iter->second = isPressed;
	}

	void SetKeyModState(int mod, bool isPressed) {
		if (isPressed) {
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
		const uint8_t* kbState = SDL_GetKeyboardState(&numKeys);

		keyMods = SDL_GetModState();

		keyVec.clear();
		keyVec.reserve(numKeys);

		for (int i = 0; i < numKeys; ++i) {
			const auto scanCode = (SDL_Scancode)i;
			const auto keyCode  = SDL_GetKeyFromScancode(scanCode);

			keyVec.emplace_back(keyCode, kbState[scanCode] != 0);
		}

		std::sort(keyVec.begin(), keyVec.end(), keyCmp);

		SetKeyModState(KMOD_GUI, IsKeyPressed(fakeMetaKey));
		SetKeyPressed(SDLK_LALT  , GetKeyModState(KMOD_ALT  ));
		SetKeyPressed(SDLK_LCTRL , GetKeyModState(KMOD_CTRL ));
		SetKeyPressed(SDLK_LGUI  , GetKeyModState(KMOD_GUI  ));
		SetKeyPressed(SDLK_LSHIFT, GetKeyModState(KMOD_SHIFT));
	}

	const std::vector<Key>& GetPressedKeys()
	{
		return keyVec;
	}

	int GetNormalizedKeySymbol(int sym)
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

	void ReleaseAllKeys()
	{
		for (const auto& key: keyVec) {
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

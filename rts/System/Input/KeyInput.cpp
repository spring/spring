#include <cstring>
#include <SDL_keyboard.h>
#include <SDL_keysym.h>

#include "KeyInput.h"

KeyInput* keyInput = NULL;

KeyInput* KeyInput::GetInstance() {
	if (keyInput == NULL) {
		keyInput = new KeyInput();
	}

	return keyInput;
}

void KeyInput::FreeInstance(KeyInput* keyInp) {
	delete keyInp; keyInput = NULL;
}



KeyInput::KeyInput() {
	keys.resize(SDLK_LAST, 0);

	// Initialize keyboard
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_SetModState(KMOD_NONE);
}

/**
 * Tests SDL keystates and sets values in key array
 */
void KeyInput::Update(boost::uint16_t currKeyUnicodeChar, boost::int8_t fakeMetaKey)
{
	int numKeys = 0;

	const SDLMod keyMods = SDL_GetModState();
	const boost::uint8_t* keyStates = SDL_GetKeyState(&numKeys);

	memcpy(&keys[0], keyStates, sizeof(boost::uint8_t) * numKeys);

	keys[SDLK_LALT]   = (keyMods & KMOD_ALT)   ? 1 : 0;
	keys[SDLK_LCTRL]  = (keyMods & KMOD_CTRL)  ? 1 : 0;
	keys[SDLK_LMETA]  = (keyMods & KMOD_META)  ? 1 : 0;
	keys[SDLK_LSHIFT] = (keyMods & KMOD_SHIFT) ? 1 : 0;

	if (fakeMetaKey >= 0) {
		keys[SDLK_LMETA] |= keys[fakeMetaKey];
	}

	currentKeyUnicodeChar = currKeyUnicodeChar;
}

boost::uint16_t KeyInput::GetNormalizedKeySymbol(boost::uint16_t sym) const {

	if (sym <= SDLK_DELETE) {
		sym = tolower(sym);
	}
	else if (sym == SDLK_RSHIFT) { sym = SDLK_LSHIFT; }
	else if (sym == SDLK_RCTRL)  { sym = SDLK_LCTRL;  }
	else if (sym == SDLK_RMETA)  { sym = SDLK_LMETA;  }
	else if (sym == SDLK_RALT)   { sym = SDLK_LALT;   }

	return sym;
}

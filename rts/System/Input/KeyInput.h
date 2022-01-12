/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYBOARD_INPUT_H
#define KEYBOARD_INPUT_H

#include <vector>

namespace KeyInput {
	void Update(int currKeycode, int fakeMetaKey);
	void ReleaseAllKeys();

	bool IsKeyPressed(int idx);
	void SetKeyModState(int mod, bool pressed);
	bool GetKeyModState(int mod);

	typedef std::pair<int, bool> Key;

	const std::vector<Key>& GetPressedKeys();

	int GetNormalizedKeySymbol(int key);
}

#endif

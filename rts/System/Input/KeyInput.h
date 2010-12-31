/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYBOARD_INPUT_H
#define KEYBOARD_INPUT_H

#include <boost/cstdint.hpp>
#include <vector>

class KeyInput {
public:
	static KeyInput* GetInstance();
	static void FreeInstance(KeyInput*);

	KeyInput();
	~KeyInput() { keys.clear(); }

	void Update(boost::uint16_t currKeyUnicodeChar, boost::int8_t fakeMetaKey);

	void SetKeyState(boost::uint16_t idx, boost::uint8_t state) { keys[idx] = state; }
	boost::uint8_t GetKeyState(boost::uint16_t idx) const { return keys[idx]; }

	boost::uint16_t GetNormalizedKeySymbol(boost::uint16_t sym) const;
	boost::uint16_t GetCurrentKeyUnicodeChar() const { return currentKeyUnicodeChar; }

	bool IsKeyPressed(boost::uint16_t idx) const { return (keys[idx] != 0); }

private:
	/**
	* @brief keys
	*
	* Array of possible keys, and which are being pressed
	*/
	std::vector<boost::uint8_t> keys;

	/**
	* @brief currentKeyUnicodeChar
	*
	* Unicode character for the current KeyPressed or KeyReleased
	*/
	boost::uint16_t currentKeyUnicodeChar;
};

extern KeyInput* keyInput;

#endif

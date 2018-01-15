/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAMEINFO_H
#define GAMEINFO_H

#include <vector>
#include <string>

#include "InputReceiver.h"

class CGameInfo : public CInputReceiver
{
public:
	static void Enable();
	static void Disable();
	static bool IsActive();

	struct FontString {
		FontString(): msg(""), width(0.0f), height(0.0f) {}

		FontString(const char* c);
		FontString(const std::string& s);
		FontString(bool b);
		FontString(float f);

		void CalcDimensions();

		std::string msg;

		float width;
		float height;
	};

private:
	CGameInfo();
	~CGameInfo();

	bool MousePress(int x, int y, int button);
	void MouseRelease(int x, int y, int button);
	bool KeyPressed(int key, bool isRepeat);
	bool IsAbove(int x, int y);
	std::string GetTooltip(int x,int y);
	void Draw();

private:
	TRectangle<float> box;

	std::vector<FontString> labels;
	std::vector<FontString> values;
};

#endif /* GAMEINFO_H */

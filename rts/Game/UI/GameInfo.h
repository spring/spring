/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

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

	bool MousePress(int x, int y, int button) override;
	void MouseRelease(int x, int y, int button) override;
	bool KeyPressed(int key, int scanCode, bool isRepeat) override;
	bool IsAbove(int x, int y) override;
	std::string GetTooltip(int x,int y) override;
	void Draw() override;
private:
	TRectangle<float> box;

	std::vector<FontString> labels;
	std::vector<FontString> values;
};
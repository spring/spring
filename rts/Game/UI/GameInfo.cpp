#include "StdAfx.h"
#include "GameInfo.h"
#include <string>
#include <vector>

#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/Wind.h"
#include "Sim/ModInfo.h"

using namespace std;


CGameInfo* CGameInfo::instance = NULL;


void CGameInfo::Enable()
{
	if (instance == NULL) {
		instance = new CGameInfo;
	}
}


void CGameInfo::Disable()
{
	delete instance;
}


bool CGameInfo::IsActive()
{
	return (instance != NULL);
}


CGameInfo::CGameInfo(void)
{
	box.x1=0.5;
	box.y1=0.5;
	box.x2=0.5;
	box.y2=0.5;
}

CGameInfo::~CGameInfo(void)
{
	instance = NULL;
	if (mouse->activeReceiver == this) {
		mouse->activeReceiver = NULL;
	}
}


static const char* boolString(bool value)
{
	if (value) {
		return "True";
	} else {
		return "False";
	}
}

static const char* floatString(float value)
{
	static char buf[16];
	sprintf(buf, "%8.3f", value);
	return buf;
}


struct FontString {
	FontString() : msg(""), width(0.0f), height(0.0f) {}
	FontString(const char* c)	  : msg(c)      { CalcDimensions(); }
	FontString(const string& s)	: msg(s)      { CalcDimensions(); }
	FontString(bool b)	: msg(boolString(b))  { CalcDimensions(); }
	FontString(float f)	: msg(floatString(f)) { CalcDimensions(); }
	void CalcDimensions() {
		width = font->CalcTextWidth(msg.c_str());
		height = font->CalcTextHeight(msg.c_str());
	}
	string msg;
	float width;
	float height;
};


static void StringListStats(const vector<FontString>& list,
                            float& maxWidth, float& maxHeight)
{
	maxWidth = 0.0f;
	maxHeight = 0.0f;
	for (int i = 0; i < (int)list.size(); i++) {
		const FontString& fs = list[i];
		if (fs.width > maxWidth) {
			maxWidth = fs.width;
		}
		if (fs.height > maxHeight) {
			maxHeight = fs.height;
		}
	}
}


/******************************************************************************/

std::string CGameInfo::GetTooltip(int x,int y)
{
	return "Game Information";
}


bool CGameInfo::IsAbove(int x, int y)
{
	float mx = float(x) / gu->screenx;
	float my = (gu->screeny - float(y)) / gu->screeny;
	return InBox(mx, my, box);
}


bool CGameInfo::MousePress(int x, int y, int button)
{
	if (IsAbove(x, y)) {
		return true;
	}
	return false;
}


void CGameInfo::MouseRelease(int x, int y, int button)
{
	if (mouse->activeReceiver == this) {
		if (IsAbove(x, y)) {
			delete this;
		}
	}
}


bool CGameInfo::KeyPressed(unsigned short key)
{
	if (key == 27) { // escape
		delete this;
		return true;
	}
	return false;
}


void CGameInfo::Draw()
{
	vector<FontString> labels;
	vector<FontString> values;
	char buf[256];
	
	if (gameSetup && gameSetup->hostDemo) {
		labels.push_back("Playback:");
		values.push_back(gameSetup->demoName);
	}
	
	labels.push_back("Commander Ends:");
	values.push_back(gs->gameMode > 0 ? true : false);

	labels.push_back("Game Speed:");
	values.push_back(gs->speedFactor);

	labels.push_back("Gravity:");
	values.push_back(gs->gravity);

	labels.push_back("Tidal:");
	values.push_back(readmap->tidalStrength);

	labels.push_back("Min Wind:");
	values.push_back(wind.minWind);

	labels.push_back("Max Wind:");
	values.push_back(wind.maxWind);

	if (gameSetup) {
		labels.push_back("Limited DGun:");
		values.push_back(gameSetup->limitDgun);

		labels.push_back("Diminishing Metal:");
		values.push_back(gameSetup->diminishingMMs);
	}

	labels.push_back("Map Size:");
	sprintf(buf, "%ix%i", readmap->width / 64, readmap->height / 64);
	values.push_back(buf);
	
	labels.push_back("Map Name:");
	values.push_back(readmap->mapName.c_str());

	labels.push_back("Mod Name:");
	values.push_back(modInfo->name.c_str());

	if (gs->cheatEnabled) {
		labels.push_back("CHEATS:");
		values.push_back("ENABLED");
	}

	// in pixels	
	const float split = 10.0f;
	const float border = 5.0f;
	const float xFontScale = 32.0f; // should not have to have these...
	const float yFontScale = 32.0f;
	float labelsWidth, labelsHeight;
	float valuesWidth, valuesHeight;
	StringListStats(labels, labelsWidth, labelsHeight);
	StringListStats(values, valuesWidth, valuesHeight);
	const float width = ((xFontScale * (labelsWidth + valuesWidth)) + split + (2.0f * border));
	const float rowHeight = (yFontScale * std::max(labelsHeight, valuesHeight)) + (2.0f * border);
	const float height = rowHeight * (float)(labels.size());

	// in screen fractions
	const float sx = (float)gu->screenx;
	const float sy = (float)gu->screeny;
	const float dy = (height / sy) / (float)labels.size();
	const float xBorder = border / sx;
	const float yBorder = border / sy;
	
	box.x1 = 0.5f - (width / (2.0f * sx)); 
	box.x2 = 0.5f + (width / (2.0f * sx)); 
	box.y1 = 0.5f - (height / (2.0f * sy)); 
	box.y2 = 0.5f + (height / (2.0f * sy)); 

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// draw the boxes
	int i;
	ContainerBox b;
	b.x1 = box.x1;
	b.x2 = box.x2;
	for (i = 0; i < (int)labels.size(); i++) {
		const float colors[][4]  = {
			{ 0.6f, 0.3f, 0.3f, 0.8f }, // red
			{ 0.3f, 0.6f, 0.3f, 0.8f }, // green
			{ 0.3f, 0.3f, 0.6f, 0.8f }  // blue
		};
		glColor4fv(colors[i % 3]);
		b.y2 = box.y2 - (dy * (float)i);
		b.y1 = b.y2 - dy;
		DrawBox(b);
	}
	
	glEnable(GL_TEXTURE_2D);

	if ((mouse->activeReceiver == this) && IsAbove(mouse->lastx, mouse->lasty)) {
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	} else {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	
	
	// draw the strings
	for (i = 0; i < (int)labels.size(); i++) {
		const FontString& lfs = labels[i];
		const FontString& vfs = values[i];
		const float y = box.y2 - (dy * (float)(i + 1)) + yBorder;
		font->glPrintAt   (box.x1 + xBorder, y, 1.0f, lfs.msg.c_str());
		font->glPrintRight(box.x2 - xBorder, y, 1.0f, vfs.msg.c_str());
	}
}

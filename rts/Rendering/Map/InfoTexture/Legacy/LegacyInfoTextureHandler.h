/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LEGACY_INFO_TEXTURE_HANDLER_H
#define _LEGACY_INFO_TEXTURE_HANDLER_H

#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Map/InfoTexture/InfoTexture.h"
#include "Rendering/GL/PBO.h"
#include <vector>



class CLegacyInfoTextureHandler : public IInfoTextureHandler
{
public:
	enum {
		COLOR_R = 2,
		COLOR_G = 1,
		COLOR_B = 0,
		COLOR_A = 3,
	};
	enum BaseGroundDrawMode {
		drawNormal   = 0,
		drawLos      = 1, // L
		drawMetal    = 2, // F4
		drawHeight   = 3, // F1
		drawPathTrav = 4, // F2
		drawPathHeat = 5, // not hotkeyed, command-only
		drawPathFlow = 6, // not hotkeyed, command-only
		drawPathCost = 7, // not hotkeyed, command-only
		NUM_INFOTEXTURES = 8,
	};

public:
	CLegacyInfoTextureHandler();
	virtual ~CLegacyInfoTextureHandler();

	void Update();
	bool UpdateExtraTexture(BaseGroundDrawMode texDrawMode);

public:
	bool IsEnabled() const;
	void DisableCurrentMode();
	void SetMode(const std::string& name);
	void ToggleMode(const std::string& name);
	const std::string& GetMode() const;

	GLuint GetCurrentInfoTexture() const;
	int2 GetCurrentInfoTextureSize() const;

public:
	CInfoTexture* GetInfoTexture(const std::string& name);

protected:
	BaseGroundDrawMode drawMode;

	int updateTextureState;
	int extraTextureUpdateRate;

	// note: first texture ID is always 0!
	GLuint infoTextureIDs[NUM_INFOTEXTURES];
	std::vector<CInfoTexture> infoTextures;

	PBO infoTexPBO;

	bool returnToLOS;
	bool highResLosTex;
	bool highResInfoTexWanted;
};

#endif // _LEGACY_INFO_TEXTURE_HANDLER_H

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _METAL_TEXTURE_H
#define _METAL_TEXTURE_H

#include "PboInfoTexture.h"
#include "System/EventHandler.h"


class CMetalTexture : public CPboInfoTexture, public CEventClient
{
public:
	CMetalTexture();

public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "MetalMapChanged");
	}
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }

	void MetalMapChanged(const int x, const int z) override { metalMapChanged = true; }

public:
	// IInfoTexture interface
	void Update() override;
	bool IsUpdateNeeded() override { return metalMapChanged; }

private:
	bool metalMapChanged;
};

#endif // _METAL_TEXTURE_H

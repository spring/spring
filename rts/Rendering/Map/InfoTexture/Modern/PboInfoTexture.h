/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PBO_INFO_TEXTURE_H
#define _PBO_INFO_TEXTURE_H

#include "Rendering/Map/InfoTexture/InfoTexture.h"
#include "Rendering/GL/PBO.h"



class CPboInfoTexture : public CInfoTexture
{
public:
	CPboInfoTexture(const std::string& _name) {
		name        = _name;
		texChannels = 0;
		texture     = 0;
	}
	CPboInfoTexture(const CPboInfoTexture&) = delete; // no-copy

	virtual ~CPboInfoTexture();

public:
	virtual void Update() = 0;
	virtual bool IsUpdateNeeded() = 0;

protected:
	PBO infoTexPBO;
};

#endif // _PBO_INFO_TEXTURE_H

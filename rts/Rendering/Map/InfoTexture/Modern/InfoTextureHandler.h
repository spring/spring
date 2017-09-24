/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _INFO_TEXTURE_HANDLER_H
#define _INFO_TEXTURE_HANDLER_H

#include <string>

#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"


class CPboInfoTexture;
class CInfoTextureCombiner;


class CInfoTextureHandler : public IInfoTextureHandler
{
public:
	CInfoTextureHandler();
	virtual ~CInfoTextureHandler();

	void Update() override;

public:
	bool IsEnabled() const override;
	bool InMetalMode() const override { return inMetalMode; }

	void DisableCurrentMode() override;
	void SetMode(const std::string& name) override;
	void ToggleMode(const std::string& name) override;
	const std::string& GetMode() const override;

	GLuint GetCurrentInfoTexture() const override;
	int2 GetCurrentInfoTextureSize() const override;

public:
	const CInfoTexture* GetInfoTextureConst(const std::string& name) const override;
	      CInfoTexture* GetInfoTexture     (const std::string& name)       override;

protected:
	friend class CPboInfoTexture;
	void AddInfoTexture(CPboInfoTexture*);

protected:
	bool returnToLOS = false;
	bool inMetalMode = false;
	bool firstUpdate =  true;

	spring::unordered_map<std::string, CPboInfoTexture*> infoTextures;

	// special; always non-NULL at runtime
	CInfoTextureCombiner* infoTex = nullptr;
};

#endif // _INFO_TEXTURE_HANDLER_H

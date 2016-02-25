/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _INFO_TEXTURE_HANDLER_H
#define _INFO_TEXTURE_HANDLER_H


#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "System/type2.h"
#include <string>
#include <unordered_map>


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
	bool returnToLOS;
	std::unordered_map<std::string, CPboInfoTexture*> infoTextures;
	CInfoTextureCombiner* infoTex;
};

#endif // _INFO_TEXTURE_HANDLER_H

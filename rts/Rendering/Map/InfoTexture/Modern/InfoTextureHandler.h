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

	void Update();

public:
	bool IsEnabled() const;
	void DisableCurrentMode();
	void SetMode(const std::string& name);
	void ToggleMode(const std::string& name);
	const std::string& GetMode() const;

	GLuint GetCurrentInfoTexture() const;
	int2 GetCurrentInfoTextureSize() const;

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

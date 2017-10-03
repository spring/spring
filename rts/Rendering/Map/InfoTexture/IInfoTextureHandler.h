/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _INTERFACE_INFO_TEXTURE_HANDLER_H
#define _INTERFACE_INFO_TEXTURE_HANDLER_H

#include "Rendering/GL/myGL.h"
#include "System/type2.h"
#include <string>


class CInfoTexture;


class IInfoTextureHandler {
public:
	static void Create();

public:
	IInfoTextureHandler() {}
	IInfoTextureHandler(const IInfoTextureHandler&) = delete; // no-copy
	virtual ~IInfoTextureHandler() {}

	virtual void Update() = 0;

public:
	virtual bool IsEnabled() const = 0;
	virtual bool InMetalMode() const = 0;

	virtual void DisableCurrentMode() = 0;
	virtual void SetMode(const std::string& name) = 0;
	virtual void ToggleMode(const std::string& name) = 0;
	virtual const std::string& GetMode() const = 0;

	virtual GLuint GetCurrentInfoTexture() const = 0;
	virtual int2   GetCurrentInfoTextureSize() const = 0;

public:
	virtual const CInfoTexture* GetInfoTextureConst(const std::string& name) const = 0;
	virtual       CInfoTexture* GetInfoTexture     (const std::string& name)       = 0;
};

extern IInfoTextureHandler* infoTextureHandler;

#endif // _INTERFACE_INFO_TEXTURE_HANDLER_H

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULL_INFO_TEXTURE_HANDLER_H
#define NULL_INFO_TEXTURE_HANDLER_H

#include "IInfoTextureHandler.h"

class NullInfoTextureHandler {
public:
	static void Create();

public:
	NullInfoTextureHandler() {}
	NullInfoTextureHandler(const NullInfoTextureHandler&) = delete;

	void Update() override {}

public:
	bool IsEnabled() const override { return false; }
	bool InMetalMode() const override { return false; }

	void DisableCurrentMode() override {}
	void SetMode(const std::string& name) override {}
	void ToggleMode(const std::string& name) override {}
	const std::string& GetMode() const override { return ""; }

	GLuint GetCurrentInfoTexture() const override { return 0; }
	int2   GetCurrentInfoTextureSize() const override { return {0, 0}; }

public:
	const CInfoTexture* GetInfoTextureConst(const std::string& name) const override { return nullptr; }
	      CInfoTexture* GetInfoTexture     (const std::string& name)       override { return nullptr; }
};

#endif


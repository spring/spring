/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InfoTextureHandler.h"
#include "AirLos.h"
#include "Combiner.h"
#include "Height.h"
#include "Los.h"
#include "Metal.h"
#include "MetalExtraction.h"
#include "Path.h"
#include "Radar.h"

#include "System/TimeProfiler.h"



CInfoTextureHandler::CInfoTextureHandler()
: returnToLOS(false)
, infoTex(nullptr)
{
	if (!infoTextureHandler)
		infoTextureHandler = this;

	AddInfoTexture(new CInfoTextureCombiner());
	AddInfoTexture(new CLosTexture());
	AddInfoTexture(new CAirLosTexture());
	AddInfoTexture(new CMetalTexture());
	AddInfoTexture(new CMetalExtractionTexture());
	AddInfoTexture(new CRadarTexture());
	AddInfoTexture(new CHeightTexture());
	AddInfoTexture(new CPathTexture());

	infoTex = dynamic_cast<CInfoTextureCombiner*>(GetInfoTexture("info"));
	assert(infoTex);

	Update();
}


CInfoTextureHandler::~CInfoTextureHandler()
{
	for (auto& pitex: infoTextures) {
		delete pitex.second;
	}
	infoTextureHandler = nullptr;
}


void CInfoTextureHandler::AddInfoTexture(CPboInfoTexture* itex)
{
	infoTextures[itex->GetName()] = itex;
}


const CInfoTexture* CInfoTextureHandler::GetInfoTextureConst(const std::string& name) const
{
	static const CDummyInfoTexture dummy;

	const auto it = infoTextures.find(name);

	if (it != infoTextures.end())
		return it->second;

	return &dummy;
}

CInfoTexture* CInfoTextureHandler::GetInfoTexture(const std::string& name)
{
	return (const_cast<CInfoTexture*>(GetInfoTextureConst(name)));
}


bool CInfoTextureHandler::IsEnabled() const
{
	return infoTex->IsEnabled();
}


void CInfoTextureHandler::DisableCurrentMode()
{
	if (returnToLOS && (infoTex->GetMode() != "los")) {
		// return to LOS-mode if it was active before
		infoTex->SwitchMode("los");
	} else {
		returnToLOS = false;
		infoTex->SwitchMode("");
	}
}


void CInfoTextureHandler::SetMode(const std::string& name)
{
	if (name == "los") {
		returnToLOS = true;
	}

	infoTex->SwitchMode(name);
}


void CInfoTextureHandler::ToggleMode(const std::string& name)
{
	if (infoTex->GetMode() == name)
		return DisableCurrentMode();

	SetMode(name);
}


const std::string& CInfoTextureHandler::GetMode() const
{
	return infoTex->GetMode();
}


GLuint CInfoTextureHandler::GetCurrentInfoTexture() const
{
	return infoTex->GetTexture();
}


int2 CInfoTextureHandler::GetCurrentInfoTextureSize() const
{
	return infoTex->GetTexSize();
}


void CInfoTextureHandler::Update()
{
	for (auto& p: infoTextures) {
		if (p.second->IsUpdateNeeded())
			p.second->Update();
	}
}

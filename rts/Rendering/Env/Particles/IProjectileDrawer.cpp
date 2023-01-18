/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IProjectileDrawer.h"

#include "Game/LoadScreen.h"
#include "Rendering/Env/Particles/NullProjectileDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/EventHandler.h"
#include "System/SafeUtil.h"


IProjectileDrawer* projectileDrawer = nullptr;


IProjectileDrawer::IProjectileDrawer():
	CEventClient("[IProjectileDrawer]", 123456, false) {
}

void IProjectileDrawer::InitStatic() {
	if (projectileDrawer == nullptr) {
		projectileDrawer = new CProjectileDrawer();
	}

	projectileDrawer->Init();
}

void IProjectileDrawer::KillStatic(bool reload) {
	projectileDrawer->Kill();

	if (reload)
		return;

	spring::SafeDestruct(projectileDrawer);
}

void IProjectileDrawer::Init() {
	eventHandler.AddClient(this);

	loadscreen->SetLoadMessage("Creating Projectile Textures");

	textureAtlas = new CTextureAtlas(); textureAtlas->SetName("ProjectileTextureAtlas");
	groundFXAtlas = new CTextureAtlas(); groundFXAtlas->SetName("ProjectileEffectsAtlas");
}

void IProjectileDrawer::Kill() {
	eventHandler.RemoveClient(this);
	autoLinkedEvents.clear();

	spring::SafeDelete(textureAtlas);
	spring::SafeDelete(groundFXAtlas);
}

bool IProjectileDrawer::WantsEvent(const std::string& eventName) {
	return (eventName == "RenderProjectileCreated" || eventName == "RenderProjectileDestroyed");
}

bool IProjectileDrawer::GetFullRead() const {
	return true;
}

int IProjectileDrawer::GetReadAllyTeam() const {
	return AllAccessTeam;
}

bool IProjectileDrawer::EnableSorting(bool b) {
	return (drawSorted = b);
}

bool IProjectileDrawer::ToggleSorting() {
	return (drawSorted = !drawSorted);
}

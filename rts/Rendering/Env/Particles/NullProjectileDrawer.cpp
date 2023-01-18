// SPDX-License-Identifier: GPL-2.0-or-later

#include "NullProjectileDrawer.h"

#include "Rendering/Textures/TextureAtlas.h"
#include "System/SafeUtil.h"

void CNullProjectileDrawer::Init() {
	IProjectileDrawer::Init();

	dummyTexture = new AtlasedTexture();

	flaretex = dummyTexture;
	dguntex = dummyTexture;
	flareprojectiletex = dummyTexture;
	sbtrailtex = dummyTexture;
	missiletrailtex = dummyTexture;
	muzzleflametex = dummyTexture;
	repulsetex = dummyTexture;
	sbflaretex = dummyTexture;
	missileflaretex = dummyTexture;
	beamlaserflaretex = dummyTexture;
	explotex = dummyTexture;
	explofadetex = dummyTexture;
	heatcloudtex = dummyTexture;
	circularthingytex = dummyTexture;
	bubbletex = dummyTexture;
	geosquaretex = dummyTexture;
	gfxtex = dummyTexture;
	projectiletex = dummyTexture;
	repulsegfxtex = dummyTexture;
	sphereparttex = dummyTexture;
	torpedotex = dummyTexture;
	wrecktex = dummyTexture;
	plasmatex = dummyTexture;
	laserendtex = dummyTexture;
	laserfallofftex = dummyTexture;
	randdotstex = dummyTexture;
	smoketrailtex = dummyTexture;
	waketex = dummyTexture;
	perlintex = dummyTexture;
	flametex = dummyTexture;

	groundflashtex = dummyTexture;
	groundringtex = dummyTexture;

	seismictex = dummyTexture;
}

void CNullProjectileDrawer::Kill() {
	IProjectileDrawer::Kill();
	spring::SafeDelete(dummyTexture);
}

unsigned int CNullProjectileDrawer::NumSmokeTextures() const {
	return 1;
}

const AtlasedTexture* CNullProjectileDrawer::GetSmokeTexture(unsigned int i) const {
	return dummyTexture;
}

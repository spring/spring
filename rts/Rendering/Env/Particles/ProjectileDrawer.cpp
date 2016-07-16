/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ProjectileDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Rendering/Env/Particles/Classes/FlyingPiece.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include <array>



CProjectileDrawer* projectileDrawer = NULL;

CProjectileDrawer::CProjectileDrawer(): CEventClient("[CProjectileDrawer]", 123456, false) {
	eventHandler.AddClient(this);

	loadscreen->SetLoadMessage("Creating Projectile Textures");

	textureAtlas = new CTextureAtlas(); textureAtlas->SetName("ProjectileTextureAtlas");
	groundFXAtlas = new CTextureAtlas(); groundFXAtlas->SetName("ProjectileEffectsAtlas");

	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	LuaParser mapResParser("gamedata/resources_map.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);

	resourcesParser.Execute();

	const LuaTable& resTable = resourcesParser.GetRoot();
	const LuaTable& resGraphicsTable = resTable.SubTable("graphics");
	const LuaTable& resProjTexturesTable = resGraphicsTable.SubTable("projectileTextures");
	const LuaTable& resSmokeTexturesTable = resGraphicsTable.SubTable("smoke");
	const LuaTable& resGroundFXTexturesTable = resGraphicsTable.SubTable("groundfx");

	// used to block resources_map.* from overriding any of
	// resources.lua:{projectile, smoke, groundfx}textures,
	// as well as various defaults (repulsegfxtexture, etc)
	std::set<std::string> blockedTexNames;

	ParseAtlasTextures(true, resProjTexturesTable, blockedTexNames, textureAtlas);
	ParseAtlasTextures(true, resGroundFXTexturesTable, blockedTexNames, groundFXAtlas);

	int smokeTexCount = -1;

	{
		// get the smoke textures, hold the count in 'smokeTexCount'
		if (resSmokeTexturesTable.IsValid()) {
			for (smokeTexCount = 0; true; smokeTexCount++) {
				const std::string& tex = resSmokeTexturesTable.GetString(smokeTexCount + 1, "");
				if (tex.empty()) {
					break;
				}
				const std::string texName = "bitmaps/" + tex;
				const std::string smokeName = "ismoke" + IntToString(smokeTexCount, "%02i");

				textureAtlas->AddTexFromFile(smokeName, texName);
				blockedTexNames.insert(StringToLower(smokeName));
			}
		} else {
			// setup the defaults
			for (smokeTexCount = 0; smokeTexCount < 12; smokeTexCount++) {
				const std::string smokeNum = IntToString(smokeTexCount, "%02i");
				const std::string smokeName = "ismoke" + smokeNum;
				const std::string texName = "bitmaps/smoke/smoke" + smokeNum + ".tga";

				textureAtlas->AddTexFromFile(smokeName, texName);
				blockedTexNames.insert(StringToLower(smokeName));
			}
		}

		if (smokeTexCount <= 0) {
			// this needs to be an exception, other code
			// assumes at least one smoke-texture exists
			throw content_error("missing smoke textures");
		}
	}

	{
		// shield-texture memory
		std::array<char, 4 * perlinTexSize * perlinTexSize> perlinTexMem;
		perlinTexMem.fill(70);
		textureAtlas->AddTexFromMem("perlintex", perlinTexSize, perlinTexSize, CTextureAtlas::RGBA32, &perlinTexMem[0]);
		blockedTexNames.insert("perlintex");
	}

	blockedTexNames.insert("flare");
	blockedTexNames.insert("explo");
	blockedTexNames.insert("explofade");
	blockedTexNames.insert("heatcloud");
	blockedTexNames.insert("laserend");
	blockedTexNames.insert("laserfalloff");
	blockedTexNames.insert("randdots");
	blockedTexNames.insert("smoketrail");
	blockedTexNames.insert("wake");
	blockedTexNames.insert("flame");

	blockedTexNames.insert("sbtrailtexture");
	blockedTexNames.insert("missiletrailtexture");
	blockedTexNames.insert("muzzleflametexture");
	blockedTexNames.insert("repulsetexture");
	blockedTexNames.insert("dguntexture");
	blockedTexNames.insert("flareprojectiletexture");
	blockedTexNames.insert("sbflaretexture");
	blockedTexNames.insert("missileflaretexture");
	blockedTexNames.insert("beamlaserflaretexture");
	blockedTexNames.insert("bubbletexture");
	blockedTexNames.insert("geosquaretexture");
	blockedTexNames.insert("gfxtexture");
	blockedTexNames.insert("projectiletexture");
	blockedTexNames.insert("repulsegfxtexture");
	blockedTexNames.insert("sphereparttexture");
	blockedTexNames.insert("torpedotexture");
	blockedTexNames.insert("wrecktexture");
	blockedTexNames.insert("plasmatexture");

	if (mapResParser.Execute()) {
		// allow map-specified atlas textures (for gaia-projectiles and ground-flashes)
		const LuaTable& mapResTable = mapResParser.GetRoot();
		const LuaTable& mapResGraphicsTable = mapResTable.SubTable("graphics");
		const LuaTable& mapResProjTexturesTable = mapResGraphicsTable.SubTable("projectileTextures");
		const LuaTable& mapResGroundFXTexturesTable = mapResGraphicsTable.SubTable("groundfx");

		ParseAtlasTextures(false, mapResProjTexturesTable, blockedTexNames, textureAtlas);
		ParseAtlasTextures(false, mapResGroundFXTexturesTable, blockedTexNames, groundFXAtlas);
	}

	if (!textureAtlas->Finalize()) {
		LOG_L(L_ERROR, "Could not finalize projectile-texture atlas. Use less/smaller textures.");
	}

	flaretex        = &textureAtlas->GetTexture("flare");
	explotex        = &textureAtlas->GetTexture("explo");
	explofadetex    = &textureAtlas->GetTexture("explofade");
	heatcloudtex    = &textureAtlas->GetTexture("heatcloud");
	laserendtex     = &textureAtlas->GetTexture("laserend");
	laserfallofftex = &textureAtlas->GetTexture("laserfalloff");
	randdotstex     = &textureAtlas->GetTexture("randdots");
	smoketrailtex   = &textureAtlas->GetTexture("smoketrail");
	waketex         = &textureAtlas->GetTexture("wake");
	perlintex       = &textureAtlas->GetTexture("perlintex");
	flametex        = &textureAtlas->GetTexture("flame");

	for (int i = 0; i < smokeTexCount; i++) {
		const std::string smokeName = "ismoke" + IntToString(i, "%02i");
		const AtlasedTexture* smokeTex = &textureAtlas->GetTexture(smokeName);
		smoketex.push_back(smokeTex);
	}

#define GETTEX(t, b) (&textureAtlas->GetTextureWithBackup((t), (b)))
	sbtrailtex         = GETTEX("sbtrailtexture",         "smoketrail"    );
	missiletrailtex    = GETTEX("missiletrailtexture",    "smoketrail"    );
	muzzleflametex     = GETTEX("muzzleflametexture",     "explo"         );
	repulsetex         = GETTEX("repulsetexture",         "explo"         );
	dguntex            = GETTEX("dguntexture",            "flare"         );
	flareprojectiletex = GETTEX("flareprojectiletexture", "flare"         );
	sbflaretex         = GETTEX("sbflaretexture",         "flare"         );
	missileflaretex    = GETTEX("missileflaretexture",    "flare"         );
	beamlaserflaretex  = GETTEX("beamlaserflaretexture",  "flare"         );
	bubbletex          = GETTEX("bubbletexture",          "circularthingy");
	geosquaretex       = GETTEX("geosquaretexture",       "circularthingy");
	gfxtex             = GETTEX("gfxtexture",             "circularthingy");
	projectiletex      = GETTEX("projectiletexture",      "circularthingy");
	repulsegfxtex      = GETTEX("repulsegfxtexture",      "circularthingy");
	sphereparttex      = GETTEX("sphereparttexture",      "circularthingy");
	torpedotex         = GETTEX("torpedotexture",         "circularthingy");
	wrecktex           = GETTEX("wrecktexture",           "circularthingy");
	plasmatex          = GETTEX("plasmatexture",          "circularthingy");
#undef GETTEX


	if (!groundFXAtlas->Finalize()) {
		LOG_L(L_ERROR, "Could not finalize groundFX texture atlas. Use less/smaller textures.");
	}

	groundflashtex = &groundFXAtlas->GetTexture("groundflash");
	groundringtex = &groundFXAtlas->GetTexture("groundring");
	seismictex = &groundFXAtlas->GetTexture("seismic");

	for (int a = 0; a < 4; ++a) {
		perlinBlend[a] = 0.0f;
	}

	{
		glGenTextures(8, perlinBlendTex);
		for (int a = 0; a < 8; ++a) {
			glBindTexture(GL_TEXTURE_2D, perlinBlendTex[a]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, perlinBlendTexSize,perlinBlendTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
	}

	perlinTexObjects = 0;
	drawPerlinTex = false;

	if (perlinFB.IsValid()) {
		// we never refresh the full texture (just the perlin part). So we need to reload it then.
		perlinFB.reloadOnAltTab = true;

		perlinFB.Bind();
		perlinFB.AttachTexture(textureAtlas->GetTexID());
		drawPerlinTex = perlinFB.CheckStatus("PERLIN");
		perlinFB.Unbind();
	}


	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		modelRenderers[modelType] = IModelRenderContainer::GetInstance(modelType);
	}
}

CProjectileDrawer::~CProjectileDrawer() {
	eventHandler.RemoveClient(this);

	glDeleteTextures(8, perlinBlendTex);
	delete textureAtlas;
	delete groundFXAtlas;

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		delete modelRenderers[modelType];
	}

	renderProjectiles.clear();
}



void CProjectileDrawer::ParseAtlasTextures(
	const bool blockTextures,
	const LuaTable& textureTable,
	std::set<std::string>& blockedTextures,
	CTextureAtlas* textureAtlas)
{
	std::vector<std::string> subTables;
	std::map<std::string, std::string> texturesMap;
	std::map<std::string, std::string>::iterator texturesMapIt;

	textureTable.GetMap(texturesMap);
	textureTable.GetKeys(subTables);

	for (texturesMapIt = texturesMap.begin(); texturesMapIt != texturesMap.end(); ++texturesMapIt) {
		const std::string textureName = StringToLower(texturesMapIt->first);

		if (blockTextures) {
			// no textures added to this atlas are allowed
			// to be overwritten later by other textures of
			// the same name
			blockedTextures.insert(textureName);
		}
		if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end())) {
			textureAtlas->AddTexFromFile(texturesMapIt->first, "bitmaps/" + texturesMapIt->second);
		}
	}

	texturesMap.clear();

	for (size_t i = 0; i < subTables.size(); i++) {
		const LuaTable& textureSubTable = textureTable.SubTable(subTables[i]);

		if (textureSubTable.IsValid()) {
			textureSubTable.GetMap(texturesMap);

			for (texturesMapIt = texturesMap.begin(); texturesMapIt != texturesMap.end(); ++texturesMapIt) {
				const std::string textureName = StringToLower(texturesMapIt->first);

				if (blockTextures) {
					blockedTextures.insert(textureName);
				}
				if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end())) {
					textureAtlas->AddTexFromFile(texturesMapIt->first, "bitmaps/" + texturesMapIt->second);
				}
			}

			texturesMap.clear();
		}
	}
}

void CProjectileDrawer::LoadWeaponTextures() {
	// post-process the synced weapon-defs to set unsynced fields
	// (this requires CWeaponDefHandler to have been initialized)
	for (WeaponDef& wd: weaponDefHandler->weaponDefs) {
		wd.visuals.texture1 = NULL;
		wd.visuals.texture2 = NULL;
		wd.visuals.texture3 = NULL;
		wd.visuals.texture4 = NULL;

		if (wd.type == "Cannon") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "AircraftBomb") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "Shield") {
			wd.visuals.texture1 = perlintex;
		} else if (wd.type == "Flame") {
			wd.visuals.texture1 = flametex;

			if (wd.visuals.colorMap == nullptr) {
				wd.visuals.colorMap = CColorMap::LoadFromDefString(
					"1.0 1.0 1.0 0.1 "
					"0.025 0.025 0.025 0.10 "
					"0.0 0.0 0.0 0.0"
				);
			}
		} else if (wd.type == "MissileLauncher") {
			wd.visuals.texture1 = missileflaretex;
			wd.visuals.texture2 = missiletrailtex;
		} else if (wd.type == "TorpedoLauncher") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "LaserCannon") {
			wd.visuals.texture1 = laserfallofftex;
			wd.visuals.texture2 = laserendtex;
		} else if (wd.type == "BeamLaser") {
			if (wd.largeBeamLaser) {
				wd.visuals.texture1 = &textureAtlas->GetTexture("largebeam");
				wd.visuals.texture2 = laserendtex;
				wd.visuals.texture3 = &textureAtlas->GetTexture("muzzleside");
				wd.visuals.texture4 = beamlaserflaretex;
			} else {
				wd.visuals.texture1 = laserfallofftex;
				wd.visuals.texture2 = laserendtex;
				wd.visuals.texture3 = beamlaserflaretex;
			}
		} else if (wd.type == "LightningCannon") {
			wd.visuals.texture1 = laserfallofftex;
		} else if (wd.type == "EmgCannon") {
			wd.visuals.texture1 = plasmatex;
		} else if (wd.type == "StarburstLauncher") {
			wd.visuals.texture1 = sbflaretex;
			wd.visuals.texture2 = sbtrailtex;
			wd.visuals.texture3 = explotex;
		} else {
			wd.visuals.texture1 = plasmatex;
			wd.visuals.texture2 = plasmatex;
		}

		// override the textures if we have specified names for them
		if (wd.visuals.texNames[0] != "") { wd.visuals.texture1 = &textureAtlas->GetTexture(wd.visuals.texNames[0]); }
		if (wd.visuals.texNames[1] != "") { wd.visuals.texture2 = &textureAtlas->GetTexture(wd.visuals.texNames[1]); }
		if (wd.visuals.texNames[2] != "") { wd.visuals.texture3 = &textureAtlas->GetTexture(wd.visuals.texNames[2]); }
		if (wd.visuals.texNames[3] != "") { wd.visuals.texture4 = &textureAtlas->GetTexture(wd.visuals.texNames[3]); }

		if (!wd.visuals.ptrailExpGenTag.empty()) {
			// these can only be custom EG's so prefix is not required game-side
			wd.ptrailExplosionGeneratorID = explGenHandler->LoadGeneratorID(CEG_PREFIX_STRING + wd.visuals.ptrailExpGenTag);
		}

		if (!wd.visuals.impactExpGenTag.empty()) {
			wd.impactExplosionGeneratorID = explGenHandler->LoadGeneratorID(wd.visuals.impactExpGenTag);
		}

		if (!wd.visuals.bounceExpGenTag.empty()) {
			wd.bounceExplosionGeneratorID = explGenHandler->LoadGeneratorID(wd.visuals.bounceExpGenTag);
		}
	}
}



void CProjectileDrawer::DrawProjectiles(int modelType, bool drawReflection, bool drawRefraction)
{
	SCOPED_GMARKER("CProjectileDrawer::DrawProjectiles");

	auto& projectileBin = modelRenderers[modelType]->GetProjectileBinMutable();

	for (auto binIt = projectileBin.cbegin(); binIt != projectileBin.cend(); ++binIt) {
		CUnitDrawer::BindModelTypeTexture(modelType, binIt->first);
		DrawProjectilesSet(binIt->second, drawReflection, drawRefraction);
	}

	DrawFlyingPieces(modelType);
}

void CProjectileDrawer::DrawProjectilesSet(const std::vector<CProjectile*>& projectiles, bool drawReflection, bool drawRefraction)
{
	for (CProjectile* p: projectiles) {
		DrawProjectileNow(p, drawReflection, drawRefraction);
	}
}

bool CProjectileDrawer::CanDrawProjectile(const CProjectile* pro, const CSolidObject* owner)
{
	auto& th = teamHandler;
	auto& lh = losHandler;
	return (gu->spectatingFullView || (owner != nullptr && th->Ally(owner->allyteam, gu->myAllyTeam)) || lh->InLos(pro, gu->myAllyTeam));
}

void CProjectileDrawer::DrawProjectileNow(CProjectile* pro, bool drawReflection, bool drawRefraction)
{
	pro->drawPos = pro->GetDrawPos(globalRendering->timeOffset);

	if (!CanDrawProjectile(pro, pro->owner()))
		return;


	if (drawRefraction && (pro->drawPos.y > pro->GetDrawRadius()) /*!pro->IsInWater()*/)
		return;
	if (drawReflection && !CUnitDrawer::ObjectVisibleReflection(pro->drawPos, camera->GetPos(), pro->GetDrawRadius()))
		return;

	const CCamera* cam = CCamera::GetActiveCamera();
	if (!cam->InView(pro->drawPos, pro->GetDrawRadius()))
		return;

	DrawProjectileModel(pro);

	if (pro->drawSorted) {
		pro->SetSortDist(cam->ProjectedDistance(pro->pos));
		zSortedProjectiles.insert(pro);
	} else {
		unsortedProjectiles.push_back(pro);
	}
}



void CProjectileDrawer::DrawProjectilesShadow(int modelType)
{
	auto& projectileBin = modelRenderers[modelType]->GetProjectileBinMutable();

	for (auto binIt = projectileBin.cbegin(); binIt != projectileBin.cend(); ++binIt) {
		DrawProjectilesSetShadow(binIt->second);
	}

	DrawFlyingPieces(modelType);
}

void CProjectileDrawer::DrawProjectilesSetShadow(const std::vector<CProjectile*>& projectiles)
{
	for (CProjectile* p: projectiles) {
		DrawProjectileShadow(p);
	}
}

void CProjectileDrawer::DrawProjectileShadow(CProjectile* p)
{
	if (CanDrawProjectile(p, p->owner())) {
		const CCamera* cam = CCamera::GetActiveCamera();
		if (!cam->InView(p->drawPos, p->GetDrawRadius()))
			return;

		// if this returns false, then projectile is
		// neither weapon nor piece, or has no model
		if (DrawProjectileModel(p))
			return;

		if (!p->castShadow)
			return;

		// don't need to z-sort particle
		// effects for the shadow pass
		p->Draw();
	}
}



void CProjectileDrawer::DrawProjectilesMiniMap()
{
	CVertexArray* lines = GetVertexArray();
	CVertexArray* points = GetVertexArray();
	lines->Initialize();
	points->Initialize();

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		const auto& projectileBin = modelRenderers[modelType]->GetProjectileBin();

		if (projectileBin.empty())
			continue;

		for (auto binIt = projectileBin.cbegin(); binIt != projectileBin.cend(); ++binIt) {
			const auto& projectileSet = binIt->second;

			lines->EnlargeArrays(projectileSet.size() * 2, 0, VA_SIZE_C);
			points->EnlargeArrays(projectileSet.size(), 0, VA_SIZE_C);

			for (CProjectile* p: projectileSet) {
				if (!CanDrawProjectile(p, p->owner()))
					continue;

				p->DrawOnMinimap(*lines, *points);
			}
		}
	}

	lines->DrawArrayC(GL_LINES);
	points->DrawArrayC(GL_POINTS);

	if (!renderProjectiles.empty()) {
		lines->Initialize();
		lines->EnlargeArrays(renderProjectiles.size() * 2, 0, VA_SIZE_C);
		points->Initialize();
		points->EnlargeArrays(renderProjectiles.size(), 0, VA_SIZE_C);

		for (CProjectile* p: renderProjectiles) {
			if (!CanDrawProjectile(p, p->owner()))
				continue;

			p->DrawOnMinimap(*lines, *points);
		}

		lines->DrawArrayC(GL_LINES);
		points->DrawArrayC(GL_POINTS);
	}
}

void CProjectileDrawer::DrawFlyingPieces(int modelType)
{
	SCOPED_GMARKER("CProjectileDrawer::DrawFlyingPieces");

	const FlyingPieceContainer& container = projectileHandler->flyingPieces[modelType];

	if (container.empty())
		return;

	glPushAttrib(GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);

	const FlyingPiece* last = nullptr;

	for (const FlyingPiece& fp: container) {
		const bool noLosTst = gu->spectatingFullView || teamHandler->AlliedTeams(gu->myTeam, fp.GetTeam());
		const bool inAirLos = noLosTst || losHandler->InAirLos(fp.GetPos(), gu->myAllyTeam);

		if (!inAirLos)
			continue;

		if (!camera->InView(fp.GetPos(), fp.GetRadius()))
			continue;

		fp.Draw(last);
		last = &fp;
	}

	if (last != nullptr)
		last->EndDraw();

	glPopAttrib();
}


void CProjectileDrawer::Draw(bool drawReflection, bool drawRefraction) {
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);

	sky->SetupFog();

	zSortedProjectiles.clear();
	unsortedProjectiles.clear();

	Update();

	{
		unitDrawer->SetupOpaqueDrawing(false);

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			unitDrawer->PushModelRenderState(modelType);
			DrawProjectiles(modelType, drawReflection, drawRefraction);
			unitDrawer->PopModelRenderState(modelType);
		}

		unitDrawer->ResetOpaqueDrawing(false);

		// note: model-less projectiles are NOT drawn by this call but
		// only z-sorted (if the projectiles indicate they want to be)
		DrawProjectilesSet(renderProjectiles, drawReflection, drawRefraction);

		CProjectile::inArray = false;
		CProjectile::va = GetVertexArray();
		CProjectile::va->Initialize();

		// draw the particle effects
		for (CProjectile* p: zSortedProjectiles) {
			p->Draw();
		}
		for (CProjectile* p: unsortedProjectiles) {
			p->Draw(); //FIXME rename to explosionSpawners ? and why to call Draw() for them??
		}
	}

	glEnable(GL_BLEND);
	glDisable(GL_FOG);

	if (CProjectile::inArray) {
		// alpha-translucent particles
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_2D);
		textureAtlas->BindTexture();
		glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
		glAlphaFunc(GL_GREATER, 0.0f);
		glEnable(GL_ALPHA_TEST);
		glDepthMask(GL_FALSE);

		CProjectile::DrawArray();
	}

	glPopAttrib();
}

void CProjectileDrawer::DrawShadowPass()
{
	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE);

	glDisable(GL_TEXTURE_2D);
	po->Enable();

	CProjectile::inArray = false;
	CProjectile::va = GetVertexArray();
	CProjectile::va->Initialize();

	{
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			DrawProjectilesShadow(modelType);
		}

		// draw the model-less projectiles
		DrawProjectilesSetShadow(renderProjectiles);
	}

	if (CProjectile::inArray) {
		glEnable(GL_TEXTURE_2D);
		textureAtlas->BindTexture();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glAlphaFunc(GL_GREATER,0.3f);
		glEnable(GL_ALPHA_TEST);
		glShadeModel(GL_SMOOTH);

		CProjectile::DrawArray();
	}

	po->Disable();
	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}



bool CProjectileDrawer::DrawProjectileModel(const CProjectile* p)
{
	if (!(p->weapon || p->piece) || (p->model == NULL))
		return false;

	if (p->weapon) {
		// weapon-projectile
		const CWeaponProjectile* wp = static_cast<const CWeaponProjectile*>(p);

		unitDrawer->SetTeamColour(wp->GetTeamID());

		glPushMatrix();
			glMultMatrixf(wp->GetTransformMatrix(wp->GetProjectileType() == WEAPON_MISSILE_PROJECTILE));

			if (!(/*p->luaDraw &&*/ eventHandler.DrawProjectile(p))) {
				wp->model->DrawStatic();
			}
		glPopMatrix();
	} else {
		// piece-projectile
		const CPieceProjectile* pp = static_cast<const CPieceProjectile*>(p);

		unitDrawer->SetTeamColour(pp->GetTeamID());

		glPushMatrix();
			glTranslatef3(pp->drawPos);
			glRotatef(pp->GetDrawAngle(), pp->spinVec.x, pp->spinVec.y, pp->spinVec.z);

			if (!(/*p->luaDraw &&*/ eventHandler.DrawProjectile(p))) {
				if (pp->explFlags & PF_Recursive) {
					pp->omp->DrawStatic();
				} else {
					glCallList(pp->dispList);
				}
			}
		glPopMatrix();
	}

	return true;
}

void CProjectileDrawer::DrawGroundFlashes()
{
	GroundFlashContainer& gfc = projectileHandler->groundFlashes;
	if (gfc.empty())
		return;

	static const GLfloat black[] = {0.0f, 0.0f, 0.0f, 0.0f};

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glActiveTexture(GL_TEXTURE0);
	groundFXAtlas->BindTexture();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);
	glPolygonOffset(-20, -1000);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glFogfv(GL_FOG_COLOR, black);

	CGroundFlash::va = GetVertexArray();
	CGroundFlash::va->Initialize();
	CGroundFlash::va->EnlargeArrays(8 * gfc.size(), 0, VA_SIZE_TC);


	bool depthTest = true;
	bool depthMask = false;

	for (CGroundFlash* gf: gfc) {
		const bool inLos = gf->alwaysVisible || gu->spectatingFullView || losHandler->InAirLos(gf, gu->myAllyTeam);
		if (!inLos)
			continue;

		if (!camera->InView(gf->pos, gf->size))
			continue;

		if (depthTest != gf->depthTest) {
			CGroundFlash::va->DrawArrayTC(GL_QUADS);
			CGroundFlash::va->Initialize();
			depthTest = gf->depthTest;

			if (depthTest) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}
		}
		if (depthMask != gf->depthMask) {
			CGroundFlash::va->DrawArrayTC(GL_QUADS);
			CGroundFlash::va->Initialize();
			depthMask = gf->depthMask;

			if (depthMask) {
				glDepthMask(GL_TRUE);
			} else {
				glDepthMask(GL_FALSE);
			}
		}

		gf->Draw();
	}

	CGroundFlash::va->DrawArrayTC(GL_QUADS);

	glFogfv(GL_FOG_COLOR, sky->fogColor);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}



void CProjectileDrawer::UpdateTextures() {
	if (perlinTexObjects > 0 && drawPerlinTex)
		UpdatePerlin();
}

void CProjectileDrawer::UpdatePerlin() {
	perlinFB.Bind();
	glViewport(perlintex->xstart * (textureAtlas->GetSize()).x, perlintex->ystart * (textureAtlas->GetSize()).y, perlinTexSize, perlinTexSize);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1,  0, 1,  -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_FOG);

	unsigned char col[4];
	float time = globalRendering->lastFrameTime * gs->speedFactor * 0.003f;
	float speed = 1.0f;
	float size = 1.0f;

	CVertexArray* va = GetVertexArray();
	va->CheckInitSize(4 * VA_SIZE_TC);

	for (int a = 0; a < 4; ++a) {
		perlinBlend[a] += time * speed;
		if (perlinBlend[a] > 1) {
			unsigned int temp = perlinBlendTex[a * 2];
			perlinBlendTex[a * 2    ] = perlinBlendTex[a * 2 + 1];
			perlinBlendTex[a * 2 + 1] = temp;

			GenerateNoiseTex(perlinBlendTex[a * 2 + 1]);
			perlinBlend[a] -= 1;
		}

		float tsize = 8.0f / size;

		if (a == 0)
			glDisable(GL_BLEND);

		for (int b = 0; b < 4; ++b)
			col[b] = int((1.0f - perlinBlend[a]) * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinBlendTex[a * 2]);
		va->Initialize();
		va->AddVertexQTC(ZeroVector, 0,         0, col);
		va->AddVertexQTC(  UpVector, 0,     tsize, col);
		va->AddVertexQTC(  XYVector, tsize, tsize, col);
		va->AddVertexQTC( RgtVector, tsize,     0, col);
		va->DrawArrayTC(GL_QUADS);

		if (a == 0)
			glEnable(GL_BLEND);

		for (int b = 0; b < 4; ++b)
			col[b] = int(perlinBlend[a] * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinBlendTex[a * 2 + 1]);
		va->Initialize();
		va->AddVertexQTC(ZeroVector,     0,     0, col);
		va->AddVertexQTC(  UpVector,     0, tsize, col);
		va->AddVertexQTC(  XYVector, tsize, tsize, col);
		va->AddVertexQTC( RgtVector, tsize,     0, col);
		va->DrawArrayTC(GL_QUADS);

		speed *= 0.6f;
		size *= 2;
	}

	perlinFB.Unbind();
	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void CProjectileDrawer::GenerateNoiseTex(unsigned int tex)
{
	std::array<unsigned char, 4 * perlinBlendTexSize * perlinBlendTexSize> mem;

	for (int a = 0; a < perlinBlendTexSize * perlinBlendTexSize; ++a) {
		const unsigned char rnd = int(std::max(0.0f, gu->RandFloat() * 555.0f - 300.0f));

		mem[a * 4 + 0] = rnd;
		mem[a * 4 + 1] = rnd;
		mem[a * 4 + 2] = rnd;
		mem[a * 4 + 3] = rnd;
	}

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, perlinBlendTexSize, perlinBlendTexSize, GL_RGBA, GL_UNSIGNED_BYTE, &mem[0]);
}



void CProjectileDrawer::RenderProjectileCreated(const CProjectile* p)
{
	if (p->model) {
		modelRenderers[MDL_TYPE(p)]->AddProjectile(p);
	} else {
		renderProjectiles.push_back(const_cast<CProjectile*>(p));
	}
}

void CProjectileDrawer::RenderProjectileDestroyed(const CProjectile* const p)
{
	if (p->model) {
		modelRenderers[MDL_TYPE(p)]->DelProjectile(p);
	} else {
		auto it = std::find(renderProjectiles.begin(), renderProjectiles.end(), const_cast<CProjectile*>(p));
		assert(it != renderProjectiles.end());
		renderProjectiles.erase(it);
	}
}

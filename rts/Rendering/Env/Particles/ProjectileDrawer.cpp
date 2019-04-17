/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ProjectileDrawer.h"

#include <algorithm>

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Game/UI/MiniMap.h"
#include "Lua/LuaParser.h"
#include "Map/ReadMap.h" // mapDims
#include "Rendering/GroundFlash.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
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
#include "System/SafeUtil.h"
#include "System/StringUtil.h"


CProjectileDrawer* projectileDrawer = nullptr;

// can not be a CProjectileDrawer; destruction in global
// scope might happen after ~EventHandler (referenced by
// ~EventClient)
static uint8_t projectileDrawerMem[sizeof(CProjectileDrawer)];


void CProjectileDrawer::InitStatic() {
	if (projectileDrawer == nullptr)
		projectileDrawer = new (projectileDrawerMem) CProjectileDrawer();

	projectileDrawer->Init();
}
void CProjectileDrawer::KillStatic(bool reload) {
	projectileDrawer->Kill();

	if (reload)
		return;

	spring::SafeDestruct(projectileDrawer);
	memset(projectileDrawerMem, 0, sizeof(projectileDrawerMem));
}

void CProjectileDrawer::Init() {
	eventHandler.AddClient(this);

	loadscreen->SetLoadMessage("Creating Projectile Textures");

	textureAtlas = new CTextureAtlas(); textureAtlas->SetName("ProjectileTextureAtlas");
	groundFXAtlas = new CTextureAtlas(); groundFXAtlas->SetName("ProjectileEffectsAtlas");

	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	LuaParser mapResParser("gamedata/resources_map.lua", SPRING_VFS_MAP_BASE, SPRING_VFS_ZIP);

	resourcesParser.Execute();

	const LuaTable& resTable = resourcesParser.GetRoot();
	const LuaTable& resGraphicsTable = resTable.SubTable("graphics");
	const LuaTable& resProjTexturesTable = resGraphicsTable.SubTable("projectileTextures");
	const LuaTable& resSmokeTexturesTable = resGraphicsTable.SubTable("smoke");
	const LuaTable& resGroundFXTexturesTable = resGraphicsTable.SubTable("groundfx");

	// used to block resources_map.* from overriding any of
	// resources.lua:{projectile, smoke, groundfx}textures,
	// as well as various defaults (repulsegfxtexture, etc)
	spring::unordered_set<std::string> blockedTexNames;

	ParseAtlasTextures(true, resProjTexturesTable, blockedTexNames, textureAtlas);
	ParseAtlasTextures(true, resGroundFXTexturesTable, blockedTexNames, groundFXAtlas);

	int smokeTexCount = -1;

	{
		// get the smoke textures, hold the count in 'smokeTexCount'
		if (resSmokeTexturesTable.IsValid()) {
			for (smokeTexCount = 0; true; smokeTexCount++) {
				const std::string& tex = resSmokeTexturesTable.GetString(smokeTexCount + 1, "");
				if (tex.empty())
					break;

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
		std::array<char, 4 * PerlinData::noiseTexSize * PerlinData::noiseTexSize> perlinTexMem;
		perlinTexMem.fill(70);
		textureAtlas->AddTexFromMem("perlintex", PerlinData::noiseTexSize, PerlinData::noiseTexSize, CTextureAtlas::RGBA32, &perlinTexMem[0]);
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

	if (!textureAtlas->Finalize())
		LOG_L(L_ERROR, "Could not finalize projectile-texture atlas. Use fewer/smaller textures.");


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

	smokeTextures.reserve(smokeTexCount);

	for (int i = 0; i < smokeTexCount; i++) {
		smokeTextures.push_back(&textureAtlas->GetTexture("ismoke" + IntToString(i, "%02i")));
	}

	sbtrailtex         = &textureAtlas->GetTextureWithBackup("sbtrailtexture",         "smoketrail"    );
	missiletrailtex    = &textureAtlas->GetTextureWithBackup("missiletrailtexture",    "smoketrail"    );
	muzzleflametex     = &textureAtlas->GetTextureWithBackup("muzzleflametexture",     "explo"         );
	repulsetex         = &textureAtlas->GetTextureWithBackup("repulsetexture",         "explo"         );
	dguntex            = &textureAtlas->GetTextureWithBackup("dguntexture",            "flare"         );
	flareprojectiletex = &textureAtlas->GetTextureWithBackup("flareprojectiletexture", "flare"         );
	sbflaretex         = &textureAtlas->GetTextureWithBackup("sbflaretexture",         "flare"         );
	missileflaretex    = &textureAtlas->GetTextureWithBackup("missileflaretexture",    "flare"         );
	beamlaserflaretex  = &textureAtlas->GetTextureWithBackup("beamlaserflaretexture",  "flare"         );
	bubbletex          = &textureAtlas->GetTextureWithBackup("bubbletexture",          "circularthingy");
	geosquaretex       = &textureAtlas->GetTextureWithBackup("geosquaretexture",       "circularthingy");
	gfxtex             = &textureAtlas->GetTextureWithBackup("gfxtexture",             "circularthingy");
	projectiletex      = &textureAtlas->GetTextureWithBackup("projectiletexture",      "circularthingy");
	repulsegfxtex      = &textureAtlas->GetTextureWithBackup("repulsegfxtexture",      "circularthingy");
	sphereparttex      = &textureAtlas->GetTextureWithBackup("sphereparttexture",      "circularthingy");
	torpedotex         = &textureAtlas->GetTextureWithBackup("torpedotexture",         "circularthingy");
	wrecktex           = &textureAtlas->GetTextureWithBackup("wrecktexture",           "circularthingy");
	plasmatex          = &textureAtlas->GetTextureWithBackup("plasmatexture",          "circularthingy");


	if (!groundFXAtlas->Finalize())
		LOG_L(L_ERROR, "Could not finalize groundFX texture atlas. Use fewer/smaller textures.");

	groundflashtex = &groundFXAtlas->GetTexture("groundflash");
	groundringtex = &groundFXAtlas->GetTexture("groundring");
	seismictex = &groundFXAtlas->GetTexture("seismic");


	std::fill(std::begin(perlinData.blendWeights), std::end(perlinData.blendWeights), 0.0f);

	{
		glGenTextures(8, perlinData.blendTextures);
		for (const GLuint blendTexture: perlinData.blendTextures) {
			glBindTexture(GL_TEXTURE_2D, blendTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, PerlinData::blendTexSize, PerlinData::blendTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
	}


	// perlinNoiseFBO is no-op constructed, has to be initialized manually
	perlinNoiseFBO.Init(false);

	if (perlinNoiseFBO.IsValid()) {
		// we never refresh the full texture (just the perlin part), so reload it on AT
		perlinNoiseFBO.reloadOnAltTab = true;

		perlinNoiseFBO.Bind();
		perlinNoiseFBO.AttachTexture(textureAtlas->GetTexID());
		perlinData.fboComplete = perlinNoiseFBO.CheckStatus("PERLIN");
		perlinNoiseFBO.Unbind();
	}

	flyingPieceVAO.Generate();


	renderProjectiles.reserve(projectileHandler.maxParticles + projectileHandler.maxNanoParticles);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		modelRenderers[modelType].Init();
	}

	LoadWeaponTextures();
}

void CProjectileDrawer::Kill() {
	eventHandler.RemoveClient(this);
	autoLinkedEvents.clear();

	glDeleteTextures(8, perlinData.blendTextures);
	spring::SafeDelete(textureAtlas);
	spring::SafeDelete(groundFXAtlas);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		modelRenderers[modelType].Kill();
	}

	smokeTextures.clear();

	renderProjectiles.clear();
	sortedProjectiles[0].clear();
	sortedProjectiles[1].clear();

	perlinNoiseFBO.Kill();
	flyingPieceVAO.Delete();

	perlinData.texObjects = 0;
	perlinData.fboComplete = false;

	drawSorted = true;

	fxBuffer = nullptr;
	fxShader = nullptr;
}



void CProjectileDrawer::ParseAtlasTextures(
	const bool blockTextures,
	const LuaTable& textureTable,
	spring::unordered_set<std::string>& blockedTextures,
	CTextureAtlas* texAtlas
) {
	std::vector<std::string> subTables;
	spring::unordered_map<std::string, std::string> texturesMap;

	textureTable.GetMap(texturesMap);
	textureTable.GetKeys(subTables);

	for (const auto& item: texturesMap) {
		const std::string textureName = StringToLower(item.first);

		// no textures added to this atlas are allowed
		// to be overwritten later by other textures of
		// the same name
		if (blockTextures)
			blockedTextures.insert(textureName);

		if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end()))
			texAtlas->AddTexFromFile(item.first, "bitmaps/" + item.second);
	}

	texturesMap.clear();

	for (const auto& subTable: subTables) {
		const LuaTable& textureSubTable = textureTable.SubTable(subTable);

		if (!textureSubTable.IsValid())
			continue;

		textureSubTable.GetMap(texturesMap);

		for (const auto& item: texturesMap) {
			const std::string textureName = StringToLower(item.first);

			if (blockTextures)
				blockedTextures.insert(textureName);

			if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end()))
				texAtlas->AddTexFromFile(item.first, "bitmaps/" + item.second);
		}

		texturesMap.clear();
	}
}

void CProjectileDrawer::LoadWeaponTextures() {
	// post-process the synced weapon-defs to set unsynced fields
	// (this requires CWeaponDefHandler to have been initialized)
	for (WeaponDef& wd: const_cast<std::vector<WeaponDef>&>(weaponDefHandler->GetWeaponDefsVec())) {
		wd.visuals.texture1 = nullptr;
		wd.visuals.texture2 = nullptr;
		wd.visuals.texture3 = nullptr;
		wd.visuals.texture4 = nullptr;

		if (!wd.visuals.colorMapStr.empty())
			wd.visuals.colorMap = CColorMap::LoadFromDefString(wd.visuals.colorMapStr);

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
		if (!wd.visuals.texNames[0].empty()) { wd.visuals.texture1 = &textureAtlas->GetTexture(wd.visuals.texNames[0]); }
		if (!wd.visuals.texNames[1].empty()) { wd.visuals.texture2 = &textureAtlas->GetTexture(wd.visuals.texNames[1]); }
		if (!wd.visuals.texNames[2].empty()) { wd.visuals.texture3 = &textureAtlas->GetTexture(wd.visuals.texNames[2]); }
		if (!wd.visuals.texNames[3].empty()) { wd.visuals.texture4 = &textureAtlas->GetTexture(wd.visuals.texNames[3]); }

		// trails can only be custom EG's, prefix is not required game-side
		if (!wd.visuals.ptrailExpGenTag.empty())
			wd.ptrailExplosionGeneratorID = explGenHandler.LoadCustomGeneratorID(wd.visuals.ptrailExpGenTag.c_str());

		if (!wd.visuals.impactExpGenTag.empty())
			wd.impactExplosionGeneratorID = explGenHandler.LoadGeneratorID(wd.visuals.impactExpGenTag.c_str());

		if (!wd.visuals.bounceExpGenTag.empty())
			wd.bounceExplosionGeneratorID = explGenHandler.LoadGeneratorID(wd.visuals.bounceExpGenTag.c_str());

	}
}



void CProjectileDrawer::DrawProjectiles(int modelType, bool drawReflection, bool drawRefraction)
{
	const auto& mdlRenderer = modelRenderers[modelType];
	// const auto& projBinKeys = mdlRenderer.GetObjectBinKeys();

	for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		CUnitDrawer::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));
		DrawProjectilesSet(mdlRenderer.GetObjectBin(i), drawReflection, drawRefraction);
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
	return (gu->spectatingFullView || (owner != nullptr && th.Ally(owner->allyteam, gu->myAllyTeam)) || lh->InLos(pro, gu->myAllyTeam));
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

	if (!camera->InView(pro->drawPos, pro->GetDrawRadius()))
		return;

	// no-op if no model
	DrawProjectileModel(pro);

	pro->SetSortDist(camera->ProjectedDistance(pro->pos));
	sortedProjectiles[drawSorted && pro->drawSorted].push_back(pro);
}



void CProjectileDrawer::DrawProjectilesShadow(int modelType)
{
	const auto& mdlRenderer = modelRenderers[modelType];
	// const auto& projBinKeys = mdlRenderer.GetObjectBinKeys();

	for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		DrawProjectilesSetShadow(mdlRenderer.GetObjectBin(i));
	}

	DrawFlyingPieces(modelType);
}

void CProjectileDrawer::DrawProjectilesSetShadow(const std::vector<CProjectile*>& projectiles)
{
	for (const CProjectile* p: projectiles) {
		DrawProjectileShadow(p);
	}
}

void CProjectileDrawer::DrawProjectileShadow(const CProjectile* p)
{
	if (!CanDrawProjectile(p, p->owner()))
		return;

	if (!camera->InView(p->drawPos, p->GetDrawRadius()))
		return;

	// if this returns false, then projectile is
	// neither weapon nor piece, or has no model
	if (DrawProjectileModel(p))
		return;

	if (!p->castShadow)
		return;

	// don't need to z-sort in the shadow pass
	p->Draw(fxBuffer);
}



void CProjectileDrawer::DrawProjectilesMiniMap()
{
	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		const auto& mdlRenderer = modelRenderers[modelType];
		// const auto& projBinKeys = mdlRenderer.GetObjectBinKeys();

		for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
			const auto& projectileBin = mdlRenderer.GetObjectBin(i);

			for (CProjectile* p: projectileBin) {
				if (!CanDrawProjectile(p, p->owner()))
					continue;

				p->DrawOnMinimap(buffer);
			}
		}
	}

	for (CProjectile* p: renderProjectiles) {
		if (!CanDrawProjectile(p, p->owner()))
			continue;

		p->DrawOnMinimap(buffer);
	}

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, minimap->GetViewMat(0));
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, minimap->GetProjMat(0));
	buffer->Submit(GL_LINES);
	shader->Disable();
}

void CProjectileDrawer::DrawFlyingPieces(int modelType)
{
	const FlyingPieceContainer& container = projectileHandler.flyingPieces[modelType];

	if (container.empty())
		return;

	glAttribStatePtr->PushPolygonBit();
	glAttribStatePtr->DisableCullFace();

	flyingPieceVAO.Bind();

	const FlyingPiece* last = nullptr;

	for (const FlyingPiece& fp: container) {
		const bool noLosTst = gu->spectatingFullView || teamHandler.AlliedTeams(gu->myTeam, fp.GetTeam());
		const bool inAirLos = noLosTst || losHandler->InAirLos(fp.GetPos(), gu->myAllyTeam);

		if (!inAirLos)
			continue;

		if (!camera->InView(fp.GetPos(), fp.GetRadius()))
			continue;

		fp.Draw(last);
		last = &fp;
	}

	if (last != nullptr) {
		last->EndDraw();
		flyingPieceVAO.Unbind();
	}

	glAttribStatePtr->PopBits();
}



void CProjectileDrawer::DrawProjectilePass(Shader::IProgramObject*, bool drawReflection, bool drawRefraction)
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

	// empty if !drawSorted
	std::sort(sortedProjectiles[1].begin(), sortedProjectiles[1].end(), zSortCmp);


	// collect the alpha-translucent particle effects in fxBuffer
	for (CProjectile* p: sortedProjectiles[1]) {
		p->Draw(fxBuffer);
	}
	for (CProjectile* p: sortedProjectiles[0]) {
		p->Draw(fxBuffer);
	}
}

void CProjectileDrawer::DrawParticlePass(Shader::IProgramObject* po, bool, bool)
{
	if (fxBuffer->NumElems() > 0) {
		glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glAttribStatePtr->DisableDepthMask();

		glActiveTexture(GL_TEXTURE0);
		// send event after the default state has been set, allows overriding
		// it for specific cases such as proper blending with depth-aware fog
		// (requires mask=true and func=always)
		eventHandler.DrawWorldPreParticles();


		po->Enable();
		po->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
		po->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
		po->SetUniform("u_alpha_test_ctrl", 0.0f, 1.0f, 0.0f, 0.0f); // test > 0.0
		textureAtlas->BindTexture();
		fxBuffer->Submit(GL_TRIANGLES);
		po->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
		po->Disable();
	} else {
		eventHandler.DrawWorldPreParticles();
	}
}

void CProjectileDrawer::Draw(bool drawReflection, bool drawRefraction) {
	glAttribStatePtr->PushBits(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);
	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->EnableDepthMask();

	sortedProjectiles[0].clear();
	sortedProjectiles[1].clear();


	fxBuffer = GL::GetRenderBufferTC();
	fxShader = fxBuffer->GetShader();

	DrawProjectilePass(fxShader, drawReflection, drawRefraction);

	glAttribStatePtr->EnableBlendMask();

	DrawParticlePass(fxShader, drawReflection, drawRefraction);

	glAttribStatePtr->PopBits();
}



void CProjectileDrawer::DrawProjectileShadowPass(Shader::IProgramObject* po)
{
	po->Enable();
	po->SetUniformMatrix4fv(1, false, shadowHandler.GetShadowViewMatrix());
	po->SetUniformMatrix4fv(2, false, shadowHandler.GetShadowProjMatrix());

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		DrawProjectilesShadow(modelType);
	}

	// draw the model-less projectiles
	DrawProjectilesSetShadow(renderProjectiles);
	po->Disable();
}

void CProjectileDrawer::DrawParticleShadowPass(Shader::IProgramObject* po)
{
	if (fxBuffer->NumElems() == 0)
		return;

	po->Enable();
	po->SetUniformMatrix4fv(1, false, shadowHandler.GetShadowViewMatrix());
	po->SetUniformMatrix4fv(2, false, shadowHandler.GetShadowProjMatrix());
	textureAtlas->BindTexture();
	fxBuffer->Submit(GL_TRIANGLES);
	po->Disable();
}

void CProjectileDrawer::DrawShadowPass()
{
	glAttribStatePtr->PushEnableBit();

	fxBuffer = GL::GetRenderBufferTC();
	fxShader = nullptr;

	DrawProjectileShadowPass(shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE));
	DrawParticleShadowPass(shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PARTICLE));

	glAttribStatePtr->PopBits();
}



bool CProjectileDrawer::DrawProjectileModel(const CProjectile* p)
{
	const S3DModel* model = p->model;
	const IUnitDrawerState* udState = unitDrawer->GetDrawerState(DRAWER_STATE_SEL);

	if (model == nullptr)
		return false;

	switch ((p->weapon * 2) + (p->piece * 1)) {
		case 2: {
			// weapon-projectile
			const CWeaponProjectile* wp = static_cast<const CWeaponProjectile*>(p);

			unitDrawer->SetTeamColour(wp->GetTeamID());
			udState->SetMatrices(wp->GetTransformMatrix(float(wp->GetProjectileType() == WEAPON_MISSILE_PROJECTILE)), model->GetPieceMatrices());

			if (p->luaDraw && eventHandler.DrawProjectile(p))
				return true;

			model->Draw();
			return true;
		} break;

		case 1: {
			// piece-projectile
			const CPieceProjectile* pp = static_cast<const CPieceProjectile*>(p);

			CMatrix44f modelMat;

			modelMat.Translate(pp->drawPos);
			modelMat.Rotate(pp->GetDrawAngle() * math::DEG_TO_RAD, pp->spinVector);

			unitDrawer->SetTeamColour(pp->GetTeamID());

			// NOTE: eventHandler.Draw{Unit,Feature,Projectile} now has no transform
			if (p->luaDraw && eventHandler.DrawProjectile(p))
				return true;

			if ((pp->explFlags & PF_Recursive) != 0) {
				udState->SetMatrices(modelMat, model->GetPieceMatrices());
				model->DrawPieceRec(pp->modelPiece);
			} else {
				// non-recursive, only use the model matrix
				udState->SetMatrices(modelMat, IUnitDrawerState::GetDummyPieceMatrixPtr(), model->numPieces);
				model->DrawPiece(pp->modelPiece);
			}

			return true;
		} break;

		default: {
		} break;
	}

	return false;
}

void CProjectileDrawer::DrawGroundFlashes()
{
	const GroundFlashContainer& gfc = projectileHandler.groundFlashes;

	if (gfc.empty())
		return;

	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE);

	glAttribStatePtr->PolygonOffset(-20.0f, -1000.0f);
	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);

	glActiveTexture(GL_TEXTURE0);
	groundFXAtlas->BindTexture();

	gfBuffer = GL::GetRenderBufferTC();
	gfShader = gfBuffer->GetShader();

	gfShader->Enable();
	gfShader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
	gfShader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
	gfShader->SetUniform("u_alpha_test_ctrl", 0.01f, 1.0f, 0.0f, 0.0f); // test > 0.01

	bool depthTest = true;
	bool depthMask = false;

	for (const CGroundFlash* gf: gfc) {
		if (!gf->alwaysVisible && !gu->spectatingFullView && !losHandler->InAirLos(gf, gu->myAllyTeam))
			continue;

		if (!camera->InView(gf->pos, gf->size))
			continue;

		if (depthTest != gf->depthTest) {
			gfBuffer->Submit(GL_TRIANGLES);

			if ((depthTest = gf->depthTest)) {
				glAttribStatePtr->EnableDepthTest();
			} else {
				glAttribStatePtr->DisableDepthTest();
			}
		}
		if (depthMask != gf->depthMask) {
			gfBuffer->Submit(GL_TRIANGLES);

			if ((depthMask = gf->depthMask)) {
				glAttribStatePtr->EnableDepthMask();
			} else {
				glAttribStatePtr->DisableDepthMask();
			}
		}

		gf->Draw(gfBuffer);
	}

	gfBuffer->Submit(GL_TRIANGLES);
	gfShader->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	gfShader->Disable();

	glAttribStatePtr->PolygonOffsetFill(GL_FALSE);
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->EnableDepthMask();
}



void CProjectileDrawer::UpdateTextures() { UpdatePerlin(); }
void CProjectileDrawer::UpdatePerlin() {
	if (perlinData.texObjects == 0 || !perlinData.fboComplete)
		return;

	perlinNoiseFBO.Bind();
	glAttribStatePtr->ViewPort(perlintex->xstart * (textureAtlas->GetSize()).x, perlintex->ystart * (textureAtlas->GetSize()).y, PerlinData::noiseTexSize, PerlinData::noiseTexSize);

	glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE);

	unsigned char col[4];
	const float time = globalRendering->lastFrameTime * gs->speedFactor * 0.003f;

	float speed = 1.0f;
	float size = 1.0f;


	GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
	shader->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test

	for (int a = 0; a < 4; ++a) {
		if ((perlinData.blendWeights[a] += (time * speed)) > 1.0f) {
			std::swap(perlinData.blendTextures[a * 2 + 0], perlinData.blendTextures[a * 2 + 1]);
			GenerateNoiseTex(perlinData.blendTextures[a * 2 + 1]);
			perlinData.blendWeights[a] -= 1.0f;
		}

		const float tsize = 8.0f / size;

		if (a == 0)
			glAttribStatePtr->DisableBlendMask();

		std::fill(std::begin(col), std::end(col), int((1.0f - perlinData.blendWeights[a]) * 16 * size));

		{
			glBindTexture(GL_TEXTURE_2D, perlinData.blendTextures[a * 2 + 0]);
			buffer->SafeAppend({ZeroVector,     0,     0, col});
			buffer->SafeAppend({  UpVector,     0, tsize, col});
			buffer->SafeAppend({  XYVector, tsize, tsize, col});

			buffer->SafeAppend({  XYVector, tsize, tsize, col});
			buffer->SafeAppend({ RgtVector, tsize,     0, col});
			buffer->SafeAppend({ZeroVector,     0,     0, col});
			buffer->Submit(GL_TRIANGLES);
		}

		if (a == 0)
			glAttribStatePtr->EnableBlendMask();

		std::fill(std::begin(col), std::end(col), int(perlinData.blendWeights[a] * 16 * size));

		{
			glBindTexture(GL_TEXTURE_2D, perlinData.blendTextures[a * 2 + 1]);
			buffer->SafeAppend({ZeroVector,     0,     0, col});
			buffer->SafeAppend({  UpVector,     0, tsize, col});
			buffer->SafeAppend({  XYVector, tsize, tsize, col});

			buffer->SafeAppend({  XYVector, tsize, tsize, col});
			buffer->SafeAppend({ RgtVector, tsize,     0, col});
			buffer->SafeAppend({ZeroVector,     0,     0, col});
			buffer->Submit(GL_TRIANGLES);
		}

		speed *= 0.6f;
		size *= 2.0f;
	}

	shader->Disable();


	perlinNoiseFBO.Unbind();
	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->EnableDepthMask();
}

void CProjectileDrawer::GenerateNoiseTex(unsigned int tex)
{
	std::array<unsigned char, 4 * PerlinData::blendTexSize * PerlinData::blendTexSize> mem;

	for (int a = 0; a < PerlinData::blendTexSize * PerlinData::blendTexSize; ++a) {
		const unsigned char rnd = int(std::max(0.0f, guRNG.NextFloat() * 555.0f - 300.0f));

		mem[a * 4 + 0] = rnd;
		mem[a * 4 + 1] = rnd;
		mem[a * 4 + 2] = rnd;
		mem[a * 4 + 3] = rnd;
	}

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PerlinData::blendTexSize, PerlinData::blendTexSize, GL_RGBA, GL_UNSIGNED_BYTE, &mem[0]);
}



void CProjectileDrawer::RenderProjectileCreated(const CProjectile* p)
{
	if (p->model != nullptr) {
		modelRenderers[MDL_TYPE(p)].AddObject(p);
		return;
	}

	const_cast<CProjectile*>(p)->SetRenderIndex(renderProjectiles.size());
	renderProjectiles.push_back(const_cast<CProjectile*>(p));
}

void CProjectileDrawer::RenderProjectileDestroyed(const CProjectile* p)
{
	if (p->model != nullptr) {
		modelRenderers[MDL_TYPE(p)].DelObject(p);
		return;
	}

	const unsigned int idx = p->GetRenderIndex();

	if (idx >= renderProjectiles.size()) {
		assert(false);
		return;
	}

	renderProjectiles[idx] = renderProjectiles.back();
	renderProjectiles[idx]->SetRenderIndex(idx);

	renderProjectiles.pop_back();
}

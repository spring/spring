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
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Models/ModelRenderContainer.h"
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

	for (int i = 0; i < smokeTexCount; i++) {
		const std::string smokeName = "ismoke" + IntToString(i, "%02i");
		const AtlasedTexture* smokeTex = &textureAtlas->GetTexture(smokeName);
		smoketex.push_back(smokeTex);
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


	for (int a = 0; a < 4; ++a) {
		perlinData.blendWeights[a] = 0.0f;
	}

	{
		glGenTextures(8, perlinData.blendTextures);
		for (int a = 0; a < 8; ++a) {
			glBindTexture(GL_TEXTURE_2D, perlinData.blendTextures[a]);
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


	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		modelRenderers[modelType] = IModelRenderContainer::GetInstance(modelType);
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
		spring::SafeDelete(modelRenderers[modelType]);
	}

	smoketex.clear();

	renderProjectiles.clear();
	zSortedProjectiles.clear();
	unsortedProjectiles.clear();

	perlinNoiseFBO.Kill();
	flyingPieceVAO.Delete();

	perlinData.texObjects = 0;
	perlinData.fboComplete = false;

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

	for (auto texturesMapIt = texturesMap.begin(); texturesMapIt != texturesMap.end(); ++texturesMapIt) {
		const std::string textureName = StringToLower(texturesMapIt->first);

		// no textures added to this atlas are allowed
		// to be overwritten later by other textures of
		// the same name
		if (blockTextures)
			blockedTextures.insert(textureName);

		if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end()))
			texAtlas->AddTexFromFile(texturesMapIt->first, "bitmaps/" + texturesMapIt->second);
	}

	texturesMap.clear();

	for (size_t i = 0; i < subTables.size(); i++) {
		const LuaTable& textureSubTable = textureTable.SubTable(subTables[i]);

		if (!textureSubTable.IsValid())
			continue;

		textureSubTable.GetMap(texturesMap);

		for (auto texturesMapIt = texturesMap.begin(); texturesMapIt != texturesMap.end(); ++texturesMapIt) {
			const std::string textureName = StringToLower(texturesMapIt->first);

			if (blockTextures)
				blockedTextures.insert(textureName);

			if (blockTextures || (blockedTextures.find(textureName) == blockedTextures.end()))
				texAtlas->AddTexFromFile(texturesMapIt->first, "bitmaps/" + texturesMapIt->second);
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

		// these can only be custom EG's so prefix is not required game-side
		if (!wd.visuals.ptrailExpGenTag.empty())
			wd.ptrailExplosionGeneratorID = explGenHandler.LoadGeneratorID(CEG_PREFIX_STRING + wd.visuals.ptrailExpGenTag);

		if (!wd.visuals.impactExpGenTag.empty())
			wd.impactExplosionGeneratorID = explGenHandler.LoadGeneratorID(wd.visuals.impactExpGenTag);

		if (!wd.visuals.bounceExpGenTag.empty())
			wd.bounceExplosionGeneratorID = explGenHandler.LoadGeneratorID(wd.visuals.bounceExpGenTag);

	}
}



void CProjectileDrawer::DrawProjectiles(int modelType, bool drawReflection, bool drawRefraction)
{
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

	const CCamera* cam = CCamera::GetActiveCamera();
	if (!cam->InView(pro->drawPos, pro->GetDrawRadius()))
		return;

	DrawProjectileModel(pro);

	if (pro->drawSorted) {
		pro->SetSortDist(cam->ProjectedDistance(pro->pos));
		zSortedProjectiles.push_back(pro);
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
	for (const CProjectile* p: projectiles) {
		DrawProjectileShadow(p);
	}
}

void CProjectileDrawer::DrawProjectileShadow(const CProjectile* p)
{
	if (!CanDrawProjectile(p, p->owner()))
		return;

	const CCamera* cam = CCamera::GetActiveCamera();
	if (!cam->InView(p->drawPos, p->GetDrawRadius()))
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
	const FlyingPieceContainer& container = projectileHandler.flyingPieces[modelType];

	if (container.empty())
		return;

	glPushAttrib(GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);

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

	glPopAttrib();
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

	std::sort(zSortedProjectiles.begin(), zSortedProjectiles.end(), zSortCmp);


	// collect the alpha-translucent particle effects in fxBuffer
	for (CProjectile* p: zSortedProjectiles) {
		p->Draw(fxBuffer);
	}
	for (CProjectile* p: unsortedProjectiles) {
		p->Draw(fxBuffer);
	}
}

void CProjectileDrawer::DrawParticlePass(Shader::IProgramObject* po, bool, bool)
{
	if (fxBuffer->NumElems() > 0) {
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glActiveTexture(GL_TEXTURE0);

		glAlphaFunc(GL_GREATER, 0.0f);
		glEnable(GL_ALPHA_TEST);
		glDepthMask(GL_FALSE);

		// send event after the default state has been set, allows overriding
		// it for specific cases such as proper blending with depth-aware fog
		// (requires mask=true and func=always)
		eventHandler.DrawWorldPreParticles();


		po->Enable();
		po->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());
		po->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());
		textureAtlas->BindTexture();
		fxBuffer->Submit(GL_QUADS);
		po->Disable();
	} else {
		eventHandler.DrawWorldPreParticles();
	}
}

void CProjectileDrawer::Draw(bool drawReflection, bool drawRefraction) {
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	zSortedProjectiles.clear();
	unsortedProjectiles.clear();


	fxBuffer = GL::GetRenderBufferTC();
	fxShader = fxBuffer->GetShader();

	DrawProjectilePass(fxShader, drawReflection, drawRefraction);

	glEnable(GL_BLEND);

	DrawParticlePass(fxShader, drawReflection, drawRefraction);

	glPopAttrib();
}



void CProjectileDrawer::DrawProjectileShadowPass(Shader::IProgramObject* po)
{
	po->Enable();
	po->SetUniformMatrix4fv(1, false, shadowHandler->GetShadowViewMatrix());
	po->SetUniformMatrix4fv(2, false, shadowHandler->GetShadowProjMatrix());

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
	po->SetUniformMatrix4fv(1, false, shadowHandler->GetShadowViewMatrix());
	po->SetUniformMatrix4fv(2, false, shadowHandler->GetShadowProjMatrix());
	textureAtlas->BindTexture();
	fxBuffer->Submit(GL_QUADS);
	po->Disable();
}

void CProjectileDrawer::DrawShadowPass()
{
	glPushAttrib(GL_ENABLE_BIT);

	fxBuffer = GL::GetRenderBufferTC();
	fxShader = nullptr;

	DrawProjectileShadowPass(shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE));
	DrawParticleShadowPass(shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PARTICLE));

	glPopAttrib();
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

			if (/*p->luaDraw &&*/ eventHandler.DrawProjectile(p))
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
			if (/*p->luaDraw &&*/ eventHandler.DrawProjectile(p))
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

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glActiveTexture(GL_TEXTURE0);
	groundFXAtlas->BindTexture();

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);

	glPolygonOffset(-20, -1000);
	glEnable(GL_POLYGON_OFFSET_FILL);

	gfBuffer = GL::GetRenderBufferTC();
	gfShader = gfBuffer->GetShader();

	gfShader->Enable();
	gfShader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, camera->GetViewMatrix());
	gfShader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, camera->GetProjectionMatrix());

	bool depthTest = true;
	bool depthMask = false;

	for (const CGroundFlash* gf: gfc) {
		if (!gf->alwaysVisible && !gu->spectatingFullView && !losHandler->InAirLos(gf, gu->myAllyTeam))
			continue;

		if (!camera->InView(gf->pos, gf->size))
			continue;

		if (depthTest != gf->depthTest) {
			gfBuffer->Submit(GL_QUADS);

			if ((depthTest = gf->depthTest)) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}
		}
		if (depthMask != gf->depthMask) {
			gfBuffer->Submit(GL_QUADS);

			if ((depthMask = gf->depthMask)) {
				glDepthMask(GL_TRUE);
			} else {
				glDepthMask(GL_FALSE);
			}
		}

		gf->Draw(gfBuffer);
	}

	gfBuffer->Submit(GL_QUADS);
	gfShader->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}



void CProjectileDrawer::UpdateTextures() { UpdatePerlin(); }
void CProjectileDrawer::UpdatePerlin() {
	if (perlinData.texObjects == 0 || !perlinData.fboComplete)
		return;

	perlinNoiseFBO.Bind();
	glViewport(perlintex->xstart * (textureAtlas->GetSize()).x, perlintex->ystart * (textureAtlas->GetSize()).y, PerlinData::noiseTexSize, PerlinData::noiseTexSize);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_ALPHA_TEST);

	unsigned char col[4];
	const float time = globalRendering->lastFrameTime * gs->speedFactor * 0.003f;

	float speed = 1.0f;
	float size = 1.0f;


	GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

	for (int a = 0; a < 4; ++a) {
		if ((perlinData.blendWeights[a] += (time * speed)) > 1.0f) {
			std::swap(perlinData.blendTextures[a * 2 + 0], perlinData.blendTextures[a * 2 + 1]);
			GenerateNoiseTex(perlinData.blendTextures[a * 2 + 1]);
			perlinData.blendWeights[a] -= 1.0f;
		}

		const float tsize = 8.0f / size;

		if (a == 0)
			glDisable(GL_BLEND);

		for (int b = 0; b < 4; ++b)
			col[b] = int((1.0f - perlinData.blendWeights[a]) * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinData.blendTextures[a * 2 + 0]);
		buffer->SafeAppend({ZeroVector, 0,         0, col});
		buffer->SafeAppend({  UpVector, 0,     tsize, col});
		buffer->SafeAppend({  XYVector, tsize, tsize, col});
		buffer->SafeAppend({ RgtVector, tsize,     0, col});
		buffer->Submit(GL_QUADS);

		if (a == 0)
			glEnable(GL_BLEND);

		for (int b = 0; b < 4; ++b)
			col[b] = int(perlinData.blendWeights[a] * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinData.blendTextures[a * 2 + 1]);
		buffer->SafeAppend({ZeroVector,     0,     0, col});
		buffer->SafeAppend({  UpVector,     0, tsize, col});
		buffer->SafeAppend({  XYVector, tsize, tsize, col});
		buffer->SafeAppend({ RgtVector, tsize,     0, col});
		buffer->Submit(GL_QUADS);

		speed *= 0.6f;
		size *= 2.0f;
	}

	shader->Disable();


	perlinNoiseFBO.Unbind();
	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
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

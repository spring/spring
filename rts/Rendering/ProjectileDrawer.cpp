/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ProjectileDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaRules.h"
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
#include "Rendering/Models/WorldObjectModelRenderer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/Unsynced/FlyingPiece.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Util.h"



CProjectileDrawer* projectileDrawer = NULL;

CProjectileDrawer::CProjectileDrawer(): CEventClient("[CProjectileDrawer]", 123456, false) {
	eventHandler.AddClient(this);

	loadscreen->SetLoadMessage("Creating Projectile Textures");

	textureAtlas = new CTextureAtlas(2048, 2048);
	groundFXAtlas = new CTextureAtlas(2048, 2048);

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
		char perlinTexMem[128][128][4];
		for (int y = 0; y < 128; y++) {
			for (int x = 0; x < 128; x++) {
				perlinTexMem[y][x][0] = 70;
				perlinTexMem[y][x][1] = 70;
				perlinTexMem[y][x][2] = 70;
				perlinTexMem[y][x][3] = 70;
			}
		}

		textureAtlas->AddTexFromMem("perlintex", 128, 128, CTextureAtlas::RGBA32, perlinTexMem);
	}

	blockedTexNames.insert("perlintex");
	blockedTexNames.insert("flare");
	blockedTexNames.insert("explo");
	blockedTexNames.insert("explofade");
	blockedTexNames.insert("heatcloud");
	blockedTexNames.insert("laserend");
	blockedTexNames.insert("laserfalloff");
	blockedTexNames.insert("randdots");
	blockedTexNames.insert("smoketrail");
	blockedTexNames.insert("wake");
	blockedTexNames.insert("perlintex");
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
		unsigned char tempmem[4 * 16 * 16] = {0};

		for (int a = 0; a < 8; ++a) {
			glGenTextures(1, &perlinTex[a]);
			glBindTexture(GL_TEXTURE_2D, perlinTex[a]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16,16, 0, GL_RGBA, GL_UNSIGNED_BYTE, tempmem);
		}
	}

	perlinTexObjects = 0;
	drawPerlinTex = false;

	if (perlinFB.IsValid()) {
		// we never refresh the full texture (just the perlin part). So we need to reload it then.
		perlinFB.reloadOnAltTab = true;

		perlinFB.Bind();
		perlinFB.AttachTexture(textureAtlas->gltex);
		drawPerlinTex = perlinFB.CheckStatus("PERLIN");
		perlinFB.Unbind();
	}


	modelRenderers.resize(MODELTYPE_OTHER, NULL);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		modelRenderers[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
	}
}

CProjectileDrawer::~CProjectileDrawer() {
	eventHandler.RemoveClient(this);

	for (int a = 0; a < 8; ++a) {
		glDeleteTextures(1, &perlinTex[a]);
	}

	delete textureAtlas;
	delete groundFXAtlas;


	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		delete modelRenderers[modelType];
	}

	modelRenderers.clear();
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
	for (int wid = 0; wid < weaponDefHandler->weaponDefs.size(); wid++) {
		WeaponDef& wd = weaponDefHandler->weaponDefs[wid];

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

			if (wd.visuals.colorMap == 0) {
				wd.visuals.colorMap = CColorMap::Load12f(
					1.000f, 1.000f, 1.000f, 0.100f,
					0.025f, 0.025f, 0.025f, 0.100f,
					0.000f, 0.000f, 0.000f, 0.000f
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

		// load weapon explosion generators
		if (wd.visuals.expGenTag.empty()) {
			wd.explosionGenerator = NULL;
		} else {
			wd.explosionGenerator = explGenHandler->LoadGenerator(wd.visuals.expGenTag);
		}

		if (wd.visuals.bounceExpGenTag.empty()) {
			wd.bounceExplosionGenerator = NULL;
		} else {
			wd.bounceExplosionGenerator = explGenHandler->LoadGenerator(wd.visuals.bounceExpGenTag);
		}
	}
}



void CProjectileDrawer::DrawProjectiles(int modelType, int numFlyingPieces, int* drawnPieces, bool drawReflection, bool drawRefraction)
{
	typedef std::set<CProjectile*> ProjectileSet;
	typedef std::map<int, ProjectileSet> ProjectileBin;
	typedef ProjectileSet::iterator ProjectileSetIt;
	typedef ProjectileBin::iterator ProjectileBinIt;

	ProjectileBin& projectileBin = modelRenderers[modelType]->GetProjectileBinMutable();

	for (ProjectileBinIt binIt = projectileBin.begin(); binIt != projectileBin.end(); ++binIt) {
		if (modelType != MODELTYPE_3DO) {
			texturehandlerS3O->SetS3oTexture(binIt->first);
		}

		DrawProjectilesSet(binIt->second, drawReflection, drawRefraction);
	}

	DrawFlyingPieces(modelType, numFlyingPieces, drawnPieces);
}

void CProjectileDrawer::DrawProjectilesSet(std::set<CProjectile*>& projectiles, bool drawReflection, bool drawRefraction)
{
	for (std::set<CProjectile*>::iterator setIt = projectiles.begin(); setIt != projectiles.end(); ++setIt) {
		DrawProjectile(*setIt, drawReflection, drawRefraction);
	}
}

void CProjectileDrawer::DrawProjectile(CProjectile* pro, bool drawReflection, bool drawRefraction)
{
	const CUnit* owner = pro->owner();

	const float time = !GML::SimEnabled() ? globalRendering->timeOffset :
		((float)spring_tomsecs(globalRendering->lastFrameStart) - (float)pro->lastProjUpdate) * globalRendering->weightedSpeedFactor;
		pro->drawPos = pro->pos + (pro->speed * time);
	const bool visible = (gu->spectatingFullView || loshandler->InLos(pro, gu->myAllyTeam) || (owner && teamHandler->Ally(owner->allyteam, gu->myAllyTeam)));

	if (!visible)
		return;
	if (!camera->InView(pro->pos, pro->drawRadius))
		return;

	if (drawReflection) {
		if (pro->pos.y < -pro->drawRadius) {
			return;
		}

		const float dif = pro->pos.y - camera->pos.y;
		const float3 zeroPos = camera->pos * (pro->pos.y / dif) + pro->pos * (-camera->pos.y / dif);

		if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z, false) > 3 + 0.5f * pro->drawRadius) {
			return;
		}
	}

	if (drawRefraction && pro->pos.y > pro->drawRadius) {
		return;
	}

	DrawProjectileModel(pro, false);

	pro->tempdist = pro->pos.dot(camera->forward);
	zSortedProjectiles.insert(pro);
}



void CProjectileDrawer::DrawProjectilesShadow(int modelType)
{
	typedef std::map<int, std::set<CProjectile*> > ProjectileBin;
	typedef ProjectileBin::iterator ProjectileBinIt;

	ProjectileBin& projectileBin = modelRenderers[modelType]->GetProjectileBinMutable();

	for (ProjectileBinIt binIt = projectileBin.begin(); binIt != projectileBin.end(); ++binIt) {
		DrawProjectilesSetShadow(binIt->second);
	}
}

void CProjectileDrawer::DrawProjectilesSetShadow(std::set<CProjectile*>& projectiles)
{
	for (std::set<CProjectile*>::iterator setIt = projectiles.begin(); setIt != projectiles.end(); ++setIt) {
		DrawProjectileShadow(*setIt);
	}
}

void CProjectileDrawer::DrawProjectileShadow(CProjectile* p)
{
	const CUnit* owner = p->owner();

	if ((gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam) ||
		(owner && teamHandler->Ally(owner->allyteam, gu->myAllyTeam)))) {

		// if this returns false, then projectile is
		// neither weapon nor piece, or has no model
		if (DrawProjectileModel(p, true))
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
	GML_RECMUTEX_LOCK(proj); // DrawProjectilesMiniMap

	typedef std::set<CProjectile*> ProjectileSet;
	typedef std::set<CProjectile*>::const_iterator ProjectileSetIt;
	typedef std::map<int, ProjectileSet> ProjectileBin;
	typedef std::map<int, ProjectileSet>::const_iterator ProjectileBinIt;

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		const ProjectileBin& projectileBin = modelRenderers[modelType]->GetProjectileBin();

		if (!projectileBin.empty()) {

			for (ProjectileBinIt binIt = projectileBin.begin(); binIt != projectileBin.end(); ++binIt) {
				CVertexArray* lines = GetVertexArray();
				CVertexArray* points = GetVertexArray();

				lines->Initialize();
				lines->EnlargeArrays((binIt->second).size() * 2, 0, VA_SIZE_C);
				points->Initialize();
				points->EnlargeArrays((binIt->second).size(), 0, VA_SIZE_C);

				for (ProjectileSetIt setIt = (binIt->second).begin(); setIt != (binIt->second).end(); ++setIt) {
					CProjectile* p = *setIt;

					CUnit *owner = p->owner();
					if ((owner && (owner->allyteam == gu->myAllyTeam)) ||
						gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam)) {
							p->DrawOnMinimap(*lines, *points);
					}
				}

				lines->DrawArrayC(GL_LINES);
				points->DrawArrayC(GL_POINTS);
			}

		}
	}

	if (!renderProjectiles.empty()) {
		CVertexArray* lines = GetVertexArray();
		CVertexArray* points = GetVertexArray();

		lines->Initialize();
		lines->EnlargeArrays(renderProjectiles.size() * 2, 0, VA_SIZE_C);

		points->Initialize();
		points->EnlargeArrays(renderProjectiles.size(), 0, VA_SIZE_C);

		for (std::set<CProjectile*>::iterator it = renderProjectiles.begin(); it != renderProjectiles.end(); ++it) {
			CProjectile* p = *it;

			const CUnit* owner = p->owner();
			if ((owner && (owner->allyteam == gu->myAllyTeam)) ||
				gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam)) {
				p->DrawOnMinimap(*lines, *points);
			}
		}

		lines->DrawArrayC(GL_LINES);
		points->DrawArrayC(GL_POINTS);
	}
}

void CProjectileDrawer::DrawFlyingPieces(int modelType, int numFlyingPieces, int* drawnPieces)
{
	static FlyingPieceContainer* containers[MODELTYPE_OTHER] = {
		&projectileHandler->flyingPieces3DO,
		&projectileHandler->flyingPiecesS3O,
		NULL
	};

	FlyingPieceContainer* container = containers[modelType];
	FlyingPieceContainer::render_iterator fpi;

	if (container != NULL) {
		CVertexArray* va = GetVertexArray();

		va->Initialize();
		va->EnlargeArrays(numFlyingPieces * 4, 0, VA_SIZE_TN);

		size_t lastTex = -1;
		size_t lastTeam = -1;

		for (fpi = container->render_begin(); fpi != container->render_end(); ++fpi) {
			(*fpi)->Draw(&lastTeam, &lastTex, va);
		}

		(*drawnPieces) += (va->drawIndex() / 32);
		va->DrawArrayTN(GL_QUADS);
	}
}


void CProjectileDrawer::Update() {
	eventHandler.UpdateDrawProjectiles();
}


void CProjectileDrawer::Draw(bool drawReflection, bool drawRefraction) {
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);

	ISky::SetupFog();

	{
		GML_STDMUTEX_LOCK(rpiece); // Draw

		projectileHandler->flyingPieces3DO.delete_delayed();
		projectileHandler->flyingPieces3DO.add_delayed();
		projectileHandler->flyingPiecesS3O.delete_delayed();
		projectileHandler->flyingPiecesS3O.add_delayed();
	}

	zSortedProjectiles.clear();

	int numFlyingPieces = projectileHandler->flyingPieces3DO.render_size() + projectileHandler->flyingPiecesS3O.render_size();
	int drawnPieces = 0;

	Update();

	{
		GML_RECMUTEX_LOCK(proj); // Draw

		unitDrawer->SetupForUnitDrawing();

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			modelRenderers[modelType]->PushRenderState();
			DrawProjectiles(modelType, numFlyingPieces, &drawnPieces, drawReflection, drawRefraction);
			modelRenderers[modelType]->PopRenderState();
		}

		unitDrawer->CleanUpUnitDrawing();

		// z-sort the model-less projectiles
		DrawProjectilesSet(renderProjectiles, drawReflection, drawRefraction);

		projectileHandler->currentParticles = 0;
		CProjectile::inArray = false;
		CProjectile::va = GetVertexArray();
		CProjectile::va->Initialize();

		// draw the particle effects
		for (std::set<CProjectile*, ProjectileDistanceComparator>::iterator it = zSortedProjectiles.begin(); it != zSortedProjectiles.end(); ++it) {
			(*it)->Draw();
		}
	}

	glEnable(GL_BLEND);
	glDisable(GL_FOG);

	if (CProjectile::inArray) {
		// Alpha transculent particles
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_2D);
		textureAtlas->BindTexture();
		glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
		glAlphaFunc(GL_GREATER, 0.0f);
		glEnable(GL_ALPHA_TEST);
		glDepthMask(GL_FALSE);

		// note: nano-particles (CGfxProjectile instances) also
		// contribute to the count, but have their own creation
		// cutoff
		projectileHandler->currentParticles += CProjectile::DrawArray();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(GL_TRUE);

	projectileHandler->currentParticles  = int(projectileHandler->currentParticles * 0.2f);
	projectileHandler->currentParticles += int(0.2f * drawnPieces + 0.3f * numFlyingPieces);
	projectileHandler->currentParticles += (renderProjectiles.size() * 0.8f);

	// NOTE: should projectiles that have a model be counted here?
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		projectileHandler->currentParticles += (modelRenderers[modelType]->GetNumProjectiles() * 0.8f);
	}

	projectileHandler->UpdateParticleSaturation();
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
		GML_RECMUTEX_LOCK(proj); // DrawShadowPass

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

		projectileHandler->currentParticles += CProjectile::DrawArray();
	}

	po->Disable();
	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}



bool CProjectileDrawer::DrawProjectileModel(const CProjectile* p, bool shadowPass) {
	if (luaRules && luaRules->DrawProjectile(p))
		return true;
	if (!(p->weapon || p->piece) || (p->model == NULL))
		return false;

	if (p->weapon) {
		// weapon-projectile
		const CWeaponProjectile* wp = static_cast<const CWeaponProjectile*>(p);

		#define SET_TRANSFORM_VECTORS(dir)           \
			float3 rightdir, updir;                  \
                                                     \
			if (math::fabs(dir.y) < 0.95f) {         \
				rightdir = dir.cross(UpVector);      \
				rightdir.SafeANormalize();           \
			} else {                                 \
				rightdir = float3(1.0f, 0.0f, 0.0f); \
			}                                        \
                                                     \
			updir = rightdir.cross(dir);

		#define TRANSFORM_DRAW(mat)                                        \
			glPushMatrix();                                                \
				glMultMatrixf(mat);                                        \
				glCallList(wp->model->GetRootPiece()->GetDisplayListID()); \
			glPopMatrix();

		switch (wp->GetProjectileType()) {
			case WEAPON_BASE_PROJECTILE:
			case WEAPON_EXPLOSIVE_PROJECTILE: 
			case WEAPON_LASER_PROJECTILE:
			case WEAPON_TORPEDO_PROJECTILE: {
				if (!shadowPass) {
					unitDrawer->SetTeamColour(wp->GetTeamID());
				}

				float3 dir(wp->speed);
				dir.SafeANormalize();
				SET_TRANSFORM_VECTORS(dir);

				CMatrix44f transMatrix(wp->drawPos, -rightdir, updir, dir);

				TRANSFORM_DRAW(transMatrix);
			} break;

			case WEAPON_MISSILE_PROJECTILE: {
				if (!shadowPass) {
					unitDrawer->SetTeamColour(wp->GetTeamID());
				}

				SET_TRANSFORM_VECTORS(wp->dir);

				CMatrix44f transMatrix(wp->drawPos + wp->dir * wp->radius * 0.9f, -rightdir, updir, wp->dir);

				TRANSFORM_DRAW(transMatrix);
			} break;

			case WEAPON_STARBURST_PROJECTILE: {
				if (!shadowPass) {
					unitDrawer->SetTeamColour(wp->GetTeamID());
				}

				SET_TRANSFORM_VECTORS(wp->dir);

				CMatrix44f transMatrix(wp->drawPos, -rightdir, updir, wp->dir);

				TRANSFORM_DRAW(transMatrix);
			} break;

			default: {
			} break;
		}

		#undef SET_TRANSFORM_VECTORS
		#undef TRANSFORM_DRAW
	} else {
		// piece-projectile
		const CPieceProjectile* pp = static_cast<const CPieceProjectile*>(p);

		if (!shadowPass) {
			unitDrawer->SetTeamColour(pp->GetTeamID());
		}

		if (pp->alphaThreshold != 0.1f) {
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			glAlphaFunc(GL_GEQUAL, pp->alphaThreshold);
		}

		glPushMatrix();
			glTranslatef3(pp->pos);
			glRotatef(pp->spinAngle, pp->spinVec.x, pp->spinVec.y, pp->spinVec.z);
			glCallList(pp->dispList);
		glPopMatrix();

		if (pp->alphaThreshold != 0.1f) {
			glPopAttrib();
		}
	}

	return true;
}

void CProjectileDrawer::DrawGroundFlashes()
{
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

	{
		GML_STDMUTEX_LOCK(rflash); // DrawGroundFlashes

		projectileHandler->groundFlashes.delete_delayed();
		projectileHandler->groundFlashes.add_delayed();
	}

	CGroundFlash::va = GetVertexArray();
	CGroundFlash::va->Initialize();
	CGroundFlash::va->EnlargeArrays(8, 0, VA_SIZE_TC);

	GroundFlashContainer& gfc = projectileHandler->groundFlashes;
	GroundFlashContainer::render_iterator gfi;

	bool depthTest = true;
	bool depthMask = false;

	for (gfi = gfc.render_begin(); gfi != gfc.render_end(); ++gfi) {
		CGroundFlash* gf = *gfi;

		const bool los = gu->spectatingFullView || loshandler->InAirLos(gf->pos, gu->myAllyTeam);
		const bool vis = camera->InView(gf->pos, gf->size);

		if (depthTest != gf->depthTest) {
			depthTest = gf->depthTest;

			if (depthTest) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}
		}
		if (depthMask != gf->depthMask) {
			depthMask = gf->depthMask;

			if (depthMask) {
				glDepthMask(GL_TRUE);
			} else {
				glDepthMask(GL_FALSE);
			}
		}

		if ((gf->alwaysVisible || los) && vis) {
			gf->Draw();
		}

		CGroundFlash::va->DrawArrayTC(GL_QUADS);
		CGroundFlash::va->Initialize();
	}


	glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
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
	glViewport(perlintex->xstart * textureAtlas->xsize, perlintex->ystart * textureAtlas->ysize, 128, 128);

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
	float time = globalRendering->lastFrameTime * gs->speedFactor * 3;
	float speed = 1.0f;
	float size = 1.0f;

	for (int a = 0; a < 4; ++a) {
		perlinBlend[a] += time * speed;
		if (perlinBlend[a] > 1) {
			unsigned int temp = perlinTex[a * 2];
			perlinTex[a * 2    ] = perlinTex[a * 2 + 1];
			perlinTex[a * 2 + 1] = temp;

			GenerateNoiseTex(perlinTex[a * 2 + 1], 16);
			perlinBlend[a] -= 1;
		}

		float tsize = 8.0f / size;

		if (a == 0)
			glDisable(GL_BLEND);

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->CheckInitSize(4 * VA_SIZE_TC, 0);

		for (int b = 0; b < 4; ++b)
			col[b] = int((1.0f - perlinBlend[a]) * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinTex[a * 2]);
		va->AddVertexQTC(float3(0, 0, 0), 0,         0, col);
		va->AddVertexQTC(float3(0, 1, 0), 0,     tsize, col);
		va->AddVertexQTC(float3(1, 1, 0), tsize, tsize, col);
		va->AddVertexQTC(float3(1, 0, 0), tsize,     0, col);
		va->DrawArrayTC(GL_QUADS);

		if (a == 0)
			glEnable(GL_BLEND);

		va = GetVertexArray();
		va->Initialize();
		va->CheckInitSize(4 * VA_SIZE_TC, 0);

		for (int b = 0; b < 4; ++b)
			col[b] = int(perlinBlend[a] * 16 * size);

		glBindTexture(GL_TEXTURE_2D, perlinTex[a * 2 + 1]);
		va->AddVertexQTC(float3(0, 0, 0),     0,     0, col);
		va->AddVertexQTC(float3(0, 1, 0),     0, tsize, col);
		va->AddVertexQTC(float3(1, 1, 0), tsize, tsize, col);
		va->AddVertexQTC(float3(1, 0, 0), tsize,     0, col);
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

void CProjectileDrawer::GenerateNoiseTex(unsigned int tex, int size)
{
	unsigned char* mem = new unsigned char[4 * size * size];

	for (int a = 0; a < size * size; ++a) {
		const unsigned char rnd = int(std::max(0.0f, gu->RandFloat() * 555.0f - 300.0f));

		mem[a * 4 + 0] = rnd;
		mem[a * 4 + 1] = rnd;
		mem[a * 4 + 2] = rnd;
		mem[a * 4 + 3] = rnd;
	}

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	delete[] mem;
}



void CProjectileDrawer::RenderProjectileCreated(const CProjectile* p)
{
	texturehandlerS3O->UpdateDraw();

	if (GML::SimEnabled() && !GML::ShareLists() && p->model && TEX_TYPE(p) < 0)
		TEX_TYPE(p) = texturehandlerS3O->LoadS3OTextureNow(p->model);

	if (p->model) {
		modelRenderers[MDL_TYPE(p)]->AddProjectile(p);
	} else {
		renderProjectiles.insert(const_cast<CProjectile*>(p));
	}
}

void CProjectileDrawer::RenderProjectileDestroyed(const CProjectile* const p)
{
	if (p->model) {
		modelRenderers[MDL_TYPE(p)]->DelProjectile(p);
	} else {
		renderProjectiles.erase(const_cast<CProjectile*>(p));
	}
}

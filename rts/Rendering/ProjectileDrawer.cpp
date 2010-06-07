#include "StdAfx.h"

#include "ProjectileDrawer.hpp"

#include "Game/Camera.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.hpp"
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
#include "Sim/Projectiles/Unsynced/FlyingPiece.hpp"
#include "Sim/Projectiles/Unsynced/ShieldPartProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/Util.h"

bool distcmp::operator() (const CProjectile* arg1, const CProjectile* arg2) const {
	if (arg1->tempdist != arg2->tempdist) // strict ordering required
		return (arg1->tempdist > arg2->tempdist);
	return (arg1 > arg2);
}






CProjectileDrawer* projectileDrawer = NULL;

CProjectileDrawer::CProjectileDrawer(): CEventClient("[CProjectileDrawer]", 123456, false) {
	eventHandler.AddClient(this);

	PrintLoadMsg("Creating Projectile Textures");

	textureAtlas = new CTextureAtlas(2048, 2048);

	// used to block resources_map.tdf from loading textures
	std::set<std::string> blockMapTexNames;

	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	resourcesParser.Execute();

	const LuaTable rootTable = resourcesParser.GetRoot();
	const LuaTable gfxTable = rootTable.SubTable("graphics");
	const LuaTable ptTable = gfxTable.SubTable("projectileTextures");

	// add all textures in projectiletextures section
	std::map<std::string, std::string> ptex;
	ptTable.GetMap(ptex);

	for (std::map<std::string, std::string>::iterator pi = ptex.begin(); pi != ptex.end(); ++pi) {
		textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
		blockMapTexNames.insert(StringToLower(pi->first));
	}
	ptex.clear();

	// add all texture from sections within projectiletextures section
	std::vector<std::string> seclist;
	ptTable.GetKeys(seclist);

	for (size_t i = 0; i < seclist.size(); i++) {
		const LuaTable ptSubTable = ptTable.SubTable(seclist[i]);

		if (ptSubTable.IsValid()) {
			ptSubTable.GetMap(ptex);

			for (std::map<std::string, std::string>::iterator pi = ptex.begin(); pi != ptex.end(); ++pi) {
				textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
				blockMapTexNames.insert(StringToLower(pi->first));
			}
			ptex.clear();
		}
	}

	// get the smoke textures, hold the count in 'smokeCount'
	const LuaTable smokeTable = gfxTable.SubTable("smoke");
	int smokeCount;

	if (smokeTable.IsValid()) {
		for (smokeCount = 0; true; smokeCount++) {
			const std::string tex = smokeTable.GetString(smokeCount + 1, "");
			if (tex.empty()) {
				break;
			}
			const std::string texName = "bitmaps/" + tex;
			const std::string smokeName = "ismoke" + IntToString(smokeCount, "%02i");

			textureAtlas->AddTexFromFile(smokeName, texName);
			blockMapTexNames.insert(StringToLower(smokeName));
		}
	} else {
		// setup the defaults
		for (smokeCount = 0; smokeCount < 12; smokeCount++) {
			const std::string smokeNum = IntToString(smokeCount, "%02i");
			const std::string smokeName = "ismoke" + smokeNum;
			const std::string texName = "bitmaps/smoke/smoke" + smokeNum + ".tga";

			textureAtlas->AddTexFromFile(smokeName, texName);
			blockMapTexNames.insert(StringToLower(smokeName));
		}
	}

	if (smokeCount <= 0) {
		throw content_error("missing smoke textures");
	}

	char tex[128][128][4];
	for (int y = 0; y < 128; y++) { // shield
		for (int x = 0; x < 128; x++) {
			tex[y][x][0] = 70;
			tex[y][x][1] = 70;
			tex[y][x][2] = 70;
			tex[y][x][3] = 70;
		}
	}

	textureAtlas->AddTexFromMem("perlintex", 128, 128, CTextureAtlas::RGBA32, tex);
	blockMapTexNames.insert("perlintex");

	blockMapTexNames.insert("flare");
	blockMapTexNames.insert("explo");
	blockMapTexNames.insert("explofade");
	blockMapTexNames.insert("heatcloud");
	blockMapTexNames.insert("laserend");
	blockMapTexNames.insert("laserfalloff");
	blockMapTexNames.insert("randdots");
	blockMapTexNames.insert("smoketrail");
	blockMapTexNames.insert("wake");
	blockMapTexNames.insert("perlintex");
	blockMapTexNames.insert("flame");

	blockMapTexNames.insert("sbtrailtexture");
	blockMapTexNames.insert("missiletrailtexture");
	blockMapTexNames.insert("muzzleflametexture");
	blockMapTexNames.insert("repulsetexture");
	blockMapTexNames.insert("dguntexture");
	blockMapTexNames.insert("flareprojectiletexture");
	blockMapTexNames.insert("sbflaretexture");
	blockMapTexNames.insert("missileflaretexture");
	blockMapTexNames.insert("beamlaserflaretexture");
	blockMapTexNames.insert("bubbletexture");
	blockMapTexNames.insert("geosquaretexture");
	blockMapTexNames.insert("gfxtexture");
	blockMapTexNames.insert("projectiletexture");
	blockMapTexNames.insert("repulsegfxtexture");
	blockMapTexNames.insert("sphereparttexture");
	blockMapTexNames.insert("torpedotexture");
	blockMapTexNames.insert("wrecktexture");
	blockMapTexNames.insert("plasmatexture");


	// allow map specified atlas textures for gaia unit projectiles
	LuaParser mapResParser("gamedata/resources_map.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	
	if (mapResParser.Execute()) {
		const LuaTable mapRoot = mapResParser.GetRoot();
		const LuaTable mapTable = mapRoot.SubTable("projectileTextures");

		// add all textures in projectiletextures section
		std::map<std::string, std::string>::iterator pi;

		mapTable.GetMap(ptex);

		for (pi = ptex.begin(); pi != ptex.end(); ++pi) {
			if (blockMapTexNames.find(StringToLower(pi->first)) == blockMapTexNames.end()) {
				textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
			}
		}
		ptex.clear();

		// add all texture from sections within projectiletextures section
		mapTable.GetKeys(seclist);
		for (size_t i = 0; i < seclist.size(); i++) {
			const LuaTable mapSubTable = mapTable.SubTable(seclist[i]);
			if (mapSubTable.IsValid()) {
				mapSubTable.GetMap(ptex);
				for (pi = ptex.begin(); pi != ptex.end(); ++pi) {
					if (blockMapTexNames.find(StringToLower(pi->first)) == blockMapTexNames.end()) {
						textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
					}
				}
				ptex.clear();
			}
		}
	}

	if (!textureAtlas->Finalize()) {
		logOutput.Print("Could not finalize projectile texture atlas. Use less/smaller textures.");
	}

	flaretex        = textureAtlas->GetTexturePtr("flare");
	explotex        = textureAtlas->GetTexturePtr("explo");
	explofadetex    = textureAtlas->GetTexturePtr("explofade");
	heatcloudtex    = textureAtlas->GetTexturePtr("heatcloud");
	laserendtex     = textureAtlas->GetTexturePtr("laserend");
	laserfallofftex = textureAtlas->GetTexturePtr("laserfalloff");
	randdotstex     = textureAtlas->GetTexturePtr("randdots");
	smoketrailtex   = textureAtlas->GetTexturePtr("smoketrail");
	waketex         = textureAtlas->GetTexturePtr("wake");
	perlintex       = textureAtlas->GetTexturePtr("perlintex");
	flametex        = textureAtlas->GetTexturePtr("flame");

	for (int i = 0; i < smokeCount; i++) {
		const std::string smokeName = "ismoke" + IntToString(i, "%02i");
		smoketex.push_back(textureAtlas->GetTexturePtr(smokeName));
	}

#define GETTEX(t, b) (textureAtlas->GetTexturePtrWithBackup((t), (b)))
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


	groundFXAtlas = new CTextureAtlas(2048, 2048);
	// add all textures in groundfx section
	const LuaTable groundfxTable = gfxTable.SubTable("groundfx");
	groundfxTable.GetMap(ptex);

	for (std::map<std::string, std::string>::iterator pi = ptex.begin(); pi != ptex.end(); ++pi) {
		groundFXAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
	}
	ptex.clear();

	// add all textures from sections within groundfx section
	groundfxTable.GetKeys(seclist);

	for (size_t i = 0; i < seclist.size(); i++) {
		const LuaTable gfxSubTable = groundfxTable.SubTable(seclist[i]);

		if (gfxSubTable.IsValid()) {
			gfxSubTable.GetMap(ptex);

			for (std::map<std::string, std::string>::iterator pi = ptex.begin(); pi != ptex.end(); ++pi) {
				groundFXAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
			}
			ptex.clear();
		}
	}

	if (!groundFXAtlas->Finalize()) {
		logOutput.Print("Could not finalize groundFX texture atlas. Use less/smaller textures.");
	}

	groundflashtex = groundFXAtlas->GetTexturePtr("groundflash");
	groundringtex = groundFXAtlas->GetTexturePtr("groundring");
	seismictex = groundFXAtlas->GetTexturePtr("seismic");

	for (int a = 0; a < 4; ++a) {
		perlinBlend[a]=0;
	}

	unsigned char tempmem[4 * 16 * 16];
	for (int a = 0; a < 4 * 16 * 16; ++a) {
		tempmem[a] = 0;
	}
	for (int a = 0; a < 8; ++a) {
		glGenTextures(1, &perlinTex[a]);
		glBindTexture(GL_TEXTURE_2D, perlinTex[a]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16,16, 0, GL_RGBA, GL_UNSIGNED_BYTE, tempmem);
	}

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



void CProjectileDrawer::LoadWeaponTextures() {
	// post-process the synced weapon-defs to set unsynced fields
	// (this requires CWeaponDefHandler to have been initialized)
	for (int wid = 0; wid < weaponDefHandler->numWeaponDefs; wid++) {
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
				wd.visuals.texture1 = textureAtlas->GetTexturePtr("largebeam");
				wd.visuals.texture2 = laserendtex;
				wd.visuals.texture3 = textureAtlas->GetTexturePtr("muzzleside");
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
		if (wd.visuals.texNames[0] != "") { wd.visuals.texture1 = textureAtlas->GetTexturePtr(wd.visuals.texNames[0]); }
		if (wd.visuals.texNames[1] != "") { wd.visuals.texture2 = textureAtlas->GetTexturePtr(wd.visuals.texNames[1]); }
		if (wd.visuals.texNames[2] != "") { wd.visuals.texture3 = textureAtlas->GetTexturePtr(wd.visuals.texNames[2]); }
		if (wd.visuals.texNames[3] != "") { wd.visuals.texture4 = textureAtlas->GetTexturePtr(wd.visuals.texNames[3]); }

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
		if (modelType == MODELTYPE_S3O) {
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

	#if defined(USE_GML) && GML_ENABLE_SIM
		pro->drawPos = pro->pos + (pro->speed * ((float)globalRendering->lastFrameStart - (float)pro->lastProjUpdate) * globalRendering->weightedSpeedFactor);
	#else
		pro->drawPos = pro->pos + (pro->speed * globalRendering->timeOffset);
	#endif

	if (camera->InView(pro->pos, pro->drawRadius) && (gu->spectatingFullView || loshandler->InLos(pro, gu->myAllyTeam) ||
		(owner && teamHandler->Ally(owner->allyteam, gu->myAllyTeam)))) {

		const bool stunned = owner? owner->stunned: false;

		if (owner && stunned && dynamic_cast<CShieldPartProjectile*>(pro)) {
			// if the unit that fired this projectile is stunned and the projectile
			// forms part of a shield (ie., the unit has a CPlasmaRepulser weapon but
			// cannot fire it), prevent the projectile (shield segment) from being drawn
			//
			// also prevents shields being drawn at unit's pre-pickup position
			// (since CPlasmaRepulser::Update() is responsible for updating
			// CShieldPartProjectile::centerPos) if the unit is in a non-fireplatform
			// transport
			return;
		}

		if (drawReflection) {
			if (pro->pos.y < -pro->drawRadius) {
				return;
			}

			float dif = pro->pos.y - camera->pos.y;
			float3 zeroPos = camera->pos * (pro->pos.y / dif) + pro->pos * (-camera->pos.y / dif);

			if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z) > 3 + 0.5f * pro->drawRadius) {
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
	if ((gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam) ||
		(p->owner() && teamHandler->Ally(p->owner()->allyteam, gu->myAllyTeam)))) {

		if (!DrawProjectileModel(p, true)) {
			if (p->castShadow) {
				// don't need to z-sort particle
				// effects for the shadow pass
				p->Draw();
			}
		}
	}
}



void CProjectileDrawer::DrawProjectilesMiniMap()
{
	typedef std::set<CProjectile*> ProjectileSet;
	typedef std::set<CProjectile*>::const_iterator ProjectileSetIt;
	typedef std::map<int, ProjectileSet> ProjectileBin;
	typedef std::map<int, ProjectileSet>::const_iterator ProjectileBinIt;

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		const ProjectileBin& projectileBin = modelRenderers[modelType]->GetProjectileBin();

		if (!projectileBin.empty()) {
			CVertexArray* lines = GetVertexArray();
			CVertexArray* points = GetVertexArray();

			lines->Initialize();
			lines->EnlargeArrays(projectileBin.size() * 2, 0, VA_SIZE_C);
			points->Initialize();
			points->EnlargeArrays(projectileBin.size(), 0, VA_SIZE_C);

			for (ProjectileBinIt binIt = projectileBin.begin(); binIt != projectileBin.end(); ++binIt) {
				for (ProjectileSetIt setIt = (binIt->second).begin(); setIt != (binIt->second).end(); ++setIt) {
					CProjectile* p = *setIt;

					if ((p->owner() && (p->owner()->allyteam == gu->myAllyTeam)) ||
						gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam)) {
						p->DrawOnMinimap(*lines, *points);
					}
				}
			}

			lines->DrawArrayC(GL_LINES);
			points->DrawArrayC(GL_POINTS);
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

			if ((p->owner() && (p->owner()->allyteam == gu->myAllyTeam)) ||
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
		&ph->flyingPieces3DO,
		&ph->flyingPiecesS3O,
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
			(*fpi)->Draw(modelType, &lastTeam, &lastTex, va);
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
	glDepthMask(1);

	if (globalRendering->drawFog) {
		glEnable(GL_FOG);
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	}

	{
		GML_STDMUTEX_LOCK(rpiece); // Draw

		ph->flyingPieces3DO.delete_delayed();
		ph->flyingPieces3DO.add_delayed();
		ph->flyingPiecesS3O.delete_delayed();
		ph->flyingPiecesS3O.add_delayed();
	}

	zSortedProjectiles.clear();

	int numFlyingPieces = ph->flyingPieces3DO.render_size() + ph->flyingPiecesS3O.render_size();
	int drawnPieces = 0;

	Update();

	{
		GML_STDMUTEX_LOCK(proj); // Draw

		unitDrawer->SetupForUnitDrawing();

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			modelRenderers[modelType]->PushRenderState();
			DrawProjectiles(modelType, numFlyingPieces, &drawnPieces, drawReflection, drawRefraction);
			modelRenderers[modelType]->PopRenderState();
		}

		unitDrawer->CleanUpUnitDrawing();

		// z-sort the model-less projectiles
		DrawProjectilesSet(renderProjectiles, drawReflection, drawRefraction);

		ph->currentParticles = 0;
		CProjectile::inArray = false;
		CProjectile::va = GetVertexArray();
		CProjectile::va->Initialize();

		// draw the particle effects
		for (std::set<CProjectile*, distcmp>::iterator it = zSortedProjectiles.begin(); it != zSortedProjectiles.end(); ++it) {
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
		glDepthMask(0);

		// note: nano-particles (CGfxProjectile instances) also
		// contribute to the count, but have their own creation
		// cutoff
		ph->currentParticles += CProjectile::DrawArray();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(1);

	ph->currentParticles  = int(ph->currentParticles * 0.2f);
	ph->currentParticles += int(0.2f * drawnPieces + 0.3f * numFlyingPieces);
	ph->currentParticles += (renderProjectiles.size() * 0.8f);

	// NOTE: should projectiles that have a model be counted here?
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		ph->currentParticles += (modelRenderers[modelType]->GetNumProjectiles() * 0.8f);
	}

	ph->particleSaturation     = ph->currentParticles     / float(ph->maxParticles);
	ph->nanoParticleSaturation = ph->currentNanoParticles / float(ph->maxNanoParticles);
}

void CProjectileDrawer::DrawShadowPass(void)
{
	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE);

	glDisable(GL_TEXTURE_2D);
	po->Enable();

	CProjectile::inArray = false;
	CProjectile::va = GetVertexArray();
	CProjectile::va->Initialize();

	{
		GML_STDMUTEX_LOCK(proj); // DrawShadowPass

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

		ph->currentParticles += CProjectile::DrawArray();
	}

	po->Disable();
	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}



bool CProjectileDrawer::DrawProjectileModel(const CProjectile* p, bool shadowPass) {
	if (!(p->weapon || p->piece) || (p->model == NULL)) {
		return false;
	}

	if (p->weapon) {
		// weapon-projectile
		const CWeaponProjectile* wp = dynamic_cast<const CWeaponProjectile*>(p);

		#define SET_TRANSFORM_VECTORS()              \
			float3 rightdir, updir;                  \
                                                     \
			if (fabs(wp->dir.y) < 0.95f) {           \
				rightdir = wp->dir.cross(UpVector);  \
				rightdir.SafeANormalize();           \
			} else {                                 \
				rightdir = float3(1.0f, 0.0f, 0.0f); \
			}                                        \
                                                     \
			updir = rightdir.cross(wp->dir);

		#define TRANSFORM_DRAW(mat)                          \
			glPushMatrix();                                  \
				glMultMatrixf(mat);                          \
				glCallList(wp->model->rootobject->displist); \
			glPopMatrix();

		switch (wp->GetProjectileType()) {
			case CWeaponProjectile::WEAPON_MISSILE_PROJECTILE: {
				if (!shadowPass) {
					unitDrawer->SetTeamColour(wp->colorTeam);
				}

				SET_TRANSFORM_VECTORS();

				CMatrix44f transMatrix(wp->drawPos + wp->dir * wp->radius * 0.9f, -rightdir, updir, wp->dir);

				TRANSFORM_DRAW(transMatrix);
			} break;

			case CWeaponProjectile::WEAPON_STARBURST_PROJECTILE: {
				SET_TRANSFORM_VECTORS();

				CMatrix44f transMatrix(wp->drawPos, -rightdir, updir, wp->dir);

				TRANSFORM_DRAW(transMatrix);
			} break;

			case CWeaponProjectile::WEAPON_BASE_PROJECTILE: {
				if (!shadowPass) {
					unitDrawer->SetTeamColour(wp->colorTeam);
				}

				SET_TRANSFORM_VECTORS();

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
		const CPieceProjectile* pp = dynamic_cast<const CPieceProjectile*>(p);

		if (!shadowPass) {
			unitDrawer->SetTeamColour(pp->colorTeam);
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

		*(pp->numCallback) = 0;
	}

	return true;
}

void CProjectileDrawer::DrawGroundFlashes(void)
{
	static const GLfloat black[] = {0.0f, 0.0f, 0.0f, 0.0f};

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glActiveTexture(GL_TEXTURE0);
	groundFXAtlas->BindTexture();
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);
	glPolygonOffset(-20, -1000);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glFogfv(GL_FOG_COLOR, black);

	CGroundFlash::va = GetVertexArray();
	CGroundFlash::va->Initialize();

	{
		GML_STDMUTEX_LOCK(rflash); // DrawGroundFlashes

		ph->groundFlashes.delete_delayed();
		ph->groundFlashes.add_delayed();
	}

	CGroundFlash::va->EnlargeArrays(8 * ph->groundFlashes.render_size(), 0, VA_SIZE_TC);

	GroundFlashContainer::render_iterator gfi;
	for (gfi = ph->groundFlashes.render_begin(); gfi != ph->groundFlashes.render_end(); ++gfi) {
		if (
			((*gfi)->alwaysVisible || 
				gu->spectatingFullView ||
				loshandler->InAirLos((*gfi)->pos,gu->myAllyTeam)
			) && camera->InView((*gfi)->pos, (*gfi)->size)
		)
			(*gfi)->Draw();
	}

	CGroundFlash::va->DrawArrayTC(GL_QUADS);

	glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}



void CProjectileDrawer::UpdateTextures() {
	if (ph->numPerlinProjectiles > 0 && drawPerlinTex)
		UpdatePerlin();
}

void CProjectileDrawer::UpdatePerlin() {
	perlinFB.Bind();
	glViewport(perlintex->ixstart, perlintex->iystart, 128, 128);

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
		const unsigned char rnd = int(std::max(0.0f, gu->usRandFloat() * 555.0f - 300.0f));

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

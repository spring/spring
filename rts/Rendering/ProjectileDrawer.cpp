#include "StdAfx.h"

#include "ProjectileDrawer.hpp"

#include "Game/Camera.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.hpp"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/FlyingPiece.hpp"
#include "Sim/Projectiles/Unsynced/ShieldPartProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponDef.h"
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

CProjectileDrawer::CProjectileDrawer() {
	PrintLoadMsg("Creating Projectile Textures");

	textureAtlas = new CTextureAtlas(2048, 2048);

	// used to block resources_map.tdf from loading textures
	std::set<std::string> blockMapTexNames;

	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		logOutput.Print(resourcesParser.GetErrorLog());
	}

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

	// add all texture from sections within projectiletextures section
	std::vector<std::string> seclist;
	ptTable.GetKeys(seclist);

	for (size_t i = 0; i < seclist.size(); i++) {
		const LuaTable ptSubTable = ptTable.SubTable(seclist[i]);

		if (ptSubTable.IsValid()) {
			std::map<std::string, std::string> ptex2;
			ptSubTable.GetMap(ptex2);

			for (std::map<std::string, std::string>::iterator pi = ptex2.begin(); pi != ptex2.end(); ++pi) {
				textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
				blockMapTexNames.insert(StringToLower(pi->first));
			}
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
	if (mapResParser.IsValid()) {
		const LuaTable mapRoot = mapResParser.GetRoot();
		const LuaTable mapTable = mapRoot.SubTable("projectileTextures");

		// add all textures in projectiletextures section
		std::map<std::string, std::string> mptex;
		std::map<std::string, std::string>::iterator pi;

		mapTable.GetMap(mptex);

		for (pi = mptex.begin(); pi != mptex.end(); ++pi) {
			if (blockMapTexNames.find(StringToLower(pi->first)) == blockMapTexNames.end()) {
				textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
			}
		}

		// add all texture from sections within projectiletextures section
		mapTable.GetKeys(seclist);
		for (size_t i = 0; i < seclist.size(); i++) {
			const LuaTable mapSubTable = mapTable.SubTable(seclist[i]);
			if (mapSubTable.IsValid()) {
				std::map<std::string, std::string> ptex2;
				mapSubTable.GetMap(ptex2);
				for (std::map<std::string, std::string>::iterator pi = ptex2.begin(); pi != ptex2.end(); ++pi) {
					if (blockMapTexNames.find(StringToLower(pi->first)) == blockMapTexNames.end()) {
						textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
					}
				}
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

	// add all texture from sections within groundfx section
	groundfxTable.GetKeys(seclist);

	for (size_t i = 0; i < seclist.size(); i++) {
		const LuaTable gfxSubTable = groundfxTable.SubTable(seclist[i]);

		if (gfxSubTable.IsValid()) {
			std::map<std::string, std::string> ptex2;
			std::map<std::string, std::string>::iterator pi;

			gfxSubTable.GetMap(ptex2);

			for (pi = ptex2.begin(); pi != ptex2.end(); ++pi) {
				groundFXAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
			}
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
}

CProjectileDrawer::~CProjectileDrawer() {
	for (int a = 0; a < 8; ++a) {
		glDeleteTextures(1, &perlinTex[a]);
	}

	delete textureAtlas;
	delete groundFXAtlas;
}

void CProjectileDrawer::LoadWeaponTextures() {
	// post-process the synced weapon-defs to set unsynced fields
	// (this requires CWeaponDefHandler to have been initialized
	// via CUnitDefHandler)
	for (int wid = 0; wid < weaponDefHandler->numWeaponDefs; wid++) {
		WeaponDef& wd = weaponDefHandler->weaponDefs[wid];

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
		} else if (wd.type == "LightingCannon" || wd.type == "LightningCannon") {
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
	}
}



void CProjectileDrawer::DrawProjectiles(const ProjectileContainer& pc, bool drawReflection, bool drawRefraction) {
	for (ProjectileContainer::render_iterator pci = pc.render_begin(); pci != pc.render_end(); ++pci) {
		CProjectile* pro = *pci;
		CUnit* owner = pro->owner();

		UpdateDrawPos(pro);

		if (camera->InView(pro->pos, pro->drawRadius) && (gu->spectatingFullView || loshandler->InLos(pro, gu->myAllyTeam) ||
			(owner && teamHandler->Ally(pro->owner()->allyteam, gu->myAllyTeam)))) {

			bool stunned = owner? owner->stunned: false;

			if (owner && stunned && dynamic_cast<CShieldPartProjectile*>(pro)) {
				// if the unit that fired this projectile is stunned and the projectile
				// forms part of a shield (ie., the unit has a CPlasmaRepulser weapon but
				// cannot fire it), prevent the projectile (shield segment) from being drawn
				//
				// also prevents shields being drawn at unit's pre-pickup position
				// (since CPlasmaRepulser::Update() is responsible for updating
				// CShieldPartProjectile::centerPos) if the unit is in a non-fireplatform
				// transport
				continue;
			}

			if (drawReflection) {
				if (pro->pos.y < -pro->drawRadius)
					continue;

				float dif = pro->pos.y - camera->pos.y;
				float3 zeroPos = camera->pos * (pro->pos.y / dif) + pro->pos * (-camera->pos.y / dif);

				if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z) > 3 + 0.5f * pro->drawRadius)
					continue;
			}

			if (drawRefraction && pro->pos.y > pro->drawRadius)
				continue;

			if (pro->model) {
				if (pro->model->type == MODELTYPE_S3O) {
					unitDrawer->QueS3ODraw(pro, pro->model->textureType);
				} else {
					pro->DrawUnitPart();
				}
			}

			pro->tempdist = pro->pos.dot(camera->forward);
			distset.insert(pro);
		}
	}
}

void CProjectileDrawer::DrawProjectilesShadow(const ProjectileContainer& pc) {
	for (ProjectileContainer::render_iterator pci = pc.render_begin(); pci != pc.render_end(); ++pci) {
		CProjectile* p = *pci;

		if ((gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam) ||
			(p->owner() && teamHandler->Ally(p->owner()->allyteam, gu->myAllyTeam)))) {

			if (p->model) {
				p->DrawUnitPart();
			} else if (p->castShadow) {
				p->Draw();
			}
		}
	}
}

void CProjectileDrawer::DrawProjectilesMiniMap(const ProjectileContainer& pc) {
	if (pc.render_size() > 0) {
		CVertexArray* lines = GetVertexArray();
		CVertexArray* points = GetVertexArray();

		lines->Initialize();
		lines->EnlargeArrays(pc.render_size() * 2, 0, VA_SIZE_C);

		points->Initialize();
		points->EnlargeArrays(pc.render_size(), 0, VA_SIZE_C);

		for (ProjectileContainer::render_iterator pci = pc.render_begin(); pci != pc.render_end(); ++pci) {
			CProjectile* p = *pci;

			if ((p->owner() && (p->owner()->allyteam == gu->myAllyTeam)) ||
				gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam)) {
				p->DrawOnMinimap(*lines, *points);
			}
		}

		lines->DrawArrayC(GL_LINES);
		points->DrawArrayC(GL_POINTS);
	}
}

void CProjectileDrawer::DrawProjectilesMiniMap() {
	DrawProjectilesMiniMap(ph->syncedProjectiles);
	DrawProjectilesMiniMap(ph->unsyncedProjectiles);
}



void CProjectileDrawer::Draw(bool drawReflection, bool drawRefraction) {
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(1);

	if (gu->drawFog) {
		glEnable(GL_FOG);
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	}

	CVertexArray* va = GetVertexArray();

	/* Putting in, say, viewport culling will deserve refactoring. */

	unitDrawer->SetupForUnitDrawing();

	{
		GML_STDMUTEX_LOCK(rpiece); // Draw

		ph->flyingPieces3DO.delete_delayed();
		ph->flyingPieces3DO.add_delayed();
		ph->flyingPiecesS3O.delete_delayed();
		ph->flyingPiecesS3O.add_delayed();
	}

	size_t lasttex = 0xFFFFFFFF;
	size_t lastteam = 0xFFFFFFFF;



	const int numFlyingPieces = ph->flyingPieces3DO.render_size() + ph->flyingPiecesS3O.render_size();
	const int drawnPieces = numFlyingPieces;

	va->Initialize();
	va->EnlargeArrays(numFlyingPieces * 4, 0, VA_SIZE_TN);

	FlyingPieceContainer::render_iterator fpi;

	// S3O flying pieces
	for (fpi = ph->flyingPiecesS3O.render_begin(); fpi != ph->flyingPiecesS3O.render_end(); ++fpi) {
		const FlyingPiece* fp = *fpi;

		if (fp->texture != lasttex) {
			lasttex = fp->texture;
			if (lasttex == 0)
				break;
			va->DrawArrayTN(GL_QUADS);
			va->Initialize();
			texturehandlerS3O->SetS3oTexture(lasttex);
		}
		if (fp->team != lastteam) {
			lastteam = fp->team;
			va->DrawArrayTN(GL_QUADS);
			va->Initialize();
			unitDrawer->SetTeamColour(lastteam);
		}

		CMatrix44f m;
		m.Rotate(fp->rot, fp->rotAxis);
		const float3 interPos = fp->pos + fp->speed * gu->timeOffset;
		const SS3OVertex* verts = fp->verts;
		float3 tp, tn;

		for (int i = 0; i < 4; i++){
			tp = m.Mul(verts[i].pos);
			tn = m.Mul(verts[i].normal);
			tp += interPos;
			va->AddVertexQTN(tp, verts[i].textureX, verts[i].textureY, tn);
		}
	}

	va->DrawArrayTN(GL_QUADS);
	va->Initialize();

	unitDrawer->SetupFor3DO();

	// 3DO flying pieces
	for (fpi = ph->flyingPieces3DO.render_begin(); fpi != ph->flyingPieces3DO.render_end(); ++fpi) {
		const FlyingPiece* fp = *fpi;

		if (fp->team != lastteam) {
			lastteam = fp->team;
			va->DrawArrayTN(GL_QUADS);
			va->Initialize();
			unitDrawer->SetTeamColour(lastteam);
		}

		CMatrix44f m;
		m.Rotate(fp->rot, fp->rotAxis);
		const float3 interPos = fp->pos + fp->speed * gu->timeOffset;
		const C3DOTextureHandler::UnitTexture* tex = fp->prim->texture;

		const std::vector<S3DOVertex>& vertices    = fp->object->vertices;
		const std::vector<int>&        verticesIdx = fp->prim->vertices;

		const S3DOVertex* v = &vertices[verticesIdx[0]];

		float3 tp = m.Mul(v->pos);
		float3 tn = m.Mul(v->normal);
		tp += interPos;
		va->AddVertexQTN(tp, tex->xstart, tex->ystart, tn);

		v = &vertices[verticesIdx[1]];
		tp = m.Mul(v->pos);
		tn = m.Mul(v->normal);
		tp += interPos;
		va->AddVertexQTN(tp, tex->xend, tex->ystart, tn);

		v = &vertices[verticesIdx[2]];
		tp = m.Mul(v->pos);
		tn = m.Mul(v->normal);
		tp += interPos;
		va->AddVertexQTN(tp, tex->xend, tex->yend, tn);

		v = &vertices[verticesIdx[3]];
		tp = m.Mul(v->pos);
		tn = m.Mul(v->normal);
		tp += interPos;
		va->AddVertexQTN(tp, tex->xstart, tex->yend, tn);
	}

	va->DrawArrayTN(GL_QUADS);

	distset.clear();

	{
		GML_STDMUTEX_LOCK(rproj); // Draw

		//! batch-insert projectiles into render queue
		ph->syncedProjectiles.add_delayed();

		ph->unsyncedProjectiles.delete_delayed();
		ph->unsyncedProjectiles.add_delayed();
	}

	{
		GML_STDMUTEX_LOCK(proj); // Draw

		//! 3DO projectiles get rendered immediately here, S3O's are queued
		DrawProjectiles(ph->syncedProjectiles, drawReflection, drawRefraction);
		DrawProjectiles(ph->unsyncedProjectiles, drawReflection, drawRefraction);

		unitDrawer->CleanUp3DO();
		unitDrawer->DrawQuedS3O(); //! draw qued S3O projectiles
		unitDrawer->CleanUpUnitDrawing();

		ph->currentParticles = 0;
		CProjectile::inArray = false;
		CProjectile::va = GetVertexArray();
		CProjectile::va->Initialize();

		for (std::set<CProjectile*, distcmp>::iterator i = distset.begin(); i != distset.end(); ++i) {
			(*i)->Draw();
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
	//glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(1);

	ph->currentParticles  = int(ph->currentParticles * 0.2f);
	ph->currentParticles += int((ph->syncedProjectiles.render_size() + ph->unsyncedProjectiles.render_size()) * 0.8f);
	ph->currentParticles += (int) (0.2f * drawnPieces + 0.3f * numFlyingPieces);

	ph->particleSaturation     = ph->currentParticles     / float(ph->maxParticles);
	ph->nanoParticleSaturation = ph->currentNanoParticles / float(ph->maxNanoParticles);
}

void CProjectileDrawer::DrawShadowPass(void)
{
	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE);

	glDisable(GL_TEXTURE_2D);

	CProjectile::inArray = false;
	CProjectile::va = GetVertexArray();
	CProjectile::va->Initialize();

	{
		GML_STDMUTEX_LOCK(proj); // DrawShadowPass

		po->Enable();
		DrawProjectilesShadow(ph->syncedProjectiles);
		DrawProjectilesShadow(ph->unsyncedProjectiles);
		po->Disable();
	}

	if (CProjectile::inArray) {
		glEnable(GL_TEXTURE_2D);
		textureAtlas->BindTexture();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glAlphaFunc(GL_GREATER,0.3f);
		glEnable(GL_ALPHA_TEST);
		glShadeModel(GL_SMOOTH);

		po->Enable();
		ph->currentParticles += CProjectile::DrawArray();
		po->Disable();
	}

	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}



void CProjectileDrawer::DrawGroundFlashes(void)
{
	static GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	groundFXAtlas->BindTexture();
	glEnable(GL_TEXTURE_2D);
	glDepthMask(0);
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
		if ((*gfi)->alwaysVisible || gu->spectatingFullView ||
			loshandler->InAirLos((*gfi)->pos,gu->myAllyTeam))
			(*gfi)->Draw();
	}

	CGroundFlash::va->DrawArrayTC(GL_QUADS);

	glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}




void CProjectileDrawer::UpdateDrawPos(CProjectile* p) {
#if defined(USE_GML) && GML_ENABLE_SIM
	p->drawPos = p->pos + (p->speed * ((float)gu->lastFrameStart - (float)p->lastProjUpdate) * gu->weightedSpeedFactor);
#else
	p->drawPos = p->pos + (p->speed * gu->timeOffset);
#endif
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
	glDepthMask(0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_FOG);

	unsigned char col[4];
	float time = gu->lastFrameTime * gs->speedFactor * 3;
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
	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_TEXTURE_2D);
	glDepthMask(1);

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

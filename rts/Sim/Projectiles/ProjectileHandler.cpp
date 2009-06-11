
// ProjectileHandler.cpp: implementation of the CProjectileHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <algorithm>
#include "mmgr.h"

#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Game/Camera.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "ConfigHandler.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Unsynced/ShieldPartProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "GlobalUnsynced.h"
#include "EventHandler.h"
#include "LogOutput.h"
#include "TimeProfiler.h"
#include "creg/STL_Map.h"
#include "creg/STL_List.h"
#include "Exceptions.h"

CProjectileHandler* ph;

using namespace std;


CR_BIND_TEMPLATE(ProjectileContainer, )
CR_REG_METADATA(ProjectileContainer, (
	CR_MEMBER(cont),
	CR_POSTLOAD(PostLoad)
));
CR_BIND_TEMPLATE(GroundFlashContainer, )
CR_REG_METADATA(GroundFlashContainer, (
	CR_MEMBER(cont),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CProjectileHandler, );
CR_REG_METADATA(CProjectileHandler, (
	CR_MEMBER(projectiles),
	CR_MEMBER(weaponProjectileIDs),
	CR_MEMBER(freeIDs),
	CR_MEMBER(maxUsedID),
	CR_MEMBER(groundFlashes),
	CR_RESERVED(32),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

bool distcmp::operator() (const CProjectile *arg1, const CProjectile *arg2) const {
	if(arg1->tempdist != arg2->tempdist) // strict ordering required
		return arg1->tempdist > arg2->tempdist;
	return arg1 > arg2;
}

bool piececmp::operator() (const FlyingPiece *fp1, const FlyingPiece *fp2) const {
	if(fp1->texture != fp2->texture)
		return fp1->texture > fp2->texture;
	if(fp1->team != fp2->team)
		return fp1->team > fp2->team;
	return fp1 > fp2;
}

FlyingPiece::~FlyingPiece() {
	if (verts != NULL)
		delete [] verts;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProjectileHandler::CProjectileHandler()
{
	PrintLoadMsg("Creating projectile texture");

	maxParticles     = configHandler->Get("MaxParticles",      4000);
	maxNanoParticles = configHandler->Get("MaxNanoParticles", 10000);

	currentParticles       = 0;
	currentNanoParticles   = 0;
	particleSaturation     = 0.0f;
	nanoParticleSaturation = 0.0f;
	numPerlinProjectiles   = 0;

	// preload some IDs
	// (note that 0 is reserved for unsynced projectiles)
	for (int i = 1; i <= 12345; i++) {
		freeIDs.push_back(i);
	}
	maxUsedID = freeIDs.size();

	textureAtlas = new CTextureAtlas(2048, 2048);

	// used to block resources_map.tdf from loading textures
	set<string> blockMapTexNames;

	LuaParser resourcesParser("gamedata/resources.lua",
	                          SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		logOutput.Print(resourcesParser.GetErrorLog());
	}
	const LuaTable rootTable = resourcesParser.GetRoot();
	const LuaTable gfxTable = rootTable.SubTable("graphics");

	const LuaTable ptTable = gfxTable.SubTable("projectileTextures");
	// add all textures in projectiletextures section
	map<string, string> ptex;
	ptTable.GetMap(ptex);

	for (map<string, string>::iterator pi=ptex.begin(); pi!=ptex.end(); ++pi) {
		textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
		blockMapTexNames.insert(StringToLower(pi->first));
	}

	// add all texture from sections within projectiletextures section
	vector<string> seclist;
	ptTable.GetKeys(seclist);
	for (size_t i = 0; i < seclist.size(); i++) {
		const LuaTable ptSubTable = ptTable.SubTable(seclist[i]);
		if (ptSubTable.IsValid()) {
			map<string, string> ptex2;
			ptSubTable.GetMap(ptex2);
			for (map<string, string>::iterator pi = ptex2.begin(); pi != ptex2.end(); ++pi) {
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
			const string tex = smokeTable.GetString(smokeCount + 1, "");
			if (tex.empty()) {
				break;
			}
			const string texName = "bitmaps/" + tex;
			const string smokeName = "ismoke" + IntToString(smokeCount, "%02i");
			textureAtlas->AddTexFromFile(smokeName, texName);
			blockMapTexNames.insert(StringToLower(smokeName));
		}
	}
	else {
		// setup the defaults
		for (smokeCount = 0; smokeCount < 12; smokeCount++) {
			const string smokeNum = IntToString(smokeCount, "%02i");
			const string smokeName = "ismoke" + smokeNum;
			const string texName = "bitmaps/smoke/smoke" + smokeNum + ".tga";
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
	LuaParser mapResParser("gamedata/resources_map.lua",
	                              SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (mapResParser.IsValid()) {
		const LuaTable mapRoot = mapResParser.GetRoot();
		const LuaTable mapTable = mapRoot.SubTable("projectileTextures");
		//add all textures in projectiletextures section
		map<string, string> mptex;
		mapTable.GetMap(mptex);
		map<string, string>::iterator pi;
		for (pi = mptex.begin(); pi != mptex.end(); ++pi) {
			if (blockMapTexNames.find(StringToLower(pi->first)) == blockMapTexNames.end()) {
				textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
			}
		}
		//add all texture from sections within projectiletextures section
		mapTable.GetKeys(seclist);
		for (size_t i = 0; i < seclist.size(); i++) {
			const LuaTable mapSubTable = mapTable.SubTable(seclist[i]);
			if (mapSubTable.IsValid()) {
				map<string, string> ptex2;
				mapSubTable.GetMap(ptex2);
				for (map<string, string>::iterator pi = ptex2.begin(); pi != ptex2.end(); ++pi) {
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

	flaretex        = textureAtlas->GetTexture("flare");
	explotex        = textureAtlas->GetTexture("explo");
	explofadetex    = textureAtlas->GetTexture("explofade");
	heatcloudtex    = textureAtlas->GetTexture("heatcloud");
	laserendtex     = textureAtlas->GetTexture("laserend");
	laserfallofftex = textureAtlas->GetTexture("laserfalloff");
	randdotstex     = textureAtlas->GetTexture("randdots");
	smoketrailtex   = textureAtlas->GetTexture("smoketrail");
	waketex         = textureAtlas->GetTexture("wake");
	perlintex       = textureAtlas->GetTexture("perlintex");
	flametex        = textureAtlas->GetTexture("flame");

	for (int i = 0; i < smokeCount; i++) {
		const string smokeName = "ismoke" + IntToString(i, "%02i");
		smoketex.push_back(textureAtlas->GetTexture(smokeName));
	}

#define GETTEX(t, b) textureAtlas->GetTextureWithBackup((t), (b))
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
	//add all textures in groundfx section
	const LuaTable groundfxTable = gfxTable.SubTable("groundfx");
	groundfxTable.GetMap(ptex);
	for (map<string, string>::iterator pi = ptex.begin(); pi != ptex.end(); ++pi) {
		groundFXAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
	}
	//add all texture from sections within groundfx section
	groundfxTable.GetKeys(seclist);
	for (size_t i = 0; i < seclist.size(); i++) {
		const LuaTable gfxSubTable = groundfxTable.SubTable(seclist[i]);
		if (gfxSubTable.IsValid()) {
			map<string, string> ptex2;
			gfxSubTable.GetMap(ptex2);
			for (map<string, string>::iterator pi = ptex2.begin(); pi != ptex2.end(); ++pi) {
				groundFXAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
			}
		}
	}

	if (!groundFXAtlas->Finalize()) {
		logOutput.Print("Could not finalize groundFX texture atlas. Use less/smaller textures.");
	}

	groundflashtex = groundFXAtlas->GetTexture("groundflash");
	groundringtex = groundFXAtlas->GetTexture("groundring");
	seismictex = groundFXAtlas->GetTexture("seismic");

	if (shadowHandler->canUseShadows) {
		projectileShadowVP = LoadVertexProgram("projectileshadow.vp");
	}

	for (int a = 0; a < 4; ++a) {
		perlinBlend[a]=0;
	}

	unsigned char tempmem[4*16*16];
	for (int a = 0; a < 4 * 16 * 16; ++a) {
		tempmem[a] = 0;
	}
	for (int a = 0; a < 8; ++a) {
		glGenTextures(1, &perlinTex[a]);
		glBindTexture(GL_TEXTURE_2D, perlinTex[a]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16,16, 0, GL_RGBA, GL_UNSIGNED_BYTE, tempmem);
	}

	drawPerlinTex=false;

	if (perlinFB.IsValid()) {
		//we never refresh the full texture (just the perlin part). So we need to reload it then.
		perlinFB.reloadOnAltTab = true;

		perlinFB.Bind();
		perlinFB.AttachTexture(textureAtlas->gltex);
		drawPerlinTex=perlinFB.CheckStatus("PERLIN");
		perlinFB.Unbind();
	}
}

CProjectileHandler::~CProjectileHandler()
{
	for (int a = 0; a < 8; ++a) {
		glDeleteTextures (1, &perlinTex[a]);
	}

	if (shadowHandler->canUseShadows) {
		glSafeDeleteProgram(projectileShadowVP);
	}

	ph=0;
	delete textureAtlas;
	delete groundFXAtlas;
}

void CProjectileHandler::Serialize(creg::ISerializer *s)
{
	if (s->IsWriting ()) {
		int size = (int)projectiles.size();
		s->Serialize(&size,sizeof(int));
		for (ProjectileContainer::iterator it = projectiles.begin(); it!=projectiles.end(); ++it) {
			void **ptr = (void**)&*it;
			s->SerializeObjectPtr(ptr,(*it)->GetClass());
		}
	} else {
		int size;
		s->Serialize(&size, sizeof(int));
		projectiles.resize(size);
		for (ProjectileContainer::iterator it = projectiles.begin(); it!=projectiles.end(); ++it) {
			void **ptr = (void**)&*it;
			s->SerializeObjectPtr(ptr,0/*FIXME*/);
		}
	}
}

void CProjectileHandler::PostLoad()
{
}


void CProjectileHandler::Update()
{
	SCOPED_TIMER("Projectile handler");

	GML_UPDATE_TICKS();

	ProjectileContainer::iterator psi = projectiles.begin();
	while (psi != projectiles.end()) {
		CProjectile* p = *psi;

		if (p->deleteMe) {
			if (p->synced && p->weapon) {
				//! iterator is always valid
				ProjectileMap::iterator it = weaponProjectileIDs.find(p->id);
				const ProjectileMapPair& pp = it->second;
				eventHandler.ProjectileDestroyed(pp.first, pp.second);
				weaponProjectileIDs.erase(it);
				if (p->id != 0) {
					freeIDs.push_back(p->id);
				}
			}
			psi = projectiles.erase_delete_synced(psi);
		} else {
			p->Update();
			GML_GET_TICKS(p->lastProjUpdate);
			++psi;
		}
	}

	{
		GML_STDMUTEX_LOCK(rproj); // Update

		if(projectiles.can_delete_synced()) {
			GML_STDMUTEX_LOCK(proj); // Update

			projectiles.delete_erased_synced();
		}
		projectiles.delay_add();
	}

	GroundFlashContainer::iterator gfi = groundFlashes.begin();
	while (gfi != groundFlashes.end()) {
		CGroundFlash* gf = *gfi;

		if (!gf->Update())
			gfi = groundFlashes.erase_delete(gfi);
		else
			++gfi;
	}

	{
		GML_STDMUTEX_LOCK(rflash); // Update

		groundFlashes.delay_delete();
		groundFlashes.delay_add();
	}

	FlyingPieceContainer::iterator pti = flyingPieces.begin();
	while(pti != flyingPieces.end()) {
		FlyingPiece* p = *pti;
		p->pos     += p->speed;
		p->speed   *= 0.996f;
		p->speed.y += mapInfo->map.gravity;
		p->rot     += p->rotSpeed;

		if (p->pos.y < ground->GetApproximateHeight(p->pos.x, p->pos.z - 10))
			pti = flyingPieces.erase_delete_set(pti);
		else
			++pti;
	}

	{
		GML_STDMUTEX_LOCK(rpiece); // Update

		flyingPieces.delay_delete();
		flyingPieces.delay_add();
	}
}


void CProjectileHandler::Draw(bool drawReflection,bool drawRefraction)
{
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(1);

	CVertexArray* va=GetVertexArray();

	/* Putting in, say, viewport culling will deserve refactoring. */

	unitDrawer->SetupForUnitDrawing();

	{
		GML_STDMUTEX_LOCK(rpiece); // Draw

		flyingPieces.delete_delayed();
		flyingPieces.add_delayed();
	}

	size_t lasttex = 0xFFFFFFFF;
	size_t lastteam = 0xFFFFFFFF;
	va->Initialize();
	int numFlyingPieces = flyingPieces.render_size();
	int drawnPieces = numFlyingPieces;
	va->EnlargeArrays(numFlyingPieces * 4, 0, VA_SIZE_TN);

	FlyingPieceContainer::render_iterator fpi = flyingPieces.render_begin();
	// S3O flying pieces
	for( ; fpi != flyingPieces.render_end(); ++fpi) {
		FlyingPiece *fp = *fpi;
		if(fp->texture != lasttex) {
			lasttex = fp->texture;
			if(lasttex == 0)
				break;
			va->DrawArrayTN(GL_QUADS);
			va->Initialize();
			texturehandlerS3O->SetS3oTexture(lasttex);
		}
		if(fp->team != lastteam) {
			lastteam = fp->team;
			va->DrawArrayTN(GL_QUADS);
			va->Initialize();
			unitDrawer->SetTeamColour(lastteam);
		}
		CMatrix44f m;
		m.Rotate(fp->rot,fp->rotAxis);
		float3 interPos=fp->pos+fp->speed*gu->timeOffset;
		SS3OVertex * verts = fp->verts;
		float3 tp, tn;

		for (int i = 0; i < 4; i++){
			tp=m.Mul(verts[i].pos);
			tn=m.Mul(verts[i].normal);
			tp+=interPos;
			va->AddVertexQTN(tp,verts[i].textureX,verts[i].textureY,tn);
		}
	}

	va->DrawArrayTN(GL_QUADS);
	va->Initialize();

	unitDrawer->SetupFor3DO();

	// 3DO flying pieces
	for( ; fpi != flyingPieces.render_end(); ++fpi) {
		FlyingPiece *fp = *fpi;
		CMatrix44f m;
		m.Rotate(fp->rot,fp->rotAxis);
		float3 interPos=fp->pos+fp->speed*gu->timeOffset;
		C3DOTextureHandler::UnitTexture* tex=fp->prim->texture;
		const std::vector<S3DOVertex>& vertices    = fp->object->vertices;
		const std::vector<int>&        verticesIdx = fp->prim->vertices;

		const S3DOVertex* v=&vertices[verticesIdx[0]];
		float3 tp=m.Mul(v->pos);
		float3 tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexQTN(tp,tex->xstart,tex->ystart,tn);

		v=&vertices[verticesIdx[1]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexQTN(tp,tex->xend,tex->ystart,tn);

		v=&vertices[verticesIdx[2]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexQTN(tp,tex->xend,tex->yend,tn);

		v=&vertices[verticesIdx[3]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexQTN(tp,tex->xstart,tex->yend,tn);
	}

	va->DrawArrayTN(GL_QUADS);

	distset.clear();

	{
		GML_STDMUTEX_LOCK(rproj); // Draw

		projectiles.add_delayed();
	}

	{
		GML_STDMUTEX_LOCK(proj); // Draw

		// Projectiles (3do's get rendered, s3o qued)
		
		for(ProjectileContainer::render_iterator psi = projectiles.render_begin(); psi != projectiles.render_end(); ++psi) {
			CProjectile* pro = *psi;

			pro->UpdateDrawPos();

			if (camera->InView(pro->pos, pro->drawRadius) && (gu->spectatingFullView || loshandler->InLos(pro, gu->myAllyTeam) ||
				(pro->owner() && teamHandler->Ally(pro->owner()->allyteam, gu->myAllyTeam)))) {

				CUnit* owner = pro->owner();
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

				if (pro->s3domodel) {
					if (pro->s3domodel->type == MODELTYPE_S3O) {
						unitDrawer->QueS3ODraw(pro, pro->s3domodel->textureType);
					} else {
						pro->DrawUnitPart();
					}
				}

				pro->tempdist = pro->pos.dot(camera->forward);
				distset.insert(pro);
			}
		}

		unitDrawer->CleanUp3DO();
		unitDrawer->DrawQuedS3O(); //! draw qued S3O projectiles
		unitDrawer->CleanUpUnitDrawing();

		currentParticles = 0;
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
		glColor4f(1,1,1,0.2f);
		glAlphaFunc(GL_GREATER,0.0f);
		glEnable(GL_ALPHA_TEST);
		glDepthMask(0);

		// note: nano-particles (CGfxProjectile instances) also
		// contribute to the count, but have their own creation
		// cutoff
		currentParticles += CProjectile::DrawArray();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(1);

	currentParticles = (int) (projectiles.render_size() * 0.8f + currentParticles * 0.2f);
	currentParticles += (int) (0.2f * drawnPieces + 0.3f * numFlyingPieces);

	particleSaturation     = currentParticles     / float(maxParticles);
	nanoParticleSaturation = currentNanoParticles / float(maxNanoParticles);
}

void CProjectileHandler::DrawShadowPass(void)
{
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, projectileShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glDisable(GL_TEXTURE_2D);

	CProjectile::inArray = false;
	CProjectile::va = GetVertexArray();
	CProjectile::va->Initialize();

	{
		GML_STDMUTEX_LOCK(proj); // DrawShadowPass

		for(ProjectileContainer::render_iterator psi = projectiles.render_begin(); psi != projectiles.render_end(); ++psi) {
			CProjectile* p = *psi;
			if ((gu->spectatingFullView || loshandler->InLos(p, gu->myAllyTeam) ||
				(p->owner() && teamHandler->Ally(p->owner()->allyteam, gu->myAllyTeam)))) {
				if (p->s3domodel) {
					p->DrawUnitPart();
				} else if (p->castShadow) {
					p->Draw();
				}
			}
		}
	}

	if (CProjectile::inArray) {
		glEnable(GL_TEXTURE_2D);
		textureAtlas->BindTexture();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glAlphaFunc(GL_GREATER,0.3f);
		glEnable(GL_ALPHA_TEST);
		glShadeModel(GL_SMOOTH);

		currentParticles += CProjectile::DrawArray();
	}

	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_VERTEX_PROGRAM_ARB);
}


void CProjectileHandler::AddProjectile(CProjectile* p)
{
	projectiles.push(p);

	if (p->synced && p->weapon) {
		// <projectiles> stores both synced and unsynced projectiles,
		// only keep track of IDs of the synced ones for Lua
		int newID = 0;
		if (!freeIDs.empty()) {
			newID = freeIDs.front();
			freeIDs.pop_front();
		} else {
			maxUsedID++;
			newID = maxUsedID;
			if (maxUsedID > (1 << 24)) {
				logOutput.Print("LUA projectile IDs are now out of range");
			}
		}
		p->id = newID;
		// projectile owner can die before projectile itself
		// does, so copy the allyteam at projectile creation
		ProjectileMapPair pp(p, p->owner() ? p->owner()->allyteam : -1);
		weaponProjectileIDs[p->id] = pp;
		eventHandler.ProjectileCreated(pp.first, pp.second);
	}
}




void CProjectileHandler::CheckUnitCollisions(
	CProjectile* p,
	std::vector<CUnit*>& tempUnits,
	CUnit** endUnit,
	const float3& ppos0,
	const float3& ppos1)
{
	CollisionQuery q;

	for (CUnit** ui = &tempUnits[0]; ui != endUnit; ++ui) {
		CUnit* unit = *ui;

		const bool friendlyShot = (p->owner() && (unit->allyteam == p->owner()->allyteam));
		const bool raytraced = (unit->collisionVolume->GetTestType() == COLVOL_TEST_CONT);

		// if this unit fired this projectile or (this unit is in the
		// same allyteam as the unit that shot this projectile and we
		// are ignoring friendly collisions)
		if (p->owner() == unit || ((p->collisionFlags & COLLISION_NOFRIENDLY) && friendlyShot)) {
			continue;
		}

		if (p->collisionFlags & COLLISION_NONEUTRAL) {
			if (unit->IsNeutral()) { continue; }
		}

		if (CCollisionHandler::DetectHit(unit, ppos0, ppos1, &q)) {
			if (q.lmp != NULL) {
				unit->SetLastAttackedPiece(q.lmp, gs->frameNum);
			}

			// The current projectile <p> won't reach the raytraced surface impact
			// position until ::Update() is called (same frame). This is a problem
			// when dealing with fast low-AOE projectiles since they would do almost
			// no damage if detonated outside the collision volume. Therefore, smuggle
			// a bit with its position now (rather than rolling it back in ::Update()
			// and waiting for the next-frame CheckUnitCol(), which is problematic
			// for noExplode projectiles).

			// const float3& pimpp = (q.b0)? q.p0: q.p1;
			const float3 pimpp =
				(q.b0 && q.b1)? ( q.p0 +  q.p1) * 0.5f:
				(q.b0        )? ( q.p0 + ppos1) * 0.5f:
								(ppos0 +  q.p1) * 0.5f;

			p->pos = (raytraced)? pimpp: ppos0;
			p->Collision(unit);
			p->pos = (raytraced)? ppos0: p->pos;
			break;
		}
	}
}

void CProjectileHandler::CheckFeatureCollisions(
	CProjectile* p,
	std::vector<CFeature*>& tempFeatures,
	CFeature** endFeature,
	const float3& ppos0,
	const float3& ppos1)
{
	CollisionQuery q;

	if (p->collisionFlags & COLLISION_NOFEATURE) {
		return;
	}

	for (CFeature** fi = &tempFeatures[0]; fi != endFeature; ++fi) {
		CFeature* feature = *fi;

		const bool raytraced =
			(feature->collisionVolume &&
			feature->collisionVolume->GetTestType() == COLVOL_TEST_CONT);

		// geothermals do not have a collision volume, skip them
		if (!feature->blocking || feature->def->geoThermal) {
			continue;
		}

		if (CCollisionHandler::DetectHit(feature, ppos0, ppos1, &q)) {
			const float3 pimpp =
				(q.b0 && q.b1)? (q.p0 + q.p1) * 0.5f:
				(q.b0        )? (q.p0 + ppos1) * 0.5f:
								(ppos0 + q.p1) * 0.5f;

			p->pos = (raytraced)? pimpp: ppos0;
			p->Collision(feature);
			p->pos = (raytraced)? ppos0: p->pos;
			break;
		}
	}
}

void CProjectileHandler::CheckCollisions()
{
	static std::vector<CUnit*> tempUnits(uh->MaxUnits(), NULL);
	static std::vector<CFeature*> tempFeatures(uh->MaxUnits(), NULL);

	for (ProjectileContainer::iterator psi = projectiles.begin(); psi != projectiles.end(); ++psi) {
		CProjectile* p = (*psi);

		if (p->checkCol && !p->deleteMe) {
			const float3 ppos0 = p->pos;
			const float3 ppos1 = p->pos + p->speed;
			const float speedf = p->speed.Length();

			CUnit** endUnit = &tempUnits[0];
			CFeature** endFeature = &tempFeatures[0];

			qf->GetUnitsAndFeaturesExact(p->pos, p->radius + speedf, endUnit, endFeature);

			CheckUnitCollisions(p, tempUnits, endUnit, ppos0, ppos1);
			CheckFeatureCollisions(p, tempFeatures, endFeature, ppos0, ppos1);
		}
	}
}



void CProjectileHandler::AddGroundFlash(CGroundFlash* flash)
{
	groundFlashes.push(flash);
}


void CProjectileHandler::DrawGroundFlashes(void)
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
	glPolygonOffset(-20,-1000);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glFogfv(GL_FOG_COLOR, black);

	CGroundFlash::va=GetVertexArray();
	CGroundFlash::va->Initialize();

	{
		GML_STDMUTEX_LOCK(rflash); // DrawGroundFlashes

		groundFlashes.delete_delayed();
		groundFlashes.add_delayed();
	}

	CGroundFlash::va->EnlargeArrays(8*groundFlashes.render_size(),0,VA_SIZE_TC);

	GroundFlashContainer::render_iterator gfi;
	for(gfi = groundFlashes.render_begin(); gfi != groundFlashes.render_end(); ++gfi){
		if ((*gfi)->alwaysVisible || gu->spectatingFullView ||
			loshandler->InAirLos((*gfi)->pos,gu->myAllyTeam))
			(*gfi)->Draw();
	}

	CGroundFlash::va->DrawArrayTC(GL_QUADS);

	glFogfv(GL_FOG_COLOR,mapInfo->atmosphere.fogColor);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}


void CProjectileHandler::AddFlyingPiece(float3 pos, float3 speed, S3DOPiece* object, S3DOPrimitive* piece)
{
	FlyingPiece* fp=new FlyingPiece;
	fp->pos=pos;
	fp->speed=speed;
	fp->prim=piece;
	fp->object=object;
	fp->verts=NULL;

	fp->rotAxis=gu->usRandVector();
	fp->rotAxis.ANormalize();
	fp->rotSpeed=gu->usRandFloat()*0.1f;
	fp->rot=0;

	fp->team = 0;
	fp->texture = 0;

	flyingPieces.insert(fp);
}


void CProjectileHandler::AddFlyingPiece(int textureType, int team, float3 pos, float3 speed, SS3OVertex * verts)
{
	if(textureType <= 0)
		return; // texture 0 means 3do

	FlyingPiece* fp=new FlyingPiece;
	fp->pos=pos;
	fp->speed=speed;
	fp->prim=NULL;
	fp->object=NULL;
	fp->verts=verts;

	/* Duplicated with AddFlyingPiece. */
	fp->rotAxis=gu->usRandVector();
	fp->rotAxis.ANormalize();
	fp->rotSpeed=gu->usRandFloat()*0.1f;
	fp->rot=0;

	fp->team = team;
	fp->texture = textureType;

	flyingPieces.insert(fp);
}

void CProjectileHandler::UpdateTextures()
{
	if (numPerlinProjectiles && drawPerlinTex)
		UpdatePerlin();
}


void CProjectileHandler::UpdatePerlin()
{
	perlinFB.Bind();
	glViewport(perlintex.ixstart, perlintex.iystart, 128, 128);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,1,0,1,-1,1);
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
	float time=gu->lastFrameTime*gs->speedFactor*3;
	float speed=1;
	float size=1;
	for(int a=0;a<4;++a){
		perlinBlend[a]+=time*speed;
		if(perlinBlend[a]>1){
			unsigned int temp=perlinTex[a*2];
			perlinTex[a*2]=perlinTex[a*2+1];
			perlinTex[a*2+1]=temp;
			GenerateNoiseTex(perlinTex[a*2+1],16);
			perlinBlend[a]-=1;
		}

		float tsize=8/size;

		if(a==0)
			glDisable(GL_BLEND);

		CVertexArray* va=GetVertexArray();
		va->Initialize();
		va->CheckInitSize(4*VA_SIZE_TC,0);
		for(int b=0;b<4;++b)
			col[b]=int((1-perlinBlend[a])*16*size);
		glBindTexture(GL_TEXTURE_2D, perlinTex[a*2]);
		va->AddVertexQTC(float3(0,0,0),0,0,col);
		va->AddVertexQTC(float3(0,1,0),0,tsize,col);
		va->AddVertexQTC(float3(1,1,0),tsize,tsize,col);
		va->AddVertexQTC(float3(1,0,0),tsize,0,col);
		va->DrawArrayTC(GL_QUADS);

		if(a==0)
			glEnable(GL_BLEND);

		va=GetVertexArray();
		va->Initialize();
		va->CheckInitSize(4*VA_SIZE_TC,0);
		for(int b=0;b<4;++b)
			col[b]=int(perlinBlend[a]*16*size);
		glBindTexture(GL_TEXTURE_2D, perlinTex[a*2+1]);
		va->AddVertexQTC(float3(0,0,0),0,0,col);
		va->AddVertexQTC(float3(0,1,0),0,tsize,col);
		va->AddVertexQTC(float3(1,1,0),tsize,tsize,col);
		va->AddVertexQTC(float3(1,0,0),tsize,0,col);
		va->DrawArrayTC(GL_QUADS);

		speed*=0.6f;
		size*=2;
	}
	perlinFB.Unbind();
	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_TEXTURE_2D);
	glDepthMask(1);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void CProjectileHandler::GenerateNoiseTex(unsigned int tex,int size)
{
	unsigned char* mem=new unsigned char[4*size*size];

	for(int a=0;a<size*size;++a){
		unsigned char rnd=int(max(0.f,gu->usRandFloat()*555-300));
		mem[a*4+0]=rnd;
		mem[a*4+1]=rnd;
		mem[a*4+2]=rnd;
		mem[a*4+3]=rnd;
	}
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,size,size,GL_RGBA,GL_UNSIGNED_BYTE,mem);
	delete[] mem;
}

#include "StdAfx.h"
// ProjectileHandler.cpp: implementation of the CProjectileHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "ProjectileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Projectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "TimeProfiler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/GroundFlash.h"
#include "Sim/Misc/LosHandler.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/Textures/TextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Projectiles/Unsynced/ShieldPartProjectile.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "LogOutput.h"
#include <algorithm>
#include "Rendering/GL/IFramebuffer.h"
#include "System/FileSystem/FileHandler.h"
#include "TdfParser.h"
#include "mmgr.h"
#include "creg/STL_List.h"

CProjectileHandler* ph;
using namespace std;

CR_BIND(CProjectileHandler,);

CR_REG_METADATA(CProjectileHandler,(
				//CR_MEMBER(ps),
				CR_MEMBER(groundFlashes),
				CR_RESERVED(32),
				CR_SERIALIZER(Serialize),
				CR_POSTLOAD(PostLoad)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProjectileHandler::CProjectileHandler()
{
	PrintLoadMsg("Creating projectile texture");

	maxParticles=configHandler.GetInt("MaxParticles",4000);

	currentParticles=0;
	particleSaturation=0;
	numPerlinProjectiles=0;

	textureAtlas = SAFE_NEW CTextureAtlas(2048, 2048);

	// used to block resources_map.tdf from loading textures
	std::set<std::string> blockMapTexNames;

	TdfParser resources("gamedata/resources.tdf");
	//add all textures in projectiletextures section
	std::map<std::string,std::string> ptex = resources.GetAllValues("resources\\graphics\\projectiletextures");
	for(std::map<std::string,std::string>::iterator pi=ptex.begin(); pi!=ptex.end(); ++pi)
	{
		textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
		blockMapTexNames.insert(pi->first);
	}
	//add all texture from sections within projectiletextures section
	std::vector<std::string> seclist = resources.GetSectionList("resources\\graphics\\projectiletextures");
	for(int i=0; i<seclist.size(); i++)
	{
		std::map<std::string,std::string> ptex2 = resources.GetAllValues("resources\\graphics\\projectiletextures\\" + seclist[i]);
		for(std::map<std::string,std::string>::iterator pi=ptex2.begin(); pi!=ptex2.end(); ++pi)
		{
			textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
			blockMapTexNames.insert(pi->first);
		}
	}


	for (int i = 0; i < 12; i++) {
		char num[10];
		sprintf(num, "%02i", i);
		textureAtlas->AddTexFromFile(std::string("ismoke") + num, std::string("bitmaps/")+resources.SGetValueDef(std::string("smoke/smoke") + num +".tga",std::string("resources\\graphics\\smoke\\smoke")+num));
		blockMapTexNames.insert(std::string("ismoke") + num);
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
	CFileHandler fh("gamedata/resources_map.tdf");
	if (fh.FileExists()) {
		TdfParser resources_map("gamedata/resources_map.tdf");
		//add all textures in projectiletextures section 
		std::map<std::string,std::string> mptex =
			resources_map.GetAllValues("resources\\graphics\\projectiletextures");
		std::map<std::string,std::string>::iterator pi;
		for (pi = mptex.begin(); pi != mptex.end(); ++pi) {
			if (blockMapTexNames.find(pi->first) == blockMapTexNames.end()) {
				textureAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
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

	for (int i = 0; i < 12; i++) {
		char num[10];
		sprintf(num, "%02i", i);
		smoketex[i] = textureAtlas->GetTexture(std::string("ismoke") + num);
	}

	sbtrailtex        = textureAtlas->GetTextureWithBackup("sbtrailtexture",         "smoketrail"    );
	missiletrailtex   = textureAtlas->GetTextureWithBackup("missiletrailtexture",    "smoketrail"    );
	muzzleflametex    = textureAtlas->GetTextureWithBackup("muzzleflametexture",     "explo"         );
	repulsetex        = textureAtlas->GetTextureWithBackup("repulsetexture",         "explo"         );
	dguntex           = textureAtlas->GetTextureWithBackup("dguntexture",            "flare"         );
	flareprojectiletex= textureAtlas->GetTextureWithBackup("flareprojectiletexture", "flare"         );
	sbflaretex        = textureAtlas->GetTextureWithBackup("sbflaretexture",         "flare"         );
	missileflaretex   = textureAtlas->GetTextureWithBackup("missileflaretexture",    "flare"         );
	beamlaserflaretex = textureAtlas->GetTextureWithBackup("beamlaserflaretexture",  "flare"         );
	bubbletex         = textureAtlas->GetTextureWithBackup("bubbletexture",          "circularthingy");
	geosquaretex      = textureAtlas->GetTextureWithBackup("geosquaretexture",       "circularthingy");
	gfxtex            = textureAtlas->GetTextureWithBackup("gfxtexture",             "circularthingy");
	projectiletex     = textureAtlas->GetTextureWithBackup("projectiletexture",      "circularthingy");
	repulsegfxtex     = textureAtlas->GetTextureWithBackup("repulsegfxtexture",      "circularthingy");
	sphereparttex     = textureAtlas->GetTextureWithBackup("sphereparttexture",      "circularthingy");
	torpedotex        = textureAtlas->GetTextureWithBackup("torpedotexture",         "circularthingy");
	wrecktex          = textureAtlas->GetTextureWithBackup("wrecktexture",           "circularthingy");
	plasmatex         = textureAtlas->GetTextureWithBackup("plasmatexture",          "circularthingy");

	groundFXAtlas = SAFE_NEW CTextureAtlas(2048, 2048);
	//add all textures in groundfx section
	ptex = resources.GetAllValues("resources\\graphics\\groundfx");
	for(std::map<std::string,std::string>::iterator pi=ptex.begin(); pi!=ptex.end(); ++pi)
	{
		groundFXAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
	}
	//add all texture from sections within groundfx section
	seclist = resources.GetSectionList("resources\\graphics\\groundfx");
	for(int i=0; i<seclist.size(); i++)
	{
		std::map<std::string,std::string> ptex2 = resources.GetAllValues("resources\\graphics\\groundfx\\" + seclist[i]);
		for(std::map<std::string,std::string>::iterator pi=ptex2.begin(); pi!=ptex2.end(); ++pi)
		{
			groundFXAtlas->AddTexFromFile(pi->first, "bitmaps/" + pi->second);
		}
	}

	if (!groundFXAtlas->Finalize())
		logOutput.Print("Could not finalize groundFX texture atlas. Use less/smaller textures.");

	groundflashtex = groundFXAtlas->GetTexture("groundflash");
	groundringtex = groundFXAtlas->GetTexture("groundring");
	seismictex = groundFXAtlas->GetTexture("seismic");

	if(shadowHandler->canUseShadows){
		projectileShadowVP=LoadVertexProgram("projectileshadow.vp");
	}


	flying3doPieces = SAFE_NEW FlyingPiece_List;
	flyingPieces.push_back(flying3doPieces);

	for(int a=0;a<4;++a){
		perlinBlend[a]=0;
	}
	unsigned char tempmem[4*16*16];
	for(int a=0;a<4*16*16;++a)
		tempmem[a]=0;
	for(int a=0;a<8;++a){
		glGenTextures(1, &perlinTex[a]);
		glBindTexture(GL_TEXTURE_2D, perlinTex[a]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16,16, 0, GL_RGBA, GL_UNSIGNED_BYTE, tempmem);
	}

	drawPerlinTex=false;

	if(shadowHandler && shadowHandler->drawShadows && GLEW_EXT_framebuffer_object && !GLEW_ATI_envmap_bumpmap){	//this seems to bug on ati cards so disable it on those (just some random ati extension to detect ati cards), should be fixed by someone that actually has a ati card
		perlinFB = instantiate_fb(512, 512, FBO_NEED_COLOR);
		if (perlinFB && perlinFB->valid()){
			drawPerlinTex=true;
			perlinFB->attachTexture(textureAtlas->gltex, GL_TEXTURE_2D, FBO_ATTACH_COLOR);
			perlinFB->checkFBOStatus();
		}
	}
	else
		perlinFB = 0;
}

CProjectileHandler::~CProjectileHandler()
{
	for(int a=0;a<8;++a)
		glDeleteTextures (1, &perlinTex[a]);

	Projectile_List::iterator psi;
	for(psi=ps.begin();psi!=ps.end();++psi)
		delete *psi;
	std::vector<CGroundFlash*>::iterator gfi;
	for(gfi=groundFlashes.begin();gfi!=groundFlashes.end();++gfi)
		delete *gfi;
	distlist.clear();

	if(shadowHandler->canUseShadows){
		glSafeDeleteProgram(projectileShadowVP);
	}

	/* Also invalidates flying3doPieces and flyings3oPieces. */
	for(std::list<FlyingPiece_List*>::iterator pti=flyingPieces.begin();pti!=flyingPieces.end();++pti){
		FlyingPiece_List * fpl = *pti;
		for(std::list<FlyingPiece*>::iterator pi=fpl->begin();pi!=fpl->end();++pi){
			if ((*pi)->verts != NULL){
				delete[] ((*pi)->verts);
			}
			delete *pi;
		}
		delete fpl;
	}
	ph=0;
	delete perlinFB;
	delete textureAtlas;
	delete groundFXAtlas;
}

void CProjectileHandler::Serialize(creg::ISerializer *s)
{
	if (s->IsWriting ()) {
		int size = (int)ps.size();
		s->Serialize (&size,sizeof(int));
		for (Projectile_List::iterator it = ps.begin(); it!=ps.end(); ++it) {
			void **ptr = (void**)&*it;
			s->SerializeObjectPtr(ptr,(*it)->GetClass());
		}
	} else {
		int size;
		s->Serialize (&size, sizeof(int));
		ps.resize (size);
		for (Projectile_List::iterator it = ps.begin(); it!=ps.end(); ++it) {
			void **ptr = (void**)&*it;
			s->SerializeObjectPtr(ptr,0/*FIXME*/);
		}
	}
}

void CProjectileHandler::PostLoad()
{

}

void CProjectileHandler::SetMaxParticles(int value)
{
	maxParticles = value;
}


void CProjectileHandler::Update()
{
	SCOPED_TIMER("Projectile handler");

	Projectile_List::iterator psi=ps.begin();
	while(psi!= ps.end()){
		CProjectile *p = *psi;
		if(p->deleteMe){
			Projectile_List::iterator prev=psi++;
			ps.erase(prev);
			delete p;
		} else {
			(*psi)->Update();
			++psi;
		}
	}

	for(unsigned int i = 0; i < groundFlashes.size();)
	{
		CGroundFlash *gf = groundFlashes[i];
		if (!gf->Update ())
		{
			// swap gf with the groundflash at the end of the list, so pop_back() can be used to erase it
			if ( i < groundFlashes.size()-1 )
				std::swap (groundFlashes.back(), groundFlashes[i]);
			groundFlashes.pop_back();
			delete gf;
		} else i++;
	}

	for(std::list<FlyingPiece_List*>::iterator pti=flyingPieces.begin();pti!=flyingPieces.end();++pti){
		FlyingPiece_List * fpl = *pti;
		/* Note: nothing in the third clause of this loop. TODO Rewrite it as a while */
		for(std::list<FlyingPiece*>::iterator pi=fpl->begin();pi!=fpl->end();){
			(*pi)->pos+=(*pi)->speed;
			(*pi)->speed*=0.996f;
			(*pi)->speed.y+=mapInfo->map.gravity;
			(*pi)->rot+=(*pi)->rotSpeed;
			if((*pi)->pos.y<ground->GetApproximateHeight((*pi)->pos.x,(*pi)->pos.z)-10){
				delete *pi;
				pi=fpl->erase(pi);
			} else {
				++pi;
			}
		}
	}
}

int CompareProjDist(CProjectileHandler::projdist const &arg1, CProjectileHandler::projdist const &arg2){
	if (arg1.dist <= arg2.dist)
	   return 0;
   return 1;
}

void CProjectileHandler::Draw(bool drawReflection,bool drawRefraction)
{
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(1);

	CVertexArray* va=GetVertexArray();

	int numFlyingPieces = 0;
	int drawnPieces = 0;

	/* Putting in, say, viewport culling will deserve refactoring. */

	/* 3DO */
	unitDrawer->SetupForUnitDrawing();

	va->Initialize();
	numFlyingPieces += flying3doPieces->size();
	for(std::list<FlyingPiece*>::iterator pi=flying3doPieces->begin();pi!=flying3doPieces->end();++pi){
		CMatrix44f m;
		m.Rotate((*pi)->rot,(*pi)->rotAxis);
		float3 interPos=(*pi)->pos+(*pi)->speed*gu->timeOffset;
		CTextureHandler::UnitTexture* tex=(*pi)->prim->texture;

		S3DOVertex* v=&(*pi)->object->vertices[(*pi)->prim->vertices[0]];
		float3 tp=m.Mul(v->pos);
		float3 tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xstart,tex->ystart,tn);

		v=&(*pi)->object->vertices[(*pi)->prim->vertices[1]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xend,tex->ystart,tn);

		v=&(*pi)->object->vertices[(*pi)->prim->vertices[2]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xend,tex->yend,tn);

		v=&(*pi)->object->vertices[(*pi)->prim->vertices[3]];
		tp=m.Mul(v->pos);
		tn=m.Mul(v->normal);
		tp+=interPos;
		va->AddVertexTN(tp,tex->xstart,tex->yend,tn);
	}
	drawnPieces+=va->drawIndex/32;
	va->DrawArrayTN(GL_QUADS);

	unitDrawer->CleanUpUnitDrawing();

	/* S3O */
	unitDrawer->SetupForS3ODrawing();

	for (int textureType = 1; textureType < flyings3oPieces.size(); textureType++){
		/* TODO Skip this if there's no FlyingPieces. */

		texturehandler->SetS3oTexture(textureType);

		for (int team = 0; team < flyings3oPieces[textureType].size(); team++){
			FlyingPiece_List * fpl = flyings3oPieces[textureType][team];

			unitDrawer->SetS3OTeamColour(team);

			va->Initialize();

			numFlyingPieces += fpl->size();

			for(std::list<FlyingPiece*>::iterator pi=fpl->begin();pi!=fpl->end();++pi){
				CMatrix44f m;
				m.Rotate((*pi)->rot,(*pi)->rotAxis);
				float3 interPos=(*pi)->pos+(*pi)->speed*gu->timeOffset;

				SS3OVertex * verts = (*pi)->verts;

				float3 tp, tn;

				for (int i = 0; i < 4; i++){
					tp=m.Mul(verts[i].pos);
					tn=m.Mul(verts[i].normal);
					tp+=interPos;
					va->AddVertexTN(tp,verts[i].textureX,verts[i].textureY,tn);
				}
			}
			drawnPieces+=va->drawIndex/32;
			va->DrawArrayTN(GL_QUADS);
		}
	}

	unitDrawer->CleanUpS3ODrawing();

	/*
	 * TODO Nearly cut here.
	 */

	unitDrawer->SetupForUnitDrawing();
	Projectile_List::iterator psi;
	distlist.clear();

	for (psi = ps.begin(); psi != ps.end(); ++psi) {
		CProjectile* pro = *psi;

		if (camera->InView(pro->pos, pro->drawRadius) && (gu->spectatingFullView || loshandler->InLos(pro, gu->myAllyTeam) ||
			(pro->owner && gs->Ally(pro->owner->allyteam, gu->myAllyTeam)))) {

			CUnit* owner = pro->owner;
			CUnit* trans = owner? (CUnit*) owner->transporter: 0;
			bool stunned = owner? owner->stunned: false;

			if (owner && trans && stunned && dynamic_cast<CShieldPartProjectile*>(pro)) {
				// if the unit that fired this projectile is inside a non-firebase
				// transport (so stunned) and the projectile forms part of a shield
				// (ie., the unit has a CPlasmaRepulser weapon but cannot fire it),
				// prevent the projectile (shield segment) from being drawn at the
				// unit's pre-pickup position (since CPlasmaRepulser::Update() is
				// responsible for updating CShieldPartProjectile::centerPos)
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
				if (pro->s3domodel->textureType) {
					unitDrawer->QueS3ODraw(pro, pro->s3domodel->textureType);
				} else {
					pro->DrawUnitPart();
				}
			}

			struct projdist tmp;
			tmp.proj = pro;
			tmp.dist = pro->pos.dot(camera->forward);
			distlist.push_back(tmp);
		}
	}
	unitDrawer->CleanUpUnitDrawing();
	unitDrawer->DrawQuedS3O();

	sort(distlist.begin(), distlist.end(), CompareProjDist);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	textureAtlas->BindTexture();
	glEnable(GL_BLEND);
	glDepthMask(0);
	glColor4f(1,1,1,0.2f);
	glAlphaFunc(GL_GREATER,0.0f);
	glEnable(GL_ALPHA_TEST);
//	glFogfv(GL_FOG_COLOR,mapInfo->atmosphere.fogColor);
	glDisable(GL_FOG);

	currentParticles=0;
	CProjectile::inArray=false;
	CProjectile::va=GetVertexArray();
	CProjectile::va->Initialize();
	for(int a=0;a<distlist.size();a++){
		distlist.at(a).proj->Draw();
	}
	if(CProjectile::inArray)
		CProjectile::DrawArray();
	glDisable(GL_TEXTURE_2D);
	glDepthMask(1);
	currentParticles=(int)(ps.size()*0.8f+currentParticles*0.2f);
	currentParticles+=(int)(0.2f*drawnPieces+0.3f*numFlyingPieces);
	particleSaturation=(float)currentParticles/(float)maxParticles;
//	glFogfv(GL_FOG_COLOR,mapInfo->atmosphere.fogColor);
}

void CProjectileHandler::DrawShadowPass(void)
{
	distlist.clear();
	Projectile_List::iterator psi;
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, projectileShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glDisable(GL_TEXTURE_2D);

	for (psi = ps.begin(); psi != ps.end(); ++psi) {
		if ((gu->spectatingFullView || loshandler->InLos(*psi, gu->myAllyTeam) ||
			((*psi)->owner && gs->Ally((*psi)->owner->allyteam, gu->myAllyTeam)))) {

			if ((*psi)->s3domodel)
				(*psi)->DrawUnitPart();
			if ((*psi)->castShadow){
				struct projdist tmp;
				tmp.proj = *psi;
				distlist.push_back(tmp);
			}
		}
	}

	glEnable(GL_TEXTURE_2D);
	textureAtlas->BindTexture();
	glColor4f(1,1,1,1);
	glAlphaFunc(GL_GREATER,0.3f);
	glEnable(GL_ALPHA_TEST);
	glShadeModel(GL_SMOOTH);

	CProjectile::inArray=false;
	CProjectile::va=GetVertexArray();
	CProjectile::va->Initialize();
	for(int b=0;b<distlist.size();b++){
		distlist.at(b).proj->Draw();
	}
	if(CProjectile::inArray)
		CProjectile::DrawArray();

	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable( GL_VERTEX_PROGRAM_ARB );
}

void CProjectileHandler::AddProjectile(CProjectile* p)
{
	ps.push_back(p);
//	toBeAddedProjectile.push(p);
}



void CProjectileHandler::CheckUnitCol()
{
	Projectile_List::iterator psi;

	CUnit* tempUnits[MAX_UNITS];
	CFeature* tempFeatures[MAX_UNITS];

	for (psi = ps.begin(); psi != ps.end(); ++psi) {
		CProjectile* p = (*psi);

		if (p->checkCol && !p->deleteMe) {
			float speedf = p->speed.Length();

			CUnit** endUnit = tempUnits;
			CFeature** endFeature = tempFeatures;

			qf->GetUnitsAndFeaturesExact(p->pos, p->radius + speedf, endUnit, endFeature);


			for (CUnit** ui = tempUnits; ui != endUnit; ++ui) {
				CUnit* unit = *ui;

				// if this unit fired this projectile or (this unit is in the
				// same allyteam as the unit that shot this projectile and we
				// are ignoring friendly collisions)
				if (p->owner == unit || ((p->collisionFlags & COLLISION_NOFRIENDLY) && p->owner && (unit->allyteam == p->owner->allyteam)))
					continue;

				if (p->collisionFlags & COLLISION_NONEUTRAL) {
					if (unit->IsNeutral()) {
						continue;
					}
				}

				if (CCollisionHandler::DetectHit(unit, p->pos, p->pos + p->speed)) {
					p->Collision(unit);
					break;
				}
			}

			if (!(p->collisionFlags & COLLISION_NOFEATURE)) {
				for (CFeature** fi = tempFeatures; fi != endFeature; ++fi) {
					CFeature* feature = *fi;
					// geothermals do not have a collision volume, skip
					if (!feature->blocking || feature->def->geoThermal)
						continue;

					if (CCollisionHandler::DetectHit(feature, p->pos, p->pos + p->speed)) {
						p->Collision(feature);
						break;
					}
				}
			}
		}
	}
}



void CProjectileHandler::AddGroundFlash(CGroundFlash* flash)
{
	groundFlashes.push_back(flash);
}


void CProjectileHandler::DrawGroundFlashes(void)
{
	static GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glActiveTextureARB(GL_TEXTURE0_ARB);
//	glBindTexture(GL_TEXTURE_2D, CGroundFlash::texture);
	groundFXAtlas->BindTexture();
	glEnable(GL_TEXTURE_2D);
	glDepthMask(0);
	glPolygonOffset(-20,-1000);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glFogfv(GL_FOG_COLOR, black);

	CGroundFlash::va=GetVertexArray();
	CGroundFlash::va->Initialize();

	std::vector<CGroundFlash*>::iterator gfi;
	for(gfi=groundFlashes.begin();gfi!=groundFlashes.end();++gfi){
		if ((*gfi)->alwaysVisible || gu->spectatingFullView ||
		    loshandler->InAirLos((*gfi)->pos,gu->myAllyTeam))
			(*gfi)->Draw();
	}
	CGroundFlash::va->DrawArrayTC(GL_QUADS);

	glFogfv(GL_FOG_COLOR,mapInfo->atmosphere.fogColor);
	glDepthMask(1);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}

void CProjectileHandler::ConvertTex(unsigned char tex[512][512][4], int startx, int starty, int endx, int endy, float absorb)
{
	for(int y=starty;y<endy;++y){
		for(int x=startx;x<endx;++x){
			float alpha=tex[y][x][3];
			float mul=alpha/255.0f;
			tex[y][x][0]=(unsigned char)(mul * (float)tex[y][x][0]);
			tex[y][x][1]=(unsigned char)(mul * (float)tex[y][x][1]);
			tex[y][x][2]=(unsigned char)(mul * (float)tex[y][x][2]);
		}
	}
}


void CProjectileHandler::AddFlyingPiece(float3 pos,float3 speed,S3DO* object,S3DOPrimitive* piece)
{
	FlyingPiece* fp=new FlyingPiece;
	fp->pos=pos;
	fp->speed=speed;
	fp->prim=piece;
	fp->object=object;
	fp->verts=NULL;

	fp->rotAxis=gu->usRandVector();
	fp->rotAxis.Normalize();
	fp->rotSpeed=gu->usRandFloat()*0.1f;
	fp->rot=0;

	flying3doPieces->push_back(fp);
}

void CProjectileHandler::AddFlyingPiece(int textureType, int team, float3 pos, float3 speed, SS3OVertex * verts){
	FlyingPiece_List * pieceList = NULL;

	while(flyings3oPieces.size()<=textureType)
		flyings3oPieces.push_back(std::vector<FlyingPiece_List*>());

	while(flyings3oPieces[textureType].size()<=team){
		//logOutput.Print("Creating piece list %d %d.", textureType, flyings3oPieces[textureType].size());

		FlyingPiece_List * fpl = SAFE_NEW FlyingPiece_List;
		flyings3oPieces[textureType].push_back(fpl);
		flyingPieces.push_back(fpl);
	}

	pieceList=flyings3oPieces[textureType][team];

	FlyingPiece* fp=new FlyingPiece;
	fp->pos=pos;
	fp->speed=speed;
	fp->prim=NULL;
	fp->object=NULL;
	fp->verts=verts;

	/* Duplicated with AddFlyingPiece. */
	fp->rotAxis=gu->usRandVector();
	fp->rotAxis.Normalize();
	fp->rotSpeed=gu->usRandFloat()*0.1f;
	fp->rot=0;

	pieceList->push_back(fp);
}

void CProjectileHandler::UpdateTextures()
{
	if(numPerlinProjectiles && drawPerlinTex)
		UpdatePerlin();
/*
	if(gs->frameNum==300){
		logOutput.Print("Saving tex");
		perlinFB->select();
		unsigned char* buf=SAFE_NEW unsigned char[512*512*4];
		glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,buf);
		CBitmap b(buf,512,512);
		b.ReverseYAxis();
		b.Save("proj2.tga");
		delete[] buf;
		perlinFB->deselect();
	}*/
}

void CProjectileHandler::UpdatePerlin()
{
	perlinFB->select();
	glViewport(perlintex.ixstart,perlintex.iystart,128,128);

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
		for(int b=0;b<4;++b)
			col[b]=int((1-perlinBlend[a])*16*size);
		glBindTexture(GL_TEXTURE_2D, perlinTex[a*2]);
		va->AddVertexTC(float3(0,0,0),0,0,col);
		va->AddVertexTC(float3(0,1,0),0,tsize,col);
		va->AddVertexTC(float3(1,1,0),tsize,tsize,col);
		va->AddVertexTC(float3(1,0,0),tsize,0,col);
		va->DrawArrayTC(GL_QUADS);

		if(a==0)
			glEnable(GL_BLEND);

		va=GetVertexArray();
		va->Initialize();
		for(int b=0;b<4;++b)
			col[b]=int(perlinBlend[a]*16*size);
		glBindTexture(GL_TEXTURE_2D, perlinTex[a*2+1]);
		va->AddVertexTC(float3(0,0,0),0,0,col);
		va->AddVertexTC(float3(0,1,0),0,tsize,col);
		va->AddVertexTC(float3(1,1,0),tsize,tsize,col);
		va->AddVertexTC(float3(1,0,0),tsize,0,col);
		va->DrawArrayTC(GL_QUADS);

		speed*=0.6f;
		size*=2;
	}
/*
	glBindTexture(GL_TEXTURE_2D, CProjectile::textures[0]);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,384,256,0,0,128,128);
*/
	perlinFB->deselect();
	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glDepthMask(1);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void CProjectileHandler::GenerateNoiseTex(unsigned int tex,int size)
{
	unsigned char* mem=SAFE_NEW unsigned char[4*size*size];

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

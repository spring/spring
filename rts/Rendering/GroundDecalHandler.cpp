#include "StdAfx.h"
#include "GroundDecalHandler.h"
#include <algorithm>
#include "Rendering/Textures/Bitmap.h"
#include "GL/myGL.h"
#include "GL/VertexArray.h"
#include "Sim/Units/Unit.h"
#include "Map/Ground.h"
#include "Sim/Units/UnitDef.h"
#include "LogOutput.h"
#include "Platform/ConfigHandler.h"
#include <cctype>
#include "Game/Camera.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Map/BaseGroundDrawer.h"
#include "ShadowHandler.h"
#include "mmgr.h"

CGroundDecalHandler* groundDecals=0;
using namespace std;

CGroundDecalHandler::CGroundDecalHandler(void)
{
	drawDecals = false;
	decalLevel = configHandler.GetInt("GroundDecals",1);

	if (decalLevel == 0) {
		return;
	}

	drawDecals = true;

	unsigned char* buf=SAFE_NEW unsigned char[512*512*4];
	memset(buf,0,512*512*4);

	TdfParser tdfparser("gamedata/resources.tdf");
	LoadScar((char*)("bitmaps/"+tdfparser.SGetValueDef("scars/scar2.bmp","resources\\graphics\\scars\\scar2")).c_str(),buf,0,0);
	LoadScar((char*)("bitmaps/"+tdfparser.SGetValueDef("scars/scar3.bmp","resources\\graphics\\scars\\scar3")).c_str(),buf,256,0);
	LoadScar((char*)("bitmaps/"+tdfparser.SGetValueDef("scars/scar1.bmp","resources\\graphics\\scars\\scar1")).c_str(),buf,0,256);
	LoadScar((char*)("bitmaps/"+tdfparser.SGetValueDef("scars/scar4.bmp","resources\\graphics\\scars\\scar4")).c_str(),buf,256,256);

	glGenTextures(1, &scarTex);			
	glBindTexture(GL_TEXTURE_2D, scarTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,512, 512, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	scarFieldX=gs->mapx/32;
	scarFieldY=gs->mapy/32;
	scarField=SAFE_NEW std::set<Scar*>[scarFieldX*scarFieldY];
	
	lastTest=0;
	maxOverlap=decalLevel+1;

	delete[] buf;

	if(shadowHandler->canUseShadows){
		decalVP=LoadVertexProgram("grounddecals.vp");
		decalFP=LoadFragmentProgram("grounddecals.fp");
	}
}

CGroundDecalHandler::~CGroundDecalHandler(void)
{
	for(std::vector<TrackType*>::iterator tti=trackTypes.begin();tti!=trackTypes.end();++tti){
		for(set<UnitTrackStruct*>::iterator ti=(*tti)->tracks.begin();ti!=(*tti)->tracks.end();++ti){
			delete *ti;
		}
		glDeleteTextures (1, &(*tti)->texture);
		delete *tti;
	}
	for(std::vector<BuildingDecalType*>::iterator tti=buildingDecalTypes.begin();tti!=buildingDecalTypes.end();++tti){
		for(set<BuildingGroundDecal*>::iterator ti=(*tti)->buildingDecals.begin();ti!=(*tti)->buildingDecals.end();++ti){
			if((*ti)->owner)
				(*ti)->owner->buildingDecal=0;
			if((*ti)->gbOwner)
				(*ti)->gbOwner->decal=0;
			delete *ti;
		}
		glDeleteTextures (1, &(*tti)->texture);
		delete *tti;
	}
	for(std::list<Scar*>::iterator si=scars.begin();si!=scars.end();++si){
		delete *si;
	}
	if(decalLevel!=0){
		delete[] scarField;

		glDeleteTextures(1,&scarTex);
	}
	if(shadowHandler->canUseShadows){
		glSafeDeleteProgram(decalVP);
		glSafeDeleteProgram(decalFP);
	}
}

void CGroundDecalHandler::Draw(void)
{
	if (!drawDecals) {
		return;
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-10, -200);
	glDepthMask(0);

	unsigned char color[4];
	color[0] = 255;
	color[1] = 255;
	color[2] = 255;
	color[3] = 255;

	glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glActiveTextureARB(GL_TEXTURE0_ARB);


	if (shadowHandler && shadowHandler->drawShadows) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, decalFP);
		glEnable(GL_FRAGMENT_PROGRAM_ARB );
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, decalVP);
		glEnable(GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
		float3 ac = readmap->ambientColor * (210.0f / 255.0f);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 10, ac.x, ac.y, ac.z, 1);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 11, 0, 0, 0, readmap->shadowDensity);
		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_MODELVIEW);
	}

	// "draw" building decals
	for (std::vector<BuildingDecalType*>::iterator bdi = buildingDecalTypes.begin(); bdi != buildingDecalTypes.end(); ++bdi) {
		if (!(*bdi)->buildingDecals.empty()) {
			CVertexArray* va = GetVertexArray();
			va->Initialize();
			glBindTexture(GL_TEXTURE_2D, (*bdi)->texture);

			for (set<BuildingGroundDecal*>::iterator bi = (*bdi)->buildingDecals.begin(); bi != (*bdi)->buildingDecals.end(); ) {
				BuildingGroundDecal* decal = *bi;

				if (decal->owner && decal->owner->buildProgress >= 0) {
					decal->alpha = decal->owner->buildProgress;
				} else if (!decal->gbOwner) {
					decal->alpha -= decal->AlphaFalloff * gu->lastFrameTime * gs->speedFactor;
				}
				if (decal->alpha < 0) {
					delete decal;
					(*bdi)->buildingDecals.erase(bi++);
					continue;
				}

				if (camera->InView(decal->pos,decal->radius) && 
					(!decal->owner || (decal->owner->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_PREVLOS)) || gu->spectatingFullView))
				{
					color[3] = int(decal->alpha * 255);
					float* heightmap = readmap->GetHeightmap();
					float xts = 1.0f / decal->xsize;
					float yts = 1.0f / decal->ysize;

					for (int x = 0; x < decal->xsize; ++x) {
						int x2 = decal->posx + x;
						for (int z = 0; z < decal->ysize; ++z) {
							int z2 = decal->posy+z;
							va->AddVertexTC(float3(x2 * 8,     heightmap[(z2    ) * (gs->mapx + 1) + x2    ] + 0.2f, z2 * 8    ), (x    ) * xts, (z    ) * yts, color);
							va->AddVertexTC(float3(x2 * 8 + 8, heightmap[(z2    ) * (gs->mapx + 1) + x2 + 1] + 0.2f, z2 * 8    ), (x + 1) * xts, (z    ) * yts, color);
							va->AddVertexTC(float3(x2 * 8 + 8, heightmap[(z2 + 1) * (gs->mapx + 1) + x2 + 1] + 0.2f, z2 * 8 + 8), (x + 1) * xts, (z + 1) * yts, color);
							va->AddVertexTC(float3(x2 * 8,     heightmap[(z2 + 1) * (gs->mapx + 1) + x2    ] + 0.2f, z2 * 8 + 8), (x    ) * xts, (z + 1) * yts, color);
						}
					}
				}
				++bi;
			}
			va->DrawArrayTC(GL_QUADS);
		}
	}

	if (shadowHandler && shadowHandler->drawShadows) {
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

		glActiveTextureARB(GL_TEXTURE0_ARB);
	}


	glPolygonOffset(-10, -20);

	unsigned char color2[4];
	color2[0] = 255;
	color2[1] = 255;
	color2[2] = 255;
	color2[3] = 255;

	// "draw" unit footprints
	for (std::vector<TrackType*>::iterator tti = trackTypes.begin(); tti != trackTypes.end(); ++tti) {
		if (!(*tti)->tracks.empty()) {
			CVertexArray* va = GetVertexArray();
			va->Initialize();
			glBindTexture(GL_TEXTURE_2D, (*tti)->texture);

			for (set<UnitTrackStruct*>::iterator ti = (*tti)->tracks.begin(); ti != (*tti)->tracks.end();) {
				UnitTrackStruct* track = *ti;

				while (!track->parts.empty() && track->parts.front().creationTime < gs->frameNum - track->lifeTime) {
					track->parts.pop_front();
				}
				if (track->parts.empty()) {
					if (track->owner)
						track->owner->myTrack = 0;
					delete track;
					(*tti)->tracks.erase(ti++);
					continue;
				}

				if (camera->InView((track->parts.front().pos1 + track->parts.back().pos1) * 0.5f, track->parts.front().pos1.distance(track->parts.back().pos1) + 500)) {
					list<TrackPart>::iterator ppi = track->parts.begin();
					color2[3] = track->trackAlpha - (int) ((gs->frameNum - ppi->creationTime) * track->alphaFalloff);

					for (list<TrackPart>::iterator pi = ++track->parts.begin(); pi != track->parts.end(); ++pi) {
						color[3] = track->trackAlpha - (int) ((gs->frameNum - ppi->creationTime) * track->alphaFalloff);
						if (pi->connected) {
							va->AddVertexTC(ppi->pos1, ppi->texPos, 0, color2);
							va->AddVertexTC(ppi->pos2, ppi->texPos, 1, color2);
							va->AddVertexTC(pi->pos2, pi->texPos, 1, color);
							va->AddVertexTC(pi->pos1, pi->texPos, 0, color);
						}
						color2[3] = color[3];
						ppi = pi;
					}
				}
				++ti;
			}
			va->DrawArrayTC(GL_QUADS);
		}
	}

	glBindTexture(GL_TEXTURE_2D, scarTex);
	CVertexArray* va = GetVertexArray();
	va->Initialize();
	glPolygonOffset(-10, -400);



	// "draw" ground scars
	for (std::list<Scar*>::iterator si = scars.begin(); si != scars.end(); ) {
		Scar* scar = *si;
		if (scar->lifeTime < gs->frameNum) {
			RemoveScar(*si, false);
			si = scars.erase(si);
			continue;
		}

		if (camera->InView(scar->pos, scar->radius + 16)) {
			float3 pos = scar->pos;
			float radius = scar->radius;
			float tx = scar->texOffsetX;
			float ty = scar->texOffsetY;

			if (scar->creationTime + 10 > gs->frameNum)
				color[3] = (int) (scar->startAlpha * (gs->frameNum - scar->creationTime) * 0.1f);
			else
				color[3] = (int) (scar->startAlpha - (gs->frameNum - scar->creationTime) * scar->alphaFalloff);

			int sx = (int) max(0.f,                  (pos.x - radius) / 16.0f);
			int ex = (int) min(float(gs->hmapx - 1), (pos.x + radius) / 16.0f);
			int sz = (int) max(0.f,                  (pos.z - radius) / 16.0f);
			int ez = (int) min(float(gs->hmapy - 1), (pos.z + radius) / 16.0f);
			float* heightmap = readmap->GetHeightmap();

			for (int x = sx; x <= ex; ++x) {
				for (int z = sz; z <= ez; ++z) {
					float px1 = x * 16;
					float pz1 = z * 16;
					float px2 = px1 + 16;
					float pz2 = pz1 + 16;

					float tx1 = min(0.5f, (pos.x - px1) / (radius * 4.0f) + 0.25f);
					float tx2 = max(0.0f, (pos.x - px2) / (radius * 4.0f) + 0.25f);
					float tz1 = min(0.5f, (pos.z - pz1) / (radius * 4.0f) + 0.25f);
					float tz2 = max(0.0f, (pos.z - pz2) / (radius * 4.0f) + 0.25f);

					va->AddVertexTC(float3(px1, heightmap[(z * 2)     * (gs->mapx + 1) + x * 2    ] + 0.5f, pz1), tx1 + tx, tz1 + ty, color);
					va->AddVertexTC(float3(px2, heightmap[(z * 2)     * (gs->mapx + 1) + x * 2 + 2] + 0.5f, pz1), tx2 + tx, tz1 + ty, color);
					va->AddVertexTC(float3(px2, heightmap[(z * 2 + 2) * (gs->mapx + 1) + x * 2 + 2] + 0.5f, pz2), tx2 + tx, tz2 + ty, color);
					va->AddVertexTC(float3(px1, heightmap[(z * 2 + 2) * (gs->mapx + 1) + x * 2    ] + 0.5f, pz2), tx1 + tx, tz2 + ty, color);
				}
			}
		}
		++si;
	}
	va->DrawArrayTC(GL_QUADS);

	glDisable(GL_POLYGON_OFFSET_FILL);
//	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);
	glDisable(GL_BLEND);

	glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

void CGroundDecalHandler::Update(void)
{
}

void CGroundDecalHandler::UnitMoved(CUnit* unit)
{
	if(decalLevel==0)
		return;

	int zp=(int(unit->pos.z)/SQUARE_SIZE*2);
	int xp=(int(unit->pos.x)/SQUARE_SIZE*2);
	int mp=zp*gs->hmapx+xp;
	if(mp<0)
		mp=0;
	if(mp>=gs->mapSquares/4)
		mp=gs->mapSquares/4-1;
	if(!readmap->terrainTypes[readmap->typemap[mp]].receiveTracks)
		return;

	TrackType* type=trackTypes[unit->unitDef->trackType];
	if(!unit->myTrack){
		UnitTrackStruct* ts=SAFE_NEW UnitTrackStruct;
		ts->owner=unit;
		ts->lifeTime=(int)(30*decalLevel*unit->unitDef->trackStrength);
		ts->trackAlpha=(int)(unit->unitDef->trackStrength*25);
		ts->alphaFalloff=float(ts->trackAlpha)/float(ts->lifeTime);
		type->tracks.insert(ts);
		unit->myTrack=ts;
	}
	float3 pos=unit->pos+unit->frontdir*unit->unitDef->trackOffset;

	TrackPart tp;
	tp.pos1=pos+unit->rightdir*unit->unitDef->trackWidth*0.5f;
	tp.pos1.y=ground->GetHeight2(tp.pos1.x,tp.pos1.z);
	tp.pos2=pos-unit->rightdir*unit->unitDef->trackWidth*0.5f;
	tp.pos2.y=ground->GetHeight2(tp.pos2.x,tp.pos2.z);
	tp.creationTime=gs->frameNum;

	if(unit->myTrack->parts.empty()){
		tp.texPos=0;
		tp.connected=false;
	} else {
		tp.texPos=unit->myTrack->parts.back().texPos+tp.pos1.distance(unit->myTrack->parts.back().pos1)/unit->unitDef->trackWidth*unit->unitDef->trackStretch;
		tp.connected=unit->myTrack->parts.back().creationTime==gs->frameNum-8;
	}

	//if the unit is moving in a straight line only place marks at half the rate by replacing old ones
	if(unit->myTrack->parts.size()>1){
		list<TrackPart>::iterator pi=unit->myTrack->parts.end();
		--pi;
		list<TrackPart>::iterator pi2=pi;
		--pi;
		if(((tp.pos1+pi->pos1)*0.5f).distance(pi2->pos1)<1){
			unit->myTrack->parts.back()=tp;
			return;
		}
	}

	unit->myTrack->parts.push_back(tp);
}

void CGroundDecalHandler::RemoveUnit(CUnit* unit)
{
	if(decalLevel==0)
		return;

	if(unit->myTrack){
		unit->myTrack->owner=0;
		unit->myTrack=0;
	}
}

int CGroundDecalHandler::GetTrackType(std::string name)
{
	if(decalLevel==0)
		return 0;

	StringToLowerInPlace(name);

	int a=0;
	for(std::vector<TrackType*>::iterator ti=trackTypes.begin();ti!=trackTypes.end();++ti){
		if((*ti)->name==name){
			return a;
		}
		++a;
	}
	TrackType* tt=SAFE_NEW TrackType;
	tt->name=name;
	tt->texture=LoadTexture(name);
	trackTypes.push_back(tt);
	return trackTypes.size()-1;
}

unsigned int CGroundDecalHandler::LoadTexture(std::string name)
{
	if(name.find_first_of('.')==string::npos)
		name+=".bmp";
	if(name.find_first_of('\\')==string::npos&&name.find_first_of('/')==string::npos)
		name=string("bitmaps/tracks/")+name;

	CBitmap bm;
	if (!bm.Load(name))
		throw content_error("Could not load ground decal from file " + name);
	for(int y=0;y<bm.ysize;++y){
		for(int x=0;x<bm.xsize;++x){
			bm.mem[(y*bm.xsize+x)*4+3]=bm.mem[(y*bm.xsize+x)*4+1];
			int brighness=bm.mem[(y*bm.xsize+x)*4+0];
			bm.mem[(y*bm.xsize+x)*4+0]=(brighness*90)/255;
			bm.mem[(y*bm.xsize+x)*4+1]=(brighness*60)/255;
			bm.mem[(y*bm.xsize+x)*4+2]=(brighness*30)/255;
		}
	}
	
	return bm.CreateTexture(true);
}


void CGroundDecalHandler::AddExplosion(float3 pos, float damage, float radius)
{
	if (decalLevel == 0)
		return;

	float height = pos.y - ground->GetHeight2(pos.x, pos.z);
	if (height >= radius)
		return;

	pos.y -= height;
	radius -= height;

	if (radius < 5)
		return;

	if (damage > radius * 30)
		damage = radius * 30;

	damage *= (radius) / (radius + height);
	if (radius > damage * 0.25f)
		radius = damage * 0.25f;

	if (damage > 400)
		damage = 400 + sqrt(damage - 399);

	pos.CheckInBounds();

	Scar* s = SAFE_NEW Scar;
	s->pos = pos;
	s->radius = radius * 1.4f;
	s->creationTime = gs->frameNum;
	s->startAlpha = max(50.f, min(255.f, damage));
	float lifeTime = decalLevel * (damage) * 3;
	s->alphaFalloff = s->startAlpha / (lifeTime);
	s->lifeTime = (int) (gs->frameNum + lifeTime);
	s->texOffsetX = (gu->usRandInt() & 128)? 0: 0.5f;
	s->texOffsetY = (gu->usRandInt() & 128)? 0: 0.5f;

	s->x1 = (int) max(0.f,                  (pos.x - radius) / 16.0f    );
	s->x2 = (int) min(float(gs->hmapx - 1), (pos.x + radius) / 16.0f + 1);
	s->y1 = (int) max(0.f,                  (pos.z - radius) / 16.0f    );
	s->y2 = (int) min(float(gs->hmapy - 1), (pos.z + radius) / 16.0f + 1);

	s->basesize = (s->x2 - s->x1) * (s->y2 - s->y1);
	s->overdrawn = 0;
	s->lastTest = 0;

	TestOverlaps(s);

	int x1 = s->x1 / 16;
	int x2 = min(scarFieldX - 1, s->x2 / 16);
	int y1 = s->y1 / 16;
	int y2 = min(scarFieldY - 1, s->y2 / 16);

	for (int y = y1; y <= y2; ++y) {
		for (int x = x1; x <= x2; ++x) {
			std::set<Scar*>* quad = &scarField[y * scarFieldX + x];
			quad->insert(s);
		}
	}

	scars.push_back(s);
}

void CGroundDecalHandler::LoadScar(std::string file, unsigned char* buf, int xoffset, int yoffset)
{
	CBitmap bm;
	if (!bm.Load(file))
		throw content_error("Could not load scar from file " + file);
	for(int y=0;y<bm.ysize;++y){
		for(int x=0;x<bm.xsize;++x){
			buf[((y+yoffset)*512+x+xoffset)*4+3]=bm.mem[(y*bm.xsize+x)*4+1];
			int brighness=bm.mem[(y*bm.xsize+x)*4+0];
			buf[((y+yoffset)*512+x+xoffset)*4+0]=(brighness*90)/255;
			buf[((y+yoffset)*512+x+xoffset)*4+1]=(brighness*60)/255;
			buf[((y+yoffset)*512+x+xoffset)*4+2]=(brighness*30)/255;
		}
	}
}

int CGroundDecalHandler::OverlapSize(Scar* s1, Scar* s2)
{
	if(s1->x1>=s2->x2 || s1->x2<=s2->x1)
		return 0;
	if(s1->y1>=s2->y2 || s1->y2<=s2->y1)
		return 0;

	int xs;
	if(s1->x1<s2->x1)
		xs=s1->x2-s2->x1;
	else
		xs=s2->x2-s1->x1;

	int ys;
	if(s1->y1<s2->y1)
		ys=s1->y2-s2->y1;
	else
		ys=s2->y2-s1->y1;

	return xs*ys;
}

void CGroundDecalHandler::TestOverlaps(Scar* scar)
{
	int x1=scar->x1/16;
	int x2=min(scarFieldX-1,scar->x2/16);
	int y1=scar->y1/16;
	int y2=min(scarFieldY-1,scar->y2/16);

	++lastTest;

	for(int y=y1;y<=y2;++y){
		for(int x=x1;x<=x2;++x){
			std::set<Scar*>* quad=&scarField[y*scarFieldX+x];
			bool redoScan=false;
			do {
				redoScan=false;
				for(std::set<Scar*>::iterator si=quad->begin();si!=quad->end();++si){
					if(lastTest!=(*si)->lastTest && scar->lifeTime>=(*si)->lifeTime){
						Scar* tested=*si;
						tested->lastTest=lastTest;
						int overlap=OverlapSize(scar,tested);
						if(overlap>0 && tested->basesize>0){
							float part=overlap/tested->basesize;
							tested->overdrawn+=part;
							if(tested->overdrawn>maxOverlap){
								RemoveScar(tested,true);
								redoScan=true;
								break;
							}
						}
					}
				}
			} while(redoScan);
		}
	}
}

void CGroundDecalHandler::RemoveScar(Scar* scar,bool removeFromScars)
{
	int x1=scar->x1/16;
	int x2=min(scarFieldX-1,scar->x2/16);
	int y1=scar->y1/16;
	int y2=min(scarFieldY-1,scar->y2/16);

	for(int y=y1;y<=y2;++y){
		for(int x=x1;x<=x2;++x){
			std::set<Scar*>* quad=&scarField[y*scarFieldX+x];
			quad->erase(scar);
		}
	}
	if(removeFromScars)
		scars.remove(scar);
	delete scar;
}

void CGroundDecalHandler::AddBuilding(CBuilding* building)
{
	if(decalLevel==0)
		return;

	BuildingDecalType* type=buildingDecalTypes[building->unitDef->buildingDecalType];
	BuildingGroundDecal* decal=SAFE_NEW BuildingGroundDecal;

	int posx=int(building->pos.x/8);
	int posy=int(building->pos.z/8);
	int sizex=building->unitDef->buildingDecalSizeX;
	int sizey=building->unitDef->buildingDecalSizeY;

	decal->owner=building;
	decal->gbOwner=0;
	decal->posx=max(0,posx-sizex);
	decal->posy=max(0,posy-sizey);
	decal->xsize=min(gs->mapx-1 - decal->posx, sizex*2);
	decal->ysize=min(gs->mapy-1 - decal->posy, sizey*2);
	decal->AlphaFalloff=building->unitDef->buildingDecalDecaySpeed;
	decal->alpha=0;
	decal->pos=building->pos;
	decal->radius=sqrt((float)(sizex*sizex+sizey*sizey))*8+20;

	building->buildingDecal=decal;

	type->buildingDecals.insert(decal);
}

void CGroundDecalHandler::RemoveBuilding(CBuilding* building,CUnitDrawer::GhostBuilding* gb)
{
	BuildingGroundDecal* decal=building->buildingDecal;
	if(!decal)
		return;

	decal->owner=0;
	decal->gbOwner=gb;
	building->buildingDecal=0;
}

int CGroundDecalHandler::GetBuildingDecalType(std::string name)
{
	if(decalLevel==0)
		return 0;

	StringToLowerInPlace(name);

	int a=0;
	for(std::vector<BuildingDecalType*>::iterator bi=buildingDecalTypes.begin();bi!=buildingDecalTypes.end();++bi){
		if((*bi)->name==name){
			return a;
		}
		++a;
	}
	BuildingDecalType* tt=SAFE_NEW BuildingDecalType;
	tt->name=name;
	CBitmap bm;
	if (!bm.Load(string("unittextures/") + name))
		throw content_error("Could not load building decal from file unittextures/" + name);
	tt->texture=bm.CreateTexture(true);
	buildingDecalTypes.push_back(tt);

	return buildingDecalTypes.size()-1;
}

void CGroundDecalHandler::SetTexGen(float scalex,float scaley, float offsetx, float offsety)
{
	GLfloat plan[]={scalex,0,0,offsetx};
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGenfv(GL_S,GL_EYE_PLANE,plan);
	glEnable(GL_TEXTURE_GEN_S);
	GLfloat plan2[]={0,0,scaley,offsety};
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGenfv(GL_T,GL_EYE_PLANE,plan2);
	glEnable(GL_TEXTURE_GEN_T);
}

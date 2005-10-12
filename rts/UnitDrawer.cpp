#include "StdAfx.h"
#include "UnitDrawer.h"
#include "myGL.h"
#include "3DOParser.h"
#include "Camera.h"
#include "myMath.h"
#include "Bitmap.h"
#include "ShadowHandler.h"
#include "BaseWater.h"
#include "ConfigHandler.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "ReadMap.h"
#include "Ground.h"
#include "UnitDef.h"
#include "VertexArray.h"
#include "FartextureHandler.h"
#include "RadarHandler.h"
#include "Team.h"
#include "SelectedUnits.h"
#include "BuilderCAI.h"
#include "GameHelper.h"
#include "LosHandler.h"
#include "BaseSky.h"
#include "BFGroundDrawer.h"
//#include "mmgr.h"
#include "SDL_types.h"
#include "SDL_keysym.h"

CUnitDrawer* unitDrawer;
using namespace std;
extern Uint8 *keys;

CUnitDrawer::CUnitDrawer(void)
:	showHealthBars(true),
	updateFace(0)
{
	if(texturehandler==0)
		texturehandler=new CTextureHandler;

	unitDrawDist=configHandler.GetInt("UnitLodDist",200);

	CBitmap white;
	for(int a=0;a<4;++a)
		white.mem[a]=255;

	whiteTex=white.CreateTexture(false);

	unsigned char rt[128*128*4];

	for(int y=0;y<128;++y){
		for(int x=0;x<128;++x){
			float r=sqrtf((y-64)*(y-64)+(x-64)*(x-64))/64.0;
			if(r>1){
				rt[(y*128+x)*4+0]=0;
				rt[(y*128+x)*4+1]=0;
				rt[(y*128+x)*4+2]=0;
				rt[(y*128+x)*4+3]=0;
			} else {
				rt[(y*128+x)*4+0]=(unsigned char)(255-r*r*r*255);
				rt[(y*128+x)*4+1]=(unsigned char)(255-r*r*r*255);
				rt[(y*128+x)*4+2]=(unsigned char)(255-r*r*r*255);
				rt[(y*128+x)*4+3]=255;
			}
		}
	}
	CBitmap radar(rt,128,128/*"bitmaps\\blip.bmp"*/);
	radarBlippTex=radar.CreateTexture(true);

	unitAmbientColor=readmap->mapDefParser.GetFloat3(float3(0.4,0.4,0.4),"MAP\\LIGHT\\UnitAmbientColor");
	unitSunColor=readmap->mapDefParser.GetFloat3(float3(0.7,0.7,0.7),"MAP\\LIGHT\\UnitSunColor");

	float3 specularSunColor=readmap->mapDefParser.GetFloat3(unitSunColor,"MAP\\LIGHT\\SpecularSunColor");

	readmap->mapDefParser.GetDef(unitShadowDensity,"0.8","MAP\\LIGHT\\UnitShadowDensity");

	if(shadowHandler->drawShadows){
		unitShadowVP=LoadVertexProgram("unitshadow.vp");
		unitVP=LoadVertexProgram("unit.vp");
		unitFP=LoadFragmentProgram("unit.fp");

		glGenTextures(1,&boxtex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, boxtex);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,0);


		glGenTextures(1,&specularTex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, specularTex);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,128,float3(1,1,1),float3(0,0,-2),float3(0,-2,0),gs->sunVector,100,specularSunColor);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,128,float3(-1,1,-1),float3(0,0,2),float3(0,-2,0),gs->sunVector,100,specularSunColor);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,128,float3(-1,1,-1),float3(2,0,0),float3(0,0,2),gs->sunVector,100,specularSunColor);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,128,float3(-1,-1,1),float3(2,0,0),float3(0,0,-2),gs->sunVector,100,specularSunColor);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,128,float3(-1,1,1),float3(2,0,0),float3(0,-2,0),gs->sunVector,100,specularSunColor);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,128,float3(1,1,-1),float3(-2,0,0),float3(0,-2,0),gs->sunVector,100,specularSunColor);
	}
}

CUnitDrawer::~CUnitDrawer(void)
{
	glDeleteTextures(1,&whiteTex);
	glDeleteTextures(1,&radarBlippTex);

	if(shadowHandler->drawShadows){
		glDeleteProgramsARB( 1, &unitShadowVP );
		glDeleteProgramsARB( 1, &unitVP );
		glDeleteProgramsARB( 1, &unitFP );
	
		glDeleteTextures(1,&boxtex);
		glDeleteTextures(1,&specularTex);
	}
}

void CUnitDrawer::Update(void)
{
	while(!tempDrawUnits.empty() && tempDrawUnits.begin()->first<gs->frameNum-1){
		tempDrawUnits.erase(tempDrawUnits.begin());
	}
	while(!tempTransperentDrawUnits.empty() && tempTransperentDrawUnits.begin()->first<=gs->frameNum){
		tempTransperentDrawUnits.erase(tempTransperentDrawUnits.begin());
	}
}

extern GLfloat FogLand[]; 

void CUnitDrawer::Draw(bool drawReflection)
{
	ASSERT_UNSYNCED_MODE;
	vector<CUnit*> drawFar;
	vector<CUnit*> drawStat;
	drawCloaked.clear();
	glFogfv(GL_FOG_COLOR,FogLand);

	list<CUnit*> drawBlipp;

	SetupForUnitDrawing();

#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* excludeUnit=gu->directControl;
#endif

	for(list<CUnit*>::iterator usi=uh->activeUnits.begin();usi!=uh->activeUnits.end();++usi){
#ifdef DIRECT_CONTROL_ALLOWED
		if((*usi)==excludeUnit)
			continue;
#endif
		if(camera->InView((*usi)->midPos,(*usi)->radius+30)){
			if(gs->allies[(*usi)->allyteam][gu->myAllyTeam] || ((*usi)->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating){
				if(drawReflection){
					float3 zeroPos;
					if((*usi)->midPos.y<0){
						zeroPos=(*usi)->midPos;
					}else{
						float dif=(*usi)->midPos.y-camera->pos.y;
						zeroPos=camera->pos*((*usi)->midPos.y/dif) + (*usi)->midPos*(-camera->pos.y/dif);
					}
					if(ground->GetApproximateHeight(zeroPos.x,zeroPos.z)>(*usi)->radius){
						continue;
					}
				}
				float sqDist=((*usi)->pos-camera->pos).SqLength2D();
				float farLength=(*usi)->sqRadius*unitDrawDist*unitDrawDist;
				if(sqDist<farLength){
					if((*usi)->isCloaked){
						drawCloaked.push_back(*usi);					
					} else {
						(*usi)->Draw();
					}
				}else{
					drawFar.push_back(*usi);
				}
				if(sqDist<unitDrawDist*unitDrawDist*500 && showHealthBars){
					drawStat.push_back(*usi);
				}
			} else if(((*usi)->losStatus[gu->myAllyTeam] & LOS_PREVLOS)){
				drawCloaked.push_back(*usi);
				if(((*usi)->losStatus[gu->myAllyTeam] & LOS_INRADAR) && !((*usi)->losStatus[gu->myAllyTeam] & LOS_CONTRADAR))
					drawBlipp.push_back(*usi);
			} else if(((*usi)->losStatus[gu->myAllyTeam] & LOS_INRADAR)){
				drawBlipp.push_back(*usi);
			}
		}
	}
	for(std::multimap<int,TempDrawUnit>::iterator ti=tempDrawUnits.begin();ti!=tempDrawUnits.end();++ti){
		if(camera->InView(ti->second.pos,100)){
			glPushMatrix();
			glTranslatef3(ti->second.pos);
			glRotatef(ti->second.rot*180/PI,0,1,0);
			unit3doparser->Load3DO(ti->second.unitdef->model.modelpath,1,ti->second.team)->rootobject->DrawStatic();
			glPopMatrix();
		}
	}
//	glCullFace(GL_BACK);
//	glDisable(GL_CULL_FACE);

	CleanUpUnitDrawing();

	va=GetVertexArray();
	va->Initialize();
	glAlphaFunc(GL_GREATER,0.8f);
	glEnable(GL_ALPHA_TEST);
	glBindTexture(GL_TEXTURE_2D,fartextureHandler->farTexture);
	camNorm=camera->forward;
	camNorm.y=-0.1;
	camNorm.Normalize();
	glColor3f(1,1,1);
	glEnable(GL_FOG);
	glFogfv(GL_FOG_COLOR,FogLand);
	for(vector<CUnit*>::iterator usi=drawFar.begin();usi!=drawFar.end();usi++){
		DrawFar(*usi);
	}
	va->DrawArrayTN(GL_QUADS);

	if(!drawReflection){
		glAlphaFunc(GL_GREATER,0.5f);
		glBindTexture(GL_TEXTURE_2D,radarBlippTex);
		va=GetVertexArray();
		va->Initialize();
		for(list<CUnit*>::iterator usi=drawBlipp.begin();usi!=drawBlipp.end();usi++){
			CUnit* u=*usi;
			float3 pos=u->midPos + u->posErrorVector*radarhandler->radarErrorSize[gu->myAllyTeam];
			float h=ground->GetHeight(pos.x,pos.z);
			if(pos.y<h+15)
				pos.y=h+15;
			va->AddVertexTC(pos+camera->up*15+camera->right*15,0,0,gs->teams[u->team]->color);
			va->AddVertexTC(pos-camera->up*15+camera->right*15,1,0,gs->teams[u->team]->color);
			va->AddVertexTC(pos-camera->up*15-camera->right*15,1,1,gs->teams[u->team]->color);
			va->AddVertexTC(pos+camera->up*15-camera->right*15,0,1,gs->teams[u->team]->color);
		}
		va->DrawArrayTC(GL_QUADS);

		glDisable(GL_TEXTURE_2D);
		for(vector<CUnit*>::iterator usi=drawStat.begin();usi!=drawStat.end();usi++){
			(*usi)->DrawStats();
		}

		if(keys[SDLK_LSHIFT] && !selectedUnits.selectedUnits.empty() && (*selectedUnits.selectedUnits.begin())->unitDef->buildSpeed>0){
			for(set<CBuilderCAI*>::iterator bi=uh->builderCAIs.begin();bi!=uh->builderCAIs.end();++bi){
				if((*bi)->owner->team==gu->myTeam){
					(*bi)->DrawQuedBuildingSquares();
				}
			}
		}
	}
	glDisable(GL_TEXTURE_2D);
}

void CUnitDrawer::DrawShadowPass(void)
{
	ASSERT_UNSYNCED_MODE;
	glColor3f(1,1,1);
	glDisable(GL_TEXTURE_2D);
//	glEnable(GL_TEXTURE_2D);
//	texturehandler->SetTexture();
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, unitShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glPolygonOffset(1,1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	for(list<CUnit*>::iterator usi=uh->activeUnits.begin();usi!=uh->activeUnits.end();++usi){
		if((gs->allies[(*usi)->allyteam][gu->myAllyTeam] || ((*usi)->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating) && camera->InView((*usi)->midPos,(*usi)->radius+700)){
			float sqDist=((*usi)->pos-camera->pos).SqLength2D();
			float farLength=(*usi)->sqRadius*unitDrawDist*unitDrawDist;
			if(sqDist<farLength){
				if(!(*usi)->isCloaked){
					(*usi)->Draw();
				}
			}
		}
	}
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable( GL_VERTEX_PROGRAM_ARB );
//	glDisable(GL_TEXTURE_2D);
}

inline void CUnitDrawer::DrawFar(CUnit *unit)
{
	float3 interPos=unit->pos+unit->speed*gu->timeOffset+UpVector*unit->model->height*0.5;
	int snurr=-unit->heading+GetHeadingFromVector(camera->pos.x-unit->pos.x,camera->pos.z-unit->pos.z)+(0xffff>>4);
	if(snurr<0)
		snurr+=0xffff;
	if(snurr>0xffff)
		snurr-=0xffff;
	snurr=snurr>>13;
	float tx=(unit->model->farTextureNum%8)*(1.0/8.0)+snurr*(1.0/64);
	float ty=(unit->model->farTextureNum/8)*(1.0/64.0);
	float offset=0;
	va->AddVertexTN(interPos-(camera->up*unit->radius*1.4f-offset)+camera->right*unit->radius,tx,ty,camNorm);
	va->AddVertexTN(interPos+(camera->up*unit->radius*1.4f+offset)+camera->right*unit->radius,tx,ty+(1.0/64.0),camNorm);
	va->AddVertexTN(interPos+(camera->up*unit->radius*1.4f+offset)-camera->right*unit->radius,tx+(1.0/64.0),ty+(1.0/64.0),camNorm);
	va->AddVertexTN(interPos-(camera->up*unit->radius*1.4f-offset)-camera->right*unit->radius,tx+(1.0/64.0),ty,camNorm);
}

void CUnitDrawer::DrawCloakedUnits(void)
{
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,0.3);
	texturehandler->SetTexture();

	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

	glDepthMask(0);

	//ok these isnt really cloaked but the effect is the same
	for(std::multimap<int,TempDrawUnit>::iterator ti=tempTransperentDrawUnits.begin();ti!=tempTransperentDrawUnits.end();++ti){
		if(camera->InView(ti->second.pos,100)){
			glPushMatrix();
			glTranslatef3(ti->second.pos);
			glRotatef(ti->second.rot*180/PI,0,1,0);
			unit3doparser->Load3DO(ti->second.unitdef->model.modelpath,1,ti->second.team)->rootobject->DrawStatic();
			glPopMatrix();
		}
		if(ti->second.drawBorder){
			float3 pos=ti->second.pos;
			UnitDef *unitdef = ti->second.unitdef;

			pos=helper->Pos2BuildPos(pos,unitdef);

			float xsize=unitdef->xsize*4;
			float ysize=unitdef->ysize*4;
			glColor4f(0.2,1,0.2,0.7);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINE_STRIP);
			glVertexf3(pos+float3(xsize,1,ysize));
			glVertexf3(pos+float3(-xsize,1,ysize));
			glVertexf3(pos+float3(-xsize,1,-ysize));
			glVertexf3(pos+float3(xsize,1,-ysize));
			glVertexf3(pos+float3(xsize,1,ysize));
			glEnd();
			glColor4f(1,1,1,0.3);
			glEnable(GL_TEXTURE_2D);
		}
	}
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER,0.1f);
	glColor4f(1,1,1,0.3);
	texturehandler->SetTexture();
	glDepthMask(0);
	
	for(vector<CUnit*>::iterator ui=drawCloaked.begin();ui!=drawCloaked.end();++ui){
		if((*ui)->losStatus[gu->myAllyTeam] & LOS_INLOS){
			glColor4f(1,1,1,0.4);
			(*ui)->Draw();
		} else {
			if((*ui)->losStatus[gu->myAllyTeam] & LOS_CONTRADAR)
				glColor4f(0.9,0.9,0.9,0.5);
			else
				glColor4f(0.6,0.6,0.6,0.4);
			glPushMatrix();
			glTranslatef3((*ui)->pos);
			(*ui)->model->rootobject->DrawStatic();
			glPopMatrix();
		}
	}
	//go through the dead but still ghosted buildings
	glColor4f(0.6,0.6,0.6,0.4);
	for(std::list<GhostBuilding>::iterator gbi=ghostBuildings.begin();gbi!=ghostBuildings.end();){
		if(loshandler->InLos(gbi->pos,gu->myAllyTeam)){
			gbi=ghostBuildings.erase(gbi);
		} else {
			if(camera->InView(gbi->pos,gbi->model->radius*2)){
				glPushMatrix();
				glTranslatef3(gbi->pos);
				gbi->model->rootobject->DrawStatic();
				glPopMatrix();
			}
			++gbi;
		}
	}

	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDepthMask(1);
}

void CUnitDrawer::SetupForUnitDrawing(void)
{
	if(shadowHandler->drawShadows && !water->drawReflection){		//standard doesnt seem to support vertex program+clipplanes at once
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, unitVP );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, unitFP );
		glEnable( GL_FRAGMENT_PROGRAM_ARB );

		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, gs->sunVector.x,gs->sunVector.y,gs->sunVector.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, unitAmbientColor.x,unitAmbientColor.y,unitAmbientColor.z,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, unitSunColor.x,unitSunColor.y,unitSunColor.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, camera->pos.x, camera->pos.y, camera->pos.z, 0);

		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, 0,0,0,unitShadowDensity);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, unitAmbientColor.x,unitAmbientColor.y,unitAmbientColor.z,1);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D,shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glEnable(GL_TEXTURE_2D);
		
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		texturehandler->SetTexture();

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, boxtex);

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, specularTex);

		glActiveTextureARB(GL_TEXTURE0_ARB);

		float t[16];
		glGetFloatv(GL_MODELVIEW_MATRIX,t);

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMultMatrixf(t);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	} else {
		glEnable(GL_LIGHTING);
		glLightfv(GL_LIGHT1, GL_POSITION,gs->sunVector4);	// Position The Light
		glEnable(GL_LIGHT1);								// Enable Light One
	//	glDisable(GL_CULL_FACE);
	//	glCullFace(GL_BACK);
		glEnable(GL_TEXTURE_2D);
		float cols[]={1,1,1,1};
		float cols2[]={1,1,1,1};
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,cols);
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,cols2);
		glColor3f(1,1,1);
		texturehandler->SetTexture();
	}
//	glAlphaFunc(GL_GREATER,0.05f);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}

void CUnitDrawer::CleanUpUnitDrawing(void)
{
	if(shadowHandler->drawShadows && !water->drawReflection){
		glDisable( GL_VERTEX_PROGRAM_ARB );
		glDisable( GL_FRAGMENT_PROGRAM_ARB );

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);

		glActiveTextureARB(GL_TEXTURE0_ARB);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	} else {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT1);
	}
}


void CUnitDrawer::CreateSpecularFace(unsigned int gltype, int size, float3 baseDir, float3 xdif, float3 ydif, float3 sundir, float exponent,float3 suncolor)
{
	unsigned char* buf=new unsigned char[size*size*4];
	for(int y=0;y<size;++y){
		for(int x=0;x<size;++x){
			float3 vec=baseDir+(xdif*(x+0.5))/size+(ydif*(y+0.5))/size;
			vec.Normalize();
			float dot=vec.dot(sundir);
			if(dot<0)
				dot=0;
			float exp=min(1.,pow(dot,exponent)+pow(dot,3)*0.25);
			buf[(y*size+x)*4+0]=(unsigned char)(suncolor.x*exp*255);
			buf[(y*size+x)*4+1]=(unsigned char)(suncolor.y*exp*255);
			buf[(y*size+x)*4+2]=(unsigned char)(suncolor.z*exp*255);
			buf[(y*size+x)*4+3]=255;
		}		
	}
	glTexImage2D(gltype,0,GL_RGBA8,size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,buf);
	delete[] buf;
}

void CUnitDrawer::UpdateReflectTex(void)
{
	switch(updateFace++){
	case 0:
		CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,float3(1,0,0));
		break;
	case 1:
		CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,float3(-1,0,0));
		break;
	case 2:
		CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,float3(0,1,0));
		break;
	case 3:
		CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,float3(0,-1,0));
		break;
	case 4:
		CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,float3(0,0,1));
		break;
	case 5:
		CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,float3(0,0,-1));
		updateFace=0;
		break;
	default:
		updateFace=0;
		break;
	}
}

void CUnitDrawer::CreateReflectionFace(unsigned int gltype, float3 camdir)
{
	glViewport(0,0,128,128);

	CCamera realCam=*camera;
	camera->fov=90;
	camera->forward=camdir;
	camera->up=-UpVector;
	if(camera->forward.y==1)
		camera->up=float3(0,0,1);
	if(camera->forward.y==-1)
		camera->up=float3(0,0,-1);
//	if(camera->pos.y<ground->GetHeight(camera->pos.x,camera->pos.z)+50)
		camera->pos.y=ground->GetHeight(camera->pos.x,camera->pos.z)+50;
	camera->Update(false);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90,1,NEAR_PLANE,gu->viewRange);
	glMatrixMode(GL_MODELVIEW);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	sky->Draw();

	groundDrawer->Draw(false,true);

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, boxtex);
	glCopyTexSubImage2D(gltype,0,0,0,0,0,128,128);

	glViewport(0,0,gu->screenx,gu->screeny);

	(*camera)=realCam;
	camera->Update(false);
}

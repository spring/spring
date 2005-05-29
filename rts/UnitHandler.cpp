// UnitHandler.cpp: implementation of the CUnitHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "myGL.h"
#include "Team.h"
#include "Camera.h"
#include "TimeProfiler.h"
#include "TextureHandler.h"
#include "VertexArray.h"
#include "Camera.h"
#include "LosHandler.h"
#include "myMath.h"
#include "Ground.h"
#include "ReadMap.h"
#include "GameHelper.h"
#include "RegHandler.h"
#include "3DOParser.h"
#include "FartextureHandler.h"
#include "UnitDefHandler.h"
#include "QuadField.h"
#include "Bitmap.h"
#include "BuilderCAI.h"
#include "SelectedUnits.h"
#include "FileHandler.h"
#include "RadarHandler.h"
#include "InfoConsole.h"
#include "UnitHandler.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "ShadowHandler.h"
#include "BaseWater.h"
#include "LoadSaveInterface.h"
#include "UnitLoader.h"
#include "SyncTracer.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitHandler* uh;
using namespace std;
extern bool keys[256];

CUnitHandler::CUnitHandler()
:	overrideId(-1),
	maxUnits(500),
	lastDamageWarning(0)
{
	//unitModelLoader=new CUnit3DLoader;
	if(texturehandler==0)
		texturehandler=new CTextureHandler;

	for(int a=1;a<MAX_UNITS;a++){
		freeIDs.push_back(a);
		units[a]=0;
	}
	units[0]=0;
	unitDrawDist=regHandler.GetInt("UnitLodDist",200);

	slowUpdateIterator=activeUnits.end();

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
				rt[(y*128+x)*4+0]=255-(unsigned char) (r*r*r*255);
				rt[(y*128+x)*4+1]=255-(unsigned char) (r*r*r*255);
				rt[(y*128+x)*4+2]=255-(unsigned char) (r*r*r*255);
				rt[(y*128+x)*4+3]=255;
			}
		}
	}
	CBitmap radar(rt,128,128);
	radarBlippTex=radar.CreateTexture(true);

	unitAmbientColor=readmap->mapDefParser.GetFloat3(float3(0.4,0.4,0.4),"MAP\\LIGHT\\UnitAmbientColor");
	unitSunColor=readmap->mapDefParser.GetFloat3(float3(0.7,0.7,0.7),"MAP\\LIGHT\\UnitSunColor");
	readmap->mapDefParser.GetDef(unitShadowDensity,"0.8","MAP\\LIGHT\\UnitShadowDensity");
	readmap->mapDefParser.GetDef(maxUnits,"500","MAP\\MaxUnits");
	if(maxUnits>MAX_UNITS/gs->activeTeams-5)
		maxUnits=MAX_UNITS/gs->activeAllyTeams-5;
	
	if(shadowHandler->drawShadows){
		unitShadowVP=LoadVertexProgram("unitshadow.vp");
		unitVP=LoadVertexProgram("unit.vp");
	}
}

CUnitHandler::~CUnitHandler()
{
	list<CUnit*>::iterator usi;
	for(usi=activeUnits.begin();usi!=activeUnits.end();usi++)
		delete (*usi);

	glDeleteTextures(1,&whiteTex);
	glDeleteTextures(1,&radarBlippTex);

	if(shadowHandler->drawShadows){
		glDeleteProgramsARB( 1, &unitShadowVP );
		glDeleteProgramsARB( 1, &unitVP );
	}
}

int CUnitHandler::AddUnit(CUnit *unit)
{
	ASSERT_SYNCED_MODE;
	int num=(int)(gs->randFloat())*((int)activeUnits.size()-1);
	std::list<CUnit*>::iterator ui=activeUnits.begin();
	for(int a=0;a<num;++a){
		++ui;
	}
	activeUnits.insert(ui,unit);		//randomize this to make the order in slowupdate random (good if one build say many buildings at once and then many mobile ones etc)

	int id;
	if(overrideId!=-1){
		id=overrideId;
	} else {
		id=freeIDs.back();
		freeIDs.pop_back();
	}
	units[id]=unit;
	gs->teams[unit->team]->AddUnit(unit,CTeam::AddBuilt);
	return id;
}

void CUnitHandler::DeleteUnit(CUnit* unit)
{
	ASSERT_SYNCED_MODE;
	toBeRemoved.push(unit);
}

void CUnitHandler::Update()
{
	ASSERT_SYNCED_MODE;
START_TIME_PROFILE;
	while(!toBeRemoved.empty()){
		CUnit* delUnit=toBeRemoved.top();
		toBeRemoved.pop();

		list<CUnit*>::iterator usi;
		for(usi=activeUnits.begin();usi!=activeUnits.end();++usi){
			if(*usi==delUnit){
				if(slowUpdateIterator!=activeUnits.end() && *usi==*slowUpdateIterator)
					slowUpdateIterator++;
				activeUnits.erase(usi);
				units[delUnit->id]=0;
				freeIDs.push_front(delUnit->id);
				gs->teams[delUnit->team]->RemoveUnit(delUnit,CTeam::RemoveDied);
				delete delUnit;
				break;
			}
		}
		//debug
		for(usi=activeUnits.begin();usi!=activeUnits.end();){
			if(*usi==delUnit){
				info->AddLine("Error: Duplicated unit found in active units on erase");
				usi=activeUnits.erase(usi);
			} else {
				++usi;
			}
		}
	}

	list<CUnit*>::iterator usi;
	for(usi=activeUnits.begin();usi!=activeUnits.end();usi++)
		(*usi)->Update();

START_TIME_PROFILE
	if(!(gs->frameNum&15)){
		slowUpdateIterator=activeUnits.begin();
	}

	int numToUpdate=activeUnits.size()/16+1;
	for(;slowUpdateIterator!=activeUnits.end() && numToUpdate!=0;++slowUpdateIterator){
		(*slowUpdateIterator)->SlowUpdate();
		numToUpdate--;
	}

END_TIME_PROFILE("Unit slow update");

END_TIME_PROFILE("Unit handler");

	while(!tempDrawUnits.empty() && tempDrawUnits.begin()->first<gs->frameNum-1){
		tempDrawUnits.erase(tempDrawUnits.begin());
	}
	while(!tempTransperentDrawUnits.empty() && tempTransperentDrawUnits.begin()->first<=gs->frameNum){
		tempTransperentDrawUnits.erase(tempTransperentDrawUnits.begin());
	}
}

void CUnitHandler::Draw(bool drawReflection)
{
	ASSERT_UNSYNCED_MODE;
	vector<CUnit*> drawFar;
	vector<CUnit*> drawStat;
	drawCloaked.clear();

	list<CUnit*> drawBlipp;

	SetupForUnitDrawing();

#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* excludeUnit=gu->directControl;
#endif

	for(list<CUnit*>::iterator usi=activeUnits.begin();usi!=activeUnits.end();++usi){
#ifdef DIRECT_CONTROL_ALLOWED
		if((*usi)==excludeUnit)
			continue;
#endif
		if(camera->InView((*usi)->midPos,(*usi)->radius+30)){
			if(gs->allies[(*usi)->allyteam][gu->myAllyTeam] || loshandler->InLos(*usi,gu->myAllyTeam) || gu->spectating){
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
				if(sqDist<unitDrawDist*unitDrawDist*500){
					drawStat.push_back(*usi);
				}
			} else if(radarhandler->InRadar(*usi,gu->myAllyTeam)){
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

		if(keys[VK_SHIFT] && !selectedUnits.selectedUnits.empty() && (*selectedUnits.selectedUnits.begin())->unitDef->buildSpeed>0){
			for(set<CBuilderCAI*>::iterator bi=builderCAIs.begin();bi!=builderCAIs.end();++bi){
				if((*bi)->owner->team==gu->myTeam){
					(*bi)->DrawQuedBuildingSquares();
				}
			}
		}
	}
	glDisable(GL_TEXTURE_2D);
}

void CUnitHandler::DrawShadowPass(void)
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

	for(list<CUnit*>::iterator usi=activeUnits.begin();usi!=activeUnits.end();++usi){
		if((gs->allies[(*usi)->allyteam][gu->myAllyTeam] || loshandler->InLos(*usi,gu->myAllyTeam) || gu->spectating) && camera->InView((*usi)->midPos,(*usi)->radius+700)){
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

int CUnitHandler::CreateChecksum()
{
	int checksum=0;

	list<CUnit*>::iterator usi;
	for(usi=activeUnits.begin();usi!=activeUnits.end();usi++){
		checksum^=*((int*)&((*usi)->midPos.x));
	}

	checksum^=gs->randSeed;

#ifdef TRACE_SYNC
		tracefile << "Checksum: ";
		tracefile << checksum << " ";
#endif

	for(int a=0;a<gs->activeTeams;++a){
		checksum^=*((int*)&(gs->teams[a]->metal));
		checksum^=*((int*)&(gs->teams[a]->energy));
	}
#ifdef TRACE_SYNC
		tracefile << checksum << "\n";
#endif
	return checksum;
}

inline void CUnitHandler::DrawFar(CUnit *unit)
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

float CUnitHandler::GetBuildHeight(float3 pos, UnitDef* unitdef)
{
	float minh=-5000;
	float maxh=5000;
	int numBorder=0;
	float borderh=0;

	float maxDif=unitdef->maxHeightDif;
	int x1 = (int)max(0.f,(pos.x-(unitdef->xsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int x2 = min(gs->mapx,x1+unitdef->xsize);
	int z1 = (int)max(0.f,(pos.z-(unitdef->ysize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int z2 = min(gs->mapy,z1+unitdef->ysize);

	for(int x=x1; x<=x2; x++){
		for(int z=z1; z<=z2; z++){
			float orgh=readmap->orgheightmap[z*(gs->mapx+1)+x];
			float h=readmap->heightmap[z*(gs->mapx+1)+x];
			if(x==x1 || x==x2 || z==z1 || z==z2){
				numBorder++;
				borderh+=h;
			}
			if(minh<min(h,orgh)-maxDif)
				minh=min(h,orgh)-maxDif;
			if(maxh>max(h,orgh)+maxDif)
				maxh=max(h,orgh)+maxDif;
		}
	}
	float h=borderh/numBorder;

	if(h<minh && minh<maxh)
		h=minh+0.01;
	if(h>maxh && maxh>minh)
		h=maxh-0.01;

	return h;
}

int CUnitHandler::TestUnitBuildSquare(const float3& pos, std::string unit,CFeature *&feature)
{
	UnitDef *unitdef = unitDefHandler->GetUnitByName(unit);
	return TestUnitBuildSquare(pos, unitdef,feature);
}

int CUnitHandler::TestUnitBuildSquare(const float3& pos, UnitDef *unitdef,CFeature *&feature)
{
	feature=0;

	int x1 = (int) (pos.x-(unitdef->xsize*0.5f*SQUARE_SIZE));
	int x2 = x1+unitdef->xsize*SQUARE_SIZE;
	int z1 = (int) (pos.z-(unitdef->ysize*0.5f*SQUARE_SIZE));
	int z2 = z1+unitdef->ysize*SQUARE_SIZE;
	float h=GetBuildHeight(pos,unitdef);

	int canBuild=2;

	if(unitdef->needGeo){
		canBuild=0;
		std::vector<CFeature*> features=qf->GetFeaturesExact(pos,max(unitdef->xsize,unitdef->ysize)*6);
		
		for(std::vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi){
			if((*fi)->def->geoThermal && fabs((*fi)->pos.x-pos.x)<unitdef->xsize*4-4 && fabs((*fi)->pos.z-pos.z)<unitdef->ysize*4-4){
				canBuild=2;
				break;
			}
		}
	}

	for(int x=x1; x<x2; x+=SQUARE_SIZE){
		for(int z=z1; z<z2; z+=SQUARE_SIZE){
			int tbs=TestBuildSquare(float3(x,h,z),unitdef,feature);
			canBuild=min(canBuild,tbs);
		}
	}

	return canBuild;
}

int CUnitHandler::TestBuildSquare(const float3& pos, UnitDef *unitdef,CFeature *&feature)
{
	int ret=2;
	if(pos.x<0 || pos.x>=gs->mapx*SQUARE_SIZE || pos.z<0 || pos.z>=gs->mapy*SQUARE_SIZE)
		return 0;

	int yardxpos=int(pos.x+4)/SQUARE_SIZE;
	int yardypos=int(pos.z+4)/SQUARE_SIZE;
	CSolidObject* s;
	if(s=readmap->GroundBlocked(yardypos*gs->mapx+yardxpos)){
		if(dynamic_cast<CFeature*>(s))
			feature=(CFeature*)s;
		else if(s->immobile)
			return 0;
		else
			ret=1;
	}
/*
	std::vector<CSolidObject*> so=qf->GetSolidsExact(pos, SQUARE_SIZE);

	for(std::vector<CSolidObject*>::iterator soi=so.begin();soi!=so.end();++soi){
		if((*soi)->blocking && !(*soi)->immobile){
			ret=1;
		}
	}
*/
	int square=ground->GetSquare(pos);

	if(!unitdef->floater){
		int x=(int) (pos.x/SQUARE_SIZE);
		int z=(int) (pos.z/SQUARE_SIZE);
		float orgh=readmap->orgheightmap[z*(gs->mapx+1)+x];
		float h=readmap->heightmap[z*(gs->mapx+1)+x];
		float hdif=unitdef->maxHeightDif;
		if(pos.y>orgh+hdif && pos.y>h+hdif)
			return 0;
		if(pos.y<orgh-hdif && pos.y<h-hdif)
			return 0;
	}
	float groundheight = ground->GetHeight2(pos.x,pos.z);
	if(!unitdef->floater && groundheight<-unitdef->maxWaterDepth)
		return 0;
	if(groundheight>-unitdef->minWaterDepth)
		return 0;

	return ret;
}

int CUnitHandler::ShowUnitBuildSquare(const float3& pos, UnitDef *unitdef)
{
	glDisable(GL_DEPTH_TEST );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBegin(GL_QUADS);

	int x1 = (int) (pos.x-(unitdef->xsize*0.5f*SQUARE_SIZE));
	int x2 = x1+unitdef->xsize*SQUARE_SIZE;
	int z1 = (int) (pos.z-(unitdef->ysize*0.5f*SQUARE_SIZE));
	int z2 = z1+unitdef->ysize*SQUARE_SIZE;
	float h=GetBuildHeight(pos,unitdef);

	int canbuild=2;

	if(unitdef->needGeo){
		canbuild=0;
		std::vector<CFeature*> features=qf->GetFeaturesExact(pos,max(unitdef->xsize,unitdef->ysize)*6);

		for(std::vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi){
			if((*fi)->def->geoThermal && fabs((*fi)->pos.x-pos.x)<unitdef->xsize*4-4 && fabs((*fi)->pos.z-pos.z)<unitdef->ysize*4-4){
				canbuild=2;
				break;
			}
		}
	}
	std::vector<float3> canbuildpos;
	std::vector<float3> featurepos;
	std::vector<float3> nobuildpos;

	for(int x=x1; x<x2; x+=SQUARE_SIZE){
		for(int z=z1; z<z2; z+=SQUARE_SIZE){

			int square=ground->GetSquare(float3(x,pos.y,z));
			CFeature* feature=0;
			int tbs=TestBuildSquare(float3(x,pos.y,z),unitdef,feature);
			if(tbs){
				if(feature || tbs==1)
					featurepos.push_back(float3(x,h,z));
				else
					canbuildpos.push_back(float3(x,h,z));
				canbuild=min(canbuild,tbs);
			} else {
				nobuildpos.push_back(float3(x,h,z));
				//glColor4f(0.8f,0.0f,0,0.4f);
				canbuild = 0;
			}
		}
	}

	if(canbuild)
		glColor4f(0,0.8f,0,1.0f);
	else
		glColor4f(0.5,0.5f,0,1.0f);

	for(unsigned int i=0; i<canbuildpos.size(); i++)
	{
		glVertexf3(canbuildpos[i]);
		glVertexf3(canbuildpos[i]+float3(SQUARE_SIZE,0,0));
		glVertexf3(canbuildpos[i]+float3(SQUARE_SIZE,0,SQUARE_SIZE));
		glVertexf3(canbuildpos[i]+float3(0,0,SQUARE_SIZE));
	}
	glColor4f(0.5,0.5f,0,1.0f);
	for(unsigned int i=0; i<featurepos.size(); i++)
	{
		glVertexf3(featurepos[i]);
		glVertexf3(featurepos[i]+float3(SQUARE_SIZE,0,0));
		glVertexf3(featurepos[i]+float3(SQUARE_SIZE,0,SQUARE_SIZE));
		glVertexf3(featurepos[i]+float3(0,0,SQUARE_SIZE));
	}

	glColor4f(0.8f,0.0f,0,1.0f);
	for(unsigned int i=0; i<nobuildpos.size(); i++)
	{
		glVertexf3(nobuildpos[i]);
		glVertexf3(nobuildpos[i]+float3(SQUARE_SIZE,0,0));
		glVertexf3(nobuildpos[i]+float3(SQUARE_SIZE,0,SQUARE_SIZE));
		glVertexf3(nobuildpos[i]+float3(0,0,SQUARE_SIZE));
	}

	glEnd();
	glEnable(GL_DEPTH_TEST );
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glDisable(GL_BLEND);

	return canbuild;
}

void CUnitHandler::PushNewWind(float x, float z, float strength)
{
	ASSERT_SYNCED_MODE;
	//todo: fixa en lista med enbart windgenerators kanske blir lite snabbare
	list<CUnit*>::iterator usi;
	for(usi=activeUnits.begin();usi!=activeUnits.end();usi++)
	{
		if((*usi)->unitDef->windGenerator)
			(*usi)->PushWind(x,z,strength);
	}
}

void CUnitHandler::AddBuilderCAI(CBuilderCAI* b)
{
	builderCAIs.insert(b);
}

void CUnitHandler::RemoveBuilderCAI(CBuilderCAI* b)
{
	builderCAIs.erase(b);
}


void CUnitHandler::DrawCloakedUnits(void)
{
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,0.3);
	texturehandler->SetTexture();
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

			pos.x=floor((pos.x+4)/SQUARE_SIZE)*SQUARE_SIZE;
			pos.z=floor((pos.z+4)/SQUARE_SIZE)*SQUARE_SIZE;
			pos.y=ground->GetHeight2(pos.x,pos.z);
			if(unitdef->floater && pos.y<0)
				pos.y = -unitdef->waterline;

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
	glColor4f(1,1,1,0.3);
	texturehandler->SetTexture();
	glDepthMask(0);

	for(vector<CUnit*>::iterator ui=drawCloaked.begin();ui!=drawCloaked.end();++ui){
		(*ui)->Draw();
	}
	glDepthMask(1);
}


void CUnitHandler::SetupForUnitDrawing(void)
{
	if(shadowHandler->drawShadows && !water->drawReflection){		//stupid nvidia doesnt seem to support vertex program+clipplanes at once
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, unitVP );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, gs->sunVector.x,gs->sunVector.y,gs->sunVector.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, unitAmbientColor.x,unitAmbientColor.y,unitAmbientColor.z,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, unitSunColor.x,unitSunColor.y,unitSunColor.z,0);

		glBindTexture(GL_TEXTURE_2D,shadowHandler->shadowTexture);
		glEnable(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 1-unitShadowDensity);
		
		float texConstant[]={unitAmbientColor.x,unitAmbientColor.y,unitAmbientColor.z,1};
		glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,texConstant); 
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB,GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB,GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_INTERPOLATE_ARB);

		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_ADD);

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		texturehandler->SetTexture();
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
		glEnable(GL_TEXTURE_2D);
		glLightfv(GL_LIGHT1, GL_POSITION,gs->sunVector4);	// Position The Light
		glEnable(GL_LIGHT1);								// Enable Light One
	//	glDisable(GL_CULL_FACE);
	//	glCullFace(GL_BACK);
		float cols[]={1,1,1,1};
		float cols2[]={1,1,1,1};
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,cols);
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,cols2);
		glColor3f(1,1,1);
		texturehandler->SetTexture();
	}
	glAlphaFunc(GL_GREATER,0.05f);
	glEnable(GL_ALPHA_TEST);
}

void CUnitHandler::CleanUpUnitDrawing(void)
{
	if(shadowHandler->drawShadows && !water->drawReflection){
		glDisable( GL_VERTEX_PROGRAM_ARB );

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
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

void CUnitHandler::LoadSaveUnits(CLoadSaveInterface* file, bool loading)
{
	for(int a=0;a<MAX_UNITS;++a){
		bool exists=!!units[a];
		file->lsBool(exists);
		if(exists){
			if(loading){
				overrideId=a;
				float3 pos;
				file->lsFloat3(pos);
				string name;
				file->lsString(name);
				int team;
				file->lsInt(team);
				bool build;
				file->lsBool(build);
				unitLoader.LoadUnit(name,pos,team,build);
			} else {
				file->lsFloat3(units[a]->pos);
				file->lsString(units[a]->unitDef->name);
				file->lsInt(units[a]->team);
				file->lsBool(units[a]->beingBuilt);
			}
		} else {
			if(loading)
				freeIDs.push_back(a);
		}
	}
	for(int a=0;a<MAX_UNITS;++a){
		if(units[a])
			units[a]->LoadSave(file,loading);
	}
	overrideId=-1;
}

bool CUnitHandler::CanCloseYard(CUnit* unit)
{
	for(int z=unit->mapPos.y;z<unit->mapPos.y+unit->ysize;++z){
		for(int x=unit->mapPos.x;x<unit->mapPos.x+unit->xsize;++x){
			CSolidObject* c=readmap->groundBlockingObjectMap[z*gs->mapx+x];
			if(c!=0 && c!=unit)
				return false;
		}
	}
	return true;
}

// UnitHandler.cpp: implementation of the CUnitHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "myGL.h"
#include "Team.h"
#include "TimeProfiler.h"
#include "myMath.h"
#include "Ground.h"
#include "ReadMap.h"
#include "ConfigHandler.h"
#include "FartextureHandler.h"
#include "UnitDefHandler.h"
#include "QuadField.h"
#include "BuilderCAI.h"
#include "SelectedUnits.h"
#include "FileHandler.h"
#include "InfoConsole.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "LoadSaveInterface.h"
#include "UnitLoader.h"
#include "SyncTracer.h"
#include "GameSetup.h"
#include "AirBaseHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitHandler* uh;
using namespace std;

CUnitHandler::CUnitHandler()
:	overrideId(-1),
	maxUnits(500),
	lastDamageWarning(0),
	lastCmdDamageWarning(0),
	metalMakerIncome(0),
	metalMakerEfficiency(1),
	diminishingMetalMakers(false),
	limitDgun(false)
{
	//unitModelLoader=new CUnit3DLoader;

	for(int a=1;a<MAX_UNITS;a++){
		freeIDs.push_back(a);
		units[a]=0;
	}
	units[0]=0;

	slowUpdateIterator=activeUnits.end();

	waterDamage=atof(readmap->mapDefParser.SGetValueDef("0","MAP\\WATER\\WaterDamage").c_str())*(16.0/30.0);

	if(gameSetup)
		maxUnits=gameSetup->maxUnits;
	if(maxUnits>MAX_UNITS/gs->activeTeams-5)
		maxUnits=MAX_UNITS/gs->activeTeams-5;
	
	if(gameSetup){
		if(gameSetup->limitDgun){
			limitDgun=true;
			dgunRadius=gs->mapx*3;
		}
		if(gameSetup->diminishingMMs)
			diminishingMetalMakers=true;
	}
	airBaseHandler=new CAirBaseHandler;
}

CUnitHandler::~CUnitHandler()
{
	list<CUnit*>::iterator usi;
	for(usi=activeUnits.begin();usi!=activeUnits.end();usi++)
		delete (*usi);

	delete airBaseHandler;
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

	if(!(gs->frameNum&15)){
		if(diminishingMetalMakers)
			metalMakerEfficiency=8.0f/(8.0f+max(0.0f,sqrtf(metalMakerIncome/gs->activeTeams)-4));
		metalMakerIncome=0;
	}

END_TIME_PROFILE("Unit handler");

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

float CUnitHandler::GetBuildHeight(float3 pos, const UnitDef* unitdef)
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

	if (x1 > gs->mapx) x1 = gs->mapx;
	if (x2 < 0) x2 = 0;
	if (z1 > gs->mapy) z1 = gs->mapy;
	if (z2 < 0) z2 = 0; 

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

int CUnitHandler::TestUnitBuildSquare(const float3& pos, const UnitDef *unitdef,CFeature *&feature)
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
			if(canBuild==0)
				return 0;
		}
	}

	return canBuild;
}

int CUnitHandler::TestBuildSquare(const float3& pos, const UnitDef *unitdef,CFeature *&feature)
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

int CUnitHandler::ShowUnitBuildSquare(const float3& pos, const UnitDef *unitdef)
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

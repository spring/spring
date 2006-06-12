// UnitHandler.cpp: implementation of the CUnitHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "Rendering/GL/myGL.h"
#include "Game/Team.h"
#include "TimeProfiler.h"
#include "myMath.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/FartextureHandler.h"
#include "UnitDefHandler.h"
#include "Sim/Misc/QuadField.h"
#include "CommandAI/BuilderCAI.h"
#include "Game/SelectedUnits.h"
#include "FileSystem/FileHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Units/Unit.h"
#include "LoadSaveInterface.h"
#include "UnitLoader.h"
#include "SyncTracer.h"
#include "Game/GameSetup.h"
#include "Game/command.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "mmgr.h"


//////////////////////////////////////////////////////////////////////
// CChecksum implementation
//////////////////////////////////////////////////////////////////////

char* CChecksum::diff(char* buf, const CChecksum& c) {
	char* p = buf;
	if (x != c.x) *(p++) = 'X';
	if (y != c.y) *(p++) = 'Y';
	if (z != c.z) *(p++) = 'Z';
	if (m != c.m) *(p++) = 'M';
	if (e != c.e) *(p++) = 'E';
	*(p++) = 0;
	return buf;
}


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
	gs->Team(unit->team)->AddUnit(unit,CTeam::AddBuilt);
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
				gs->Team(delUnit->team)->RemoveUnit(delUnit,CTeam::RemoveDied);
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

CChecksum CUnitHandler::CreateChecksum()
{
	CChecksum checksum;

	list<CUnit*>::iterator usi;
	for(usi=activeUnits.begin();usi!=activeUnits.end();usi++){
		checksum.x^=*((int*)&((*usi)->midPos.x));
		checksum.y^=*((int*)&((*usi)->midPos.y));
		checksum.z^=*((int*)&((*usi)->midPos.z));
	}

#ifdef TRACE_SYNC
		tracefile << gs->frameNum << " X "<< checksum.x << "\n";
		tracefile << gs->frameNum << " Y "<< checksum.y << "\n";
		tracefile << gs->frameNum << " Z "<< checksum.z << "\n";
#endif

	for(int a=0;a<gs->activeTeams;++a){
		checksum.m^=*((int*)&(gs->Team(a)->metal));
		checksum.e^=*((int*)&(gs->Team(a)->energy));
	}
#ifdef TRACE_SYNC
		tracefile << gs->frameNum << " M "<< checksum.m << "\n";
		tracefile << gs->frameNum << " E "<< checksum.e << "\n";
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
	std::set<CUnit*>::iterator ui = selectedUnits.selectedUnits.begin();
	std::vector<Command> cv;
	for(;ui != selectedUnits.selectedUnits.end(); ui++){
	
	}

	return ret;
}

int CUnitHandler::ShowUnitBuildSquare(const float3& pos, const UnitDef *unitdef)
{
	return ShowUnitBuildSquare(pos, unitdef, std::vector<Command>());
}

int CUnitHandler::ShowUnitBuildSquare(const float3& pos, const UnitDef *unitdef, const std::vector<Command> &cv)
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
				UnitDef* ud;
				float3 cPos;
				std::vector<Command>::const_iterator ci = cv.begin();
				for(;ci != cv.end() && tbs; ci++){
					ud = unitDefHandler->GetUnitByID(-ci->id);
					cPos.x = ci->params[0];
					cPos.z = ci->params[2];
					if(max(cPos.x-x-SQUARE_SIZE,x-cPos.x)*2 < ud->xsize*SQUARE_SIZE
						&& max(cPos.z-z-SQUARE_SIZE,z-cPos.z)*2 < ud->ysize*SQUARE_SIZE){
						tbs=0;
					}
				}
				if(!tbs){
					nobuildpos.push_back(float3(x,h,z));
					canbuild = 0;
				} else if(feature || tbs==1)
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

/**
* returns a build Command that intersects the ray described by pos and dir from the command queues of the
* units units on team number team
* @breif returns a build Command that intersects the ray described by pos and dir
* @return the build Command, or 0 if one is not found
*/

Command CUnitHandler::GetBuildCommand(float3 pos, float3 dir){
	float3 tempF1 = pos;
	std::list<CUnit*>::iterator ui = this->activeUnits.begin();
	std::deque<Command>::iterator ci;
	for(; ui != this->activeUnits.end(); ui++){
		if((*ui)->team == gu->myTeam){
			ci = (*ui)->commandAI->commandQue.begin();
			for(; ci != (*ui)->commandAI->commandQue.end(); ci++){
				if((*ci).id < 0 && (*ci).params.size() >= 3){
					tempF1.x = (*ci).params[0];
					tempF1.y = (*ci).params[1];
					tempF1.z = (*ci).params[2];
					tempF1 = pos + dir*((tempF1.y - pos.y)/dir.y) - tempF1;
					UnitDef* ud = unitDefHandler->GetUnitByID(-(*ci).id);
					if(ud && ud->xsize/2*SQUARE_SIZE > fabs(tempF1.x) && ud->ysize/2*SQUARE_SIZE > fabs(tempF1.z)){
						return (*ci);
					}
				}
			}
		}
	}
	Command c;
	c.id = 0;
	return c;
}

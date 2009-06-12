// UnitHandler.cpp: implementation of the CUnitHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <assert.h>
#include "mmgr.h"

#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "UnitLoader.h"
#include "CommandAI/BuilderCAI.h"
#include "Game/GameSetup.h"
#include "Game/SelectedUnits.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/FartextureHandler.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "CommandAI/Command.h"
#include "Unit.h"
#include "GlobalUnsynced.h"
#include "FileSystem/FileHandler.h"
#include "LoadSaveInterface.h"
#include "LogOutput.h"
#include "TimeProfiler.h"
#include "myMath.h"
#include "ConfigHandler.h"
#include "Sync/SyncTracer.h"
#include "creg/STL_Deque.h"
#include "creg/STL_List.h"
#include "creg/STL_Set.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Lua/LuaUnsyncedCtrl.h"

using std::min;
using std::max;

BuildInfo::BuildInfo(const std::string& name, const float3& p, int facing)
{
	def = unitDefHandler->GetUnitByName(name);
	pos = p;
	buildFacing = facing;
}


void BuildInfo::FillCmd(Command& c) const
{
	c.id=-def->id;
	c.params.resize(4);
	c.params[0]=pos.x;
	c.params[1]=pos.y;
	c.params[2]=pos.z;
	c.params[3]=(float)buildFacing;
}


bool BuildInfo::Parse(const Command& c)
{
	if (c.params.size() >= 3) {
		pos = float3(c.params[0],c.params[1],c.params[2]);

		if(c.id < 0) {
			def = unitDefHandler->GetUnitByID(-c.id);

			buildFacing = 0;
			if (c.params.size()==4)
				buildFacing = int(c.params[3]);

			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitHandler* uh;

CR_BIND(CUnitHandler, (true));
CR_REG_METADATA(CUnitHandler, (
	CR_MEMBER(activeUnits),
	CR_MEMBER(units),
	CR_MEMBER(freeIDs),
	CR_MEMBER(waterDamage),
	CR_MEMBER(unitsPerTeam),
	CR_MEMBER(maxUnitRadius),
	CR_MEMBER(lastDamageWarning),
	CR_MEMBER(lastCmdDamageWarning),
	CR_MEMBER(limitDgun),
	CR_MEMBER(dgunRadius),
	CR_MEMBER(diminishingMetalMakers),
	CR_MEMBER(metalMakerIncome),
	CR_MEMBER(metalMakerEfficiency),
	CR_MEMBER(toBeRemoved),
	CR_MEMBER(morphUnitToFeature),
//	CR_MEMBER(toBeRemoved),
	CR_MEMBER(toBeAdded),
	CR_MEMBER(renderUnits),
	CR_MEMBER(builderCAIs),
	CR_MEMBER(unitsByDefs),
	CR_POSTLOAD(PostLoad),
	CR_SERIALIZER(Serialize)
	));


void CUnitHandler::Serialize(creg::ISerializer& s)
{
}


void CUnitHandler::PostLoad()
{
	// reset any synced stuff that is not saved
	slowUpdateIterator = activeUnits.end();
}


CUnitHandler::CUnitHandler(bool serializing)
:
	maxUnitRadius(0.0f),
	lastDamageWarning(0),
	lastCmdDamageWarning(0),
	limitDgun(false),
	diminishingMetalMakers(false),
	metalMakerIncome(0),
	metalMakerEfficiency(1),
	morphUnitToFeature(true)
{
	const size_t maxUnitsTemp = std::min(gameSetup->maxUnits * teamHandler->ActiveTeams(), MAX_UNITS);
	units.resize(maxUnitsTemp);
	unitsPerTeam = maxUnitsTemp / teamHandler->ActiveTeams() - 5;

	freeIDs.reserve(units.size()-1);
	for (size_t a = 1; a < units.size(); a++) {
		freeIDs.push_back(a);
		units[a] = NULL;
	}
	units[0] = NULL;

	slowUpdateIterator = activeUnits.end();

	waterDamage = mapInfo->water.damage;

	if (gameSetup->limitDgun) {
		limitDgun = true;
		dgunRadius = gs->mapx * 3;
	}
	if (gameSetup->diminishingMMs) {
		diminishingMetalMakers = true;
	}

	if (!serializing) {
		airBaseHandler = new CAirBaseHandler;

		unitsByDefs.resize(teamHandler->ActiveTeams(), std::vector<CUnitSet>(unitDefHandler->numUnitDefs + 1));
	}
}


CUnitHandler::~CUnitHandler()
{
	for (std::list<CUnit*>::iterator usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		(*usi)->delayedWreckLevel = -1; // dont create wreckages since featureHandler may be destroyed already
		delete (*usi);
	}

	delete airBaseHandler;
}


int CUnitHandler::AddUnit(CUnit *unit)
{
	int num = (int)(gs->randFloat()) * ((int)activeUnits.size() - 1);
	std::list<CUnit*>::iterator ui = activeUnits.begin();
	for (int a = 0; a < num;++a) {
		++ui;
	}

	// randomize this to make the order in slowupdate random (good if one
	// builds say many buildings at once and then many mobile ones etc)
	activeUnits.insert(ui, unit);

	// randomize the unitID assignment so that lua widgets can
	// not easily determine enemy unit counts from unitIDs alone
	assert(freeIDs.size() > 0);
	const unsigned int freeSlot = gs->randInt() % freeIDs.size();
	const unsigned int freeMax  = freeIDs.size() - 1;
	unit->id = freeIDs[freeSlot]; // set the unit ID
	freeIDs[freeSlot] = freeIDs[freeMax];
	freeIDs.resize(freeMax);

	units[unit->id] = unit;
	teamHandler->Team(unit->team)->AddUnit(unit, CTeam::AddBuilt);
	unitsByDefs[unit->team][unit->unitDef->id].insert(unit);

	maxUnitRadius = max(unit->radius, maxUnitRadius);

	GML_STDMUTEX_LOCK(runit); // AddUnit

	toBeAdded.insert(unit);

	return unit->id;
}


void CUnitHandler::DeleteUnit(CUnit* unit)
{
	toBeRemoved.push_back(unit);
}


void CUnitHandler::DeleteUnitNow(CUnit* delUnit)
{
#if defined(USE_GML) && GML_ENABLE_SIM
	{
		GML_STDMUTEX_LOCK(dque); // DeleteUnitNow

		LuaUnsyncedCtrl::drawCmdQueueUnits.erase(delUnit);
	}
#endif
	
	int delTeam = 0;
	int delType = 0;
	std::list<CUnit*>::iterator usi;
	for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		if (*usi == delUnit) {
			if (slowUpdateIterator != activeUnits.end() && *usi == *slowUpdateIterator) {
				slowUpdateIterator++;
			}
			delTeam = delUnit->team;
			delType = delUnit->unitDef->id;

			activeUnits.erase(usi);
			units[delUnit->id] = 0;
			freeIDs.push_back(delUnit->id);
			teamHandler->Team(delTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);

			unitsByDefs[delTeam][delType].erase(delUnit);

			delete delUnit;

			break;
		}
	}
	//debug
	for (usi = activeUnits.begin(); usi != activeUnits.end(); /* no post-op */) {
		if (*usi == delUnit) {
			logOutput.Print("Error: Duplicated unit found in active units on erase");
			usi = activeUnits.erase(usi);
		} else {
			++usi;
		}
	}

	GML_STDMUTEX_LOCK(runit); // DeleteUnitNow

	for(usi=renderUnits.begin(); usi!=renderUnits.end(); ++usi) {
		if(*usi==delUnit) {
			renderUnits.erase(usi);
			break;
		}
	}

#if defined(USE_GML) && GML_ENABLE_SIM
	for(int i=0, dcs=unitDrawer->drawCloaked.size(); i<dcs; ++i)
		if(unitDrawer->drawCloaked[i]==delUnit)
			unitDrawer->drawCloaked[i]=NULL;

	for(int i=0, dcs=unitDrawer->drawCloakedS3O.size(); i<dcs; ++i)
		if(unitDrawer->drawCloakedS3O[i]==delUnit)
			unitDrawer->drawCloakedS3O[i]=NULL;
#endif

	toBeAdded.erase(delUnit);
}


void CUnitHandler::Update()
{
	SCOPED_TIMER("Unit handler");

	if(!toBeRemoved.empty()) {

		GML_RECMUTEX_LOCK(unit); // Update - for anti-deadlock purposes.
		GML_RECMUTEX_LOCK(sel); // Update - unit is removed from selectedUnits in ~CObject, which is too late.
		GML_RECMUTEX_LOCK(quad); // Update - make sure unit does not get partially deleted before before being removed from the quadfield
		GML_STDMUTEX_LOCK(proj); // Update - projectile drawing may access owner() and lead to crash

		while (!toBeRemoved.empty()) {
			CUnit* delUnit = toBeRemoved.back();
			toBeRemoved.pop_back();

			DeleteUnitNow(delUnit);
		}
	}

	GML_UPDATE_TICKS();

	std::list<CUnit*>::iterator usi;
	for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		(*usi)->Update();
	}

	{
		SCOPED_TIMER("Unit slow update");
		if (!(gs->frameNum & 15)) {
			slowUpdateIterator = activeUnits.begin();
		}

		int numToUpdate = activeUnits.size() / 16 + 1;
		for (; slowUpdateIterator != activeUnits.end() && numToUpdate != 0; ++ slowUpdateIterator) {
			(*slowUpdateIterator)->SlowUpdate();
			numToUpdate--;
		}
	} // for timer destruction

	if (!(gs->frameNum & 15)) {
		if (diminishingMetalMakers)
			metalMakerEfficiency = 8.0f / (8.0f + max(0.0f, sqrt(metalMakerIncome / teamHandler->ActiveTeams()) - 4));
		metalMakerIncome = 0;
	}
}


float CUnitHandler::GetBuildHeight(float3 pos, const UnitDef* unitdef)
{
	float minh=-5000;
	float maxh=5000;
	int numBorder=0;
	float borderh=0;
	const float* heightmap=readmap->GetHeightmap();

	int xsize=1;
	int zsize=1;

	float maxDif=unitdef->maxHeightDif;
	int x1 = (int)max(0.f,(pos.x-(xsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int x2 = min(gs->mapx,x1+xsize);
	int z1 = (int)max(0.f,(pos.z-(zsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int z2 = min(gs->mapy,z1+zsize);

	if (x1 > gs->mapx) x1 = gs->mapx;
	if (x2 < 0) x2 = 0;
	if (z1 > gs->mapy) z1 = gs->mapy;
	if (z2 < 0) z2 = 0;

	for(int x=x1; x<=x2; x++){
		for(int z=z1; z<=z2; z++){
			float orgh=readmap->orgheightmap[z*(gs->mapx+1)+x];
			float h=heightmap[z*(gs->mapx+1)+x];
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
		h=minh+0.01f;
	if(h>maxh && maxh>minh)
		h=maxh-0.01f;

	return h;
}


int CUnitHandler::TestUnitBuildSquare(const BuildInfo& buildInfo, CFeature *&feature, int allyteam)
{
	feature = NULL;
	int xsize = buildInfo.GetXSize();
	int zsize = buildInfo.GetZSize();
	float3 pos = buildInfo.pos;

	int x1 = (int) (pos.x - (xsize * 0.5f * SQUARE_SIZE));
	int x2 = x1 + xsize * SQUARE_SIZE;
	int z1 = (int) (pos.z - (zsize * 0.5f * SQUARE_SIZE));
	int z2 = z1 + zsize * SQUARE_SIZE;
	float h=GetBuildHeight(pos,buildInfo.def);

	int canBuild = 2;

	if (buildInfo.def->needGeo) {
		canBuild = 0;
		std::vector<CFeature*> features=qf->GetFeaturesExact(pos,max(xsize,zsize)*6);

		for (std::vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi) {
			if ((*fi)->def->geoThermal
			    && fabs((*fi)->pos.x - pos.x) < (xsize * 4 - 4)
			    && fabs((*fi)->pos.z - pos.z) < (zsize * 4 - 4)){
				canBuild = 2;
				break;
			}
		}
	}

	for (int x = x1; x < x2; x += SQUARE_SIZE) {
		for (int z = z1; z < z2; z += SQUARE_SIZE) {
			int tbs = TestBuildSquare(float3(x, h, z), buildInfo.def, feature, allyteam);
			canBuild = min(canBuild, tbs);

			if (canBuild == 0) {
				return 0;
			}
		}
	}

	return canBuild;
}


int CUnitHandler::TestBuildSquare(const float3& pos, const UnitDef* unitdef, CFeature*& feature, int allyteam)
{
	if (pos.x < 0 || pos.x >= gs->mapx * SQUARE_SIZE || pos.z < 0 || pos.z >= gs->mapy * SQUARE_SIZE) {
		return 0;
	}

	int ret = 2;
	int yardxpos = int(pos.x + 4) / SQUARE_SIZE;
	int yardypos = int(pos.z + 4) / SQUARE_SIZE;
	CSolidObject* s;

	if ((s = groundBlockingObjectMap->GroundBlocked(yardypos * gs->mapx + yardxpos))) {
		if (dynamic_cast<CFeature*>(s)) {
			feature = (CFeature*) s;
		} else if (!dynamic_cast<CUnit*>(s) || (allyteam < 0) ||
			(((CUnit*) s)->losStatus[allyteam] & LOS_INLOS)) {
			if (s->immobile) {
				return 0;
			} else {
				ret = 1;
			}
		}
	}

	const float groundheight = ground->GetHeight2(pos.x, pos.z);

	if (!unitdef->floater || groundheight > 0.0f) {
		// if we are capable of floating, only test local
		// height difference if terrain is above sea-level
		const float* heightmap = readmap->GetHeightmap();
		int x = (int) (pos.x / SQUARE_SIZE);
		int z = (int) (pos.z / SQUARE_SIZE);
		float orgh = readmap->orgheightmap[z * (gs->mapx + 1) + x];
		float h = heightmap[z * (gs->mapx + 1) + x];
		float hdif = unitdef->maxHeightDif;

		if (pos.y > orgh + hdif && pos.y > h + hdif) { return 0; }
		if (pos.y < orgh - hdif && pos.y < h - hdif) { return 0; }
	}

	if (!unitdef->floater && groundheight < -unitdef->maxWaterDepth) {
		// ground is deeper than our maxWaterDepth, cannot build here
		return 0;
	}
	if (groundheight > -unitdef->minWaterDepth) {
		// ground is shallower than our minWaterDepth, cannot build here
		return 0;
	}

	return ret;
}


int CUnitHandler::ShowUnitBuildSquare(const BuildInfo& buildInfo)
{
	return ShowUnitBuildSquare(buildInfo, std::vector<Command>());
}


int CUnitHandler::ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command> &cv)
{
	glDisable(GL_DEPTH_TEST );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBegin(GL_QUADS);

	int xsize=buildInfo.GetXSize();
	int zsize=buildInfo.GetZSize();
	const float3& pos = buildInfo.pos;

	int x1 = (int) (pos.x-(xsize*0.5f*SQUARE_SIZE));
	int x2 = x1+xsize*SQUARE_SIZE;
	int z1 = (int) (pos.z-(zsize*0.5f*SQUARE_SIZE));
	int z2 = z1+zsize*SQUARE_SIZE;
	float h=GetBuildHeight(pos,buildInfo.def);

	int canbuild=2;

	if(buildInfo.def->needGeo)
	{
		canbuild=0;
		std::vector<CFeature*> features=qf->GetFeaturesExact(pos,max(xsize,zsize)*6);

		for(std::vector<CFeature*>::iterator fi=features.begin();fi!=features.end();++fi){
			if((*fi)->def->geoThermal && fabs((*fi)->pos.x-pos.x)<xsize*4-4 && fabs((*fi)->pos.z-pos.z)<zsize*4-4){
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

			CFeature* feature=0;
			int tbs=TestBuildSquare(float3(x,pos.y,z),buildInfo.def,feature,gu->myAllyTeam);
			if(tbs){
				std::vector<Command>::const_iterator ci = cv.begin();
				for(;ci != cv.end() && tbs; ci++){
					BuildInfo bc(*ci);
					if(max(bc.pos.x-x-SQUARE_SIZE,x-bc.pos.x)*2 < bc.GetXSize()*SQUARE_SIZE
						&& max(bc.pos.z-z-SQUARE_SIZE,z-bc.pos.z)*2 < bc.GetZSize()*SQUARE_SIZE){
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
		glColor4f(0.5f,0.5f,0,1.0f);

	for(unsigned int i=0; i<canbuildpos.size(); i++)
	{
		glVertexf3(canbuildpos[i]);
		glVertexf3(canbuildpos[i]+float3(SQUARE_SIZE,0,0));
		glVertexf3(canbuildpos[i]+float3(SQUARE_SIZE,0,SQUARE_SIZE));
		glVertexf3(canbuildpos[i]+float3(0,0,SQUARE_SIZE));
	}
	glColor4f(0.5f,0.5f,0,1.0f);
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

	if (h < 0.0f) {
		const float s[4] = { 0.0f, 0.0f, 1.0f, 0.5f }; // start color
		const float e[4] = { 0.0f, 0.5f, 1.0f, 1.0f }; // end color

		glBegin(GL_LINES);
		glColor4fv(s); glVertex3f(x1, h, z1); glColor4fv(e); glVertex3f(x1, 0.0f, z1);
		glColor4fv(s); glVertex3f(x2, h, z1); glColor4fv(e); glVertex3f(x2, 0.0f, z1);
		glColor4fv(s); glVertex3f(x1, h, z2); glColor4fv(e); glVertex3f(x1, 0.0f, z2);
		glColor4fv(s); glVertex3f(x2, h, z2); glColor4fv(e); glVertex3f(x2, 0.0f, z2);
		glEnd();
		// using the last end color
		glBegin(GL_LINE_LOOP);
		glVertex3f(x1, 0.0f, z1);
		glVertex3f(x1, 0.0f, z2);
		glVertex3f(x2, 0.0f, z2);
		glVertex3f(x2, 0.0f, z1);
		glEnd();
	}

	glEnable(GL_DEPTH_TEST );
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glDisable(GL_BLEND);

	return canbuild;
}


void CUnitHandler::UpdateWind(float x, float z, float strength)
{
	//todo: save windgens in list (would be a little faster)
	for(std::list<CUnit*>::iterator usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		if((*usi)->unitDef->windGenerator)
			(*usi)->UpdateWind(x,z,strength);
	}
}


void CUnitHandler::AddBuilderCAI(CBuilderCAI* b)
{
	GML_STDMUTEX_LOCK(cai); // AddBuilderCAI

	builderCAIs.insert(builderCAIs.end(),b);
}


void CUnitHandler::RemoveBuilderCAI(CBuilderCAI* b)
{
	GML_STDMUTEX_LOCK(cai); // RemoveBuilderCAI

	ListErase<CBuilderCAI*>(builderCAIs, b);
}


void CUnitHandler::LoadSaveUnits(CLoadSaveInterface* file, bool loading)
{
}


/**
* returns a build Command that intersects the ray described by pos and dir from the command queues of the
* units units on team number team
* @brief returns a build Command that intersects the ray described by pos and dir
* @return the build Command, or 0 if one is not found
*/

Command CUnitHandler::GetBuildCommand(float3 pos, float3 dir){
	float3 tempF1 = pos;

	GML_STDMUTEX_LOCK(cai); // GetBuildCommand

	CCommandQueue::iterator ci;
	for(std::list<CUnit*>::iterator ui = this->activeUnits.begin(); ui != this->activeUnits.end(); ++ui){
		if((*ui)->team == gu->myTeam){
			ci = (*ui)->commandAI->commandQue.begin();
			for(; ci != (*ui)->commandAI->commandQue.end(); ci++){
				if((*ci).id < 0 && (*ci).params.size() >= 3){
					BuildInfo bi(*ci);
					tempF1 = pos + dir*((bi.pos.y - pos.y)/dir.y) - bi.pos;
					if(bi.def && bi.GetXSize()/2*SQUARE_SIZE > fabs(tempF1.x) && bi.GetZSize()/2*SQUARE_SIZE > fabs(tempF1.z)){
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


bool CUnitHandler::CanBuildUnit(const UnitDef* unitdef, int team)
{
	if (teamHandler->Team(team)->units.size() >= unitsPerTeam) {
		return false;
	}
	if (unitsByDefs[team][unitdef->id].size() >= unitdef->maxThisUnit) {
		return false;
	}

	return true;
}

#include "StdAfx.h"
#include "FeatureHandler.h"
#include "Feature.h"
#include "QuadField.h"
#include "FileSystem/FileHandler.h"
#include "Game/Camera.h"
#include "LoadSaveInterface.h"
#include "LogOutput.h"
#include "LosHandler.h"
#include "Lua/LuaCallInHandler.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "myMath.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/FartextureHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/UnitHandler.h"
#include "System/TdfParser.h"
#include "System/TimeProfiler.h"
#include <GL/glu.h> // after myGL.h
#include "mmgr.h"
#include "creg/STL_Deque.h"
#include "creg/STL_List.h"
#include "creg/STL_Set.h"

CR_BIND(FeatureDef, );

CR_REG_METADATA(FeatureDef, (
		CR_MEMBER(myName),
		CR_MEMBER(description),
		CR_MEMBER(metal),
		CR_MEMBER(id),
		CR_MEMBER(energy),
		CR_MEMBER(maxHealth),
		CR_MEMBER(radius),
		CR_MEMBER(mass),
		CR_MEMBER(upright),
		CR_MEMBER(drawType),
		//CR_MEMBER(model), FIXME
		CR_MEMBER(modelname),
		CR_MEMBER(modelType),
		CR_MEMBER(destructable),
		CR_MEMBER(blocking),
		CR_MEMBER(burnable),
		CR_MEMBER(floating),
		CR_MEMBER(geoThermal),
		CR_MEMBER(deathFeature),
		CR_MEMBER(xsize),
		CR_MEMBER(ysize)
		));

using namespace std;
CFeatureHandler* featureHandler=0;

CR_BIND_DERIVED(CFeatureHandler,CObject, );

CR_REG_METADATA(CFeatureHandler, (

//	CR_MEMBER(wreckParser),
//	CR_MEMBER(featureDefs),
//	CR_MEMBER(featureDefsVector),

	CR_MEMBER(nextFreeID),
	CR_MEMBER(freeIDs),
	CR_MEMBER(activeFeatures),

	CR_MEMBER(toBeRemoved),
	CR_MEMBER(updateFeatures),

//	CR_MEMBER(drawQuads),

//	CR_MEMBER(drawQuadsX),
//	CR_MEMBER(drawQuadsY),

	CR_RESERVED(128),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
		));

CR_BIND(CFeatureHandler::DrawQuad, );

CR_REG_METADATA_SUB(CFeatureHandler,DrawQuad,(
	CR_MEMBER(features)
	));


CFeatureHandler::CFeatureHandler() :
		nextFreeID(0)
{
	PrintLoadMsg("Initializing map features");

	drawQuadsX=gs->mapx/DRAW_QUAD_SIZE;
	drawQuadsY=gs->mapy/DRAW_QUAD_SIZE;
	drawQuads.resize(drawQuadsX * drawQuadsY);

	LoadWreckFeatures();

	treeDrawer=CBaseTreeDrawer::GetTreeDrawer();
}

CFeatureHandler::~CFeatureHandler()
{
	for(CFeatureSet::iterator fi=activeFeatures.begin(); fi != activeFeatures.end(); ++fi)
		delete *fi;
	activeFeatures.clear();

	while(!featureDefs.empty()){
		std::map<std::string,FeatureDef*>::iterator fi=featureDefs.begin();
		delete fi->second;
		featureDefs.erase(fi);
	}
	delete treeDrawer;
}

void CFeatureHandler::Serialize(creg::ISerializer *s)
{
}

void CFeatureHandler::PostLoad()
{
	drawQuadsX=gs->mapx/DRAW_QUAD_SIZE;
	drawQuadsY=gs->mapy/DRAW_QUAD_SIZE;
	drawQuads.clear();
	drawQuads.resize(drawQuadsX * drawQuadsY);

	for (CFeatureSet::const_iterator it = activeFeatures.begin(); it != activeFeatures.end(); ++it)
		if ((*it)->drawQuad >= 0)
			drawQuads[(*it)->drawQuad].features.insert(*it);
}

void CFeatureHandler::AddFeatureDef(const std::string& name, FeatureDef* fd)
{
	std::map<std::string,FeatureDef*>::const_iterator it = featureDefs.find(name);

	if (it != featureDefs.end()) {
		featureDefsVector[it->second->id] = fd;
	} else {
		fd->id = featureDefsVector.size();
		featureDefsVector.push_back(fd);
	}
	featureDefs[name] = fd;
}


CFeature* CFeatureHandler::CreateWreckage(const float3& pos, const std::string& name, float rot, int facing, int iter, int team, int allyteam, bool emitSmoke,std::string fromUnit)
{
	ASSERT_SYNCED_MODE;
	if(name.empty())
		return 0;
	FeatureDef* fd=GetFeatureDef(name);

	if(!fd)
		return 0;

	if(iter>1){
		return CreateWreckage(pos, fd->deathFeature, rot, facing, iter - 1, team, allyteam, emitSmoke, "");
	} else {
		if (luaRules && !luaRules->AllowFeatureCreation(fd, team, pos)) {
			return NULL;
		}
		if(!fd->modelname.empty()){
			CFeature* f=SAFE_NEW CFeature;
			f->Initialize (pos, fd, (short int)rot, facing, team, fromUnit);
			// allow area-reclaiming wrecks of all units, including your own (they set allyteam = -1)
			f->allyteam = allyteam;
			if(emitSmoke && f->blocking)
				f->emitSmokeTime=300;
			return f;
		}
	}
	return 0;
}


void CFeatureHandler::Draw()
{
	ASSERT_UNSYNCED_MODE;
	vector<CFeature*> drawFar;

	unitDrawer->SetupForUnitDrawing();
	DrawRaw(0, &drawFar);
	unitDrawer->CleanUpUnitDrawing();

	unitDrawer->DrawQuedS3O();

	CVertexArray* va=GetVertexArray();
	va->Initialize();
	glAlphaFunc(GL_GREATER,0.8f);
	glEnable(GL_ALPHA_TEST);
	glBindTexture(GL_TEXTURE_2D,fartextureHandler->farTexture);
	glColor3f(1,1,1);
	glEnable(GL_FOG);
	for(vector<CFeature*>::iterator usi=drawFar.begin();usi!=drawFar.end();usi++){
		DrawFar(*usi,va);
	}
	va->DrawArrayTN(GL_QUADS);
}


void CFeatureHandler::DrawShadowPass()
{
	ASSERT_UNSYNCED_MODE;
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, unitDrawer->unitShadowGenVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glPolygonOffset(1,1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	unitDrawer->SetupForUnitDrawing();
	DrawRaw(1, NULL);
	unitDrawer->CleanUpUnitDrawing();

	unitDrawer->DrawQuedS3O();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable( GL_VERTEX_PROGRAM_ARB );
}

void CFeatureHandler::Update()
{
	ASSERT_SYNCED_MODE;
	START_TIME_PROFILE("Feature::Update");
	while (!toBeRemoved.empty()) {
		CFeatureSet::iterator it = activeFeatures.find(toBeRemoved.back());
		toBeRemoved.pop_back();
		if (it != activeFeatures.end()) {
			CFeature* feature = *it;
			freeIDs.push_back(feature->id);
			activeFeatures.erase(feature);

			if (feature->drawQuad >= 0) {
				DrawQuad* dq = &drawQuads[feature->drawQuad];
				dq->features.erase(feature);
			}

			if (feature->inUpdateQue) {
				updateFeatures.erase(feature);
			}

			delete feature;
		}
	}

	CFeatureSet::iterator fi=updateFeatures.begin();
	while (fi != updateFeatures.end()) {
		CFeature* feature = *fi;
		++fi;

		if (!feature->Update()) {
			// remove it
			feature->inUpdateQue = false;
			updateFeatures.erase(feature);
		}
	}
	END_TIME_PROFILE("Feature::Update");
}


void CFeatureHandler::LoadWreckFeatures()
{
	std::vector<string> files=CFileHandler::FindFiles("features/corpses/", "*.tdf");
	std::vector<string> files2=CFileHandler::FindFiles("features/All Worlds/", "*.tdf");

	for(vector<string>::iterator fi=files.begin();fi!=files.end();++fi){
		wreckParser.LoadFile(*fi);
	}
	for(vector<string>::iterator fi=files2.begin();fi!=files2.end();++fi){
		wreckParser.LoadFile(*fi);
	}
}


int CFeatureHandler::AddFeature(CFeature* feature)
{
	ASSERT_SYNCED_MODE;

	if (freeIDs.empty()) {
		feature->id = nextFreeID++;
	} else {
		feature->id = freeIDs.front();
		freeIDs.pop_front();
	}
	activeFeatures.insert(feature);
	SetFeatureUpdateable(feature);

	if(feature->def->drawType==DRAWTYPE_3DO){
		int quad = int(feature->pos.z / DRAW_QUAD_SIZE / SQUARE_SIZE) * drawQuadsX +
		           int(feature->pos.x / DRAW_QUAD_SIZE / SQUARE_SIZE);
		DrawQuad* dq=&drawQuads[quad];
		dq->features.insert(feature);
		feature->drawQuad=quad;
	}

	luaCallIns.FeatureCreated(feature);

	return feature->id ;
}


void CFeatureHandler::DeleteFeature(CFeature* feature)
{
	ASSERT_SYNCED_MODE;
	toBeRemoved.push_back(feature->id);

	luaCallIns.FeatureDestroyed(feature);
}


void CFeatureHandler::UpdateDrawQuad(CFeature* feature, const float3& newPos)
{
	const int oldDrawQuad = feature->drawQuad;
	if (oldDrawQuad >= 0) {
		const int newDrawQuad =
			int(newPos.z / DRAW_QUAD_SIZE / SQUARE_SIZE) * drawQuadsX +
			int(newPos.x / DRAW_QUAD_SIZE / SQUARE_SIZE);
		if (oldDrawQuad != newDrawQuad) {
			DrawQuad* oldDQ = &drawQuads[oldDrawQuad];
			oldDQ->features.erase(feature);
			DrawQuad* newDQ = &drawQuads[newDrawQuad];
			newDQ->features.insert(feature);
			feature->drawQuad = newDrawQuad;
		}
	}
}


void CFeatureHandler::LoadFeaturesFromMap(bool onlyCreateDefs)
{
	int numType = readmap->GetNumFeatureTypes ();

	for (int a = 0; a < numType; ++a) {
		const string name = StringToLower(readmap->GetFeatureType(a));
		if (name.find("treetype") != string::npos) {
			FeatureDef* fd=SAFE_NEW FeatureDef;
			fd->blocking=1;
			fd->burnable=true;
			fd->destructable=1;
			fd->reclaimable=true;
			fd->drawType=DRAWTYPE_TREE;
			fd->modelType=atoi(name.substr(8).c_str());
			fd->energy=250;
			fd->metal=0;
			fd->maxHealth=5;
			fd->radius=20;
			fd->xsize=2;
			fd->ysize=2;
			fd->myName=name;
			fd->description="Tree";
			fd->mass=20;
			AddFeatureDef(name, fd);
		} else if (name.find("geovent") != string::npos) {
			FeatureDef* fd=SAFE_NEW FeatureDef;
			fd->blocking=0;
			fd->burnable=0;
			fd->destructable=0;
			fd->reclaimable=false;
			fd->geoThermal=1;
			fd->drawType=DRAWTYPE_NONE;	//geos are drawn into the ground texture and emit smoke to be visible
			fd->modelType=0;
			fd->energy=0;
			fd->metal=0;
			fd->maxHealth=0;
			fd->radius=0;
			fd->xsize=0;
			fd->ysize=0;
			fd->myName=name;
			fd->mass=100000;
			AddFeatureDef(name, fd);
		} else if (wreckParser.SectionExist(name)) {
			GetFeatureDef(name);
		} else {
			logOutput.Print("Unknown feature type %s",name.c_str());
		}
	}

	if(!onlyCreateDefs){
		int numFeatures = readmap->GetNumFeatures ();
		MapFeatureInfo *mfi = SAFE_NEW MapFeatureInfo [numFeatures];
		readmap->GetFeatureInfo (mfi);

		for(int a=0;a<numFeatures;++a){
			string name = StringToLower(readmap->GetFeatureType (mfi[a].featureType));
			std::map<std::string,FeatureDef*>::iterator def = featureDefs.find(name);

			if (def == featureDefs.end()){
				logOutput.Print("Unknown feature named '%s'", name.c_str());
				continue;
			}

			const float ypos = ground->GetHeight2(mfi[a].pos.x, mfi[a].pos.z);
			(SAFE_NEW CFeature)->Initialize (float3(mfi[a].pos.x, ypos, mfi[a].pos.z),
			                                 featureDefs[name], (short int)mfi[a].rotation,
			                                 0, -1, "");
		}
		delete[] mfi;
	}
}


void CFeatureHandler::SetFeatureUpdateable(CFeature* feature)
{
	if(feature->inUpdateQue)
		return;

	updateFeatures.insert(feature);
	feature->inUpdateQue = true;
}


void CFeatureHandler::TerrainChanged(int x1, int y1, int x2, int y2)
{
	ASSERT_SYNCED_MODE;
	vector<int> quads=qf->GetQuadsRectangle(float3(x1*SQUARE_SIZE,0,y1*SQUARE_SIZE),
	                                        float3(x2*SQUARE_SIZE,0,y2*SQUARE_SIZE));
//	logOutput.Print("Checking feature pos %i",quads.size());

	for(vector<int>::iterator qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator fi;
		list<CFeature*>& features = qf->baseQuads[*qi].features;
		for(fi = features.begin(); fi != features.end(); ++fi) {
			CFeature* feature = *fi;
			float3& fpos = feature->pos;
			if (fpos.y > ground->GetHeight(fpos.x, fpos.z)) {
				SetFeatureUpdateable(feature);

				if (feature->def->floating){
					feature->finalHeight = ground->GetHeight(fpos.x, fpos.z);
				} else {
					feature->finalHeight = ground->GetHeight2(fpos.x, fpos.z);
				}

				feature->CalculateTransform ();
			}
		}
	}
}



struct CFeatureDrawer : CReadMap::IQuadDrawer
{
	void DrawQuad (int x,int y);

	CFeatureHandler *fh;
//	CFeatureHandler::DrawQuad *drawQuads;
	std::vector<CFeatureHandler::DrawQuad> *drawQuads;
	int drawQuadsX;
	bool drawReflection, drawRefraction;
	float unitDrawDist;
	std::vector<CFeature*>* farFeatures;
};


void CFeatureDrawer::DrawQuad (int x,int y)
{
	CFeatureHandler::DrawQuad* dq=&(*drawQuads)[y*drawQuadsX+x];

	for(CFeatureSet::iterator fi=dq->features.begin();fi!=dq->features.end();++fi){
		CFeature* f=(*fi);
		FeatureDef* def=f->def;

		if((f->allyteam==-1 || f->allyteam==gu->myAllyTeam ||
		    loshandler->InLos(f->pos,gu->myAllyTeam) || gu->spectatingFullView)
		   && def->drawType==DRAWTYPE_3DO){
			if(drawReflection){
				float3 zeroPos;
				if(f->midPos.y<0){
					zeroPos=f->midPos;
				}else{
					float dif=f->midPos.y-camera->pos.y;
					zeroPos=camera->pos*(f->midPos.y/dif) + f->midPos*(-camera->pos.y/dif);
				}
				if(ground->GetApproximateHeight(zeroPos.x,zeroPos.z)>f->radius){
					continue;
				}
			}
			if(drawRefraction){
				if(f->pos.y>0)
					continue;
			}
			float sqDist=(f->pos-camera->pos).SqLength2D();
			float farLength=f->sqRadius*unitDrawDist*unitDrawDist;
			if(sqDist<farLength){
				if(!f->model->textureType) {
					f->DrawS3O ();
				} else {
					unitDrawer->QueS3ODraw(f,f->model->textureType);
				}
			} else {
				if(farFeatures)
					farFeatures->push_back(f);
			}
		}
	}
}


void CFeatureHandler::DrawRaw(int extraSize, std::vector<CFeature*>* farFeatures)
{
	float featureDist=3000;
	if(!extraSize)
		featureDist=6000;		//farfeatures wont be drawn for shadowpass anyway

	CFeatureDrawer drawer;
	drawer.drawQuads = &drawQuads;
	drawer.fh = this;
	drawer.drawQuadsX = drawQuadsX;
	drawer.drawReflection=water->drawReflection;
	drawer.drawRefraction=water->drawRefraction;
	drawer.unitDrawDist=unitDrawer->unitDrawDist;
	drawer.farFeatures = farFeatures;

	readmap->GridVisibility (camera, DRAW_QUAD_SIZE, featureDist, &drawer, extraSize);
}


void CFeatureHandler::DrawFar(CFeature* feature, CVertexArray* va)
{
	float3 interPos=feature->pos+UpVector*feature->model->height*0.5f;
	int snurr=-feature->heading+GetHeadingFromVector(camera->pos.x-feature->pos.x,camera->pos.z-feature->pos.z)+(0xffff>>4);
	if(snurr<0)
		snurr+=0xffff;
	if(snurr>0xffff)
		snurr-=0xffff;
	snurr=snurr>>13;
	float tx=(feature->model->farTextureNum%8)*(1.0f/8.0f)+snurr*(1.0f/64);
	float ty=(feature->model->farTextureNum/8)*(1.0f/64.0f);
	float offset=0;
	va->AddVertexTN(interPos-(camera->up*feature->radius*1.4f-offset)+camera->right*feature->radius,tx,ty,unitDrawer->camNorm);
	va->AddVertexTN(interPos+(camera->up*feature->radius*1.4f+offset)+camera->right*feature->radius,tx,ty+(1.0f/64.0f),unitDrawer->camNorm);
	va->AddVertexTN(interPos+(camera->up*feature->radius*1.4f+offset)-camera->right*feature->radius,tx+(1.0f/64.0f),ty+(1.0f/64.0f),unitDrawer->camNorm);
	va->AddVertexTN(interPos-(camera->up*feature->radius*1.4f-offset)-camera->right*feature->radius,tx+(1.0f/64.0f),ty,unitDrawer->camNorm);
}


FeatureDef* CFeatureHandler::GetFeatureDef(const std::string mixedCase)
{
	const string name = StringToLower(mixedCase);
	std::map<std::string,FeatureDef*>::iterator fi=featureDefs.find(name);

	if (fi == featureDefs.end()) {
		if (!wreckParser.SectionExist(name)) {
			logOutput.Print("Couldnt find wreckage info %s", name.c_str());
			return 0;
		}
		FeatureDef* fd=SAFE_NEW FeatureDef;
		fd->blocking=!!atoi(wreckParser.SGetValueDef("1",name+"\\blocking").c_str());
		fd->destructable=!atoi(wreckParser.SGetValueDef("0",name+"\\indestructible").c_str());
		fd->reclaimable=!!atoi(wreckParser.SGetValueDef(
			fd->destructable ? "1" : "0",name+"\\reclaimable").c_str());
		fd->burnable=!!atoi(wreckParser.SGetValueDef("0",name+"\\flammable").c_str());
		//this seem to be the closest thing to floating that ta wreckage contains
		fd->floating=!!atoi(wreckParser.SGetValueDef("1",name+"\\nodrawundergray").c_str());
		if (fd->floating && !fd->blocking) {
			fd->floating=false;
		}
		fd->noSelect=!!atoi(wreckParser.SGetValueDef("0",name+"\\noselect").c_str());

		fd->deathFeature=wreckParser.SGetValueDef("",name+"\\featuredead");
		fd->upright=!!atoi(wreckParser.SGetValueDef("0",name+"\\upright").c_str());
		fd->drawType=DRAWTYPE_3DO;
		fd->energy=atof(wreckParser.SGetValueDef("0",name+"\\energy").c_str());
		fd->maxHealth=atof(wreckParser.SGetValueDef("0",name+"\\damage").c_str());
		fd->metal=atof(wreckParser.SGetValueDef("0",name+"\\metal").c_str());
		fd->modelname=wreckParser.SGetValueDef("",name+"\\object");
		if(!fd->modelname.empty()){
			fd->modelname=string("objects3d/")+fd->modelname;
		}
		fd->radius=0;
		fd->collisionSphereScale=atof(wreckParser.SGetValueDef("1",name+"\\collisionspherescale").c_str());
		float3 cso = ZeroVector;
		std::string strCSOffset = wreckParser.SGetValueDef("0.0 0.0 0.0",name+"\\CollisionSphereOffset").c_str();
		if (sscanf(strCSOffset.c_str(), "%f %f %f", &cso.x, &cso.y, &cso.z) == 3) {
			fd->useCSOffset = true;
			fd->collisionSphereOffset = cso;
		}
		else {
			fd->useCSOffset = false;
		}
		fd->xsize=atoi(wreckParser.SGetValueDef("1",name+"\\FootprintX").c_str())*2;		//our res is double TAs
		fd->ysize=atoi(wreckParser.SGetValueDef("1",name+"\\FootprintZ").c_str())*2;
		const string massStr = wreckParser.SGetValueDef("", name + "\\mass");
		if (massStr.empty()) {
			// generate the mass from the metal and health values
			fd->mass = (fd->metal * 0.4f) + (fd->maxHealth * 0.1f);
		} else {
			fd->mass = (float)atof(massStr.c_str());
		}
		fd->mass = max(0.001f, fd->mass);

		fd->description=wreckParser.SGetValueDef("",name+"\\description");

		fd->myName = name;

		AddFeatureDef(name, fd);

		fi = featureDefs.find(name);
	}
	return fi->second;
}


FeatureDef* CFeatureHandler::GetFeatureDefByID(int id)
{
	if ((id < 0) || (id >= (int) featureDefsVector.size())) {
		return NULL;
	}
	return featureDefsVector[id];
}


S3DOModel* FeatureDef::LoadModel(int team) const
{
		if (!useCSOffset) {
			return modelParser->Load3DO(modelname.c_str(),
			                            collisionSphereScale, team);
		} else {
			return modelParser->Load3DO(modelname.c_str(),
			                            collisionSphereScale, team,
			                            collisionSphereOffset);
		}
}

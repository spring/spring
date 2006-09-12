#include "StdAfx.h"
#include "FeatureHandler.h"
#include "Feature.h"
#include "FileSystem/FileHandler.h"
#include "TdfParser.h"
#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>
#include "Rendering/UnitModels/3DOParser.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "QuadField.h"
#include "Sim/Units/UnitHandler.h"
#include "LoadSaveInterface.h"
#include "TimeProfiler.h"
#include "LosHandler.h"
#include "myMath.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/FartextureHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "mmgr.h"

CR_BIND(FeatureDef);

using namespace std;
CFeatureHandler* featureHandler=0;

CFeatureHandler::CFeatureHandler()
:	overrideId(-1)
{
	PrintLoadMsg("Initializing map features");

	for(int a=0;a<MAX_FEATURES;++a){
		freeIDs.push_back(a);
		features[a]=0;
	}

	drawQuadsX=gs->mapx/DRAW_QUAD_SIZE;
	drawQuadsY=gs->mapy/DRAW_QUAD_SIZE;
	numQuads=drawQuadsX*drawQuadsY;
	drawQuads=new DrawQuad[numQuads];

	LoadWreckFeatures();

	treeDrawer=CBaseTreeDrawer::GetTreeDrawer();
}

CFeatureHandler::~CFeatureHandler()
{
//	for(std::set<CFeature*>::iterator fi=featureSet.begin();fi!=featureSet.end();++fi)
//		delete *fi;
//	featureSet.clear();

	for(int a=0;a<MAX_FEATURES;++a){
		if(features[a]!=0){
			delete features[a];
			features[a]=0;
		}
	}
	while(!featureDefs.empty()){
		std::map<std::string,FeatureDef*>::iterator fi=featureDefs.begin();
		delete fi->second;
		featureDefs.erase(fi);
	}
	delete treeDrawer;
	delete[] drawQuads;
}

CFeature* CFeatureHandler::CreateWreckage(const float3& pos, const std::string& name, float rot, int facing, int iter, int allyteam, bool emitSmoke,std::string fromUnit)
{
	ASSERT_SYNCED_MODE;
	if(name.empty())
		return 0;
	FeatureDef* fd=GetFeatureDef(name);

	if(!fd)
		return 0;

	if(iter>1){
		return CreateWreckage(pos,fd->deathFeature,rot,facing, iter-1,allyteam,emitSmoke,"");
	} else {
		if(!fd->modelname.empty()){
			CFeature* f=new CFeature;
			f->Initialize (pos,fd,(short int)rot,facing,allyteam,fromUnit);
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
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, unitDrawer->unitShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glPolygonOffset(1,1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	DrawRaw(1, 0);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable( GL_VERTEX_PROGRAM_ARB );
}

void CFeatureHandler::Update()
{
	ASSERT_SYNCED_MODE;
START_TIME_PROFILE
	while(!toBeRemoved.empty()){
		CFeature* feature=features[toBeRemoved.back()];
		toBeRemoved.pop_back();
		if(feature){
			freeIDs.push_back(feature->id);
			features[feature->id]=0;

			if(feature->drawQuad>=0){
				DrawQuad* dq=&drawQuads[feature->drawQuad];
				dq->features.erase(feature);
			}

			if(feature->inUpdateQue)
				updateFeatures.erase(feature->id);

			delete feature;
		}
	}

	SPRING_HASH_SET<int>::iterator fi=updateFeatures.begin();
	while(fi!= updateFeatures.end()){
		CFeature* feature=features[*fi];
		
		bool remove=!feature->Update();
		if(remove){
			feature->inUpdateQue=false;
			updateFeatures.erase(fi++);
		} else {
			++fi;
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
	int ret;
	if(overrideId!=-1){
		ret=overrideId;
	}else{
		ret=freeIDs.front();
		freeIDs.pop_front();
	}
	features[ret]=feature;
	feature->id=ret;
	SetFeatureUpdateable(feature);

	if(feature->def->drawType==DRAWTYPE_3DO){
		int quad=int(feature->pos.z/DRAW_QUAD_SIZE/SQUARE_SIZE)*drawQuadsX+int(feature->pos.x/DRAW_QUAD_SIZE/SQUARE_SIZE);
		DrawQuad* dq=&drawQuads[quad];
		dq->features.insert(feature);
		feature->drawQuad=quad;
	}

	return ret;
}

void CFeatureHandler::DeleteFeature(CFeature* feature)
{
	ASSERT_SYNCED_MODE;
	toBeRemoved.push_back(feature->id);
}

void CFeatureHandler::LoadFeaturesFromMap(bool onlyCreateDefs)
{
	int numType = readmap->GetNumFeatureTypes ();

	for(int a=0;a<numType;++a){
		string name=readmap->GetFeatureType (a);
		if(name.find("TreeType")!=string::npos){
			FeatureDef* fd=new FeatureDef;
			fd->blocking=1;
			fd->burnable=true;
			fd->destructable=1;
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
			featureDefs[name]=fd;
		} else if(name.find("GeoVent")!=string::npos){
			FeatureDef* fd=new FeatureDef;
			fd->blocking=0;
			fd->burnable=0;
			fd->destructable=0;
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
			featureDefs[name]=fd;
		} else if(wreckParser.SectionExist(name)){
			GetFeatureDef(name);
		} else {
			logOutput.Print("Unknown feature type %s",name.c_str());
		}
	}

	if(!onlyCreateDefs){
		int numFeatures = readmap->GetNumFeatures ();
		MapFeatureInfo *mfi = new MapFeatureInfo [numFeatures];
		readmap->GetFeatureInfo (mfi);

		for(int a=0;a<numFeatures;++a){
			string name=readmap->GetFeatureType (mfi[a].featureType);
			
			std::map<std::string,FeatureDef*>::iterator def = featureDefs.find(name);
			
			if (def == featureDefs.end()){
				logOutput.Print("Unknown feature named '%s'", name.c_str());
				continue;
			}

			float ypos = ground->GetHeight2(mfi[a].pos.x,mfi[a].pos.z);
			(new CFeature)->Initialize (float3(mfi[a].pos.x, ypos,mfi[a].pos.z),featureDefs[name],(short int)mfi[a].rotation,0,-1,"");
		}
		delete[] mfi;
	}
}

void CFeatureHandler::SetFeatureUpdateable(CFeature* feature)
{
	if(feature->inUpdateQue)
		return;

	updateFeatures.insert(feature->id);
	feature->inUpdateQue=true;	
}

void CFeatureHandler::TerrainChanged(int x1, int y1, int x2, int y2)
{
	ASSERT_SYNCED_MODE;
	vector<int> quads=qf->GetQuadsRectangle(float3(x1*SQUARE_SIZE,0,y1*SQUARE_SIZE),float3(x2*SQUARE_SIZE,0,y2*SQUARE_SIZE));
//	logOutput.Print("Checking feature pos %i",quads.size());

	for(vector<int>::iterator qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator fi;
		for(fi=qf->baseQuads[*qi].features.begin();fi!=qf->baseQuads[*qi].features.end();++fi){
			if((*fi)->pos.y>ground->GetHeight((*fi)->pos.x,(*fi)->pos.z)){
//				logOutput.Print("Updating feature pos %f %f %i",(*fi)->pos.y,ground->GetHeight((*fi)->pos.x,(*fi)->pos.z),(*fi)->id);
				SetFeatureUpdateable(*fi);
			
				if((*fi)->def->floating){
					(*fi)->finalHeight=ground->GetHeight((*fi)->pos.x,(*fi)->pos.z);
				} else {
					(*fi)->finalHeight=ground->GetHeight2((*fi)->pos.x,(*fi)->pos.z);
				}

				(*fi)->CalculateTransform ();
			}
		}
	}
}


void CFeatureHandler::LoadSaveFeatures(CLoadSaveInterface* file, bool loading)
{
	if(loading)
		freeIDs.clear();
	for(int a=0;a<MAX_FEATURES;++a){
		bool exists=!!features[a];
		file->lsBool(exists);
		if(exists){
			if(loading){
				overrideId=a;
				float3 pos;
				file->lsFloat3(pos);

				string def;
				file->lsString(def);
				if(featureDefs.find(def)==featureDefs.end())
					GetFeatureDef(def);

				short rotation;
				file->lsShort(rotation);
				string fromUnit;
				file->lsString(fromUnit);
				CFeature* f = new CFeature;
				f->Initialize (pos,featureDefs[def],rotation,0,-1,fromUnit);
			} else {
				file->lsFloat3(features[a]->pos);
				file->lsString(features[a]->def->myName);
				file->lsShort(features[a]->heading);
				file->lsString(features[a]->createdFromUnit);
			}
			CFeature* f=features[a];
			file->lsFloat(f->health);
			file->lsFloat(f->reclaimLeft);
			file->lsInt(f->allyteam);
		} else {
			if(loading)
				freeIDs.push_back(a);
		}
	}
	overrideId=-1;
}

struct CFeatureDrawer : CReadMap::IQuadDrawer
{
	void DrawQuad (int x,int y);

	CFeatureHandler *fh;
	CFeatureHandler::DrawQuad *drawQuads;
	int drawQuadsX;
	bool drawReflection, drawRefraction;
	float unitDrawDist;
	std::vector<CFeature*>* farFeatures;
};

void CFeatureDrawer::DrawQuad (int x,int y)
{
	CFeatureHandler::DrawQuad* dq=&drawQuads[y*drawQuadsX+x];

	for(set<CFeature*>::iterator fi=dq->features.begin();fi!=dq->features.end();++fi){
		CFeature* f=(*fi);
		FeatureDef* def=f->def;

		if((f->allyteam==-1 || f->allyteam==gu->myAllyTeam || loshandler->InLos(f->pos,gu->myAllyTeam) || gu->spectating) && def->drawType==DRAWTYPE_3DO){
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
				if(!def->model->textureType || shadowHandler->inShadowPass){
					f->DrawS3O ();
				} else {
					unitDrawer->QueS3ODraw(f,def->model->textureType);
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
	int cx=(int)(camera->pos.x/(SQUARE_SIZE*DRAW_QUAD_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*DRAW_QUAD_SIZE));

	float featureDist=3000;
	if(!extraSize)
		featureDist=6000;		//farfeatures wont be drawn for shadowpass anyway

	CFeatureDrawer drawer;
	drawer.drawQuads = drawQuads;
	drawer.fh = this;
	drawer.drawQuadsX = drawQuadsX;
	drawer.drawReflection=water->drawReflection;
	drawer.drawRefraction=water->drawRefraction;
	drawer.unitDrawDist=unitDrawer->unitDrawDist;
	drawer.farFeatures = farFeatures;

	readmap->GridVisibility (camera, DRAW_QUAD_SIZE, featureDist, &drawer, extraSize);
}

void CFeatureHandler::DrawFar(CFeature* feature,CVertexArray* va)
{
	float3 interPos=feature->pos+UpVector*feature->def->model->height*0.5;
	int snurr=-feature->heading+GetHeadingFromVector(camera->pos.x-feature->pos.x,camera->pos.z-feature->pos.z)+(0xffff>>4);
	if(snurr<0)
		snurr+=0xffff;
	if(snurr>0xffff)
		snurr-=0xffff;
	snurr=snurr>>13;
	float tx=(feature->def->model->farTextureNum%8)*(1.0/8.0)+snurr*(1.0/64);
	float ty=(feature->def->model->farTextureNum/8)*(1.0/64.0);
	float offset=0;
	va->AddVertexTN(interPos-(camera->up*feature->radius*1.4f-offset)+camera->right*feature->radius,tx,ty,unitDrawer->camNorm);
	va->AddVertexTN(interPos+(camera->up*feature->radius*1.4f+offset)+camera->right*feature->radius,tx,ty+(1.0/64.0),unitDrawer->camNorm);
	va->AddVertexTN(interPos+(camera->up*feature->radius*1.4f+offset)-camera->right*feature->radius,tx+(1.0/64.0),ty+(1.0/64.0),unitDrawer->camNorm);
	va->AddVertexTN(interPos-(camera->up*feature->radius*1.4f-offset)-camera->right*feature->radius,tx+(1.0/64.0),ty,unitDrawer->camNorm);
}

FeatureDef* CFeatureHandler::GetFeatureDef(const std::string name)
{
	std::map<std::string,FeatureDef*>::iterator fi=featureDefs.find(name);

	if(fi==featureDefs.end()){
		if(!wreckParser.SectionExist(name)){
			logOutput.Print("Couldnt find wreckage info %s",name.c_str());
			return 0;
		}
		FeatureDef* fd=new FeatureDef;
		fd->blocking=!!atoi(wreckParser.SGetValueDef("1",name+"\\blocking").c_str());
		fd->destructable=!atoi(wreckParser.SGetValueDef("0",name+"\\indestructible").c_str());
		fd->burnable=!!atoi(wreckParser.SGetValueDef("0",name+"\\flammable").c_str());
		fd->floating=!!atoi(wreckParser.SGetValueDef("1",name+"\\nodrawundergray").c_str());		//this seem to be the closest thing to floating that ta wreckage contains
		if(fd->floating && !fd->blocking)
			fd->floating=false;

		fd->deathFeature=wreckParser.SGetValueDef("",name+"\\featuredead");
		fd->upright=!!atoi(wreckParser.SGetValueDef("0",name+"\\upright").c_str());
		fd->drawType=DRAWTYPE_3DO;
		fd->energy=atof(wreckParser.SGetValueDef("0",name+"\\energy").c_str());
		fd->maxHealth=atof(wreckParser.SGetValueDef("0",name+"\\damage").c_str());
		fd->metal=atof(wreckParser.SGetValueDef("0",name+"\\metal").c_str());
		fd->model=0;
		fd->modelname=wreckParser.SGetValueDef("",name+"\\object");
		if(!fd->modelname.empty())
			fd->modelname=string("objects3d/")+fd->modelname;
		fd->radius=0;
		fd->xsize=atoi(wreckParser.SGetValueDef("1",name+"\\FootprintX").c_str())*2;		//our res is double TAs
		fd->ysize=atoi(wreckParser.SGetValueDef("1",name+"\\FootprintZ").c_str())*2;
		fd->mass=fd->metal*0.4+fd->maxHealth*0.1;
		fd->description=wreckParser.SGetValueDef("",name+"\\description");

		fd->myName=name;
		featureDefs[name]=fd;

		fi=featureDefs.find(name);
	}
	return fi->second;
}


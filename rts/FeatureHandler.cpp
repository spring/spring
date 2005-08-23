#include "StdAfx.h"
#include "FeatureHandler.h"
#include "Feature.h"
#include "FileHandler.h"
#include "SunParser.h"
#include "InfoConsole.h"
#include "myGL.h"
#include <GL/glu.h>
#include "3DOParser.h"
#include "Ground.h"
#include "Camera.h"
#include "BaseGroundDrawer.h"
#include "mapfile.h"
#include "QuadField.h"
#include "UnitHandler.h"
#include "LoadSaveInterface.h"
#include "TimeProfiler.h"
#include "LosHandler.h"
#include "myMath.h"
#include "VertexArray.h"
#include "FartextureHandler.h"
//#include "mmgr.h"

using namespace std;
CFeatureHandler* featureHandler=0;

CFeatureHandler::CFeatureHandler(void)
:	curClean(0),
	overrideId(-1)
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

	for(int a=0;a<numQuads;++a){
		drawQuads[a].lastSeen=0;
		drawQuads[a].lastCamPos=float3(0,0,0);
		drawQuads[a].displist=0;
		drawQuads[a].hasFarList=false;
	}

	LoadWreckFeatures();

	treeDrawer=CBaseTreeDrawer::GetTreeDrawer();
}

CFeatureHandler::~CFeatureHandler(void)
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
	delete wreckParser;
}

CFeature*  CFeatureHandler::CreateWreckage(const float3& pos, const std::string& name, float rot, int iter,int allyteam,bool emitSmoke,std::string fromUnit)
{
	ASSERT_SYNCED_MODE;
	if(name.empty())
		return 0;
	FeatureDef* fd=GetFeatureDef(name);

	if(!fd)
		return 0;

	if(iter>1){
		return CreateWreckage(pos,fd->deathFeature,rot,iter-1,allyteam,emitSmoke,"");
	} else {
		if(!fd->modelname.empty()){
			CFeature* f=new CFeature(pos,fd,(short int)rot,allyteam,fromUnit);
			if(emitSmoke && f->blocking)
				f->emitSmokeTime=300;
			return f;
		}
	}
	return 0;
}

void CFeatureHandler::Draw(void)
{
	ASSERT_UNSYNCED_MODE;
	uh->SetupForUnitDrawing();

	DrawRaw(0);

	uh->CleanUpUnitDrawing();

	DrawFarQuads();

	for(int a=0;a<50 && curClean<numQuads;++a,++curClean){
		DrawQuad* dq=&drawQuads[curClean];
		if(dq->lastSeen<gs->frameNum-50 && dq->displist){
			glDeleteLists(dq->displist,1);
			dq->displist=0;
		}
	}
}

void CFeatureHandler::DrawShadowPass(void)
{
	ASSERT_UNSYNCED_MODE;
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, uh->unitShadowVP );
	glEnable( GL_VERTEX_PROGRAM_ARB );
	glPolygonOffset(1,1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	DrawRaw(1);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable( GL_VERTEX_PROGRAM_ARB );
}

void CFeatureHandler::Update(void)
{
	ASSERT_SYNCED_MODE;
START_TIME_PROFILE
	while(!toBeRemoved.empty()){
		CFeature* feature=features[toBeRemoved.back()];
		toBeRemoved.pop_back();
		if(feature){
			freeIDs.push_back(feature->id);
			features[feature->id]=0;

			if(feature->drawQueType==1){
				DrawQuad* dq=&drawQuads[feature->drawQuad];
				dq->staticFeatures.erase(feature);
				ResetDrawQuad(feature->drawQuad);
			} else if(feature->drawQueType==2){
				DrawQuad* dq=&drawQuads[feature->drawQuad];
				dq->nonStaticFeatures.erase(feature);
			}

			if(feature->inUpdateQue)
				updateFeatures.erase(feature->id);

			delete feature;
		}
	}

	std::set<int>::iterator fi=updateFeatures.begin();
	while(fi!= updateFeatures.end()){
		CFeature* feature=features[*fi];
		
		bool remove=!feature->Update();
		if(remove){
			feature->inUpdateQue=false;
#ifndef ENABLE_CPPFIX
			fi=updateFeatures.erase(fi);
#else
			updateFeatures.erase(++fi); //TODO check that
#endif
		} else {
			++fi;
		}
	}
END_TIME_PROFILE("Feature::Update");
}

void CFeatureHandler::LoadWreckFeatures(void)
{
	std::vector<string> files=CFileHandler::FindFiles("features/corpses/*.tdf");
	std::vector<string> files2=CFileHandler::FindFiles("features/All Worlds/*.tdf");
	wreckParser=new CSunParser();

	for(vector<string>::iterator fi=files.begin();fi!=files.end();++fi){
		wreckParser->LoadFile(*fi);	
	}
	for(vector<string>::iterator fi=files2.begin();fi!=files2.end();++fi){
		wreckParser->LoadFile(*fi);	
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
		dq->nonStaticFeatures.insert(feature);
		feature->drawQueType=2;
		feature->drawQuad=quad;
	}

	return ret;
}

void CFeatureHandler::DeleteFeature(CFeature* feature)
{
	ASSERT_SYNCED_MODE;
	toBeRemoved.push_back(feature->id);
}

void CFeatureHandler::ResetDrawQuad(int quad)
{
	DrawQuad* dq=&drawQuads[quad];
	if(dq->displist){
		glDeleteLists(dq->displist,1);
		dq->displist=0;
	}
}

void CFeatureHandler::LoadFeaturesFromMap(CFileHandler* file,bool onlyCreateDefs)
{
	file->Seek(readmap->header.featurePtr);
	MapFeatureHeader fh;
	file->Read(&fh,sizeof(fh));
	if(file->Eof()){
		info->AddLine("No features in map file?");
		return;
	}
	string* mapids=new string[fh.numFeatureType];

	for(int a=0;a<fh.numFeatureType;++a){
		char c;
		file->Read(&c,1);
		while(c){
			mapids[a]+=c;
			file->Read(&c,1);
		}
		string name=mapids[a];
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
		} else if(wreckParser->SectionExist(name)){
			GetFeatureDef(name);
		} else {
			info->AddLine("Unknown feature type %s",name.c_str());
		}
	}

	if(!onlyCreateDefs){
		for(int a=0;a<fh.numFeatures;++a){
			MapFeatureStruct ffs;
			file->Read(&ffs,sizeof(ffs));

			string name=mapids[ffs.featureType];

			ffs.ypos=ground->GetHeight2(ffs.xpos,ffs.zpos);
			new CFeature(float3(ffs.xpos,ffs.ypos,ffs.zpos),featureDefs[name],(short int)ffs.rotation,-1,"");
		}
	}

	delete[] mapids;
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
//	info->AddLine("Checking feature pos %i",quads.size());

	for(vector<int>::iterator qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator fi;
		for(fi=qf->baseQuads[*qi].features.begin();fi!=qf->baseQuads[*qi].features.end();++fi){
			if((*fi)->pos.y>ground->GetHeight((*fi)->pos.x,(*fi)->pos.z)){
//				info->AddLine("Updating feature pos %f %f %i",(*fi)->pos.y,ground->GetHeight((*fi)->pos.x,(*fi)->pos.z),(*fi)->id);
				SetFeatureUpdateable(*fi);
			
				if((*fi)->def->floating){
					(*fi)->finalHeight=ground->GetHeight((*fi)->pos.x,(*fi)->pos.z);
				} else {
					(*fi)->finalHeight=ground->GetHeight2((*fi)->pos.x,(*fi)->pos.z);
				}

				(*fi)->transMatrix=CMatrix44f();
				(*fi)->transMatrix.Translate((*fi)->pos.x,(*fi)->pos.y,(*fi)->pos.z);
				(*fi)->transMatrix.RotateY(-(*fi)->heading*PI/0x7fff);
				(*fi)->transMatrix.SetUpVector(ground->GetNormal((*fi)->pos.x,(*fi)->pos.z));
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
				new CFeature(pos,featureDefs[def],rotation,-1,fromUnit);
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

void CFeatureHandler::DrawRaw(int extraSize)
{
	int cx=(int)(camera->pos.x/(SQUARE_SIZE*DRAW_QUAD_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*DRAW_QUAD_SIZE));

	float featureDist=3000;
	if(!extraSize)
		featureDist=6000;		//farfeatures wont be drawn for shadowpass anyway
	int featureSquare=int(featureDist/(SQUARE_SIZE*DRAW_QUAD_SIZE))+1;

	farQuads.clear();

	int sy=cy-featureSquare;
	if(sy<0)
		sy=0;
	int ey=cy+featureSquare;
	if(ey>drawQuadsY-1)
		ey=drawQuadsY-1;

	for(int y=sy;y<=ey;y++){
		int sx=cx-featureSquare;
		if(sx<0)
			sx=0;
		int ex=cx+featureSquare;
		if(ex>drawQuadsX-1)
			ex=drawQuadsX-1;
		float xtest,xtest2;
		std::vector<CBaseGroundDrawer::fline>::iterator fli;
		for(fli=groundDrawer->left.begin();fli!=groundDrawer->left.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*DRAW_QUAD_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*DRAW_QUAD_SIZE)+DRAW_QUAD_SIZE)));
			if(xtest>xtest2)
				xtest=xtest2;
			xtest=xtest/DRAW_QUAD_SIZE;
			if(xtest-extraSize>sx)
				sx=((int)xtest)-extraSize;
		}
		for(fli=groundDrawer->right.begin();fli!=groundDrawer->right.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*DRAW_QUAD_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*DRAW_QUAD_SIZE)+DRAW_QUAD_SIZE)));
			if(xtest<xtest2)
				xtest=xtest2;
			xtest=xtest/DRAW_QUAD_SIZE;
			if(xtest+extraSize<ex)
				ex=((int)xtest)+extraSize;
		}
		for(int x=sx;x<=ex;x++){/**/

			float3 dif;
			dif.x=camera->pos.x-(x*SQUARE_SIZE*DRAW_QUAD_SIZE+SQUARE_SIZE*DRAW_QUAD_SIZE/2);
			dif.y=0;
			dif.z=camera->pos.z-(y*SQUARE_SIZE*DRAW_QUAD_SIZE+SQUARE_SIZE*DRAW_QUAD_SIZE/2);
			float dist=dif.Length();
			dif/=dist;

			DrawQuad* dq=&drawQuads[y*drawQuadsX+x];
			if(dq->staticFeatures.empty() && dq->nonStaticFeatures.empty())
				continue;

			if(dist<6000 && dist>3000){//far features
				farQuads.push_back(dq);				
			} else {		//near features
				//draw non static features
				for(set<CFeature*>::iterator fi=dq->nonStaticFeatures.begin();fi!=dq->nonStaticFeatures.end();){
					CFeature* f=(*fi);
					FeatureDef* def=f->def;
					if((f->allyteam==-1 || f->allyteam==gu->myAllyTeam || loshandler->InLos(f->pos,gu->myAllyTeam) || gu->spectating) && def->drawType==DRAWTYPE_3DO){
						if(f->inUpdateQue || f->allyteam!=-1){
							glPushMatrix();
							glMultMatrixf(f->transMatrix.m);
							glTranslatef3(def->model->rootobject->offset);
							glCallList(def->model->rootobject->displist);
							glPopMatrix();
						} else {		//we have become static
						        
#ifndef ENABLE_CPPFIX							
							fi=dq->nonStaticFeatures.erase(fi);
#else			
							dq->nonStaticFeatures.erase(++fi); //check this
#endif
							dq->staticFeatures.insert(f);
							f->drawQueType=1;
							if(dq->displist){
								glDeleteLists(dq->displist,1);
								dq->displist=0;
							}
							continue;
						}
					}
					++fi;
				}
				//draw static features
				dq->lastSeen=gs->frameNum;
				if(dq->displist && dq->hasFarList){
					glDeleteLists(dq->displist,1);
					dq->displist=0;
				}
				if(!dq->displist){
					dq->displist=glGenLists(1);
					glNewList(dq->displist,GL_COMPILE);
					dq->hasFarList=false;

					for(set<CFeature*>::iterator fi=dq->staticFeatures.begin();fi!=dq->staticFeatures.end();++fi){
						CFeature* f=(*fi);
						FeatureDef* def=f->def;
						switch(def->drawType){
						case DRAWTYPE_3DO:
							glPushMatrix();
							glMultMatrixf(f->transMatrix.m);
							glTranslatef3(def->model->rootobject->offset);
							glCallList(def->model->rootobject->displist);
							glPopMatrix();
							break;
						default:
							info->AddLine("Unknown feature draw type");
						}
					}

					glEndList();
				}
				glCallList(dq->displist);
			}
		}
	}
}

void CFeatureHandler::DrawFarQuads(void)
{
	glAlphaFunc(GL_GREATER,0.8f);
	glEnable(GL_ALPHA_TEST);
	glBindTexture(GL_TEXTURE_2D,fartextureHandler->farTexture);
	glColor3f(1,1,1);

	for(std::vector<DrawQuad*>::iterator dqi=farQuads.begin();dqi!=farQuads.end();++dqi){
		DrawQuad* dq=*dqi;
		if(!dq->nonStaticFeatures.empty()){
			CVertexArray* va=GetVertexArray();
			va->Initialize();
			for(set<CFeature*>::iterator fi=dq->nonStaticFeatures.begin();fi!=dq->nonStaticFeatures.end();){
				CFeature* f=(*fi);
				FeatureDef* def=f->def;
				if((f->allyteam==-1 || f->allyteam==gu->myAllyTeam || loshandler->InLos(f->pos,gu->myAllyTeam)) && def->drawType==DRAWTYPE_3DO){
					if(f->inUpdateQue || f->allyteam!=-1){
						DrawFar(f,va);
					} else {		//we have become static

#ifndef ENABLE_CPPFIX
						fi=dq->nonStaticFeatures.erase(fi);
#else
						dq->nonStaticFeatures.erase(++fi);
#endif
						dq->staticFeatures.insert(f);
						f->drawQueType=1;
						if(dq->displist){
							glDeleteLists(dq->displist,1);
							dq->displist=0;
						}
						continue;
					}
				}
				++fi;
			}
			va->DrawArrayTN(GL_QUADS);
		}
		dq->lastSeen=gs->frameNum;
		if(dq->displist && (!dq->hasFarList || camera->pos.distance(dq->lastCamPos)>300)){
			glDeleteLists(dq->displist,1);
			dq->displist=0;
		}
		if(!dq->displist){
			CVertexArray* va=GetVertexArray();
			va->Initialize();
			dq->displist=glGenLists(1);
			glNewList(dq->displist,GL_COMPILE);
			dq->hasFarList=true;
			dq->lastCamPos=camera->pos;

			for(set<CFeature*>::iterator fi=dq->staticFeatures.begin();fi!=dq->staticFeatures.end();++fi){
				CFeature* f=(*fi);
				FeatureDef* def=f->def;
				switch(def->drawType){
				case DRAWTYPE_3DO:
					DrawFar(f,va);
					break;
				default:
					info->AddLine("Unknown feature draw type");
				}
			}
			va->DrawArrayTN(GL_QUADS);
			glEndList();
		}
		glCallList(dq->displist);
	}
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
	va->AddVertexTN(interPos-(camera->up*feature->radius*1.4f-offset)+camera->right*feature->radius,tx,ty,uh->camNorm);
	va->AddVertexTN(interPos+(camera->up*feature->radius*1.4f+offset)+camera->right*feature->radius,tx,ty+(1.0/64.0),uh->camNorm);
	va->AddVertexTN(interPos+(camera->up*feature->radius*1.4f+offset)-camera->right*feature->radius,tx+(1.0/64.0),ty+(1.0/64.0),uh->camNorm);
	va->AddVertexTN(interPos-(camera->up*feature->radius*1.4f-offset)-camera->right*feature->radius,tx+(1.0/64.0),ty,uh->camNorm);
}

FeatureDef* CFeatureHandler::GetFeatureDef(const std::string name)
{
	std::map<std::string,FeatureDef*>::iterator fi=featureDefs.find(name);

	if(fi==featureDefs.end()){
		if(!wreckParser->SectionExist(name)){
			info->AddLine("Couldnt find wreckage info %s",name.c_str());
			return 0;
		}
		FeatureDef* fd=new FeatureDef;
		fd->blocking=!!atoi(wreckParser->SGetValueDef("1",name+"\\blocking").c_str());
		fd->destructable=true;
		fd->burnable=false;
		fd->floating=!!atoi(wreckParser->SGetValueDef("1",name+"\\nodrawundergray").c_str());		//this seem to be the closest thing to floating that ta wreckage contains
		if(fd->floating && !fd->blocking)
			fd->floating=false;

		fd->deathFeature=wreckParser->SGetValueDef("",name+"\\featuredead");
		fd->drawType=DRAWTYPE_3DO;
		fd->energy=atof(wreckParser->SGetValueDef("0",name+"\\energy").c_str());
		fd->maxHealth=atof(wreckParser->SGetValueDef("0",name+"\\damage").c_str());
		fd->metal=atof(wreckParser->SGetValueDef("0",name+"\\metal").c_str());
		fd->model=0;
		fd->modelname=wreckParser->SGetValueDef("",name+"\\object");
		if(fd->modelname.find(".3do")==string::npos && !fd->modelname.empty())
			fd->modelname=string("objects3d/")+fd->modelname+".3do";
		fd->radius=0;
		fd->xsize=atoi(wreckParser->SGetValueDef("1",name+"\\FootprintX").c_str())*2;		//our res is double TAs
		fd->ysize=atoi(wreckParser->SGetValueDef("1",name+"\\FootprintZ").c_str())*2;
		fd->mass=fd->metal*0.4+fd->maxHealth*0.1;

		fd->myName=name;
		featureDefs[name]=fd;

		fi=featureDefs.find(name);
	}
	return fi->second;
}


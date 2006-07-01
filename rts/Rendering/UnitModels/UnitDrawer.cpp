#include "StdAfx.h"
#include "UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "3DModelParser.h"
#include "Game/Camera.h"
#include "myMath.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/BaseWater.h"
#include "Platform/ConfigHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Map/ReadMap.h"
#include "Map/Ground.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/FartextureHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Game/Team.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/LosHandler.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Textures/TextureHandler.h"
#include "Game/GameSetup.h"
#include "Game/UI/InfoConsole.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/GroundDecalHandler.h"
#include "SDL_types.h"
#include "SDL_keysym.h"
#include "mmgr.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/IconHandler.h"

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
	unitIconDist=configHandler.GetInt("UnitIconDist",200); // TODO: make this a config option just like 'unitDrawDist'.
	iconLength=750*unitIconDist*unitIconDist;

	CBitmap white;
	white.Alloc(1,1);
	for(int a=0;a<4;++a)
		white.mem[a]=255;

	whiteTex=white.CreateTexture(false);

	unitAmbientColor=readmap->mapDefParser.GetFloat3(float3(0.4,0.4,0.4),"MAP\\LIGHT\\UnitAmbientColor");
	unitSunColor=readmap->mapDefParser.GetFloat3(float3(0.7,0.7,0.7),"MAP\\LIGHT\\UnitSunColor");

	float3 specularSunColor=readmap->mapDefParser.GetFloat3(unitSunColor,"MAP\\LIGHT\\SpecularSunColor");
	readmap->mapDefParser.GetDef(unitShadowDensity,"0.8","MAP\\LIGHT\\UnitShadowDensity");

	advShading=!!configHandler.GetInt("AdvUnitShading", GLEW_ARB_fragment_program ? 1 : 0);
	if (advShading && !GLEW_ARB_fragment_program) {
		info->AddLine("You are missing an OpenGL extension needed to use advanced unit shading (GL_ARB_fragment_program)");
		advShading = false;
	}

	if (advShading)
	{
		if(shadowHandler->drawShadows){
			unitShadowVP=LoadVertexProgram("unit_genshadow.vp");
			unitFP=LoadFragmentProgram("unit_shadow.fp");
			units3oFP=LoadFragmentProgram("units3o_shadow.fp");
		} else {
			unitShadowVP=0;
			unitFP=LoadFragmentProgram("unit.fp");
			units3oFP=LoadFragmentProgram("units3o.fp");
		}

		unitVP=LoadVertexProgram("unit.vp");
		units3oVP=LoadVertexProgram("units3o.vp");

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

	if(advShading){
		if (unitShadowVP) glDeleteProgramsARB( 1, &unitShadowVP );
		glDeleteProgramsARB( 1, &unitVP );
		glDeleteProgramsARB( 1, &unitFP );
		glDeleteProgramsARB( 1, &units3oVP );
		glDeleteProgramsARB( 1, &units3oFP );

		glDeleteTextures(1,&boxtex);
		glDeleteTextures(1,&specularTex);
	}
	for(std::list<GhostBuilding*>::iterator gbi=ghostBuildings.begin();gbi!=ghostBuildings.end();){
		if((*gbi)->decal)
			(*gbi)->decal->gbOwner=0;
		delete *gbi;
		gbi=ghostBuildings.erase(gbi);
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

void CUnitDrawer::Draw(bool drawReflection,bool drawRefraction)
{
	ASSERT_UNSYNCED_MODE;

	vector<CUnit*> drawFar;
	vector<CUnit*> drawStat;
	drawCloaked.clear();
	glFogfv(GL_FOG_COLOR,FogLand);

	vector<CUnit*> drawIcon;
	vector<CUnit*> drawRadarIcon;

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
			if(gs->Ally((*usi)->allyteam,gu->myAllyTeam) || ((*usi)->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating){
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
				if(drawRefraction){
					if((*usi)->pos.y > 0)
						continue;
				}
				float sqDist=((*usi)->pos-camera->pos).SqLength();
				float iconDistMult=iconHandler->GetDistance((*usi)->unitDef->iconType);
				float realIconLength=iconLength*iconDistMult*iconDistMult;
				if(sqDist>realIconLength){
					drawIcon.push_back(*usi);
				} else {
					float farLength=(*usi)->sqRadius*unitDrawDist*unitDrawDist;
					if(sqDist>farLength){
						drawFar.push_back(*usi);
					} else {
						if((*usi)->isCloaked){
							drawCloaked.push_back(*usi);
						} else {
							if((*usi)->model->textureType){
								QueS3ODraw(*usi,(*usi)->model->textureType);
							} else {
								(*usi)->Draw();
							}
						}
					}
					if(sqDist<unitDrawDist*unitDrawDist*500 && showHealthBars){
						drawStat.push_back(*usi);
					}
				}
			} else if((*usi)->losStatus[gu->myAllyTeam] & LOS_PREVLOS){
				if (!gameSetup || gameSetup->ghostedBuildings)
					drawCloaked.push_back(*usi);

				if(((*usi)->losStatus[gu->myAllyTeam] & LOS_INRADAR) && !((*usi)->losStatus[gu->myAllyTeam] & LOS_CONTRADAR)){
					drawRadarIcon.push_back(*usi);
				}
			} else if(((*usi)->losStatus[gu->myAllyTeam] & LOS_INRADAR)){
				drawRadarIcon.push_back(*usi);
			}
		}
	}
	for(std::multimap<int,TempDrawUnit>::iterator ti=tempDrawUnits.begin();ti!=tempDrawUnits.end();++ti){
		if(camera->InView(ti->second.pos,100)){
			glPushMatrix();
			glTranslatef3(ti->second.pos);
			glRotatef(ti->second.rot*180/PI,0,1,0);
			modelParser->Load3DO(ti->second.unitdef->model.modelpath,1,ti->second.team)->DrawStatic();
			glPopMatrix();
		}
	}

	DrawQuedS3O();

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
		// Draw unit icons and radar blips.
		glAlphaFunc(GL_GREATER,0.5f);
		for(vector<CUnit*>::iterator ui=drawIcon.begin();ui!=drawIcon.end();ui++){
			DrawIcon(*ui,false);
		}
		for(vector<CUnit*>::iterator ui=drawRadarIcon.begin();ui!=drawRadarIcon.end();ui++){
			DrawIcon(*ui,true);
		}
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
		if((gs->Ally((*usi)->allyteam,gu->myAllyTeam) || ((*usi)->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating) && camera->InView((*usi)->midPos,(*usi)->radius+700)){
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

void CUnitDrawer::DrawIcon(CUnit * unit, bool asRadarBlip)
{
	// If the icon is to be drawn as a radar blip, we want to get the default icon.
	std::string iconType;
	if(asRadarBlip){
		iconType="default";
	} else {
		iconType=unit->unitDef->iconType;
	}

	// Fetch the icon information.
	CIcon* icon=iconHandler->GetIcon(iconType);

	unsigned char color[4];
	color[3]=255;

	// Calculate the icon size. It scales with:
	//  * The square root of the camera distance.
	//  * The mod defined 'iconSize' (which acts a multiplier).
	//  * The unit radius, depending on whether the mod defined 'radiusadjust' is true or false.
	float3 pos=unit->midPos;
	if(asRadarBlip)
		pos+=unit->posErrorVector*radarhandler->radarErrorSize[gu->myAllyTeam];
	float dist=sqrt((pos-camera->pos).Length());
	float scale=icon->size*dist/2;
	if(icon->radiusAdjust && !asRadarBlip)
		scale=scale*unit->radius/30; // I take the standard unit radius to be 30 ... call it an educated guess. (Teake Nutma)

	// Is the unit selected? Then draw it white.
	set<CUnit*>::iterator ui=selectedUnits.selectedUnits.find(unit);
	if(*ui==unit){
		color[0]=255;
		color[1]=255;
		color[2]=255;
	} else {
		color[0] = gs->Team(unit->team)->color[0];
		color[1] = gs->Team(unit->team)->color[1];
		color[2] = gs->Team(unit->team)->color[2];
	}

	// If the icon is partly under the ground, move it up.
	float h=ground->GetHeight(pos.x,pos.z);
	if(pos.y<h+scale)
		pos.y=h+scale;

	// Draw the icon.
	glBindTexture(GL_TEXTURE_2D,icon->texture);
	va=GetVertexArray();
	va->Initialize();
	va->AddVertexTC(pos+camera->up*scale+camera->right*scale,1,0,color);
	va->AddVertexTC(pos-camera->up*scale+camera->right*scale,1,1,color);
	va->AddVertexTC(pos-camera->up*scale-camera->right*scale,0,1,color);
	va->AddVertexTC(pos+camera->up*scale-camera->right*scale,0,0,color);
	va->DrawArrayTC(GL_QUADS);
}

void CUnitDrawer::SetupForGhostDrawing ()
{
	texturehandler->SetTATexture();
	glPushAttrib (GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
	glDepthMask(0);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void CUnitDrawer::CleanUpGhostDrawing ()
{
	glPopAttrib ();
	glDisable(GL_TEXTURE_2D);
	glDepthMask(1);
}

void CUnitDrawer::DrawCloakedUnits(void)
{
	SetupForGhostDrawing ();
	glColor4f(1,1,1,0.3);

	//ok these isnt really cloaked but the effect is the same
	for(std::multimap<int,TempDrawUnit>::iterator ti=tempTransperentDrawUnits.begin();ti!=tempTransperentDrawUnits.end();++ti){
		if(camera->InView(ti->second.pos,100)){
			glPushMatrix();
			glTranslatef3(ti->second.pos);
			glRotatef(ti->second.rot*180/PI,0,1,0);
			modelParser->Load3DO(ti->second.unitdef->model.modelpath,1,ti->second.team)->DrawStatic();
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
	texturehandler->SetTATexture();
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
			(*ui)->model->DrawStatic();
			glPopMatrix();
		}
	}
	//go through the dead but still ghosted buildings
	glColor4f(0.6,0.6,0.6,0.4);
	for(std::list<GhostBuilding*>::iterator gbi=ghostBuildings.begin();gbi!=ghostBuildings.end();){
		if(loshandler->InLos((*gbi)->pos,gu->myAllyTeam)){
			if((*gbi)->decal)
				(*gbi)->decal->gbOwner=0;
			delete *gbi;
			gbi=ghostBuildings.erase(gbi);
		} else {
			if(camera->InView((*gbi)->pos,(*gbi)->model->radius*2)){
				glPushMatrix();
				glTranslatef3((*gbi)->pos);
				(*gbi)->model->DrawStatic();
				glPopMatrix();
			}
			++gbi;
		}
	}

	// reset gl states
	CleanUpGhostDrawing ();
}

void CUnitDrawer::SetupForUnitDrawing(void)
{
	if(advShading && !water->drawReflection){		//standard doesnt seem to support vertex program+clipplanes at once
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

		if (shadowHandler->drawShadows) {
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D,shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
			glEnable(GL_TEXTURE_2D);
		}

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		texturehandler->SetTATexture();

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
		texturehandler->SetTATexture();
	}
//	glAlphaFunc(GL_GREATER,0.05f);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}

void CUnitDrawer::CleanUpUnitDrawing(void)
{
	if(advShading && !water->drawReflection){
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

void CUnitDrawer::SetS3OTeamColour(int team)
{
	if(advShading){
		unsigned char* col=gs->Team(team)->color;
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,14, col[0]*(1./255.),col[1]*(1./255.),col[2]*(1./255.),1);
	} else {
		SetBasicS3OTeamColour(team);
	}
}

void CUnitDrawer::SetBasicS3OTeamColour(int team)
{
	unsigned char* col=gs->Team(team)->color;
	float texConstant[]={col[0]*(1./255.),col[1]*(1./255.),col[2]*(1./255.),1};
	glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,texConstant);
}

/**
 * Set up the texture environment in texture unit 0
 * to give an S3O texture its team-colour.
 *
 * Also:
 * - call SetBasicS3OTeamColour to set the team colour to transform to.
 * - Replace the output alpha channel. If not, only the team-coloured bits will show, if that. Or something.
 */
void CUnitDrawer::SetupBasicS3OTexture0(void)
{
	// RGB = Texture * (1-Alpha) + Teamcolor * Alpha
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
	glEnable(GL_TEXTURE_2D);
}

/**
 * This sets the first texture unit to GL_MODULATE the colours from the
 * first texture unit with the current glColor.
 *
 * Normal S3O drawing sets the color to full white; translucencies
 * use this setup to 'tint' the drawn model.
 *
 * - Leaves glActivateTextureARB at the first unit.
 * - This doesn't tinker with the output alpha, either.
 */
void CUnitDrawer::SetupBasicS3OTexture1(void)
{
	// RGB = Primary Color * Previous
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, whiteTex);
}


void CUnitDrawer::SetupForS3ODrawing(void)
{
	if(advShading && !water->drawReflection){		//standard doesnt seem to support vertex program+clipplanes at once
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, units3oVP );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, units3oFP );
		glEnable( GL_FRAGMENT_PROGRAM_ARB );

		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, gs->sunVector.x,gs->sunVector.y,gs->sunVector.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, unitAmbientColor.x,unitAmbientColor.y,unitAmbientColor.z,1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,11, unitSunColor.x,unitSunColor.y,unitSunColor.z,0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, camera->pos.x, camera->pos.y, camera->pos.z, 0);

		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,10, 0,0,0,unitShadowDensity);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,11, unitAmbientColor.x,unitAmbientColor.y,unitAmbientColor.z,1);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);

		if (shadowHandler->drawShadows) {
			glActiveTextureARB(GL_TEXTURE2_ARB);
			glBindTexture(GL_TEXTURE_2D,shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
			glEnable(GL_TEXTURE_2D);
		}

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, boxtex);

		glActiveTextureARB(GL_TEXTURE4_ARB);
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

		SetupBasicS3OTexture0();

		// Set material color and fallback texture (3DO texture)
		float cols[]={1,1,1,1};
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,cols);
		glColor3f(1,1,1);
		texturehandler->SetTATexture();

		SetupBasicS3OTexture1();

		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
//	glAlphaFunc(GL_GREATER,0.05f);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
}

void CUnitDrawer::CleanUpS3ODrawing(void)
{
	if(advShading && !water->drawReflection){
		glDisable( GL_VERTEX_PROGRAM_ARB );
		glDisable( GL_FRAGMENT_PROGRAM_ARB );

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);

		glActiveTextureARB(GL_TEXTURE4_ARB);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);

		glActiveTextureARB(GL_TEXTURE0_ARB);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	} else {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT1);


		CleanupBasicS3OTexture1();
		CleanupBasicS3OTexture0();
	}
	glDisable(GL_CULL_FACE);
}

void CUnitDrawer::CleanupBasicS3OTexture1(void)
{
	// reset texture1 state
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB, GL_TEXTURE);
}

void CUnitDrawer::CleanupBasicS3OTexture0(void)
{
	// reset texture0 state
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
}

/**
 * Between a pair of SetupFor/CleanUpUnitDrawing (or SetupForS3ODrawing),
 * temporarily turns off textures and shaders.
 *
 * Used by CUnit::Draw() for drawing a unit under construction.
 *
 * Unfortunately, it doesn't work! With advanced shading on, the green
 * is darker than usual; with shadows as well, it's almost black. -- krudat
 */
void CUnitDrawer::UnitDrawingTexturesOff(S3DOModel *model)
{
	if (model->textureType == 0){
		/* 3DO */
		/* If SetupForUnitDrawing is changed, this may need tweaking too. */
		if(advShading && !water->drawReflection){
			glDisable(GL_VERTEX_PROGRAM_ARB);
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			/* TEXTURE0: Shadows. */
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE1_ARB); // Unit texture.
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE2_ARB); // boxtex
			glDisable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE3_ARB); // specularTex
			glDisable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glDisable(GL_FOG);
		} else {
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_2D);
		}
	} else {
		/* S3O */
		/* If SetupForS3ODrawing is changed, this may need tweaking too. */
		if(advShading && !water->drawReflection){
			/* Odd. Units with only the first texture build cyan rather than
			   green. Presume it's an improvement on black. :S -- krudat */
			glDisable(GL_VERTEX_PROGRAM_ARB);
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			/* TEXTURE0: Colour texture. */
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE1_ARB); // 'Shiny' texture.
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE2_ARB); // Shadows.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE3_ARB); // boxtex
			glDisable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE4_ARB); // specularTex
			glDisable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		} else {
			glDisable(GL_LIGHTING);
			/* TEXTURE0: Colour texture. */
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE1_ARB); // GL lighting, I think.
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}
}

/**
 * The companion to UnitDrawingTexturesOff(), re-enables the texture units
 * needed for drawing a model.
 *
 * Does *not* restore the texture bindings.
 */
void CUnitDrawer::UnitDrawingTexturesOn(S3DOModel *model)
{
	/* If UnitDrawingTextureOff is changed, this may need tweaking too. */
	if (model->textureType == 0){
		/* 3DO */
		if(advShading && !water->drawReflection){
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE2_ARB);
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE3_ARB);
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glEnable(GL_FOG);
		} else {
			glEnable(GL_LIGHTING);
			glColor3f(1,1,1);
			glEnable(GL_TEXTURE_2D);
		}
	} else {
		/* S3O */
		if(advShading && !water->drawReflection){
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);

			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE2_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE3_ARB);
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE4_ARB);
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		} else {
			glEnable(GL_LIGHTING);
			glColor3f(1,1,1);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
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

	readmap->GetGroundDrawer()->Draw(false,true);

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, boxtex);
	glCopyTexSubImage2D(gltype,0,0,0,0,0,128,128);

	glViewport(0,0,gu->screenx,gu->screeny);

	(*camera)=realCam;
	camera->Update(false);
}

void CUnitDrawer::QueS3ODraw(CWorldObject* object,int textureType)
{
	while(quedS3Os.size()<=textureType)
		quedS3Os.push_back(std::vector<CWorldObject*>());

	quedS3Os[textureType].push_back(object);
	usedS3OTextures.insert(textureType);
}

void CUnitDrawer::DrawQuedS3O(void)
{
	SetupForS3ODrawing();
	for(std::set<int>::iterator uti=usedS3OTextures.begin();uti!=usedS3OTextures.end();++uti){
		int tex=*uti;
		texturehandler->SetS3oTexture(tex);
		for(std::vector<CWorldObject*>::iterator ui=quedS3Os[tex].begin();ui!=quedS3Os[tex].end();++ui){
			(*ui)->DrawS3O();
		}
		quedS3Os[tex].clear();
	}
	usedS3OTextures.clear();
	CleanUpS3ODrawing();
}

/**
 * Draw one unit.
 *
 * Used for drawing the view of the controlled unit.
 *
 * Note: does all the GL state setting for that one unit, so you might want
 * something else for drawing many units.
 */
void CUnitDrawer::DrawIndividual(CUnit * unit)
{
	if (unit->model->textureType == 0){
		/* 3DO */
		SetupForUnitDrawing();
		unit->Draw();
		CleanUpUnitDrawing();
	} else {
		/* S3O */
		SetupForS3ODrawing();
		texturehandler->SetS3oTexture(unit->model->textureType);
		unit->Draw();
		CleanUpS3ODrawing();
	}

}

/**
 * Draw one unit,
 * - with depth-buffering(!) and lighting off,
 * - 'tinted' by the current glColor, *including* alpha.
 *
 * Used for drawing building orders.
 *
 * Note: does all the GL state setting for that one unit, so you might want
 * something else for drawing many translucent units.
 */
void CUnitDrawer::DrawBuildingSample(UnitDef* unitdef, int side, float3 pos)
{
	S3DOModel* model=modelParser->Load3DO(unitdef->model.modelpath.c_str(), 1, side);

	if (model->textureType == 0){
		/* 3DO */
		SetupForGhostDrawing();
		glPushMatrix();
		glTranslatef3(pos);
		model->DrawStatic();
		CleanUpGhostDrawing();
		glPopMatrix();
		return;
	}

	/* S3O */

	/* From SetupForGhostDrawing. */
	glPushAttrib (GL_TEXTURE_BIT | GL_ENABLE_BIT);

	/* *No* GL lighting. */

	/* Get the team-coloured texture constructed by unit 0. */
	SetBasicS3OTeamColour(side);
	SetupBasicS3OTexture0();
	texturehandler->SetS3oTexture(model->textureType);

	/* Tint it with the current glColor in unit 1. */
	SetupBasicS3OTexture1();

	/* Use the alpha given by glColor for the outgoing alpha.
	   (Might need to change this if we ever have transparent bits on units?)
	 */
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PRIMARY_COLOR_ARB);

	glActiveTextureARB(GL_TEXTURE0_ARB);

	/* From SetupForGhostDrawing. */
	glDepthMask(0);
	glDisable(GL_CULL_FACE); /* Leave out face culling, as 3DO and 3DO translucents does. */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	/* Push out the polygons. */
	glPushMatrix();
	glTranslatef3(pos);
	model->DrawStatic();
	glPopMatrix();

	// reset texture1 state
	CleanupBasicS3OTexture1();

	/* Also reset the alpha generation. */
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);

	// reset texture0 state
	CleanupBasicS3OTexture0();

	/* From CleanUpGhostDrawing. */
	glPopAttrib ();
	glDisable(GL_TEXTURE_2D);
	glDepthMask(1);
}

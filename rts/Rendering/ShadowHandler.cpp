#include "StdAfx.h"
#include "mmgr.h"

#include "ShadowHandler.h"
#include "ConfigHandler.h"
#include "Game/Camera.h"
#include "UnitModels/UnitDrawer.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Matrix44f.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Game/UI/MiniMap.h"
#include "LogOutput.h"
#include "Rendering/GL/FBO.h"
#include "Sim/Features/FeatureHandler.h"
#include "EventHandler.h"

CShadowHandler* shadowHandler=0;

#define DEFAULT_SHADOWMAPSIZE 2048


bool CShadowHandler::canUseShadows = false;
bool CShadowHandler::useFPShadows  = false;
bool CShadowHandler::firstInstance = true;


CShadowHandler::CShadowHandler(void)
{
	const bool tmpFirstInstance = firstInstance;
	firstInstance = false;

	drawShadows   = false;
	inShadowPass  = false;
	showShadowMap = false;
	firstDraw     = true;
	shadowTexture = 0;

	if (!tmpFirstInstance && !canUseShadows) {
		return;
	}

	const bool haveShadowExts =
		GLEW_ARB_vertex_program &&
		GLEW_ARB_shadow &&
		GLEW_ARB_depth_texture &&
		GLEW_ARB_texture_env_combine;
	const int configValue = configHandler->Get("Shadows", 0);

	if (configValue < 0 || !haveShadowExts) {
		logOutput.Print("shadows disabled or required OpenGL extension missing");
		return;
	}

	shadowMapSize = configHandler->Get("ShadowMapSize", DEFAULT_SHADOWMAPSIZE);

	if (tmpFirstInstance) {
		// this already checks for GLEW_ARB_fragment_program
		if (!ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "unit.fp")) {
			logOutput.Print("Your GFX card does not support the fragment programs needed for shadows");
			return;
		}

		// this was previously set to true (redundantly since
		// it was actually never made false anywhere) if either
		//      1. (!GLEW_ARB_texture_env_crossbar && haveShadowExts)
		//      2. (!GLEW_ARB_shadow_ambient && GLEW_ARB_shadow)
		// but the non-FP result isn't nice anyway so just always
		// use the program if we are guaranteed of shadow support
		useFPShadows = true;

		if (!GLEW_ARB_shadow_ambient) {
			// can't use arbitrary texvals in case the depth comparison op fails (only 0)
			logOutput.Print("You are missing the \"ARB_shadow_ambient\" extension (this will probably make shadows darker than they should be)");
		}
	}

	if (!InitDepthTarget()) {
		return;
	}

	if (tmpFirstInstance) {
		canUseShadows = true;
	}

	if (configValue == 0) {
		// free any resources allocated by InitDepthTarget()
		glDeleteTextures(1, &shadowTexture);
		shadowTexture = 0;
		return; // drawShadows is still false
	}

	drawShadows = true;
}


bool CShadowHandler::InitDepthTarget()
{
	// this can be enabled for debugging
	// it turns the shadow render buffer in a buffer with color
	bool useColorTexture = false;
	if (!fb.IsValid()) {
		logOutput.Print("framebuffer not valid!");
		return false;
	}
	glGenTextures(1,&shadowTexture);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (useColorTexture) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	fb.Bind();
	if (useColorTexture)
		fb.AttachTexture(shadowTexture);
	else
		fb.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);
	int buffer = useColorTexture ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE;
	glDrawBuffer(buffer);
	glReadBuffer(buffer);
	bool status = fb.CheckStatus("SHADOW");
	fb.Unbind();
	return status;
}

CShadowHandler::~CShadowHandler(void)
{
	if (drawShadows)
		glDeleteTextures(1, &shadowTexture);
}

void CShadowHandler::DrawShadowPasses(void)
{
	inShadowPass = true;

	ph->DrawShadowPass();
	unitDrawer->DrawShadowPass();
	featureHandler->DrawShadowPass();
	readmap->GetGroundDrawer()->DrawShadowPass();
	treeDrawer->DrawShadowPass();
	eventHandler.DrawWorldShadow();

	inShadowPass = false;
}

void CShadowHandler::GetShadowMapSizeFactors(float& p17, float& p18)
{
	if (shadowMapSize == 2048) {
		p17 =  0.01f;
		p18 = -0.1f;
	} else {
		p17 =  0.0025f;
		p18 = -0.05f;
	}
}

void CShadowHandler::CreateShadows(void)
{
	fb.Bind();

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glColor4f(1, 1, 1, 1);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glViewport(0,0,shadowMapSize,shadowMapSize);

	//glClearColor(0, 0, 0, 0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	firstDraw=false;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,1,0,1,0,-1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float3 sundir=mapInfo->light.sunDir;
	cross1=(sundir.cross(UpVector)).ANormalize();
	cross2=cross1.cross(sundir);
	centerPos=camera->pos;
//	centerPos.x=((int)((centerPos.x+4)/8))*8;
//	centerPos.y=((int)((centerPos.y+4)/8))*8;
//	centerPos.z=((int)((centerPos.z+4)/8))*8;
//	centerPos.y=(ground->GetHeight(centerPos.x,centerPos.z));

	CalcMinMaxView();

	//it should be possible to tweak a bit more shadow map resolution from this
	float maxLength=12000;
	float maxLengthX=(x2-x1)*1.5f;
	float maxLengthY=(y2-y1)*1.5f;
	xmid=1-(sqrt(fabs(x2))/(sqrt(fabs(x2))+sqrt(fabs(x1))));
	ymid=1-(sqrt(fabs(y2))/(sqrt(fabs(y2))+sqrt(fabs(y1))));
	//logOutput.Print("%.0f %.0f %.2f %.0f",y1,y2,ymid,maxLengthY);

	shadowMatrix[0]=cross1.x/maxLengthX;
	shadowMatrix[4]=cross1.y/maxLengthX;
	shadowMatrix[8]=cross1.z/maxLengthX;
	shadowMatrix[12]=-(cross1.dot(centerPos))/maxLengthX;
	shadowMatrix[1]=cross2.x/maxLengthY;
	shadowMatrix[5]=cross2.y/maxLengthY;
	shadowMatrix[9]=cross2.z/maxLengthY;
	shadowMatrix[13]=-(cross2.dot(centerPos))/maxLengthY;
	shadowMatrix[2]=-sundir.x/maxLength;
	shadowMatrix[6]=-sundir.y/maxLength;
	shadowMatrix[10]=-sundir.z/maxLength;
	shadowMatrix[14]=(centerPos.x*sundir.x+centerPos.z*sundir.z)/maxLength+0.5f;
	glLoadMatrixf(shadowMatrix.m);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,16, xmid,ymid,0,0);	//these registers should not be overwritten by anything

	GetShadowMapSizeFactors(p17, p18);

	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,17, p17,p17,0.0f,0.0f);	//these registers should not be overwritten by anything
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,18, p18,p18,0.0f,0.0f);	//these registers should not be overwritten by anything

	float3 oldup=camera->up;
	camera->right=shadowHandler->cross1;
	camera->up=shadowHandler->cross2;
	camera->pos2=camera->pos+sundir*8000;

	DrawShadowPasses();

	camera->up=oldup;
	camera->pos2=camera->pos;

	shadowMatrix[14]-=0.00001f;

	glShadeModel(GL_SMOOTH);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	//we do this later to save render context switches (this is one of the slowest opengl operations!)
	//fb.Unbind();
	//glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);
}


void CShadowHandler::DrawShadowTex(void)
{
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glColor3f(1,1,1);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,shadowTexture);

	glBegin(GL_QUADS);

	glTexCoord2f(0,0);
	glVertex3f(0,0,1);

	glTexCoord2f(0,1);
	glVertex3f(0,0.5f,1);

	glTexCoord2f(1,1);
	glVertex3f(0.5f,0.5f,1);

	glTexCoord2f(1,0);
	glVertex3f(0.5f,0,1);
	glEnd();
}

void CShadowHandler::CalcMinMaxView(void)
{
	left.clear();
	//Add restraints for camera sides
	GetFrustumSide(cam2->bottom,false);
	GetFrustumSide(cam2->top,true);
	GetFrustumSide(cam2->rightside,false);
	GetFrustumSide(cam2->leftside,false);

	std::vector<fline>::iterator fli,fli2;
  for(fli=left.begin();fli!=left.end();fli++){
	  for(fli2=left.begin();fli2!=left.end();fli2++){
			if(fli==fli2)
				continue;
			float colz=0;
			if(fli->dir-fli2->dir==0)
				continue;
			colz=-(fli->base-fli2->base)/(fli->dir-fli2->dir);
			if(fli2->left*(fli->dir-fli2->dir)>0){
				if(colz>fli->minz && colz<gs->mapy*SQUARE_SIZE+20000)
					fli->minz=colz;
			} else {
				if(colz<fli->maxz && colz>-20000)
					fli->maxz=colz;
			}
		}
	}

	x1=-100;
	x2=100;
	y1=-100;
	y2=100;

	//if someone could figure out how the frustum and nonlinear shadow transform really works (and not use the SJan trial and error method)
	//so that we can skip this sort of fudge factors it would be good
	float borderSize=270;
	float maxSize=gu->viewRange*0.75f;
	if(shadowMapSize==1024){
		borderSize*=1.5f;
		maxSize*=1.2f;
	}

	if(!left.empty()){
		std::vector<fline>::iterator fli;
		for(fli=left.begin();fli!=left.end();fli++){
			if(fli->minz<fli->maxz){
				float3 p[5];
				p[0]=float3(fli->base+fli->dir*fli->minz,0,fli->minz);
				p[1]=float3(fli->base+fli->dir*fli->maxz,0,fli->maxz);
				p[2]=float3(fli->base+fli->dir*fli->minz,readmap->maxheight+200,fli->minz);
				p[3]=float3(fli->base+fli->dir*fli->maxz,readmap->maxheight+200,fli->maxz);
				p[4]=float3(camera->pos.x,0,camera->pos.z);

				for(int a=0;a<5;++a){
					float xd=(p[a]-centerPos).dot(cross1);
					float yd=(p[a]-centerPos).dot(cross2);
					if(xd+borderSize>x2)
						x2=xd+borderSize;
					if(xd-borderSize<x1)
						x1=xd-borderSize;
					if(yd+borderSize>y2)
						y2=yd+borderSize;
					if(yd-borderSize<y1)
						y1=yd-borderSize;
				}
			}
		}
		if(x1<-maxSize)
			x1=-maxSize;
		if(x2>maxSize)
			x2=maxSize;
		if(y1<-maxSize)
			y1=-maxSize;
		if(y2>maxSize)
			y2=maxSize;
	} else {
		x1=-maxSize;
		x2=maxSize;
		y1=-maxSize;
		y2=maxSize;
	}
}

//maybe standardize all these things in one place sometime (and maybe one day i should try to understand how i made them work)
void CShadowHandler::GetFrustumSide(float3& side,bool upside)
{
	fline temp;

	float3 b=UpVector.cross(side);		//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)<0.0001f)
		b.z=0.00011f;
	if(fabs(b.z)>0.0001f){
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(side);			//get vector from camera to collision line
		c.ANormalize();
		float3 colpoint;				//a point on the collision line

		if(side.y>0){
			if(b.dot(UpVector.cross(cam2->forward))<0 && upside){
				colpoint=cam2->pos+cam2->forward*20000;
				//logOutput.Print("upward frustum");
			}else
				colpoint=cam2->pos-c*((cam2->pos.y)/c.y);
		}else
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);

		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		temp.left=-1;
		if(b.z>0)
			temp.left=1;
		if(side.y>0 && (b.dot(UpVector.cross(cam2->forward))<0 && upside))
			temp.left*=-1;
		temp.maxz=gs->mapy*SQUARE_SIZE+500;
		temp.minz=-500;
		left.push_back(temp);
	}
}

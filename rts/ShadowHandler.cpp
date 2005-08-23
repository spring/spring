#include "StdAfx.h"
#include "ShadowHandler.h"
#include "myGL.h"
#include "RegHandler.h"
#include "Camera.h"
#include "UnitHandler.h"
#include "BaseGroundDrawer.h"
#include "Matrix44f.h"
#include "Ground.h"
#include "ProjectileHandler.h"
#include "MiniMap.h"
#include "InfoConsole.h"
#include "FeatureHandler.h"

CShadowHandler* shadowHandler=0;

#ifndef NO_SHADOWS
extern HDC hDC;
extern HGLRC hRC;
extern HWND hWnd;
#endif //NO_SHADOWS

CShadowHandler::CShadowHandler(void)
{
#ifndef NO_SHADOWS
	hPBuffer=0;
	hDCPBuffer=0;
	hRCPBuffer=0;
	
	drawShadows=false;
	inShadowPass=false;
	showShadowMap=false;
	firstDraw=true;
	useFPShadows=false;
	copyDepthTexture=true;
	
	if(!regHandler.GetInt("Shadows",0))
		return;
	
	if(!(GLEW_ARB_shadow && GL_ARB_depth_texture && WGLEW_ARB_pbuffer && GLEW_ARB_vertex_program && GLEW_ARB_texture_env_combine && GLEW_ARB_texture_env_crossbar)){
		if(GLEW_ARB_shadow && GL_ARB_depth_texture && WGLEW_ARB_pbuffer && GLEW_ARB_vertex_program && GLEW_ARB_texture_env_combine && GLEW_ARB_fragment_program && GLEW_ARB_fragment_program_shadow){
			info->AddLine("Using ARB_fragment_program_shadow");
			useFPShadows=true;
		} else {
			info->AddLine("You are missing an OpenGL extension needed to use shadowmaps");
			return;
		}
	}
	if(!GLEW_ARB_shadow_ambient){
		if(GLEW_ARB_fragment_program && GLEW_ARB_fragment_program_shadow){
			if(!useFPShadows)
				info->AddLine("Using ARB_fragment_program_shadow");
			useFPShadows=true;
		} else {
			info->AddLine("You are missing the extension ARB_shadow_ambient, this will make shadows darker than they should be");
		}
	}

	if(WGLEW_NV_render_depth_texture){
		info->AddLine("Using NV_render_depth_texture");
		copyDepthTexture=false;
	}
	drawShadows=true;
	shadowMapSize=regHandler.GetInt("ShadowMapSize",2048);
	glGenTextures(1,&shadowTexture);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, shadowMapSize, shadowMapSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexImage2D(	GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0,GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	CreatePBuffer();
#endif //NO_SHADOWS
}

CShadowHandler::~CShadowHandler(void)
{
#ifndef NO_SHADOWS
	if(drawShadows)
		glDeleteTextures(1,&shadowTexture);

	if( hPBuffer != NULL ){
		wglReleasePbufferDCARB( hPBuffer, hDCPBuffer );
		wglDestroyPbufferARB( hPBuffer );
		hPBuffer=0;
	}
	if( hDCPBuffer != NULL )
	{
		ReleaseDC( hWnd, hDCPBuffer );
		hDCPBuffer = NULL;
	}
	wglMakeCurrent( hDC, hRC );
#endif
}

void CShadowHandler::CreateShadows(void)
{
#ifndef NO_SHADOWS
	if(!copyDepthTexture && !firstDraw){
		glBindTexture(GL_TEXTURE_2D, shadowTexture);
		if( wglReleaseTexImageARB( hPBuffer, WGL_DEPTH_COMPONENT_NV ) == FALSE )
		{
			MessageBox(NULL,"Could not release p-buffer from render texture!",
				"ERROR",MB_OK|MB_ICONEXCLAMATION);
			exit(-1);
		}
	}

	if( wglMakeCurrent( hDCPBuffer, hRCPBuffer) == FALSE )
	{
		MessageBox(NULL,"Could not make the p-buffer's context current!",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}/**/

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glColorMask(0, 0, 0, 0);

	glViewport(0,0,shadowMapSize,shadowMapSize);

	glClear(GL_DEPTH_BUFFER_BIT);

	if(firstDraw && copyDepthTexture){
		glBindTexture(GL_TEXTURE_2D, shadowTexture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowMapSize, shadowMapSize);
	}
	firstDraw=false;

	glMatrixMode(GL_PROJECTION);					
	glLoadIdentity();									
	glOrtho(0,1,0,1,0,-1);

	glMatrixMode(GL_MODELVIEW);	
	glLoadIdentity();

	float3 sundir=gs->sunVector;
	cross1=(sundir.cross(UpVector)).Normalize();
	cross2=cross1.cross(sundir);
	centerPos=camera->pos;
//	centerPos.x=((int)((centerPos.x+4)/8))*8;
//	centerPos.y=((int)((centerPos.y+4)/8))*8;
//	centerPos.z=((int)((centerPos.z+4)/8))*8;
//	centerPos.y=(ground->GetHeight(centerPos.x,centerPos.z));

	CalcMinMaxView();

	//it should be possible to tweak a bit more shadow map resolution from this
	float maxLength=12000;
	float maxLengthX=(x2-x1)*1.5;
	float maxLengthY=(y2-y1)*1.5;
	float xmid=1-(sqrt(fabs(x2))/(sqrt(fabs(x2))+sqrt(fabs(x1))));
	float ymid=1-(sqrt(fabs(y2))/(sqrt(fabs(y2))+sqrt(fabs(y1))));
	//info->AddLine("%.0f %.0f %.2f %.0f",y1,y2,ymid,maxLengthY);

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
	shadowMatrix[14]=(centerPos.x*sundir.x+centerPos.z*sundir.z)/maxLength+0.5;
	glLoadMatrixf(shadowMatrix.m);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,16, xmid,ymid,0,0);	//these registers should not be overwritten by anything
	if(shadowMapSize==2048){
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,17, 0.01,0.01,0,0);	//these registers should not be overwritten by anything
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,18, -0.1,-0.1,0,0);	//these registers should not be overwritten by anything
	} else {
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,17, 0.0025,0.0025,0,0);	//these registers should not be overwritten by anything
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,18, -0.05,-0.05,0,0);	//these registers should not be overwritten by anything
	}

	float3 oldup=camera->up;
	camera->right=shadowHandler->cross1;
	camera->up=shadowHandler->cross2;
	camera->pos2=camera->pos+sundir*8000;
	inShadowPass=true;

	ph->DrawShadowPass();
	uh->DrawShadowPass();
	featureHandler->DrawShadowPass();
	groundDrawer->DrawShadowPass();
	treeDrawer->DrawShadowPass();

	inShadowPass=false;
	camera->up=oldup;
	camera->pos2=camera->pos;

	shadowMatrix[14]-=0.00001;

	glShadeModel(GL_SMOOTH);
	glColorMask(1, 1, 1, 1);

	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	if(copyDepthTexture){
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, shadowMapSize-2, shadowMapSize-2);
	}

	if( wglMakeCurrent( hDC, hRC ) == FALSE )
	{
		MessageBox(NULL,"Could not make the window's context current!",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}
	glViewport(0,0,gu->screenx,gu->screeny);
	if(!copyDepthTexture){
		if( wglBindTexImageARB( hPBuffer, WGL_DEPTH_COMPONENT_NV ) == FALSE )
		{
			MessageBox(NULL,"Could not bind p-buffer to render texture!",
				"ERROR",MB_OK|MB_ICONEXCLAMATION);
			exit(-1);
		}
	}
#endif //NO_SHADOWS
}


void CShadowHandler::DrawShadowTex(void)
{
#ifndef NO_SHADOWS
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glColor3f(1,1,1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,shadowTexture);
/*	if( wglBindTexImageARB( hPBuffer, WGL_DEPTH_COMPONENT_NV ) == FALSE )
	{
		MessageBox(NULL,"Could not bind p-buffer to render texture!",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}/**/

	glBegin(GL_QUADS);

	glTexCoord2f(0,0);
	glVertex3f(0,0,1);

	glTexCoord2f(0,1);
	glVertex3f(0,0.5,1);

	glTexCoord2f(1,1);
	glVertex3f(0.5,0.5,1);

	glTexCoord2f(1,0);
	glVertex3f(0.5,0,1);
	glEnd();

/*	if( wglReleaseTexImageARB( hPBuffer, WGL_DEPTH_COMPONENT_NV ) == FALSE )
	{
		MessageBox(NULL,"Could not release p-buffer from render texture!",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}/**/
#endif
}
extern GLuint		PixelFormat;
void CShadowHandler::CreatePBuffer(void)
{
#ifndef NO_SHADOWS
	//
	// Define the minimum pixel format requirements we will need for our 
	// p-buffer. A p-buffer is just like a frame buffer, it can have a depth 
	// buffer associated with it and it can be double buffered.
	//
/*
	int pf_attr[] =
	{
		WGL_SUPPORT_OPENGL_ARB, TRUE,       // P-buffer will be used with OpenGL
			WGL_DRAW_TO_PBUFFER_ARB, TRUE,      // Enable render to p-buffer
			WGL_BIND_TO_TEXTURE_RGBA_ARB, TRUE, // P-buffer will be used as a texture
			WGL_DOUBLE_BUFFER_ARB, FALSE,       // We don't require double buffering
			0                                   // Zero terminates the list
	};

	unsigned int count = 0;
	int pixelFormat;

	if( !wglChoosePixelFormatARB( hDC, pf_attr, NULL, 1, &pixelFormat, &count ) )
	{
		MessageBox(NULL,"pbuffer creation error:  wglChoosePixelFormatARB() failed.",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}

	if( count <= 0 )
	{
		MessageBox(NULL,"pbuffer creation error:  Couldn't find a suitable pixel format.",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}
*/
	//
	// Set some p-buffer attributes so that we can use this p-buffer as a
	// 2D RGBA texture target.
	//

	int pb_attr[16] = 
	{
			WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB,                // Our p-buffer will have a texture format of RGBA
			WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB,                  // Of texture target will be GL_TEXTURE_2D
			0                                                            // Zero terminates the list
	};
	if(!copyDepthTexture){
		pb_attr[4]=WGL_DEPTH_TEXTURE_FORMAT_NV;												//add nvidia depth texture support if supported
		pb_attr[5]=WGL_TEXTURE_DEPTH_COMPONENT_NV;
		pb_attr[6]=0;
	}

	//
	// Create the p-buffer...
	//

	hPBuffer = wglCreatePbufferARB( hDC, PixelFormat, shadowMapSize, shadowMapSize, pb_attr );
	hDCPBuffer      = wglGetPbufferDCARB( hPBuffer );
	hRCPBuffer=hRC;//wglGetCurrentContext(); 
	//hRCPBuffer      = wglCreateContext( hDCPBuffer );

	if( !hPBuffer )
	{
		MessageBox(NULL,"pbuffer creation error: wglCreatePbufferARB() failed!",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}
#endif //NO_SHADOWS
}

#ifndef NO_SHADOWS
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
	float borderSize=200;

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
		if(x1<-5000)
			x1=-5000;
		if(x2>5000)
			x2=5000;
		if(y1<-5000)
			y1=-5000;
		if(y2>5000)
			y2=5000;
	} else {
		x1=-5000;
		x2=5000;
		y1=-5000;
		y2=5000;
	}
}
#endif //NO_SHADOWS

//maybe standardize all these things in one place sometime (and maybe one day i should try to understand how i made them work)
void CShadowHandler::GetFrustumSide(float3& side,bool upside)
{
	fline temp;
	
	float3 b=UpVector.cross(side);		//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)<0.0001)
		b.z=0.00011f;
	if(fabs(b.z)>0.0001){
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(side);			//get vector from camera to collision line
		c.Normalize();
		float3 colpoint;				//a point on the collision line

		if(side.y>0){
			if(b.dot(UpVector.cross(cam2->forward))<0 && upside){
				colpoint=cam2->pos+cam2->forward*20000;
				//info->AddLine("upward frustum");
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


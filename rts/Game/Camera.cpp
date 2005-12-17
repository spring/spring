#include "System/StdAfx.h"
// camera->cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "Camera.h"

#include "math.h"
#include "Rendering/GL/myGL.h"
#include "UI/InfoConsole.h"
//#include "System/mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera* camera;
CCamera* cam2;

CCamera::CCamera()
{
	fov=45;
	oldFov=0;
	forward=float3(1,0,0);
	rot=float3(0,0,0);
	up=UpVector;

	rot.y=0;
	rot.x=0;
	pos.x=2000;
	pos.y=70;
	pos.z=1800;
}

CCamera::~CCamera()
{

}

void CCamera::operator=(const CCamera& c)
{
	pos=c.pos;
	pos2=pos;
	rot=c.rot;	
	forward=c.forward;
	right=c.right;
	up=c.up;
	top=c.top;
	bottom=c.bottom;
	rightside=c.rightside;
	leftside=c.leftside;
	fov=c.fov;
	oldFov=c.oldFov;
}

void CCamera::Update(bool freeze)
{
	pos2=pos;
//	fov=60;
	up.Normalize();

	right=forward.cross(up);
	right.Normalize();

	up=right.cross(forward);
	up.Normalize();

	float viewx=(float)tan(float(gu->screenx)/gu->screeny*PI/4*fov/90);
	float viewy=(float)tan(PI/4*fov/90);

	bottom=-forward*viewy-up;
	bottom.Normalize();
	top=up-forward*viewy;
	top.Normalize();
	rightside=-forward*viewx+right;
	rightside.Normalize();
	leftside=-forward*viewx-right;
	leftside.Normalize();

	if(!freeze){
		cam2->bottom=bottom;
		cam2->forward=forward;
		cam2->leftside=leftside;
		cam2->pos=pos;
		cam2->right=right;
		cam2->rightside=rightside;
		cam2->rot=rot;
		cam2->top=top;
		cam2->up=up;
	}
	oldFov=fov;
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	gluPerspective(fov,(GLfloat)gu->screenx/(GLfloat)gu->screeny,NEAR_PLANE,gu->viewRange);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix

	glLoadIdentity();
	gluLookAt(camera->pos.x,camera->pos.y,camera->pos.z,camera->pos.x+camera->forward.x,camera->pos.y+camera->forward.y,camera->pos.z+camera->forward.z,camera->up.x,camera->up.y,camera->up.z);
}

bool CCamera::InView(const float3 &p,float radius)
{
	float3 t=p-pos;
	float l=t.Length();
	if(l<20)
		return true;
	else if (l>MAX_VIEW_RANGE)
		return false;
	if(t.dot(rightside)>radius)
		return false;
	if(t.dot(leftside)>radius)
		return false;
	if(t.dot(bottom)>radius)
		return false;
	if(t.dot(top)>radius)
		return false;
	return true;
}

void CCamera::UpdateForward()
{
	forward.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	forward.y=(float)(sin(camera->rot.x));
	forward.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	forward.Normalize();
}

float3 CCamera::CalcPixelDir(int x, int y)
{
	float dx=float(x-gu->screenx/2)/gu->screeny*tan(fov/180/2*PI)*2;
	float dy=float(y-gu->screeny/2)/gu->screeny*tan(fov/180/2*PI)*2;
	float3 dir=camera->forward-camera->up*dy+camera->right*dx;
	dir.Normalize();
	return dir;
}

//not used from here
/*
float frustum[6][4];
void ExtractFrustum()
{
   float   proj[16];
   float   modl[16];
   float   clip[16];
   float   t;

   //* Get the current PROJECTION matrix from OpenGL 
   glGetFloatv( GL_PROJECTION_MATRIX, proj );

   //* Get the current MODELVIEW matrix from OpenGL 
   glGetFloatv( GL_MODELVIEW_MATRIX, modl );

   //* Combine the two matrices (multiply projection by modelview) 
   clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
   clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
   clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
   clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

   clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
   clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
   clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
   clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

   clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
   clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
   clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
   clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

   clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
   clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
   clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
   clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

   /* Extract the numbers for the RIGHT plane 
   frustum[0][0] = clip[ 3] - clip[ 0];
   frustum[0][1] = clip[ 7] - clip[ 4];
   frustum[0][2] = clip[11] - clip[ 8];
   frustum[0][3] = clip[15] - clip[12];

   /* Normalize the result 
   t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
   frustum[0][0] /= t;
   frustum[0][1] /= t;
   frustum[0][2] /= t;
   frustum[0][3] /= t;

   /* Extract the numbers for the LEFT plane 
   frustum[1][0] = clip[ 3] + clip[ 0];
   frustum[1][1] = clip[ 7] + clip[ 4];
   frustum[1][2] = clip[11] + clip[ 8];
   frustum[1][3] = clip[15] + clip[12];

   /* Normalize the result 
   t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
   frustum[1][0] /= t;
   frustum[1][1] /= t;
   frustum[1][2] /= t;
   frustum[1][3] /= t;

   /* Extract the BOTTOM plane 
   frustum[2][0] = clip[ 3] + clip[ 1];
   frustum[2][1] = clip[ 7] + clip[ 5];
   frustum[2][2] = clip[11] + clip[ 9];
   frustum[2][3] = clip[15] + clip[13];

   /* Normalize the result 
   t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
   frustum[2][0] /= t;
   frustum[2][1] /= t;
   frustum[2][2] /= t;
   frustum[2][3] /= t;

   /* Extract the TOP plane 
   frustum[3][0] = clip[ 3] - clip[ 1];
   frustum[3][1] = clip[ 7] - clip[ 5];
   frustum[3][2] = clip[11] - clip[ 9];
   frustum[3][3] = clip[15] - clip[13];

   /* Normalize the result 
   t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
   frustum[3][0] /= t;
   frustum[3][1] /= t;
   frustum[3][2] /= t;
   frustum[3][3] /= t;

   /* Extract the FAR plane 
   frustum[4][0] = clip[ 3] - clip[ 2];
   frustum[4][1] = clip[ 7] - clip[ 6];
   frustum[4][2] = clip[11] - clip[10];
   frustum[4][3] = clip[15] - clip[14];

   /* Normalize the result 
   t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
   frustum[4][0] /= t;
   frustum[4][1] /= t;
   frustum[4][2] /= t;
   frustum[4][3] /= t;

   /* Extract the NEAR plane 
   frustum[5][0] = clip[ 3] + clip[ 2];
   frustum[5][1] = clip[ 7] + clip[ 6];
   frustum[5][2] = clip[11] + clip[10];
   frustum[5][3] = clip[15] + clip[14];

   /* Normalize the result 
   t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
   frustum[5][0] /= t;
   frustum[5][1] /= t;
   frustum[5][2] /= t;
   frustum[5][3] /= t;
}

bool SphereInFrustum( float x, float y, float z, float radius )
{
   int p;

   for( p = 0; p < 6; p++ )
      if( frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= -radius )
         return false;
   return true;
}

bool PointInFrustum( float x, float y, float z )
{
   int p;

   for( p = 0; p < 6; p++ )
      if( frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= 0 )
         return false;
   return true;
}

*/


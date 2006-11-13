#include "StdAfx.h"
// camera->cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "Camera.h"

#include "Rendering/GL/myGL.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "mmgr.h"

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

	float viewx=(float)tan(float(gu->viewSizeX)/gu->viewSizeY*PI/4*fov/90);
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
	float rangemod=1+max(0.f,pos.y-ground->GetHeight(pos.x,pos.z)-500)*0.0003f;
	gu->viewRange=MAX_VIEW_RANGE*rangemod;

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	gluPerspective(fov,(GLfloat)gu->viewSizeX/(GLfloat)gu->viewSizeY,NEAR_PLANE*rangemod,gu->viewRange);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();
	gluLookAt(camera->pos.x,camera->pos.y,camera->pos.z,camera->pos.x+camera->forward.x,camera->pos.y+camera->forward.y,camera->pos.z+camera->forward.z,camera->up.x,camera->up.y,camera->up.z);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = gu->viewSizeX;
	viewport[3] = gu->viewSizeY;
}

bool CCamera::InView(const float3 &p,float radius)
{
	float3 t=p-pos;
	float l=t.Length();
	if(l<50)
		return true;
	else if (l>gu->viewRange)
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
	float dx=float(x-gu->viewPosX-gu->viewSizeX/2)/gu->viewSizeY*tan(fov/180/2*PI)*2;
	float dy=float(y-gu->viewSizeY/2)/gu->viewSizeY*tan(fov/180/2*PI)*2;
	float3 dir=camera->forward-camera->up*dy+camera->right*dx;
	dir.Normalize();
	return dir;
}

float3 CCamera::CalcWindowCoordinates(const float3& objPos)
{
	double winPos[3];
	gluProject((double)objPos.x, (double)objPos.y, (double)objPos.z,
	           modelview, projection, viewport,
	           &winPos[0], &winPos[1], &winPos[2]);
	return float3((float)winPos[0], (float)winPos[1], (float)winPos[2]);
}


#include "StdAfx.h"
#include "BaseGroundDrawer.h"
#include "ConfigHandler.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "myGL.h"
#include "Camera.h"
#include "ReadMap.h"
//#include "mmgr.h"

CBaseGroundDrawer* groundDrawer;

CBaseGroundDrawer::CBaseGroundDrawer(void)
{
	viewRadius=configHandler.GetInt("GroundDetail",40);
	viewRadius+=viewRadius%2;

	baseTreeDistance=configHandler.GetInt("TreeRadius",(unsigned int) (5.5f*256))/256.0;

	updateFov=true;

	striptype=GL_TRIANGLE_STRIP;

	infoTexAlpha=0.25f;

	drawLos=false;
	drawExtraTex=false;
	drawHeightMap=false;
	infoTex=0;
}

CBaseGroundDrawer::~CBaseGroundDrawer(void)
{
	if(infoTex!=0)
		glDeleteTextures (1, &infoTex);
	configHandler.SetInt("GroundDetail",viewRadius);
	configHandler.SetInt("TreeRadius",(unsigned int)(baseTreeDistance*256));
}

void CBaseGroundDrawer::AddFrustumRestraint(float3 side)
{
	fline temp;
	float3 up(0,1,0);
	
	float3 b=up.cross(side);		//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)<0.0001f)
		b.z=0.0001f;
	{
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(side);			//get vector from camera to collision line
		float3 colpoint;				//a point on the collision line
		
		if(side.y>0)								
			colpoint=cam2->pos-c*((cam2->pos.y-(readmap->minheight-100))/c.y);
		else
			colpoint=cam2->pos-c*((cam2->pos.y-(readmap->maxheight+30))/c.y);
		
		
		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		if(b.z>0){
			left.push_back(temp);			
		}else{
			right.push_back(temp);
		}
	}	
}


void CBaseGroundDrawer::DrawShadowPass(void)
{
}

void CBaseGroundDrawer::UpdateCamRestraints(void)
{
	left.clear();
	right.clear();

	//Add restraints for camera sides
	AddFrustumRestraint(cam2->bottom);
	AddFrustumRestraint(cam2->top);
	AddFrustumRestraint(cam2->rightside);
	AddFrustumRestraint(cam2->leftside);

	//Add restraint for maximum view distance
	fline temp;
	float3 up(0,1,0);
	float3 side=cam2->forward;
	float3 camHorizontal=cam2->forward;
	camHorizontal.y=0;
	camHorizontal.Normalize();
	float3 b=up.cross(camHorizontal);			//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)>0.0001){
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(camHorizontal);			//get vector from camera to collision line
		float3 colpoint;				//a point on the collision line
		
		if(side.y>0)								
			colpoint=cam2->pos+camHorizontal*gu->viewRange*1.05f-c*(cam2->pos.y/c.y);
		else
			colpoint=cam2->pos+camHorizontal*gu->viewRange*1.05f-c*((cam2->pos.y-255/3.5f)/c.y);
		
		
		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		if(b.z>0){
			left.push_back(temp);			
		}else{
			right.push_back(temp);
		}
	}

}

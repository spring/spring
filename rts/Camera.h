// camera->h: interface for the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "archdef.h"

class CCamera  
{
public:
	float3 CalcPixelDir(int x,int y);
	void UpdateForward();
	bool InView(const float3& p,float radius=0);
	void Update(bool freeze);
	CCamera();
	virtual ~CCamera();
	void operator=(const CCamera& c);
	float3 pos;
	float3 pos2;		//use this for calculating orthodirections (might differ from pos when calcing shadows)
	float3 rot;			//varning inte alltid uppdaterad
	float3 forward;
	float3 right;
	float3 up;
	float3 top;
	float3 bottom;
	float3 rightside;
	float3 leftside;
	float fov;
	float oldFov;
};

extern CCamera* camera;
extern CCamera* cam2;

#endif // __CAMERA_H__

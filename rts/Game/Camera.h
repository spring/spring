// camera->h: interface for the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "Rendering/GL/myGL.h"


class CCamera  
{
public:
	float3 CalcPixelDir(int x,int y);
	float3 CalcWindowCoordinates(const float3& objPos);
	void UpdateForward();
	bool InView(const float3& p,float radius=0);
	bool InView(const float3& mins, const float3& maxs);
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
	float3 posOffset;
	float3 tiltOffset;
	float fov;
	float oldFov;
	float lppScale; // length-per-pixel scale
	GLdouble modelview[16];
	GLdouble projection[16];
	GLdouble billboard[16];
	GLdouble modelviewInverse[16];
	GLint viewport[4];

	static unsigned int billboardList;
};

extern CCamera* camera;
extern CCamera* cam2;

#endif // __CAMERA_H__

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
	void Update(bool freeze, bool resetUp = true);

	CCamera();
	~CCamera();
	
	void Roll(float rad);
	void Pitch(float rad);

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
	
	float lppScale; // length-per-pixel scale

	GLint viewport[4];

	const GLdouble* GetProjMat() const { return projMat; }
	const GLdouble* GetViewMat() const { return viewMat; }
	const GLdouble* GetViewMatInv() const { return viewMatInv; }
	const GLdouble* GetBBoardMat() const { return bboardMat; }

	float GetFov() const { return fov; }
	float GetHalfFov() const { return halfFov; }
	float GetTanHalfFov() const { return tanHalfFov; }
	
	static GLuint billboardList;
	
	void SetFov(float fov); // in degree
	
private:
	void myGluPerspective(float, float, float);
	void myGluLookAt(const float3&, const float3&, const float3&);

	GLdouble projMat[16];
	GLdouble viewMat[16];
	GLdouble viewMatInv[16];
	GLdouble bboardMat[16];

	float fov; // in degrees
	float halfFov; // half the fov in radians
	float tanHalfFov; // tan(halfFov)
};

extern CCamera* camera;
extern CCamera* cam2;

#endif // __CAMERA_H__

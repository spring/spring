/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "Rendering/GL/myGL.h"
#include "System/float3.h"
#include "System/Matrix44f.h"


class CCamera
{
public:
	CCamera();

	float3 CalcPixelDir(int x,int y) const;
	float3 CalcWindowCoordinates(const float3& objPos) const;
	void UpdateForward();
	bool InView(const float3& p, float radius = 0);
	bool InView(const float3& mins, const float3& maxs);
	void Update(bool freeze, bool resetUp = true);

	const CMatrix44f& GetViewMatrix() const { return viewMatrix; }
	const CMatrix44f& GetViewMatrixInverse() const { return viewMatrixInverse; }
	const CMatrix44f& GetProjectionMatrix() const { return projectionMatrix; }
	const CMatrix44f& GetProjectionMatrixInverse() const { return projectionMatrixInverse; }
	const CMatrix44f& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const CMatrix44f& GetViewProjectionMatrixInverse() const { return viewProjectionMatrixInverse; }
	const CMatrix44f& GetBillBoardMatrix() const { return billboardMatrix; }

	float GetFov() const { return fov; }
	float GetHalfFov() const { return halfFov; }
	float GetTanHalfFov() const { return tanHalfFov; }

	/// @param fov in degree
	void SetFov(float fov);


	float3 pos;
	float3 pos2;       ///< use this for calculating orthodirections (might differ from pos when calcing shadows)
	float3 rot;        ///< warning is not always updated
	float3 forward;
	float3 right;
	float3 up;
	float3 top;
	float3 bottom;
	float3 rightside;
	float3 leftside;
	float3 posOffset;
	float3 tiltOffset;

	float lppScale;    ///< length-per-pixel scale

	GLint viewport[4];

private:
	void myGluPerspective(float aspect, float zNear, float zFar);
	void myGluLookAt(const float3&, const float3&, const float3&);

	CMatrix44f projectionMatrix;
	CMatrix44f projectionMatrixInverse;
	CMatrix44f viewMatrix;
	CMatrix44f viewMatrixInverse;
	CMatrix44f viewProjectionMatrix;
	CMatrix44f viewProjectionMatrixInverse;
	CMatrix44f billboardMatrix;

	std::vector<GLdouble> viewMatrixD;
	std::vector<GLdouble> projectionMatrixD;

	float fov; ///< in degrees
	float halfFov; ///< half the fov in radians
	float tanHalfFov; ///< tan(halfFov)
};

extern CCamera* camera;
extern CCamera* cam2;

#endif // __CAMERA_H__

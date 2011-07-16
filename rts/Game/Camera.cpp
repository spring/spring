/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string.h>
#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "Camera.h"
#include "Map/Ground.h"
#include "System/myMath.h"
#include "System/float3.h"
#include "System/Matrix44f.h"
#include "Rendering/GlobalRendering.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera* camera;
CCamera* cam2;



inline void GetGLdoubleMatrix(const CMatrix44f& m, GLdouble* dm)
{
	for (int i=0; i<16; i+=4) {
		dm[i+0] = m[i+0];
		dm[i+1] = m[i+1];
		dm[i+2] = m[i+2];
		dm[i+3] = m[i+3];
	}
}


CCamera::CCamera()
	: pos(2000.0f, 70.0f, 1800.0f)
	, rot(0.0f, 0.0f, 0.0f)
	, forward(1.0f, 0.0f, 0.0f)
	, posOffset(0.0f, 0.0f, 0.0f)
	, tiltOffset(0.0f, 0.0f, 0.0f)
	, lppScale(0.0f)
	, viewMatrixD(16, 0.)
	, projectionMatrixD(16, 0.)
	, fov(0.0f)
	, halfFov(0.0f)
	, tanHalfFov(0.0f)
{
	memset(viewport, 0, 4*sizeof(int));

	// stuff that will not change can be initialised here,
	// so it does not need to be reinitialised every update
	projectionMatrix[15] = 0.0f;
	billboardMatrix[15] = 1.0f;

	SetFov(45.0f);

	up = UpVector;
}


void CCamera::Update(bool freeze, bool resetUp)
{
	pos2 = pos;

	if (resetUp) {
		up = UpVector;
	}

	right = forward.cross(up);
	right.UnsafeANormalize();

	up = right.cross(forward);
	up.UnsafeANormalize();

	const float aspect = globalRendering->aspectRatio;
	const float viewx = tan(aspect * halfFov);
	const float viewy = tanHalfFov;

	if (globalRendering->viewSizeY <= 0) {
		lppScale = 0.0f;
	} else {
		const float span = 2.0f * tanHalfFov;
		lppScale = span / (float) globalRendering->viewSizeY;
	}

	const float3 forwardy = (-forward * viewy);
	top    = forwardy + up;
	top.UnsafeANormalize();
	bottom = forwardy - up;
	bottom.UnsafeANormalize();

	const float3 forwardx = (-forward * viewx);
	rightside = forwardx + right;
	rightside.UnsafeANormalize();
	leftside  = forwardx - right;
	leftside.UnsafeANormalize();

	if (!freeze) {
		cam2->bottom    = bottom;
		cam2->forward   = forward;
		cam2->leftside  = leftside;
		cam2->pos       = pos;
		cam2->right     = right;
		cam2->rightside = rightside;
		cam2->rot       = rot;
		cam2->top       = top;
		cam2->up        = up;
		cam2->lppScale  = lppScale;
	}

	const float gndHeight = ground->GetHeightAboveWater(pos.x, pos.z, false);
	const float rangemod = 1.0f + std::max(0.0f, pos.y - gndHeight - 500.0f) * 0.0003f;
	const float zNear = (CGlobalRendering::NEAR_PLANE * rangemod);

	globalRendering->viewRange = CGlobalRendering::MAX_VIEW_RANGE * rangemod;
	
	// apply and store the projection transform
	glMatrixMode(GL_PROJECTION);
	myGluPerspective(aspect, zNear, globalRendering->viewRange);

	
	// FIXME: should be applying the offsets to pos/up/right/forward/etc,
	//        but without affecting the real positions (need an intermediary)
	float3 fShake = (forward * (1.0f + tiltOffset.z)) +
	                (right   * tiltOffset.x) +
	                (up      * tiltOffset.y);

	const float3 camPos = pos + posOffset;
	const float3 center = camPos + fShake.ANormalize();

	// apply and store the view transform
	glMatrixMode(GL_MODELVIEW);
	myGluLookAt(camPos, center, up);

	// create extra matrices
	viewProjectionMatrix = viewMatrix * projectionMatrix;
	viewMatrixInverse = viewMatrix.InvertAffine();
	projectionMatrixInverse = projectionMatrix.Invert();
	viewProjectionMatrixInverse = viewProjectionMatrix.Invert();

	// GLdouble versions
	GetGLdoubleMatrix(viewMatrix, &viewMatrixD[0]);
	GetGLdoubleMatrix(projectionMatrix, &projectionMatrixD[0]);

	// Billboard Matrix
	billboardMatrix = viewMatrix;
	billboardMatrix.SetPos(ZeroVector);
	billboardMatrix.Transpose(); //! viewMatrix is affine, equals inverse
	billboardMatrix[15] = 1.0f; //! SetPos() touches m[15]

	// viewport
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = globalRendering->viewSizeX;
	viewport[3] = globalRendering->viewSizeY;
}


static inline bool AABBInOriginPlane(const float3& plane, const float3& camPos,
                                     const float3& mins, const float3& maxs)
{
	float3 fp; // far point
	fp.x = (plane.x > 0.0f) ? mins.x : maxs.x;
	fp.y = (plane.y > 0.0f) ? mins.y : maxs.y;
	fp.z = (plane.z > 0.0f) ? mins.z : maxs.z;
	return (plane.dot(fp - camPos) < 0.0f);
}


bool CCamera::InView(const float3& mins, const float3& maxs)
{
	// Axis-aligned bounding box test  (AABB)
	if (AABBInOriginPlane(rightside, pos, mins, maxs) &&
	    AABBInOriginPlane(leftside,  pos, mins, maxs) &&
	    AABBInOriginPlane(bottom,    pos, mins, maxs) &&
	    AABBInOriginPlane(top,       pos, mins, maxs)) {
		return true;
	}
	return false;
}

bool CCamera::InView(const float3& p, float radius)
{
	const float3 t   = (p - pos);
	const float  lsq = t.SqLength();

	if (lsq < 2500.0f) { //FIXME WHY?
		return true;
	}
	else if (lsq > Square(globalRendering->viewRange)) {
		return false;
	}

	if ((t.dot(rightside) > radius) ||
	    (t.dot(leftside)  > radius) ||
	    (t.dot(bottom)    > radius) ||
	    (t.dot(top)       > radius)) {
		return false;
	}

	return true;
}


void CCamera::UpdateForward()
{
	forward.z = cos(rot.y) * cos(rot.x);
	forward.x = sin(rot.y) * cos(rot.x);
	forward.y = sin(rot.x);
	forward.Normalize();
}


void CCamera::SetFov(float myfov)
{
	fov = myfov;
	halfFov = fov * 0.5f * PI / 180.f;
	tanHalfFov = tan(halfFov);
}


float3 CCamera::CalcPixelDir(int x, int y) const
{
	const int vsx = std::max(1, globalRendering->viewSizeX);
	const int vsy = std::max(1, globalRendering->viewSizeY);

	const float dx = float(x - globalRendering->viewPosX - (vsx >> 1)) / vsy * (tanHalfFov * 2.0f);
	const float dy = float(y -                             (vsy >> 1)) / vsy * (tanHalfFov * 2.0f);

	const float3 dir = (forward - up * dy + right * dx).Normalize();
	return dir;
}


float3 CCamera::CalcWindowCoordinates(const float3& objPos) const
{
	double winPos[3];
	gluProject((GLdouble)objPos.x, (GLdouble)objPos.y, (GLdouble)objPos.z,
	           &viewMatrixD[0], &projectionMatrixD[0], viewport,
	           &winPos[0], &winPos[1], &winPos[2]);
	return float3((float)winPos[0], (float)winPos[1], (float)winPos[2]);
}


inline void CCamera::myGluPerspective(float aspect, float zNear, float zFar) {
	const float t = zNear * tanHalfFov;
	const float b = -t;
	const float l = b * aspect;
	const float r = t * aspect;

	projectionMatrix[ 0] = (2.0f * zNear) / (r - l);

	projectionMatrix[ 5] = (2.0f * zNear) / (t - b);

	projectionMatrix[ 8] = (r + l) / (r - l);
	projectionMatrix[ 9] = (t + b) / (t - b);
	projectionMatrix[10] = -(zFar + zNear) / (zFar - zNear);
	projectionMatrix[11] = -1.0f;

	projectionMatrix[14] = -(2.0f * zFar * zNear) / (zFar - zNear);

	glLoadMatrixf(projectionMatrix);
}


inline void CCamera::myGluLookAt(const float3& eye, const float3& center, const float3& up) {
	const float3 f = (center - eye).ANormalize();
	const float3 s = f.cross(up);
	const float3 u = s.cross(f);

	viewMatrix[ 0] =  s.x;
	viewMatrix[ 1] =  u.x;
	viewMatrix[ 2] = -f.x;

	viewMatrix[ 4] =  s.y;
	viewMatrix[ 5] =  u.y;
	viewMatrix[ 6] = -f.y;

	viewMatrix[ 8] =  s.z;
	viewMatrix[ 9] =  u.z;
	viewMatrix[10] = -f.z;

	// save a glTranslated(-eye.x, -eye.y, -eye.z) call
	viewMatrix[12] = ( s.x * -eye.x) + ( s.y * -eye.y) + ( s.z * -eye.z);
	viewMatrix[13] = ( u.x * -eye.x) + ( u.y * -eye.y) + ( u.z * -eye.z);
	viewMatrix[14] = (-f.x * -eye.x) + (-f.y * -eye.y) + (-f.z * -eye.z);

	glLoadMatrixf(viewMatrix);
}

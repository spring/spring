/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Camera.h"
#include "Map/Ground.h"
#include "System/myMath.h"
#include "System/Matrix44f.h"
#include "System/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera* camera;
CCamera* cam2;

unsigned int CCamera::billboardList = 0;

CCamera::CCamera() :
	pos(2000.0f, 70.0f, 1800.0f),
	rot(0.0f, 0.0f, 0.0f),
	forward(1.0f, 0.0f, 0.0f),   posOffset(0.0f, 0.0f, 0.0f), tiltOffset(0.0f, 0.0f, 0.0f), lppScale(0.0f)
{
	// stuff that wont change can be initialised here, it doesn't need to be reinitialised every update
	viewMat[ 3] =  0.0f;
	viewMat[ 7] =  0.0f;
	viewMat[11] =  0.0f;
	viewMat[15] =  1.0f;

	projMat[ 1] = 0.0f;
	projMat[ 2] = 0.0f;
	projMat[ 3] = 0.0f;

	projMat[ 4] = 0.0f;
	projMat[ 6] = 0.0f;
	projMat[ 7] = 0.0f;

	projMat[12] = 0.0f;
	projMat[13] = 0.0f;
	projMat[15] = 0.0f;

	bboardMat[ 3] = bboardMat[ 7] = bboardMat[11] = 0.0;
	bboardMat[12] = bboardMat[13] = bboardMat[14] = 0.0;
	bboardMat[15] = 1.0;

	SetFov(45.0f);

	up      = UpVector;

	if (billboardList == 0) {
		billboardList = glGenLists(1);
	}
}

CCamera::~CCamera()
{
	glDeleteLists(billboardList, 1);
}

void CCamera::Roll(float rad)
{
	CMatrix44f rotate;
	rotate.Rotate(rad, forward);
	up = rotate.Mul(up);
}

void CCamera::Pitch(float rad)
{
	CMatrix44f rotate;
	rotate.Rotate(rad, right);
	forward = rotate.Mul(forward);
	forward.Normalize();
	rot.y = atan2(forward.x,forward.z);
	rot.x = asin(forward.y);
	UpdateForward();
}

void CCamera::Update(bool freeze, bool resetUp)
{
	pos2 = pos;

	if (resetUp) {
		up.x = 0.0f;
		up.y = 1.0f;
		up.z = 0.0f;
	}

	right = forward.cross(up);
	right.UnsafeANormalize();

	up = right.cross(forward);
	up.UnsafeANormalize();

	const float aspect = globalRendering->aspectRatio;
	const float viewx = tan(aspect * halfFov);
	// const float viewx = aspect * tanHalfFov;
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

	const float gndHeight = ground->GetHeight(pos.x, pos.z);
	const float rangemod = 1.0f + std::max(0.0f, pos.y - gndHeight - 500.0f) * 0.0003f;
	const float zNear = (NEAR_PLANE * rangemod);
	globalRendering->viewRange = MAX_VIEW_RANGE * rangemod;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// apply and store the projection transform
	myGluPerspective(aspect, zNear, globalRendering->viewRange);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// FIXME: should be applying the offsets to pos/up/right/forward/etc,
	//        but without affecting the real positions (need an intermediary)
	float3 fShake = (forward * (1.0f + tiltOffset.z)) +
	                (right   * tiltOffset.x) +
	                (up      * tiltOffset.y);

	const float3 camPos = pos + posOffset;
	const float3 center = camPos + fShake.ANormalize();

	// apply and store the view transform
	myGluLookAt(camPos, center, up);


	// get the inverse view matrix (handy for shaders to have)
	CMatrix44f::Invert((double(*)[4]) viewMat, (double(*)[4]) viewMatInv);

	// transpose the 3x3
	bboardMat[ 0] = viewMat[ 0];
	bboardMat[ 1] = viewMat[ 4];
	bboardMat[ 2] = viewMat[ 8];
	bboardMat[ 4] = viewMat[ 1];
	bboardMat[ 5] = viewMat[ 5];
	bboardMat[ 6] = viewMat[ 9];
	bboardMat[ 8] = viewMat[ 2];
	bboardMat[ 9] = viewMat[ 6];
	bboardMat[10] = viewMat[10];

	glNewList(billboardList, GL_COMPILE);
	glMultMatrixd(bboardMat);
	glEndList();

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

bool CCamera::InView(const float3 &p, float radius)
{
	const float3 t   = (p - pos);
	const float  lsq = t.SqLength();

	if (lsq < 2500.0f) {
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


float3 CCamera::CalcPixelDir(int x, int y)
{
	float dx = float(x-globalRendering->viewPosX-globalRendering->viewSizeX/2)/globalRendering->viewSizeY * tanHalfFov * 2;
	float dy = float(y-globalRendering->viewSizeY/2)/globalRendering->viewSizeY * tanHalfFov * 2;
	float3 dir = forward - up * dy + right * dx;
	dir.Normalize();
	return dir;
}


float3 CCamera::CalcWindowCoordinates(const float3& objPos)
{
	double winPos[3];
	gluProject((GLdouble)objPos.x, (GLdouble)objPos.y, (GLdouble)objPos.z,
	           viewMat, projMat, viewport,
	           &winPos[0], &winPos[1], &winPos[2]);
	return float3((float)winPos[0], (float)winPos[1], (float)winPos[2]);
}

inline void CCamera::myGluPerspective(float aspect, float zNear, float zFar) {
	GLdouble t = zNear * tanHalfFov;
	GLdouble b = -t;
	GLdouble l = b * aspect;
	GLdouble r = t * aspect;

	projMat[ 0] = (2.0f * zNear) / (r - l);

	projMat[ 5] = (2.0f * zNear) / (t - b);

	projMat[ 8] = (r + l) / (r - l);
	projMat[ 9] = (t + b) / (t - b);
	projMat[10] = -(zFar + zNear) / (zFar - zNear);
	projMat[11] = -1.0f;

	projMat[14] = -(2.0f * zFar * zNear) / (zFar - zNear);

	glMultMatrixd(projMat);
}

inline void CCamera::myGluLookAt(const float3& eye, const float3& center, const float3& up) {
	float3 f = (center - eye).ANormalize();
	float3 s = f.cross(up);
	float3 u = s.cross(f);

	viewMat[ 0] =  s.x;
	viewMat[ 1] =  u.x;
	viewMat[ 2] = -f.x;

	viewMat[ 4] =  s.y;
	viewMat[ 5] =  u.y;
	viewMat[ 6] = -f.y;

	viewMat[ 8] =  s.z;
	viewMat[ 9] =  u.z;
	viewMat[10] = -f.z;

	// save a glTranslated(-eye.x, -eye.y, -eye.z) call
	viewMat[12] = ( s.x * -eye.x) + ( s.y * -eye.y) + ( s.z * -eye.z);
	viewMat[13] = ( u.x * -eye.x) + ( u.y * -eye.y) + ( u.z * -eye.z);
	viewMat[14] = (-f.x * -eye.x) + (-f.y * -eye.y) + (-f.z * -eye.z);

	glMultMatrixd(viewMat);
}

void CCamera::SetFov(float myfov)
{
	fov = myfov;
	halfFov = fov * 0.008726646f;
	tanHalfFov = tan(halfFov);
}

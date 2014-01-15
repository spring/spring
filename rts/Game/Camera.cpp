/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string.h>

#include "Camera.h"
#include "UI/MouseHandler.h"
#include "Map/ReadMap.h"
#include "System/myMath.h"
#include "System/float3.h"
#include "System/Matrix44f.h"
#include "Rendering/GlobalRendering.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera* camera;
CCamera* cam2;


CCamera::CCamera()
	: rot(ZeroVector)
	, forward(RgtVector)
	, up(UpVector)
	, posOffset(ZeroVector)
	, tiltOffset(ZeroVector)
	, pos(ZeroVector)
	, fov(0.0f)
	, halfFov(0.0f)
	, tanHalfFov(0.0f)
	, lppScale(0.0f)
{
	if (gs) {
		// center map
		pos = float3(gs->mapx * 0.5f * SQUARE_SIZE, 1000.f, gs->mapy * 0.5f * SQUARE_SIZE);
	}

	memset(viewport, 0, 4 * sizeof(int));
	memset(movState, 0, sizeof(movState));
	memset(rotState, 0, sizeof(rotState));

	// stuff that will not change can be initialised here,
	// so it does not need to be reinitialised every update
	projectionMatrix[15] = 0.0f;
	billboardMatrix[15] = 1.0f;

	SetFov(45.0f);
}

void CCamera::CopyState(const CCamera* cam) {
	topFrustumSideDir = cam->topFrustumSideDir;
	botFrustumSideDir = cam->botFrustumSideDir;
	rgtFrustumSideDir = cam->rgtFrustumSideDir;
	lftFrustumSideDir = cam->lftFrustumSideDir;

	forward   = cam->forward;
	right     = cam->right;
	up        = cam->up;

	pos       = cam->pos;
	rot       = cam->rot;

	lppScale  = cam->lppScale;
}

void CCamera::Update(bool terrainReflectionPass)
{
	UpdateRightAndUp(terrainReflectionPass);

	const float aspect = globalRendering->aspectRatio;
	const float viewx = math::tan(aspect * halfFov);
	const float viewy = tanHalfFov;

	if (globalRendering->viewSizeY <= 0) {
		lppScale = 0.0f;
	} else {
		lppScale = (2.0f * tanHalfFov) / globalRendering->viewSizeY;
	}

	const float3 forwardy = (-forward * viewy);
	const float3 forwardx = (-forward * viewx);

	// note: top- and bottom-dir should be parallel to <forward>
	topFrustumSideDir = (forwardy +    up).UnsafeANormalize();
	botFrustumSideDir = (forwardy -    up).UnsafeANormalize();
	rgtFrustumSideDir = (forwardx + right).UnsafeANormalize();
	lftFrustumSideDir = (forwardx - right).UnsafeANormalize();

	if (this == camera)
		cam2->CopyState(this);

	// apply and store the projection transform
	ComputeViewRange();
	glMatrixMode(GL_PROJECTION);
	myGluPerspective(aspect, globalRendering->zNear, globalRendering->viewRange);

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
	viewProjectionMatrix = projectionMatrix * viewMatrix;
	viewMatrixInverse = viewMatrix.InvertAffine();
	projectionMatrixInverse = projectionMatrix.Invert();
	viewProjectionMatrixInverse = viewProjectionMatrix.Invert();

	// Billboard Matrix
	billboardMatrix = viewMatrix;
	billboardMatrix.SetPos(ZeroVector);
	billboardMatrix.Transpose(); // viewMatrix is affine, equals inverse
	billboardMatrix[15] = 1.0f; // SetPos() touches m[15]

	// viewport
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = globalRendering->viewSizeX;
	viewport[3] = globalRendering->viewSizeY;
}


void CCamera::ComputeViewRange()
{
	float wantedViewRange = CGlobalRendering::MAX_VIEW_RANGE;

	const float azimuthCos       = forward.dot(UpVector);
	const float maxDistToBorderX = std::max(pos.x, float3::maxxpos - pos.x);
	const float maxDistToBorderZ = std::max(pos.z, float3::maxzpos - pos.z);
	const float minViewRange     = (1.0f - azimuthCos) * math::sqrt(Square(maxDistToBorderX) + Square(maxDistToBorderZ));

	// Camera-height dependent (i.e. TAB-view)
	wantedViewRange = std::max(wantedViewRange, (pos.y - std::max(0.0f, readMap->GetCurrMinHeight())) * 2.4f);
	// View-angle dependent (i.e. FPS-view)
	wantedViewRange = std::max(wantedViewRange, minViewRange);

	// Update
	const float factor = wantedViewRange / CGlobalRendering::MAX_VIEW_RANGE;

	globalRendering->zNear     = CGlobalRendering::NEAR_PLANE * factor;
	globalRendering->viewRange = CGlobalRendering::MAX_VIEW_RANGE * factor;
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


bool CCamera::InView(const float3& mins, const float3& maxs) const
{
	// Axis-aligned bounding box test  (AABB)
	if (!AABBInOriginPlane(rgtFrustumSideDir, pos, mins, maxs)) return false;
	if (!AABBInOriginPlane(lftFrustumSideDir, pos, mins, maxs)) return false;
	if (!AABBInOriginPlane(botFrustumSideDir, pos, mins, maxs)) return false;
	if (!AABBInOriginPlane(topFrustumSideDir, pos, mins, maxs)) return false;

	return true;
}

bool CCamera::InView(const float3& p, float radius) const
{
	const float3 t(p - pos);

	if ((t.dot(rgtFrustumSideDir) > radius) ||
	    (t.dot(lftFrustumSideDir) > radius) ||
	    (t.dot(botFrustumSideDir) > radius) ||
	    (t.dot(topFrustumSideDir) > radius)) {
		return false;
	}

	const float lsq = t.SqLength();
	if (lsq > Square(globalRendering->viewRange)) {
		return false;
	}

	return true;
}


void CCamera::UpdateRightAndUp(bool terrainReflectionPass)
{
	// terrain (not water) cubemap reflection passes set forward
	// to {+/-}UpVector which would cause vector degeneracy when
	// calculating right and up
	//
	if (std::fabs(forward.y) >= 0.99f) {
		// make sure we can still yaw at limits of pitch
		// (since CamHandler only updates forward, which
		// is derived from rot)
		right = float3(-std::cos(rot.y), 0.0f, std::sin(camera->rot.y));
		up = (right.cross(forward)).UnsafeANormalize();
	} else {
		// in the terrain reflection pass everything is upside-down!
		up = UpVector * -Sign(int(terrainReflectionPass));

		right = (forward.cross(up)).UnsafeANormalize();
		up = (right.cross(forward)).UnsafeANormalize();
	}
}


void CCamera::SetFov(float myfov)
{
	fov = myfov;
	halfFov = (fov * 0.5f) * (PI / 180.f);
	tanHalfFov = math::tan(halfFov);
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
	// does same as gluProject()
	const float4 v = viewProjectionMatrix * float4(objPos, 1.0f);
	float3 winPos;
	winPos.x = viewport[0] + viewport[2] * (v.x / v.w + 1.0f) * 0.5f;
	winPos.y = viewport[1] + viewport[3] * (v.y / v.w + 1.0f) * 0.5f;
	winPos.z =                             (v.z / v.w + 1.0f) * 0.5f;
	return winPos;
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



void CCamera::GetFrustumSides(float miny, float maxy, float scale, bool negSide) {

	ClearFrustumSides();
	// note: order does not matter
	GetFrustumSide(topFrustumSideDir, ZeroVector,  miny, maxy, scale,  (topFrustumSideDir.y > 0.0f), negSide);
	GetFrustumSide(botFrustumSideDir, ZeroVector,  miny, maxy, scale,  (botFrustumSideDir.y > 0.0f), negSide);
	GetFrustumSide(lftFrustumSideDir, ZeroVector,  miny, maxy, scale,  (lftFrustumSideDir.y > 0.0f), negSide);
	GetFrustumSide(rgtFrustumSideDir, ZeroVector,  miny, maxy, scale,  (rgtFrustumSideDir.y > 0.0f), negSide);
}

void CCamera::GetFrustumSide(
	const float3& zdir,
	const float3& offset,
	float miny,
	float maxy,
	float scale,
	bool upwardDir,
	bool negSide)
{
	// compose an orthonormal axis-system around <zdir>
	float3 xdir = (zdir.cross(UpVector)).UnsafeANormalize();
	float3 ydir = (zdir.cross(xdir)).UnsafeANormalize();

	// intersection of vector from <pos> along <ydir> with xz-plane
	float3 pInt;

	// prevent DIV0 when calculating line.dir
	if (math::fabs(xdir.z) < 0.001f)
		xdir.z = 0.001f;

	if (ydir.y != 0.0f) {
		// if <zdir> is angled toward the sky instead of the ground,
		// subtract <miny> from the camera's y-position, else <maxy>
		if (upwardDir) {
			pInt = (pos + offset) - ydir * ((pos.y - miny) / ydir.y);
		} else {
			pInt = (pos + offset) - ydir * ((pos.y - maxy) / ydir.y);
		}
	}

	// <line.dir> is the direction coefficient (0 ==> parallel to z-axis, inf ==> parallel to x-axis)
	// in the xz-plane; <line.base> is the x-coordinate at which line intersects x-axis; <line.sign>
	// indicates line direction, ie. left-to-right (whenever <xdir.z> is negative) or right-to-left
	// NOTE:
	//     (b.x / b.z) is actually the reciprocal of the DC (ie. the number of steps along +x for
	//     one step along +y); the world z-axis is inverted wrt. a regular Carthesian grid, so the
	//     DC is also inverted
	FrustumLine line;
	line.dir  = (xdir.x / xdir.z);
	line.base = (pInt.x - (pInt.z * line.dir)) / scale;
	line.sign = (xdir.z <= 0.0f)? 1: -1;
	line.minz = (                  0.0f) - (gs->mapy);
	line.maxz = (gs->mapy * SQUARE_SIZE) + (gs->mapy);

	if (line.sign == 1 || negSide) {
		negFrustumSides.push_back(line);
	} else {
		posFrustumSides.push_back(line);
	}
}

void CCamera::ClipFrustumLines(bool neg, const float zmin, const float zmax) {

	std::vector<FrustumLine>& lines = neg? negFrustumSides: posFrustumSides;
	std::vector<FrustumLine>::iterator fli, fli2;

	for (fli = lines.begin(); fli != lines.end(); ++fli) {
		for (fli2 = lines.begin(); fli2 != lines.end(); ++fli2) {
			if (fli == fli2)
				continue;

			const float dbase = fli->base - fli2->base;
			const float ddir = fli->dir - fli2->dir;

			if (ddir == 0.0f)
				continue;

			const float colz = -(dbase / ddir);

			if ((fli2->sign * ddir) > 0.0f) {
				if ((colz > fli->minz) && (colz < zmax))
					fli->minz = colz;
			} else {
				if ((colz < fli->maxz) && (colz > zmin))
					fli->maxz = colz;
			}
		}
	}
}



float CCamera::GetMoveDistance(float* time, float* speed, int idx) const
{
	// NOTE:
	//   lastFrameTime is MUCH smaller when map edge is in view
	//   timer is not accurate enough to return non-zero values
	//   for the majority of the time this condition holds, and
	//   so the camera will barely react to key input since most
	//   frames will effectively be 'skipped' (looks like lag)
	float camDeltaTime = globalRendering->lastFrameTime;
	float camMoveSpeed = 1.0f;

	camMoveSpeed *= (1.0f - movState[MOVE_STATE_SLW] * 0.9f);
	camMoveSpeed *= (1.0f + movState[MOVE_STATE_FST] * 9.0f);

	if (time != NULL) { *time = camDeltaTime; }
	if (speed != NULL) { *speed = camMoveSpeed; }

	switch (idx) {
		case MOVE_STATE_UP:  { camMoveSpeed *= ( 1.0f * movState[idx]); } break;
		case MOVE_STATE_DWN: { camMoveSpeed *= (-1.0f * movState[idx]); } break;

		default: {
		} break;
	}

	return (camDeltaTime * 0.2f * camMoveSpeed);
}

float3 CCamera::GetMoveVectorFromState(bool fromKeyState, bool* disableTracker)
{
	float camDeltaTime = 1.0f;
	float camMoveSpeed = 1.0f;

	(void) GetMoveDistance(&camDeltaTime, &camMoveSpeed, -1);

	float3 v = FwdVector * camMoveSpeed;

	if (fromKeyState) {
		v.y += (camDeltaTime * 0.001f * movState[MOVE_STATE_FWD]);
		v.y -= (camDeltaTime * 0.001f * movState[MOVE_STATE_BCK]);
		v.x += (camDeltaTime * 0.001f * movState[MOVE_STATE_RGT]);
		v.x -= (camDeltaTime * 0.001f * movState[MOVE_STATE_LFT]);
	} else {
		const int screenW = globalRendering->dualScreenMode?
			(globalRendering->viewSizeX << 1):
			(globalRendering->viewSizeX     );

		v.y += (camDeltaTime * 0.001f * (mouse->lasty <                               2));
		v.y -= (camDeltaTime * 0.001f * (mouse->lasty > (globalRendering->viewSizeY - 2)));
		v.x += (camDeltaTime * 0.001f * (mouse->lastx >                    (screenW - 2)));
		v.x -= (camDeltaTime * 0.001f * (mouse->lastx <                               2));
	}

	(*disableTracker) |= (v.x != 0.0f);
	(*disableTracker) |= (v.y != 0.0f);

	return v;
}


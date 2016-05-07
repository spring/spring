/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string.h>

#include "Camera.h"
#include "UI/MouseHandler.h"
#include "Map/ReadMap.h"
#include "System/myMath.h"
#include "System/float3.h"
#include "System/Matrix44f.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"


CONFIG(float, EdgeMoveWidth)
	.defaultValue(0.02f)
	.minimumValue(0.0f)
	.description("The width (in percent of screen size) of the EdgeMove scrolling area.");
CONFIG(bool, EdgeMoveDynamic)
	.defaultValue(true)
	.description("If EdgeMove scrolling speed should fade with edge distance.");


CCamera* CCamera::camTypes[CCamera::CAMTYPE_COUNT] = {nullptr};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera::CCamera(unsigned int cameraType)
	: pos(ZeroVector)
	, rot(ZeroVector)
	, forward(RgtVector)
	, up(UpVector)
	, fov(0.0f)
	, halfFov(0.0f)
	, tanHalfFov(0.0f)
	, lppScale(0.0f)
	, frustumScales(0.0f, 0.0f, CGlobalRendering::NEAR_PLANE, CGlobalRendering::MAX_VIEW_RANGE)
	, posOffset(ZeroVector)
	, tiltOffset(ZeroVector)

	, camType(cameraType)
	, projType((cameraType == CAMTYPE_SHADOW)? PROJTYPE_ORTHO: PROJTYPE_PERSP)
{
	assert(cameraType < CAMTYPE_ACTIVE);

	memset(viewport, 0, 4 * sizeof(int));
	memset(movState, 0, sizeof(movState));
	memset(rotState, 0, sizeof(rotState));

	SetVFOV(45.0f);
}

void CCamera::CopyState(const CCamera* cam)
{
	for (unsigned int i = 0; i < FRUSTUM_PLANE_CNT; i++) {
		frustumPlanes[i] = cam->frustumPlanes[i];
	}

	// note: xy-scales are only relevant for CAMTYPE_SHADOW
	frustumScales = cam->frustumScales;

	forward    = cam->GetForward();
	right      = cam->GetRight();
	up         = cam->GetUp();

	pos        = cam->GetPos();
	rot        = cam->GetRot();

	fov        = cam->GetVFOV();
	halfFov    = cam->GetHalfFov();
	tanHalfFov = cam->GetTanHalfFov();
	lppScale   = cam->GetLPPScale();

	// do not copy this, each camera knows its own type
	// camType = cam->GetCamType();
}

void CCamera::CopyStateReflect(const CCamera* cam)
{
	assert(cam->GetCamType() != CAMTYPE_UWREFL);
	assert(     GetCamType() == CAMTYPE_UWREFL);

	SetDir(cam->GetDir() * float3(1.0f, -1.0f, 1.0f));
	SetPos(cam->GetPos() * float3(1.0f, -1.0f, 1.0f));
	SetRotZ(-cam->GetRot().z);
	Update(false, true, false);
}

void CCamera::Update(bool updateDirs, bool updateMats, bool updatePort)
{
	if (globalRendering->viewSizeY <= 0) {
		lppScale = 0.0f;
	} else {
		lppScale = (2.0f * tanHalfFov) / globalRendering->viewSizeY;
	}

	// should be set before UpdateMatrices
	UpdateViewRange();

	if (updateDirs)
		UpdateDirsFromRot(rot);
	if (updateMats)
		UpdateMatrices(globalRendering->viewSizeX, globalRendering->viewSizeY, globalRendering->aspectRatio);
	if (updatePort)
		UpdateViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	UpdateFrustum();

	LoadMatrices();
	// not done here
	// LoadViewPort();
}

void CCamera::UpdateFrustum()
{
	switch (projType) {
		case PROJTYPE_PERSP: {
			// NOTE: "-" because we want normals
			const float3 forwardy = (-forward *                                          tanHalfFov );
			const float3 forwardx = (-forward * math::tan(globalRendering->aspectRatio *    halfFov));

			frustumPlanes[FRUSTUM_PLANE_TOP] = (forwardy +    up).UnsafeANormalize();
			frustumPlanes[FRUSTUM_PLANE_BOT] = (forwardy -    up).UnsafeANormalize();
			frustumPlanes[FRUSTUM_PLANE_RGT] = (forwardx + right).UnsafeANormalize();
			frustumPlanes[FRUSTUM_PLANE_LFT] = (forwardx - right).UnsafeANormalize();
		} break;
		case PROJTYPE_ORTHO: {
			frustumPlanes[FRUSTUM_PLANE_TOP] =     up;
			frustumPlanes[FRUSTUM_PLANE_BOT] =    -up;
			frustumPlanes[FRUSTUM_PLANE_RGT] =  right;
			frustumPlanes[FRUSTUM_PLANE_LFT] = -right;
		} break;
		default: {
			assert(false);
		} break;
	}

	frustumPlanes[FRUSTUM_PLANE_FRN] = -forward;
	frustumPlanes[FRUSTUM_PLANE_BCK] =  forward;

	if (camType != CAMTYPE_VISCUL) {
		// vis-culling is always performed from player's (or light's)
		// POV but also happens during e.g. cubemap generation; copy
		// over the frustum planes we just calculated above such that
		// GetFrustumSides can be called by all parties interested in
		// VC
		//
		// note that this is the only place where VISCUL is updated!
		camTypes[CAMTYPE_VISCUL]->CopyState(camTypes[camType]);
	}
}

void CCamera::UpdateMatrices(unsigned int vsx, unsigned int vsy, float var)
{
	// recalculate the projection transform
	switch (projType) {
		case PROJTYPE_PERSP: {
			gluPerspectiveSpring(var, frustumScales.z, frustumScales.w);
		} break;
		case PROJTYPE_ORTHO: {
			glOrthoScaledSpring(vsx, vsy, frustumScales.z, frustumScales.w);
		} break;
		default: {
			assert(false);
		} break;
	}


	// FIXME:
	//   should be applying the offsets to pos/up/right/forward/etc,
	//   but without affecting the real positions (need an intermediary)
	const float3 fShake = ((forward * (1.0f + tiltOffset.z)) + (right * tiltOffset.x) + (up * tiltOffset.y)).ANormalize();
	const float3 camPos = pos + posOffset;
	const float3 center = camPos + fShake;

	// recalculate the view transform
	gluLookAtSpring(camPos, center, up);


	// create extra matrices (useful for shaders)
	viewProjectionMatrix = projectionMatrix * viewMatrix;
	viewMatrixInverse = viewMatrix.InvertAffine();
	projectionMatrixInverse = projectionMatrix.Invert();
	viewProjectionMatrixInverse = viewProjectionMatrix.Invert();

	billboardMatrix = viewMatrix;
	billboardMatrix.SetPos(ZeroVector);
	billboardMatrix.Transpose(); // viewMatrix is affine, equals inverse
}

void CCamera::UpdateViewPort(int px, int py, int sx, int sy)
{
	viewport[0] = px;
	viewport[1] = py;
	viewport[2] = sx;
	viewport[3] = sy;
}

void CCamera::UpdateLoadViewPort(int px, int py, int sx, int sy)
{
	UpdateViewPort(px, py, sx, sy);
	LoadViewPort();
}


void CCamera::LoadMatrices() const
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&projectionMatrix.m[0]);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(&viewMatrix.m[0]);
}

void CCamera::LoadViewPort() const
{
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}


void CCamera::UpdateViewRange()
{
	const float maxDistToBorderX = std::max(pos.x, float3::maxxpos - pos.x);
	const float maxDistToBorderZ = std::max(pos.z, float3::maxzpos - pos.z);
	const float maxDistToBorder  = math::sqrt(Square(maxDistToBorderX) + Square(maxDistToBorderZ));

	const float angleViewRange   = (1.0f - forward.dot(UpVector)) * maxDistToBorder;
	const float heightViewRange  = (pos.y - std::max(0.0f, readMap->GetCurrMinHeight())) * 2.4f;

	float wantedViewRange = CGlobalRendering::MAX_VIEW_RANGE;

	// camera-height dependent (i.e. TAB-view)
	wantedViewRange = std::max(wantedViewRange, heightViewRange);
	// view-angle dependent (i.e. FPS-view)
	wantedViewRange = std::max(wantedViewRange, angleViewRange);

	const float factor = wantedViewRange / CGlobalRendering::MAX_VIEW_RANGE;

	frustumScales.z = CGlobalRendering::NEAR_PLANE * factor;
	frustumScales.w = CGlobalRendering::MAX_VIEW_RANGE * factor;

	globalRendering->zNear     = frustumScales.z;
	globalRendering->viewRange = frustumScales.w;
}




static inline bool AABBInOriginPlane(
	const float3& camPos,
	const float3& mins,
	const float3& maxs,
	const float3& normal,
	const float offset
) {
	float3 fp; // far point
	fp.x = (normal.x > 0.0f) ? mins.x : maxs.x;
	fp.y = (normal.y > 0.0f) ? mins.y : maxs.y;
	fp.z = (normal.z > 0.0f) ? mins.z : maxs.z;
	return (normal.dot(fp - camPos) < offset);
}


bool CCamera::InView(const float3& mins, const float3& maxs) const
{
	// orthographic plane offsets along each respective normal; [0] = LFT&RGT, [1] = TOP&BOT
	const float planeOffsets[2] = {frustumScales.x, frustumScales.y};

	// axis-aligned bounding box test (AABB)
	for (unsigned int i = FRUSTUM_PLANE_LFT; i < FRUSTUM_PLANE_FRN; i++) {
		if (!AABBInOriginPlane(pos, mins, maxs, frustumPlanes[i], planeOffsets[i >> 1])) {
			return false;
		}
	}

	return true;
}

bool CCamera::InView(const float3& p, float radius) const
{
	// use arrays because neither float2 nor float4 have an operator[]
	const float xyPlaneOffsets[2] = {frustumScales.x, frustumScales.y};
	const float  zPlaneOffsets[2] = {frustumScales.z, frustumScales.w};

	const float3 objectVector = p - pos;

	assert(FRUSTUM_PLANE_LFT == 0);
	assert(FRUSTUM_PLANE_FRN == 4);

	#if 0
	// test if <p> is in front of the near-plane
	if (objectVector.dot(frustumPlanes[FRUSTUM_PLANE_FRN]) > (zPlaneOffsets[0] + radius))
		return false;
	#endif

	// test if <p> is in front of a side-plane (LRTB)
	for (unsigned int i = FRUSTUM_PLANE_LFT; i < FRUSTUM_PLANE_FRN; i++) {
		if (objectVector.dot(frustumPlanes[i]) > (xyPlaneOffsets[i >> 1] + radius)) {
			return false;
		}
	}

	// test if <p> is behind the far-plane
	return (objectVector.dot(frustumPlanes[FRUSTUM_PLANE_BCK]) <= (zPlaneOffsets[1] + radius));
}



void CCamera::SetVFOV(const float angle)
{
	fov = angle;
	halfFov = (fov * 0.5f) * (PI / 180.f);
	tanHalfFov = math::tan(halfFov);
}

float CCamera::GetHFOV() const {
	const float hAspect = (viewport[2] * 1.0f) / viewport[3];
	const float fovFact = math::tan(fov * 0.5f) * hAspect;
	return (2.0f * math::atan(fovFact) * (180.0f / PI));
}



float3 CCamera::GetRotFromDir(float3 fwd)
{
	fwd.Normalize();

	// NOTE:
	//   atan2(0.0,  0.0) returns 0.0
	//   atan2(0.0, -0.0) returns PI
	//   azimuth (yaw) 0 is on negative z-axis
	//   roll-angle (rot.z) is always 0 by default
	float3 r;
	r.x = math::acos(fwd.y);
	r.y = math::atan2(fwd.x, -fwd.z);
	r.z = 0.0f;
	return r;
}

float3 CCamera::GetFwdFromRot(const float3 r)
{
	float3 fwd;
	fwd.x = std::sin(r.x) *   std::sin(r.y);
	fwd.z = std::sin(r.x) * (-std::cos(r.y));
	fwd.y = std::cos(r.x);
	return fwd;
}

float3 CCamera::GetRgtFromRot(const float3 r)
{
	// FIXME:
	//   right should always be "right" relative to forward
	//   (i.e. up should always point "up" in WS and camera
	//   can not flip upside down) but is not
	//
	//   fwd=(0,+1,0) -> rot=GetRotFromDir(fwd)=(0.0, PI, 0.0) -> GetRgtFromRot(rot)=(-1.0, 0.0, 0.0)
	//   fwd=(0,-1,0) -> rot=GetRotFromDir(fwd)=( PI, PI, 0.0) -> GetRgtFromRot(rot)=(+1.0, 0.0, 0.0)
	//
	float3 rgt;
	rgt.x = std::sin(HALFPI - r.z) *   std::sin(r.y + HALFPI);
	rgt.z = std::sin(HALFPI - r.z) * (-std::cos(r.y + HALFPI));
	rgt.y = std::cos(HALFPI - r.z);
	return rgt;
}


void CCamera::UpdateDirsFromRot(const float3 r)
{
	forward  = std::move(GetFwdFromRot(r));
	right    = std::move(GetRgtFromRot(r));
	up       = (right.cross(forward)).Normalize();
}

void CCamera::SetDir(const float3 dir)
{
	// if (dir == forward) return;
	// update our axis-system from the angles
	SetRot(GetRotFromDir(dir) + (FwdVector * rot.z));
	assert(dir.dot(forward) > 0.9f);
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
	// same as gluProject()
	const float4 v = viewProjectionMatrix * float4(objPos, 1.0f);
	float3 winPos;
	winPos.x = viewport[0] + viewport[2] * (v.x / v.w + 1.0f) * 0.5f;
	winPos.y = viewport[1] + viewport[3] * (v.y / v.w + 1.0f) * 0.5f;
	winPos.z =                             (v.z / v.w + 1.0f) * 0.5f;
	return winPos;
}




inline void CCamera::gluPerspectiveSpring(float aspect, float zn, float zf) {
	const float t = zn * tanHalfFov;
	const float b = -t;
	const float l = b * aspect;
	const float r = t * aspect;

	glFrustumSpring(l, r,  b, t,  zn, zf);
}

inline void CCamera::glFrustumSpring(
	const float l,
	const float r,
	const float b,
	const float t,
	const float zn,
	const float zf
) {
	projectionMatrix[ 0] = (2.0f * zn) / (r - l);
	projectionMatrix[ 1] =  0.0f;
	projectionMatrix[ 2] =  0.0f;
	projectionMatrix[ 3] =  0.0f;

	projectionMatrix[ 4] =  0.0f;
	projectionMatrix[ 5] = (2.0f * zn) / (t - b);
	projectionMatrix[ 6] =  0.0f;
	projectionMatrix[ 7] =  0.0f;

	projectionMatrix[ 8] = (r + l) / (r - l);
	projectionMatrix[ 9] = (t + b) / (t - b);
	projectionMatrix[10] = -(zf + zn) / (zf - zn);
	projectionMatrix[11] = -1.0f;

	projectionMatrix[12] =   0.0f;
	projectionMatrix[13] =   0.0f;
	projectionMatrix[14] = -(2.0f * zf * zn) / (zf - zn);
	projectionMatrix[15] =   0.0f;
}

// same as glOrtho(-1, 1, -1, 1, zn, zf) plus glScalef(sx, sy, 1)
inline void CCamera::glOrthoScaledSpring(
	const float sx,
	const float sy,
	const float zn,
	const float zf
) {
	const float l = -1.0f * sx;
	const float r =  1.0f * sx;
	const float b = -1.0f * sy;
	const float t =  1.0f * sy;

	glOrthoSpring(l, r,  b, t,  zn, zf);
}

inline void CCamera::glOrthoSpring(
	const float l,
	const float r,
	const float b,
	const float t,
	const float zn,
	const float zf
) {
	const float tx = -(( r +  l) / ( r -  l));
	const float ty = -(( t +  b) / ( t -  b));
	const float tz = -((zf + zn) / (zf - zn));

	projectionMatrix[ 0] =  2.0f / (r - l);
	projectionMatrix[ 1] =  0.0f;
	projectionMatrix[ 2] =  0.0f;
	projectionMatrix[ 3] =  0.0f;

	projectionMatrix[ 4] =  0.0f;
	projectionMatrix[ 5] =  2.0f / (t - b);
	projectionMatrix[ 6] =  0.0f;
	projectionMatrix[ 7] =  0.0f;

	projectionMatrix[ 8] =  0.0f;
	projectionMatrix[ 9] =  0.0f;
	projectionMatrix[10] = -2.0f / (zf - zn);
	projectionMatrix[11] =  0.0f;

	projectionMatrix[12] = tx;
	projectionMatrix[13] = ty;
	projectionMatrix[14] = tz;
	projectionMatrix[15] = 1.0f;
}

inline void CCamera::gluLookAtSpring(const float3& eye, const float3& center, const float3& up)
{
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
}




void CCamera::GetFrustumSides(float miny, float maxy, float scale, bool negOnly) {

	frustumLines[FRUSTUM_SIDE_POS].clear();
	frustumLines[FRUSTUM_SIDE_NEG].clear();

	// only non-zero for orthographic cameras
	const float3 posOffsets[FRUSTUM_PLANE_FRN] = {
		-right * frustumScales.x,
		 right * frustumScales.x,
		    up * frustumScales.y,
		   -up * frustumScales.y,
	};

	// note: order does not matter
	for (unsigned int i = 0; i < FRUSTUM_PLANE_FRN; i++) {
		GetFrustumSide(frustumPlanes[i], posOffsets[i],  miny, maxy, scale, negOnly? FRUSTUM_SIDE_NEG: FRUSTUM_SIDE_POS);
	}
}

void CCamera::GetFrustumSide(
	const float3& normal,
	const float3& offset,
	float miny,
	float maxy,
	float scale,
	unsigned int side
) {
	// compose an orthonormal axis-system around <normal>
	float3 xdir = (normal.cross(UpVector)).UnsafeANormalize();
	float3 ydir = (normal.cross(xdir)).UnsafeANormalize();

	// intersection of vector from <pos> along <ydir> with xz-plane
	float3 pInt;

	// prevent DIV0 when calculating line.dir
	if (math::fabs(xdir.z) < 0.001f)
		xdir.z = 0.001f;

	if (ydir.y != 0.0f) {
		// if normal is angled toward the sky instead of the ground,
		// subtract <miny> from the camera's y-position, else <maxy>
		if (normal.y > 0.0f) {
			pInt = (pos + offset) - ydir * (((pos.y + offset.y) - miny) / ydir.y);
		} else {
			pInt = (pos + offset) - ydir * (((pos.y + offset.y) - maxy) / ydir.y);
		}
	}

	// <line.dir> is the direction coefficient (0 ==> parallel to z-axis, inf ==> parallel to x-axis)
	// in the xz-plane; <line.base> is the x-coordinate at which line intersects x-axis; <line.sign>
	// indicates line direction, ie. left-to-right (whenever <xdir.z> is negative) or right-to-left
	// NOTE:
	//   (b.x / b.z) is actually the reciprocal of the DC (ie. the number of steps along +x for
	//   one step along +y); the world z-axis is inverted wrt. a regular Carthesian grid, so the
	//   DC is also inverted
	FrustumLine line;
	line.dir  = (xdir.x / xdir.z);
	line.base = (pInt.x - (pInt.z * line.dir)) / scale;
	line.sign = Sign(int(xdir.z <= 0.0f));
	line.minz = (                      0.0f) - (mapDims.mapy);
	line.maxz = (mapDims.mapy * SQUARE_SIZE) + (mapDims.mapy);

	// store all lines in [NEG] (regardless of actual sign) if wanted by caller
	frustumLines[line.sign == 1 || side == FRUSTUM_SIDE_NEG].push_back(line);
}

void CCamera::ClipFrustumLines(bool neg, const float zmin, const float zmax) {

	std::vector<FrustumLine>& lines = frustumLines[neg];
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


float3 CCamera::GetMoveVectorFromState(bool fromKeyState) const
{
	float camDeltaTime = globalRendering->lastFrameTime;
	float camMoveSpeed = 1.0f;
	camMoveSpeed *= (1.0f - movState[MOVE_STATE_SLW] * 0.9f);
	camMoveSpeed *= (1.0f + movState[MOVE_STATE_FST] * 9.0f);

	float3 v;
	if (fromKeyState) {
		v.y += (camDeltaTime * 0.001f * movState[MOVE_STATE_FWD]);
		v.y -= (camDeltaTime * 0.001f * movState[MOVE_STATE_BCK]);
		v.x += (camDeltaTime * 0.001f * movState[MOVE_STATE_RGT]);
		v.x -= (camDeltaTime * 0.001f * movState[MOVE_STATE_LFT]);
	} else {
		const int screenH = globalRendering->viewSizeY;
		const int screenW = globalRendering->dualScreenMode?
			(globalRendering->viewSizeX << 1):
			(globalRendering->viewSizeX     );

		const float width  = configHandler->GetFloat("EdgeMoveWidth");
		const bool dynamic = configHandler->GetBool("EdgeMoveDynamic");

		int2 border;
		border.x = std::max<int>(1, screenW * width);
		border.y = std::max<int>(1, screenH * width);

		float2 distToEdge; // must be float, ints don't save the sign in case of 0 and we need it for copysign()
		distToEdge.x = Clamp(mouse->lastx, 0, screenW);
		distToEdge.y = Clamp(mouse->lasty, 0, screenH);
		if (((screenW-1) - distToEdge.x) < distToEdge.x) distToEdge.x = -((screenW-1) - distToEdge.x);
		if (((screenH-1) - distToEdge.y) < distToEdge.y) distToEdge.y = -((screenH-1) - distToEdge.y);
		distToEdge.x = -distToEdge.x;

		float2 move;
		if (dynamic) {
			move.x = Clamp(float(border.x - std::abs(distToEdge.x)) / border.x, 0.f, 1.f);
			move.y = Clamp(float(border.y - std::abs(distToEdge.y)) / border.y, 0.f, 1.f);
		} else {
			move.x = int(std::abs(distToEdge.x) < border.x);
			move.y = int(std::abs(distToEdge.y) < border.y);
		}
		move.x = std::copysign(move.x, distToEdge.x);
		move.y = std::copysign(move.y, distToEdge.y);

		v.x = (camDeltaTime * 0.001f * move.x);
		v.y = (camDeltaTime * 0.001f * move.y);
	}

	v.z = camMoveSpeed;
	return v;
}


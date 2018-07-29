/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>

#include "Camera.h"
#include "CameraHandler.h"
#include "UI/MouseHandler.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "System/myMath.h"
#include "System/float3.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"


CONFIG(float, EdgeMoveWidth)
	.defaultValue(0.02f)
	.minimumValue(0.0f)
	.description("The width (in percent of screen size) of the EdgeMove scrolling area.");
CONFIG(bool, EdgeMoveDynamic)
	.defaultValue(true)
	.description("If EdgeMove scrolling speed should fade with edge distance.");




CCamera::CCamera(unsigned int cameraType, unsigned int projectionType)
	: pos(ZeroVector)
	, rot(ZeroVector)
	, forward(FwdVector)
	, up(UpVector)
	, fov(0.0f)
	, halfFov(0.0f)
	, tanHalfFov(0.0f)
	, lppScale(0.0f)
	, frustumScales(0.0f, 0.0f, CGlobalRendering::NEAR_PLANE, CGlobalRendering::MAX_VIEW_RANGE)
	, posOffset(ZeroVector)
	, tiltOffset(ZeroVector)

	, camType(cameraType)
	, projType(projectionType)
{
	assert(cameraType < CAMTYPE_COUNT);

	memset(viewport, 0, 4 * sizeof(int));
	memset(movState, 0, sizeof(movState));
	memset(rotState, 0, sizeof(rotState));

	SetVFOV(45.0f);
}


CCamera* CCamera::GetActive()
{
	return (CCameraHandler::GetActiveCamera());
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
	SetVFOV(cam->GetVFOV());

	Update(false, true, false);
}

void CCamera::Update(bool updateDirs, bool updateMats, bool updatePort)
{
	lppScale = (2.0f * tanHalfFov) * globalRendering->pixelY;

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
	// scale-factors for {x,y}-axes
	float2 nAxisScales;
	float2 fAxisScales;

	switch (projType) {
		case PROJTYPE_PERSP: {
			// NOTE: "-" because we want normals
			const float3 forwardy = (-forward *                                          tanHalfFov );
			const float3 forwardx = (-forward * math::tan(globalRendering->aspectRatio *    halfFov));

			const float2 tanHalfFOVs = {math::tan(GetHFOV() * 0.5f * math::DEG_TO_RAD), tanHalfFov}; // horz, vert

			frustumPlanes[FRUSTUM_PLANE_TOP] = (forwardy +    up).UnsafeANormalize();
			frustumPlanes[FRUSTUM_PLANE_BOT] = (forwardy -    up).UnsafeANormalize();
			frustumPlanes[FRUSTUM_PLANE_RGT] = (forwardx + right).UnsafeANormalize();
			frustumPlanes[FRUSTUM_PLANE_LFT] = (forwardx - right).UnsafeANormalize();

			nAxisScales = {frustumScales.z * tanHalfFOVs.x, frustumScales.z * tanHalfFOVs.y}; // x, y
			fAxisScales = {frustumScales.w * tanHalfFOVs.x, frustumScales.w * tanHalfFOVs.y}; // x, y
		} break;
		case PROJTYPE_ORTHO: {
			frustumPlanes[FRUSTUM_PLANE_TOP] =     up;
			frustumPlanes[FRUSTUM_PLANE_BOT] =    -up;
			frustumPlanes[FRUSTUM_PLANE_RGT] =  right;
			frustumPlanes[FRUSTUM_PLANE_LFT] = -right;

			nAxisScales = {frustumScales.x, frustumScales.y};
			fAxisScales = {frustumScales.x, frustumScales.y};
		} break;
		default: {
			assert(false);
		} break;
	}

	frustumPlanes[FRUSTUM_PLANE_FRN] = -forward;
	frustumPlanes[FRUSTUM_PLANE_BCK] =  forward;

	frustumVerts[0] = pos + (forward * frustumScales.z) + (right * -nAxisScales.x) + (up *  nAxisScales.y);
	frustumVerts[1] = pos + (forward * frustumScales.z) + (right *  nAxisScales.x) + (up *  nAxisScales.y);
	frustumVerts[2] = pos + (forward * frustumScales.z) + (right *  nAxisScales.x) + (up * -nAxisScales.y);
	frustumVerts[3] = pos + (forward * frustumScales.z) + (right * -nAxisScales.x) + (up * -nAxisScales.y);
	frustumVerts[4] = pos + (forward * frustumScales.w) + (right * -fAxisScales.x) + (up *  fAxisScales.y);
	frustumVerts[5] = pos + (forward * frustumScales.w) + (right *  fAxisScales.x) + (up *  fAxisScales.y);
	frustumVerts[6] = pos + (forward * frustumScales.w) + (right *  fAxisScales.x) + (up * -fAxisScales.y);
	frustumVerts[7] = pos + (forward * frustumScales.w) + (right * -fAxisScales.x) + (up * -fAxisScales.y);

	if (camType == CAMTYPE_VISCUL)
		return;

	// vis-culling is always performed from player's (or light's)
	// POV but also happens during e.g. cubemap generation; copy
	// over the frustum planes we just calculated above such that
	// GetFrustumSides can be called by all parties interested in
	// VC
	//
	// note that this is the only place where VISCUL is updated!
	CCamera* visCam = CCameraHandler::GetCamera(CAMTYPE_VISCUL);
	CCamera* curCam = CCameraHandler::GetCamera(camType);

	visCam->CopyState(curCam);
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




// axis-aligned bounding box test (AABB)
bool CCamera::InView(const float3& mins, const float3& maxs) const
{
	const float3 boxCenter = (maxs + mins) * 0.5f;
	const float3 boxScales = (maxs - mins) * 0.5f;

	if (!InView(boxCenter, boxScales.Length()))
		return false;

	// orthographic plane offsets along each respective normal; [0] = LFT&RGT, [1] = TOP&BOT
	const float xyPlaneOffsets[2] = {frustumScales.x, frustumScales.y};
	const float zwPlaneOffsets[2] = {frustumScales.z, frustumScales.w};


	// [i*2+0] := point, [i*2+1] := normal
	const float3 boxFaces[6 * 2] = {
		boxCenter + FwdVector * boxScales.z,  FwdVector, // front
		boxCenter - FwdVector * boxScales.z, -FwdVector, // back
		boxCenter + RgtVector * boxScales.x,  RgtVector, // right
		boxCenter - RgtVector * boxScales.x, -RgtVector, // left
		boxCenter +  UpVector * boxScales.y,   UpVector, // top
		boxCenter -  UpVector * boxScales.y,  -UpVector, // bottom
	};
	const float3 boxVerts[8] = {
		// bottom
		{mins.x,  mins.y,  mins.z},
		{maxs.x,  mins.y,  mins.z},
		{maxs.x,  mins.y,  maxs.z},
		{mins.x,  mins.y,  maxs.z},
		// top
		{mins.x,  maxs.y,  mins.z},
		{maxs.x,  maxs.y,  mins.z},
		{maxs.x,  maxs.y,  maxs.z},
		{mins.x,  maxs.y,  maxs.z},
	};

	{
		// test box planes
		for (unsigned int i = 0; i < 6; i++) {
			unsigned int n = 0;

			for (unsigned int j = 0; j < 8; j++) {
				n += (boxFaces[i * 2 + 1].dot(frustumVerts[j] - boxFaces[i * 2 + 0]) > 0.0f);
			}

			if (n == 8)
				return false;
		}
	}
	{
		// test cam planes (LRTB)
		for (unsigned int i = FRUSTUM_PLANE_LFT; i < FRUSTUM_PLANE_FRN; i++) {
			unsigned int n = 0;

			for (unsigned int j = 0; j < 8; j++) {
				n += (frustumPlanes[i].dot(boxVerts[j] - pos) > xyPlaneOffsets[i >> 1]);
			}

			// fully in front of this plane, so outside frustum (normals point outward)
			if (n == 8)
				return false;
		}
	}
	{
		// test cam planes (NF)
		for (unsigned int i = FRUSTUM_PLANE_FRN; i < FRUSTUM_PLANE_CNT; i++) {
			unsigned int n = 0;

			for (unsigned int j = 0; j < 8; j++) {
				n += (frustumPlanes[i].dot(boxVerts[j] - (pos + forward * zwPlaneOffsets[i & 1])) > 0.0f);
			}

			if (n == 8)
				return false;
		}
	}

	return true;
}

bool CCamera::InView(const float3& p, float radius) const
{
	// use arrays because neither float2 nor float4 have an operator[]
	const float xyPlaneOffsets[2] = {frustumScales.x, frustumScales.y};
	const float zwPlaneOffsets[2] = {frustumScales.z, frustumScales.w};

	const float3 objectVector = p - pos;

	static_assert(FRUSTUM_PLANE_LFT == 0, "");
	static_assert(FRUSTUM_PLANE_FRN == 4, "");

	#if 0
	// test if <p> is in front of the near-plane
	if (objectVector.dot(frustumPlanes[FRUSTUM_PLANE_FRN]) > (zwPlaneOffsets[0] + radius))
		return false;
	#endif

	// test if <p> is in front of a side-plane (LRTB)
	for (unsigned int i = FRUSTUM_PLANE_LFT; i < FRUSTUM_PLANE_FRN; i++) {
		if (objectVector.dot(frustumPlanes[i]) > (xyPlaneOffsets[i >> 1] + radius))
			return false;
	}

	// test if <p> is behind the far-plane
	return !(objectVector.dot(frustumPlanes[FRUSTUM_PLANE_BCK]) > (zwPlaneOffsets[1] + radius));
}



void CCamera::SetVFOV(const float angle)
{
	fov = angle;
	halfFov = (fov * 0.5f) * math::DEG_TO_RAD;
	tanHalfFov = math::tan(halfFov);
}

float CCamera::GetHFOV() const {
	const float hAspect = (viewport[2] * 1.0f) / viewport[3]; // globalRendering->aspectRatio
	const float fovFact = tanHalfFov * hAspect;
	return (2.0f * math::atan(fovFact) * math::RAD_TO_DEG);
}
#if 0
float CCamera::CalcTanHalfHFOV() const {
	const float half_h_fov_deg = math::atan(thvfov * h_aspect_ratio) * math::RAD_TO_DEG;
	const float half_h_fov_rad = half_h_fov_deg * math::DEG_TO_RAD;
	return (math::tan(half_h_fov_rad));
}
#endif



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
	rgt.x = std::sin(math::HALFPI - r.z) *   std::sin(r.y + math::HALFPI);
	rgt.z = std::sin(math::HALFPI - r.z) * (-std::cos(r.y + math::HALFPI));
	rgt.y = std::cos(math::HALFPI - r.z);
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

	projectionMatrix = clipControlMatrix * CMatrix44f::PerspProj(l, r,  b, t,  zn, zf);
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

	projectionMatrix = clipControlMatrix * CMatrix44f::OrthoProj(l, r,  b, t,  zn, zf);
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




void CCamera::CalcFrustumLines(float miny, float maxy, float scale, bool neg) {
	const float3 params = {miny, maxy, scale};
	// only non-zero for orthographic cameras
	const float3 planeOffsets[FRUSTUM_PLANE_FRN] = {
		-right * frustumScales.x,
		 right * frustumScales.x,
		    up * frustumScales.y,
		   -up * frustumScales.y,
	};

	// reset counts per side
	frustumLines[FRUSTUM_SIDE_POS][4].sign = 0;
	frustumLines[FRUSTUM_SIDE_NEG][4].sign = 0;

	// note: order does not matter
	for (unsigned int i = FRUSTUM_PLANE_LFT, side = neg? FRUSTUM_SIDE_NEG: FRUSTUM_SIDE_POS; i < FRUSTUM_PLANE_FRN; i++) {
		CalcFrustumLine(frustumPlanes[i], planeOffsets[i],  params, side);
	}

	assert(!neg || frustumLines[FRUSTUM_SIDE_NEG][4].sign == 4);
}

void CCamera::CalcFrustumLine(
	const float3& normal,
	const float3& offset,
	const float3& params,
	unsigned int side
) {
	// compose an orthonormal axis-system around <normal>
	float3 xdir = (normal.cross(UpVector)).UnsafeANormalize();
	float3 ydir = (normal.cross(xdir)).UnsafeANormalize();

	// intersection of vector from <pos> along <ydir> with xz-plane
	float3 pInt;

	// prevent DIV0 when calculating line.dir
	xdir.z = std::max(std::fabs(xdir.z), 0.001f) * std::copysign(1.0f, xdir.z);

	if (ydir.y != 0.0f) {
		// if normal is angled toward the sky instead of the ground,
		// subtract <miny> from the camera's y-position, else <maxy>
		if (normal.y > 0.0f) {
			pInt = (pos + offset) - ydir * (((pos.y + offset.y) - params.x) / ydir.y);
		} else {
			pInt = (pos + offset) - ydir * (((pos.y + offset.y) - params.y) / ydir.y);
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

	line.sign = Sign(int(xdir.z <= 0.0f));
	line.dir  = (xdir.x / xdir.z);
	line.base = (pInt.x - (pInt.z * line.dir)) / params.z;
	line.minz = (                      0.0f) - (mapDims.mapy);
	line.maxz = (mapDims.mapy * SQUARE_SIZE) + (mapDims.mapy);

	int  index = (line.sign == 1 || side == FRUSTUM_SIDE_NEG);
	int& count = frustumLines[index][4].sign;

	// store all lines in [NEG] (regardless of actual sign) if wanted by caller
	frustumLines[index][count++] = line;
}

void CCamera::ClipFrustumLines(const float zmin, const float zmax, bool neg)
{
	auto& lines = frustumLines[neg];

	for (int i = 0, cnt = lines[4].sign; i < cnt; i++) {
		for (int j = 0; j < cnt; j++) {
			if (i == j)
				continue;

			FrustumLine& fli = lines[i];
			FrustumLine& flj = lines[j];

			const float dbase = fli.base - flj.base;
			const float ddir = fli.dir - flj.dir;

			if (ddir == 0.0f)
				continue;

			const float colz = -(dbase / ddir);

			if ((flj.sign * ddir) > 0.0f) {
				if ((colz > fli.minz) && (colz < zmax))
					fli.minz = colz;
			} else {
				if ((colz < fli.maxz) && (colz > zmin))
					fli.maxz = colz;
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
		const int screenW = globalRendering->viewSizeX << globalRendering->dualScreenMode;

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


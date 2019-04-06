/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>

#include "Camera.h"
#include "CameraHandler.h"
#include "UI/MouseHandler.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/MatrixState.hpp"
#include "System/SpringMath.h"
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
	: camType(cameraType)
	, projType(projectionType)
{
	assert(cameraType < CAMTYPE_COUNT);

	memset(viewport, 0, 4 * sizeof(int));
	memset(movState, 0, sizeof(movState));
	memset(rotState, 0, sizeof(rotState));

	frustum.scales.z = CGlobalRendering::NEAR_PLANE;
	frustum.scales.w = CGlobalRendering::MAX_VIEW_RANGE;

	SetVFOV(45.0f);
	UpdateFrustum();
}


CCamera* CCamera::GetActive()
{
	return (CCameraHandler::GetActiveCamera());
}


void CCamera::CopyState(const CCamera* cam)
{
	// note: xy-scales are only relevant for CAMTYPE_SHADOW
	frustum     = cam->frustum;

	forward     = cam->GetForward();
	right       = cam->GetRight();
	up          = cam->GetUp();

	pos         = cam->GetPos();
	rot         = cam->GetRot();

	fov         = cam->GetVFOV();
	halfFov     = cam->GetHalfFov();
	tanHalfFov  = cam->GetTanHalfFov();

	lppScale    = cam->GetLPPScale();
	aspectRatio = cam->GetAspectRatio();

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

void CCamera::Update(const UpdateParams& p)
{
	lppScale = (2.0f * tanHalfFov) * globalRendering->pixelY;
	aspectRatio = globalRendering->aspectRatio;

	// should be set before UpdateMatrices
	if (p.updateViewRange)
		UpdateViewRange();
	if (p.updateDirs)
		UpdateDirsFromRot(rot);
	if (p.updateMats)
		UpdateMatrices(globalRendering->viewSizeX, globalRendering->viewSizeY, aspectRatio);
	if (p.updateViewPort)
		UpdateViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	if (p.updateFrustum)
		UpdateFrustum();

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
			const float3 forwardy = (-forward *                         tanHalfFov );
			const float3 forwardx = (-forward * math::tan(aspectRatio *    halfFov));

			const float2 tanHalfFOVs = {math::tan(GetHFOV() * 0.5f * math::DEG_TO_RAD), tanHalfFov}; // horz, vert

			frustum.planes[FRUSTUM_PLANE_TOP] = (forwardy +    up).UnsafeANormalize();
			frustum.planes[FRUSTUM_PLANE_BOT] = (forwardy -    up).UnsafeANormalize();
			frustum.planes[FRUSTUM_PLANE_RGT] = (forwardx + right).UnsafeANormalize();
			frustum.planes[FRUSTUM_PLANE_LFT] = (forwardx - right).UnsafeANormalize();

			nAxisScales = {frustum.scales.z * tanHalfFOVs.x, frustum.scales.z * tanHalfFOVs.y}; // x, y
			fAxisScales = {frustum.scales.w * tanHalfFOVs.x, frustum.scales.w * tanHalfFOVs.y}; // x, y
		} break;
		case PROJTYPE_ORTHO: {
			frustum.planes[FRUSTUM_PLANE_TOP] =     up;
			frustum.planes[FRUSTUM_PLANE_BOT] =    -up;
			frustum.planes[FRUSTUM_PLANE_RGT] =  right;
			frustum.planes[FRUSTUM_PLANE_LFT] = -right;

			nAxisScales = {frustum.scales.x, frustum.scales.y};
			fAxisScales = {frustum.scales.x, frustum.scales.y};
		} break;
		default: {
			assert(false);
		} break;
	}

	frustum.planes[FRUSTUM_PLANE_FRN] = -forward;
	frustum.planes[FRUSTUM_PLANE_BCK] =  forward;

	frustum.verts[0] = pos + (forward * frustum.scales.z) + (right * -nAxisScales.x) + (up *  nAxisScales.y); // ntl
	frustum.verts[1] = pos + (forward * frustum.scales.z) + (right *  nAxisScales.x) + (up *  nAxisScales.y); // ntr
	frustum.verts[2] = pos + (forward * frustum.scales.z) + (right *  nAxisScales.x) + (up * -nAxisScales.y); // nbr
	frustum.verts[3] = pos + (forward * frustum.scales.z) + (right * -nAxisScales.x) + (up * -nAxisScales.y); // nbl
	frustum.verts[4] = pos + (forward * frustum.scales.w) + (right * -fAxisScales.x) + (up *  fAxisScales.y); // ftl
	frustum.verts[5] = pos + (forward * frustum.scales.w) + (right *  fAxisScales.x) + (up *  fAxisScales.y); // ftr
	frustum.verts[6] = pos + (forward * frustum.scales.w) + (right *  fAxisScales.x) + (up * -fAxisScales.y); // fbr
	frustum.verts[7] = pos + (forward * frustum.scales.w) + (right * -fAxisScales.x) + (up * -fAxisScales.y); // fbl

	frustum.edges[0] = (frustum.verts[1] - frustum.verts[0]).UnsafeANormalize(); // ntr - ntl (same as ftr - ftl)
	frustum.edges[1] = (frustum.verts[0] - frustum.verts[3]).UnsafeANormalize(); // ntl - nbl (same as ftl - fbl)
	frustum.edges[2] = (frustum.verts[4] - frustum.verts[0]).UnsafeANormalize(); // ftl - ntl
	frustum.edges[3] = (frustum.verts[5] - frustum.verts[1]).UnsafeANormalize(); // ftr - ntr
	frustum.edges[4] = (frustum.verts[6] - frustum.verts[2]).UnsafeANormalize(); // fbr - nbr
	frustum.edges[5] = (frustum.verts[7] - frustum.verts[3]).UnsafeANormalize(); // fbl - nbl

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
			gluPerspectiveSpring(var, frustum.scales.z, frustum.scales.w);
		} break;
		case PROJTYPE_ORTHO: {
			glOrthoScaledSpring(vsx, vsy, frustum.scales.z, frustum.scales.w);
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


void CCamera::LoadViewPort() const
{
	glAttribStatePtr->ViewPort(viewport[0], viewport[1], viewport[2], viewport[3]);
}


void CCamera::UpdateViewRange()
{
	const float maxDistToBorderX = std::max(pos.x, float3::maxxpos - pos.x);
	const float maxDistToBorderZ = std::max(pos.z, float3::maxzpos - pos.z);
	const float maxDistToBorder  = math::sqrt(Square(maxDistToBorderX) + Square(maxDistToBorderZ));

	const float  angleViewRange  = (1.0f - forward.dot(UpVector)) * maxDistToBorder;
	const float heightViewRange  = (pos.y - std::max(0.0f, readMap->GetCurrMinHeight())) * 2.4f;

	float wantedViewRange = CGlobalRendering::MAX_VIEW_RANGE;

	// camera-height dependent (i.e. TAB-view)
	wantedViewRange = std::max(wantedViewRange, heightViewRange);
	// view-angle dependent (i.e. FPS-view)
	wantedViewRange = std::max(wantedViewRange, angleViewRange);

	// NOTE: factor is always >= 1, not what we want when near ground
	const float factor = wantedViewRange / CGlobalRendering::MAX_VIEW_RANGE;

	frustum.scales.z = CGlobalRendering::NEAR_PLANE * factor;
	frustum.scales.w = CGlobalRendering::MAX_VIEW_RANGE * factor;
}



#if 0
// axis-aligned bounding box test (AABB)
bool CCamera::InView(const AABB& aabb) const
{
	// orthographic plane offsets along each respective normal; [0] = LFT&RGT, [1] = TOP&BOT
	const float xyPlaneOffsets[2] = {frustum.scales.x, frustum.scales.y};
	const float zwPlaneOffsets[2] = {frustum.scales.z, frustum.scales.w};


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
				n += (boxFaces[i * 2 + 1].dot(frustum.verts[j] - boxFaces[i * 2 + 0]) > 0.0f);
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
				n += (frustum.planes[i].dot(boxVerts[j] - pos) > xyPlaneOffsets[i >> 1]);
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
				n += (frustum.planes[i].dot(boxVerts[j] - (pos + forward * zwPlaneOffsets[i & 1])) > 0.0f);
			}

			if (n == 8)
				return false;
		}
	}
	{
		for (unsigned int i = 0; i < 6; i++) {
			for (unsigned int j = 0; j < 6; j++) {
				if (boxFaces[i * 2 + 1] == frustum.planes[j])
					continue;

				float3 testAxis  = boxFaces[i * 2 + 1].cross(frustum.planes[j]);
				float3 testAxisN = testAxis.Normalize();

				float2   boxAxisDists = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}; // .x=min,.y=max
				float2 frustAxisDists = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}; // .x=min,.y=max
				float4  projAxisDists;

				for (unsigned int k = 0; k < 8; k++) {
					boxAxisDists.x = std::min(boxAxisDists.x, boxVerts[k].dot(testAxisN));
					boxAxisDists.y = std::max(boxAxisDists.y, boxVerts[k].dot(testAxisN));

					frustAxisDists.x = std::min(frustAxisDists.x, frustum.verts[k].dot(testAxisN));
					frustAxisDists.y = std::max(frustAxisDists.y, frustum.verts[k].dot(testAxisN));
				}

				projAxisDists.x = std::min(boxAxisDists.x, frustAxisDists.x); // min(minDists)
				projAxisDists.y = std::min(boxAxisDists.y, frustAxisDists.y); // min(maxDists)
				projAxisDists.z = std::max(boxAxisDists.x, frustAxisDists.x); // max(minDists)
				projAxisDists.w = std::max(boxAxisDists.y, frustAxisDists.y); // max(maxDists)

				if ((projAxisDists.y >= projAxisDists.z) && (projAxisDists.x <= projAxisDists.w))
					continue;

				return false;
			}
		}
	}

	return true;
}
#endif



void CCamera::SetVFOV(const float angle)
{
	fov = angle;
	halfFov = (fov * 0.5f) * math::DEG_TO_RAD;
	tanHalfFov = math::tan(halfFov);
}

float CCamera::GetHFOV() const {
	return (2.0f * math::atan(tanHalfFov * aspectRatio) * math::RAD_TO_DEG);
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

float3 CCamera::GetFwdFromRot(const float3& r)
{
	float3 fwd;
	fwd.x = std::sin(r.x) *   std::sin(r.y);
	fwd.z = std::sin(r.x) * (-std::cos(r.y));
	fwd.y = std::cos(r.x);
	return fwd;
}

float3 CCamera::GetRgtFromRot(const float3& r)
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


void CCamera::UpdateDirsFromRot(const float3& r)
{
	forward  = GetFwdFromRot(r);
	right    = GetRgtFromRot(r);
	up       = (right.cross(forward)).Normalize();
}

void CCamera::SetDir(const float3& dir)
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

	return ((forward - up * dy + right * dx).Normalize());
}


float3 CCamera::CalcWindowCoordinates(const float3& objPos) const
{
	// same as gluProject()
	const float4 projPos = viewProjectionMatrix * float4(objPos, 1.0f);
	const float3 clipPos = projPos / projPos.w;

	float3 winPos;
	winPos.x = viewport[0] + viewport[2] * (clipPos.x + 1.0f) * 0.5f;
	winPos.y = viewport[1] + viewport[3] * (clipPos.y + 1.0f) * 0.5f;
	winPos.z =                             (clipPos.z + 1.0f) * 0.5f;
	return winPos;
}


inline void CCamera::gluPerspectiveSpring(float aspect, float zn, float zf) {
	const float t = zn * tanHalfFov;
	const float b = -t;
	const float l = b * aspect;
	const float r = t * aspect;

	projectionMatrix = clipControlMatrix * CMatrix44f::PerspProj(l, r,  b, t,  zn, zf);
}


// same as glOrtho(-1, 1, -1, 1, zn, zf) plus glScale(sx, sy, 1)
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
	viewMatrix[12] = s.dot(-eye);
	viewMatrix[13] = u.dot(-eye);
	viewMatrix[14] = f.dot( eye);
}




void CCamera::CalcFrustumLines(float miny, float maxy, float scale, bool neg) {
	const float3 isectParams = {miny, maxy, 1.0f / scale};
	// only non-zero for orthographic cameras
	const float3 planeOffsets[FRUSTUM_PLANE_FRN] = {
		-right * frustum.scales.x,
		 right * frustum.scales.x,
		    up * frustum.scales.y,
		   -up * frustum.scales.y,
	};

	// reset counts per side
	frustumLines[FRUSTUM_SIDE_POS][4].sign = 0;
	frustumLines[FRUSTUM_SIDE_NEG][4].sign = 0;

	// note: order does not matter
	for (unsigned int i = FRUSTUM_PLANE_LFT, side = neg? FRUSTUM_SIDE_NEG: FRUSTUM_SIDE_POS; i < FRUSTUM_PLANE_FRN; i++) {
		CalcFrustumLine(frustum.planes[i], planeOffsets[i],  isectParams, side);
	}

	assert(!neg || frustumLines[FRUSTUM_SIDE_NEG][4].sign == 4);
}

void CCamera::CalcFrustumLine(
	const float3& normal,
	const float3& offset,
	const float3& params,
	unsigned int side
) {
	FrustumLine line;

	// compose an orthonormal axis-system around the frustum plane normal
	// top plane normal can point straight up if camera is angled downward
	const float3 aux = (std::fabs(normal.dot(UpVector)) > 0.995f)? -forward: UpVector;

	float3 xdir = (normal.cross( aux)).UnsafeANormalize();
	float3 ydir = (normal.cross(xdir)).UnsafeANormalize();
	// intersection of vector from <pos> along <ydir> with xz-plane
	// (on <miny> if <normal> is angled toward the sky, else <maxy>)
	float3 pInt;

	// prevent DIV0 when calculating line.dir
	xdir.z *= (std::fabs(xdir.z) > 0.001f);
	xdir.z = std::max(std::fabs(xdir.z), 0.001f) * std::copysign(1.0f, xdir.z);
	ydir.y = -std::fabs(ydir.y); // maintenance HACK for dot(N,-F)

	if (ydir.y != 0.0f) {
		const float py = params[normal.y <= 0.0f];
		const float dy = pos.y + offset.y - py;

		pInt = (pos + offset) - ydir * (dy / ydir.y);
	}

	// <line.dir> is the direction coefficient (0 ==> parallel to z-axis, inf ==> parallel to x-axis)
	// in the xz-plane; <line.base> is the x-coordinate at which line intersects x-axis; <line.sign>
	// indicates line direction, ie. left-to-right (whenever <xdir.z> is negative) or right-to-left
	// NOTE:
	//   (b.x / b.z) is actually the reciprocal of the DC (ie. the number of steps along +x for
	//   one step along +y); the world z-axis is inverted wrt. a regular Carthesian grid, so the
	//   DC is also inverted
	// FIXME: slope-intercept form is terrible
	line.sign = Sign(int(xdir.z <= 0.0f));
	line.dir  = (xdir.x / xdir.z);
	line.base = (pInt.x * params.z) - ((pInt.z * params.z) * line.dir);

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

	float3 v = FwdVector * camMoveSpeed;

	if (fromKeyState) {
		v.y += (camDeltaTime * 0.001f * movState[MOVE_STATE_FWD]);
		v.y -= (camDeltaTime * 0.001f * movState[MOVE_STATE_BCK]);
		v.x += (camDeltaTime * 0.001f * movState[MOVE_STATE_RGT]);
		v.x -= (camDeltaTime * 0.001f * movState[MOVE_STATE_LFT]);
	} else {
		const int screenH = globalRendering->viewSizeY;
		const int screenW = globalRendering->viewSizeX << globalRendering->dualScreenMode;

		const float width = configHandler->GetFloat("EdgeMoveWidth");

		int2 border;
		border.x = std::max<int>(1, screenW * width);
		border.y = std::max<int>(1, screenH * width);

		float2 move;
		// must be float, ints don't save the sign in case of 0 and we need it for copysign()
		float2 distToEdge = {Clamp(mouse->lastx, 0, screenW) * 1.0f, Clamp(mouse->lasty, 0, screenH) * 1.0f};

		if (((screenW - 1) - distToEdge.x) < distToEdge.x) distToEdge.x = -((screenW - 1) - distToEdge.x);
		if (((screenH - 1) - distToEdge.y) < distToEdge.y) distToEdge.y = -((screenH - 1) - distToEdge.y);

		if (configHandler->GetBool("EdgeMoveDynamic")) {
			move.x = Clamp(float(border.x - std::abs(distToEdge.x)) / border.x, 0.0f, 1.0f);
			move.y = Clamp(float(border.y - std::abs(distToEdge.y)) / border.y, 0.0f, 1.0f);
		} else {
			move.x = int(std::abs(distToEdge.x) < border.x);
			move.y = int(std::abs(distToEdge.y) < border.y);
		}

		move.x = std::copysign(move.x, -distToEdge.x);
		move.y = std::copysign(move.y,  distToEdge.y);

		v.x = (camDeltaTime * 0.001f * move.x);
		v.y = (camDeltaTime * 0.001f * move.y);
	}

	return v;
}



bool CCamera::Frustum::IntersectSphere(const float3& cp, const float4& sp) const
{
	// need a vector since planes do not carry origin-distance
	const float3 vec = sp - cp;

	// use arrays because neither float2 nor float4 have an operator[]
	const float xyPlaneOffsets[2] = {scales.x, scales.y};
	const float zwPlaneOffsets[2] = {scales.z, scales.w};

	static_assert(FRUSTUM_PLANE_LFT == 0, "");
	static_assert(FRUSTUM_PLANE_FRN == 4, "");

	#if 0
	// test if <sp> is in front of the near-plane
	if (vec.dot(planes[FRUSTUM_PLANE_FRN]) > (zwPlaneOffsets[0] + sp.w))
		return false;
	#endif

	// test if <sp> is in front of a side-plane (LRTB)
	for (unsigned int i = FRUSTUM_PLANE_LFT; i < FRUSTUM_PLANE_FRN; i++) {
		if (vec.dot(planes[i]) > (xyPlaneOffsets[i >> 1] + sp.w))
			return false;
	}

	// test if <sp> is behind the far-plane
	return !(vec.dot(planes[FRUSTUM_PLANE_BCK]) > (zwPlaneOffsets[1] + sp.w));
}

bool CCamera::Frustum::IntersectAABB(const AABB& b) const
{
	// edge axes and normals are identical for AABBs
	constexpr float3 aabbPlanes[3] = {
		RgtVector,
		UpVector,
		FwdVector
	};

	float3 aabbVerts[8];
	float3 crossAxes[3 * 6];

	b.CalcCorners(aabbVerts);

	const auto IsSepAxis = [](const float3& axis, const float3* frustVerts, const float3* aabbVerts) {
		float2 frustProjRange = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
		float2  aabbProjRange = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};

		float frustProjDists[8];
		float  aabbProjDists[8];

		for (int i = 0; i < 8; i++) {
			frustProjDists[i] = axis.dot(frustVerts[i]);

			frustProjRange.x = std::min(frustProjRange.x, frustProjDists[i]);
			frustProjRange.y = std::max(frustProjRange.y, frustProjDists[i]);

			aabbProjDists[i] = axis.dot(aabbVerts[i]);

			aabbProjRange.x = std::min(aabbProjRange.x, aabbProjDists[i]);
			aabbProjRange.y = std::max(aabbProjRange.y, aabbProjDists[i]);
		}

		return (!AABB::RangeOverlap(frustProjRange, aabbProjRange));
	};
	const auto AxisTestPred = [&](const float3& testAxis) {
		return (IsSepAxis(testAxis, &verts[0], aabbVerts));
	};

	if (std::find_if(&aabbPlanes[0], &aabbPlanes[0] + 3, AxisTestPred) != (&aabbPlanes[0] + 3))
		return false;
	if (std::find_if(&planes[0], &planes[0] + 6, AxisTestPred) != (&planes[0] + 6))
		return false;

	for (unsigned int i = 0; i < 3; i++) {
		for (unsigned int j = 0; j < 6; j++) {
			crossAxes[i * 6 + j] = (aabbPlanes[i].cross(edges[j])).SafeNormalize();
		}
	}

	return (std::find_if(&crossAxes[0], &crossAxes[0] + 3 * 6, AxisTestPred) == (&crossAxes[0] + 3 * 6));
}


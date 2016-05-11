/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_H
#define _CAMERA_H

#include <vector>

#include "Rendering/GL/myGL.h"
#include "System/float3.h"
#include "System/Matrix44f.h"


class CCamera
{
public:
	struct FrustumLine {
		float base;
		float dir;
		int sign;
		float minz;
		float maxz;
	};

	enum {
		CAMTYPE_PLAYER = 0, // main camera
		CAMTYPE_UWREFL = 1, // used for underwater reflections
		CAMTYPE_SHADOW = 2, // used for shadowmap generation
		CAMTYPE_ENVMAP = 3, // used for cubemap generation
		CAMTYPE_VISCUL = 4, // used for frustum culling
		CAMTYPE_ACTIVE = 5, // pointer to currently active camera
		CAMTYPE_COUNT  = 6,
	};

	enum {
		PROJTYPE_PERSP = 0,
		PROJTYPE_ORTHO = 1,
		PROJTYPE_COUNT = 2,
	};

	enum {
		FRUSTUM_PLANE_LFT = 0,
		FRUSTUM_PLANE_RGT = 1,
		FRUSTUM_PLANE_TOP = 2,
		FRUSTUM_PLANE_BOT = 3,
		FRUSTUM_PLANE_FRN = 4, // near
		FRUSTUM_PLANE_BCK = 5, // far
		FRUSTUM_PLANE_CNT = 6,
	};
	enum {
		FRUSTUM_SIDE_POS = 0,
		FRUSTUM_SIDE_NEG = 1,
	};

	enum {
		MOVE_STATE_FWD = 0, // forward
		MOVE_STATE_BCK = 1, // back
		MOVE_STATE_LFT = 2, // left
		MOVE_STATE_RGT = 3, // right
		MOVE_STATE_UP  = 4, // up
		MOVE_STATE_DWN = 5, // down
		MOVE_STATE_FST = 6, // fast
		MOVE_STATE_SLW = 7, // slow
	};

public:
	CCamera(unsigned int cameraType);

	void CopyState(const CCamera*);
	void CopyStateReflect(const CCamera*);
	void Update(bool updateDirs = true, bool updateMats = true, bool updatePort = true);

	/// @param fov in degree
	void SetPos(const float3& p) { pos = p; }
	void SetDir(const float3 dir);

	const float3& GetPos() const { return pos; }
	const float3& GetDir() const { return forward; }

	const float3& GetForward() const { return forward; }
	const float3& GetRight() const   { return right; }
	const float3& GetUp() const      { return up; }
	const float3& GetRot() const     { return rot; }

	void SetRot(const float3 r) { UpdateDirsFromRot(rot = r); }
	void SetRotX(const float x) { SetRot(float3(    x, rot.y, rot.z)); }
	void SetRotY(const float y) { SetRot(float3(rot.x,     y, rot.z)); }
	void SetRotZ(const float z) { SetRot(float3(rot.x, rot.y,     z)); }

	float3 CalcPixelDir(int x,int y) const;
	float3 CalcWindowCoordinates(const float3& objPos) const;

	bool InView(const float3& p, float radius = 0) const;
	bool InView(const float3& mins, const float3& maxs) const;

	void GetFrustumSides(float miny, float maxy, float scale, bool negOnly = false);
	void GetFrustumSide(
		const float3& normal,
		const float3& offset,
		float miny,
		float maxy,
		float scale,
		unsigned int side
	);

	void ClipFrustumLines(bool left, const float zmin, const float zmax);
	void SetFrustumScales(const float4 scales) { frustumScales = scales; }

	const std::vector<FrustumLine>& GetPosFrustumSides() const { return frustumLines[FRUSTUM_SIDE_POS]; }
	const std::vector<FrustumLine>& GetNegFrustumSides() const { return frustumLines[FRUSTUM_SIDE_NEG]; }

	void SetProjMatrix(const CMatrix44f& mat) { projectionMatrix = mat; }
	void SetViewMatrix(const CMatrix44f& mat) {
		viewMatrix = mat;

		// FIXME: roll-angle might not be 0
		pos = viewMatrix.GetPos();
		rot = GetRotFromDir(viewMatrix.GetZ());

		forward = viewMatrix.GetZ();
		right   = viewMatrix.GetX();
		up      = viewMatrix.GetY();
	}

	const CMatrix44f& GetViewMatrix() const { return viewMatrix; }
	const CMatrix44f& GetViewMatrixInverse() const { return viewMatrixInverse; }
	const CMatrix44f& GetProjectionMatrix() const { return projectionMatrix; }
	const CMatrix44f& GetProjectionMatrixInverse() const { return projectionMatrixInverse; }
	const CMatrix44f& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const CMatrix44f& GetViewProjectionMatrixInverse() const { return viewProjectionMatrixInverse; }
	const CMatrix44f& GetBillBoardMatrix() const { return billboardMatrix; }

	void LoadMatrices() const;
	void LoadViewPort() const;
	void UpdateLoadViewPort(int px, int py, int sx, int sy);

	void SetVFOV(const float angle);

	float GetVFOV() const { return fov; }
	float GetHFOV() const;
	float GetHalfFov() const { return halfFov; }
	float GetTanHalfFov() const { return tanHalfFov; }
	float GetLPPScale() const { return lppScale; }

	float3 GetMoveVectorFromState(bool fromKeyState) const;

	void SetMovState(int idx, bool b) { movState[idx] = b; }
	void SetRotState(int idx, bool b) { rotState[idx] = b; }
	const bool* GetMovState() const { return movState; }
	const bool* GetRotState() const { return rotState; }

	static float3 GetRotFromDir(float3 fwd);
	static float3 GetFwdFromRot(const float3 r);
	static float3 GetRgtFromRot(const float3 r);

	float ProjectedDistance(const float3 objPos) const {
		const float3 diff = objPos - GetPos();
		const float dist = diff.dot(GetDir());
		return dist;
	}

	/*
	float ProjectedDistanceShadow(const float3 objPos, const float3 sunDir) const {
		// FIXME: fix it, cap it for shallow shadows?
		const float3 diff = (GetPos() - objPos);
		const float  dot  = diff.dot(sunDir);
		const float3 gap  = diff - (sunDir * dot);
		return (gap.Length());
	}
	*/

	unsigned int GetCamType() const { return camType; }


	static void SetCamera(unsigned int camType, CCamera* cam) { camTypes[camType] = cam; }
	static void SetActiveCamera(unsigned int camType) { SetCamera(CAMTYPE_ACTIVE, GetCamera(camType)); }

	static CCamera* GetCamera(unsigned int camType) { return camTypes[camType]; }
	static CCamera* GetActiveCamera() { return (GetCamera(CAMTYPE_ACTIVE)); }

	// sets the current active camera, returns the previous
	static CCamera* GetSetActiveCamera(unsigned int camType) {
		CCamera* cam = GetActiveCamera();
		SetActiveCamera(camType);
		return cam;
	}

public:
	void UpdateViewRange();
	void UpdateFrustum();
	void UpdateMatrices(unsigned int vsx, unsigned int vsy, float var);
	void UpdateViewPort(int px, int py, int sx, int sy);

private:
	void gluPerspectiveSpring(const float aspect, const float zn, const float zf);
	void glFrustumSpring(const float l, const float r,  const float b, const float t,  const float zn, const float zf);
	void glOrthoScaledSpring(const float sx, const float sy, const float zn, const float zf);
	void glOrthoSpring(const float l, const float r,  const float b, const float t,  const float zn, const float zf);
	void gluLookAtSpring(const float3&, const float3&, const float3&);

	void UpdateDirsFromRot(const float3 r);

public:
	float3 pos;
	float3 rot;        ///< x = inclination, y = azimuth (to the -z axis!), z = roll
	float3 forward;    ///< local z-axis
	float3 right;      ///< local x-axis
	float3 up;         ///< local y-axis

	float fov;         ///< vertical viewing angle, in degrees
	float halfFov;     ///< half the fov in radians
	float tanHalfFov;  ///< math::tan(halfFov)
	float lppScale;    ///< length-per-pixel scale

public:
	// frustum-volume planes (infinite)
	float3 frustumPlanes[FRUSTUM_PLANE_CNT];
	// xy-scales (for orthographic cameras only), .z := znear, .w := zfar
	float4 frustumScales;

	// Lua-controlled parameters, player-camera only
	float3 posOffset;
	float3 tiltOffset;

	GLint viewport[4];

private:
	CMatrix44f projectionMatrix;
	CMatrix44f projectionMatrixInverse;
	CMatrix44f viewMatrix;
	CMatrix44f viewMatrixInverse;
	CMatrix44f viewProjectionMatrix;
	CMatrix44f viewProjectionMatrixInverse;
	CMatrix44f billboardMatrix;

	// positive sides [0], negative sides [1]
	std::vector<FrustumLine> frustumLines[2];

	static CCamera* camTypes[CAMTYPE_COUNT];

	// CAMTYPE_*
	unsigned int camType;
	// PROJTYPE_*
	unsigned int projType;

	bool movState[8]; // fwd, back, left, right, up, down, fast, slow
	bool rotState[4]; // unused
};

#define camera (CCamera::GetCamera(CCamera::CAMTYPE_ACTIVE))
#endif // _CAMERA_H


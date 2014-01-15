/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_H
#define _CAMERA_H

#include "Rendering/GL/myGL.h"
#include "System/float3.h"
#include "System/Matrix44f.h"


class CCamera
{
public:
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

	CCamera();

	float3 CalcPixelDir(int x,int y) const;
	float3 CalcWindowCoordinates(const float3& objPos) const;

	void CopyState(const CCamera*);
	// NOTE:
	//   only FreeController calls this, others just seem to manipulate
	//   azimuth (.x) and zenith (.y) angles for their own (redundant?)
	//   copy of Camera::forward (CameraController::dir)
	//
	//   <forward> is set by CameraHandler::UpdateCam via CamCon::GetDir
	//   <right> and <up> are derived from this, never from <rot> directly
	void UpdateForward(const float3& fwd) { forward = fwd; }
	void UpdateRightAndUp(bool terrainReflectionPass);

	bool InView(const float3& p, float radius = 0) const;
	bool InView(const float3& mins, const float3& maxs) const;
	void Update(bool terrainReflectionPass = false);

	void GetFrustumSides(float miny, float maxy, float scale, bool negSide = false);
	void GetFrustumSide(
		const float3& zdir,
		const float3& offset,
		float miny,
		float maxy,
		float scale,
		bool upwardDir,
		bool negSide);
	void ClearFrustumSides() {

		posFrustumSides.clear();
		negFrustumSides.clear();
	}
	void ClipFrustumLines(bool left, const float zmin, const float zmax);

	const CMatrix44f& GetViewMatrix() const { return viewMatrix; }
	const CMatrix44f& GetViewMatrixInverse() const { return viewMatrixInverse; }
	const CMatrix44f& GetProjectionMatrix() const { return projectionMatrix; }
	const CMatrix44f& GetProjectionMatrixInverse() const { return projectionMatrixInverse; }
	const CMatrix44f& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const CMatrix44f& GetViewProjectionMatrixInverse() const { return viewProjectionMatrixInverse; }
	const CMatrix44f& GetBillBoardMatrix() const { return billboardMatrix; }

	const float3& GetPos() const { return pos; }
	float GetFov() const { return fov; }
	float GetHalfFov() const { return halfFov; }
	float GetTanHalfFov() const { return tanHalfFov; }
	float GetLPPScale() const { return lppScale; }

	/// @param fov in degree
	void SetPos(const float3& p) { pos = p; }
	void SetFov(float fov);

	float GetMoveDistance(float* time, float* speed, int idx) const;
	float3 GetMoveVectorFromState(bool fromKeyState, bool* disableTracker);

	void SetMovState(int idx, bool b) { movState[idx] = b; }
	void SetRotState(int idx, bool b) { rotState[idx] = b; }
	const bool* GetMovState() const { return movState; }
	const bool* GetRotState() const { return rotState; }

public:
	float3 rot;        ///< warning is not always updated

	float3 forward;    ///< local z-axis
	float3 right;      ///< local x-axis
	float3 up;         ///< local y-axis

	float3 topFrustumSideDir;
	float3 botFrustumSideDir;
	float3 rgtFrustumSideDir;
	float3 lftFrustumSideDir;

	// Lua-controlled parameters, camera-only (not cam2)
	float3 posOffset;
	float3 tiltOffset;

	GLint viewport[4];

	struct FrustumLine {
		float base;
		float dir;
		int sign;
		float minz;
		float maxz;
	};

	const std::vector<FrustumLine> GetNegFrustumSides() const {

		return negFrustumSides;
	}
	const std::vector<FrustumLine> GetPosFrustumSides() const {

		return posFrustumSides;
	}

private:
	void ComputeViewRange();

	void myGluPerspective(float aspect, float zNear, float zFar);
	void myGluLookAt(const float3&, const float3&, const float3&);

private:
	CMatrix44f projectionMatrix;
	CMatrix44f projectionMatrixInverse;
	CMatrix44f viewMatrix;
	CMatrix44f viewMatrixInverse;
	CMatrix44f viewProjectionMatrix;
	CMatrix44f viewProjectionMatrixInverse;
	CMatrix44f billboardMatrix;

	std::vector<FrustumLine> posFrustumSides;
	std::vector<FrustumLine> negFrustumSides;

	float3 pos;
	float fov;         ///< in degrees
	float halfFov;     ///< half the fov in radians
	float tanHalfFov;  ///< math::tan(halfFov)
	float lppScale;    ///< length-per-pixel scale

	bool movState[8]; // fwd, back, left, right, up, down, fast, slow
	bool rotState[4]; // unused
};

extern CCamera* camera;
extern CCamera* cam2;

#endif // _CAMERA_H

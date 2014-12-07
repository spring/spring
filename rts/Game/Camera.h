/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_H
#define _CAMERA_H

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
	CCamera();
	void CopyState(const CCamera*);

	void Update();

	/// @param fov in degree
	void SetPos(const float3& p) { pos = p; }
	void SetFov(const float fov);
	void SetDir(const float3 dir);

	const float3& GetPos() const { return pos; }
	float GetFov() const { return fov; }
	const float3& GetDir() const     { return forward; }

	const float3& GetForward() const { return forward; }
	const float3& GetRight() const   { return right; }
	const float3& GetUp() const      { return up; }

	const float3& GetRot() { return rot; }
	void SetRot(const float3 rot);
	void SetRotX(const float x);
	void SetRotY(const float y);
	void SetRotZ(const float z);

	float3 CalcPixelDir(int x,int y) const;
	float3 CalcWindowCoordinates(const float3& objPos) const;

	bool InView(const float3& p, float radius = 0) const;
	bool InView(const float3& mins, const float3& maxs) const;

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
	const std::vector<FrustumLine>& GetNegFrustumSides() const { return negFrustumSides; }
	const std::vector<FrustumLine>& GetPosFrustumSides() const { return posFrustumSides; }

	const CMatrix44f& GetViewMatrix() const { return viewMatrix; }
	const CMatrix44f& GetViewMatrixInverse() const { return viewMatrixInverse; }
	const CMatrix44f& GetProjectionMatrix() const { return projectionMatrix; }
	const CMatrix44f& GetProjectionMatrixInverse() const { return projectionMatrixInverse; }
	const CMatrix44f& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const CMatrix44f& GetViewProjectionMatrixInverse() const { return viewProjectionMatrixInverse; }
	const CMatrix44f& GetBillBoardMatrix() const { return billboardMatrix; }

	float GetHalfFov() const { return halfFov; }
	float GetTanHalfFov() const { return tanHalfFov; }
	float GetLPPScale() const { return lppScale; }

	float GetMoveDistance(float* time, float* speed, int idx) const;
	float3 GetMoveVectorFromState(bool fromKeyState) const;

	void SetMovState(int idx, bool b) { movState[idx] = b; }
	void SetRotState(int idx, bool b) { rotState[idx] = b; }
	const bool* GetMovState() const { return movState; }
	const bool* GetRotState() const { return rotState; }

	static float3 GetRotFromDir(float3 dir);

private:
	void ComputeViewRange();

	void myGluPerspective(float aspect, float zNear, float zFar);
	void myGluLookAt(const float3&, const float3&, const float3&);

	void UpdateDirFromRot();

public:
	float3 pos;
	float3 rot;        ///< x = inclination, y = azimuth (to the -z axis!), z = roll
	float3 forward;    ///< local z-axis
	float3 right;      ///< local x-axis
	float3 up;         ///< local y-axis

	float fov;         ///< in degrees
	float halfFov;     ///< half the fov in radians
	float tanHalfFov;  ///< math::tan(halfFov)
	float lppScale;    ///< length-per-pixel scale

public:
	float3 topFrustumSideDir;
	float3 botFrustumSideDir;
	float3 rgtFrustumSideDir;
	float3 lftFrustumSideDir;

	// Lua-controlled parameters, camera-only (not cam2)
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

	std::vector<FrustumLine> posFrustumSides;
	std::vector<FrustumLine> negFrustumSides;

	bool movState[8]; // fwd, back, left, right, up, down, fast, slow
	bool rotState[4]; // unused
};

extern CCamera* camera;
extern CCamera* cam2;

#endif // _CAMERA_H

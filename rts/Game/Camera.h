/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_H
#define _CAMERA_H

#include "Rendering/GL/myGL.h"
#include "System/float3.h"
#include "System/Matrix44f.h"


class CCamera
{
public:
	CCamera();

	float3 CalcPixelDir(int x,int y) const;
	float3 CalcWindowCoordinates(const float3& objPos) const;
	void CopyState(const CCamera*);
	void UpdateForward();
	bool InView(const float3& p, float radius = 0) const;
	bool InView(const float3& mins, const float3& maxs) const;
	void Update(bool resetUp = true);

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
		GML_RECMUTEX_LOCK(cam); // ClearFrustumSides

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

	float GetFov() const { return fov; }
	float GetHalfFov() const { return halfFov; }
	float GetTanHalfFov() const { return tanHalfFov; }
	float GetLPPScale() const { return lppScale; }

	/// @param fov in degree
	void SetFov(float fov);


	float3 pos;
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
		GML_RECMUTEX_LOCK(cam); // GetNegFrustumSides

		return negFrustumSides;
	}
	const std::vector<FrustumLine> GetPosFrustumSides() const {
		GML_RECMUTEX_LOCK(cam); // GetPosFrustumSides

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

	std::vector<GLdouble> viewMatrixD;
	std::vector<GLdouble> projectionMatrixD;

	std::vector<FrustumLine> posFrustumSides;
	std::vector<FrustumLine> negFrustumSides;

	float fov;         ///< in degrees
	float halfFov;     ///< half the fov in radians
	float tanHalfFov;  ///< math::tan(halfFov)
	float lppScale;    ///< length-per-pixel scale
};

extern CCamera* camera;
extern CCamera* cam2;

#endif // _CAMERA_H

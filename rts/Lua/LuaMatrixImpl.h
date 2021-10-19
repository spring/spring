/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATRIX_IMPL_H
#define LUA_MATRIX_IMPL_H

#include <map>
#include <array>
#include <string>
#include <tuple>

#include "lib/sol2/sol.hpp"

#include "System/Matrix44f.h"
#include "System/MathConstants.h"
#include "System/float4.h"

class CSolidObject;
struct LocalModelPiece;
class CCamera;

class CMatrix44fProxy : public CMatrix44f {
public:
	//begin, end, size
	float* begin() noexcept { return std::begin(m); }
	const float* begin() const noexcept { return std::begin(m); }
	float* end() noexcept { return std::end(m); }
	const float* end() const noexcept { return std::end(m); }
	constexpr std::size_t size() const noexcept { return std::size(m); }
};

class float4Proxy : public float4 {
public:
	//begin, end, size
	float* begin() noexcept { return std::begin(xyz); }
	const float* begin() const noexcept { return std::begin(xyz); }
	float* end() noexcept { return std::end(xyz) + 1; }
	const float* end() const noexcept { return std::end(xyz) + 1; }
	constexpr std::size_t size() const noexcept { return std::size(xyz) + 1; }
};

using tuple16f = std::tuple< float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float >;

class LuaMatrixImpl {
public:
	LuaMatrixImpl() = default; //matrices should default to identity
	LuaMatrixImpl(LuaMatrixImpl&& lmi) = default;
	LuaMatrixImpl(CMatrix44f mm) : mat(mm) {};
	LuaMatrixImpl(const LuaMatrixImpl& lmi) : mat(lmi.mat) {}; //for deepcopy
public:
	void Zero();
	void Identity() { mat = CMatrix44f::Identity(); };

	void SetMatrixElements(
		const float  m0, const float  m1, const float  m2, const float  m3,
		const float  m4, const float  m5, const float  m6, const float  m7,
		const float  m8, const float  m9, const float m10, const float m11,
		const float m12, const float m13, const float m14, const float m15)
	{
		mat = CMatrix44f{m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15};
	};

	LuaMatrixImpl DeepCopy() const { return LuaMatrixImpl(*this); }

	void InvertAffine() { mat.InvertAffineInPlace(); };
	void Invert() { mat.InvertInPlace(); };

	void Transpose() { mat.Transpose(); };

	void Translate(const float x, const float y, const float z);
	void Scale(const float x, const float y, const float z);
	void Scale(const float s);

	void RotateRad(const float rad, const float x, const float y, const float z);
	void RotateDeg(const float deg, const float x, const float y, const float z) { RotateRad(deg * math::DEG_TO_RAD, x, y, z); };

	void RotateRadX(const float rad) { RotateRad(rad, 1.0f, 0.0f, 0.0f); };
	void RotateRadY(const float rad) { RotateRad(rad, 0.0f, 1.0f, 0.0f); };
	void RotateRadZ(const float rad) { RotateRad(rad, 0.0f, 0.0f, 1.0f); };

	void RotateDegX(const float deg) { RotateDeg(deg, 1.0f, 0.0f, 0.0f); };
	void RotateDegY(const float deg) { RotateDeg(deg, 0.0f, 1.0f, 0.0f); };
	void RotateDegZ(const float deg) { RotateDeg(deg, 0.0f, 0.0f, 1.0f); };

	// NB: sol::this_state is passed implicitly by sol. It's nothing more than a POD type carrying lua_State*, thus no need to pass it by reference
	bool UnitMatrix(const unsigned int unitID, const sol::optional<bool> mult, sol::this_state L);
	bool UnitPieceMatrix(const unsigned int unitID, const unsigned int pieceNum, const sol::optional<bool> mult, sol::this_state L);

	bool FeatureMatrix(const unsigned int featureID, const sol::optional<bool> mult, sol::this_state L);
	bool FeaturePieceMatrix(const unsigned int featureID, const unsigned int pieceNum, const sol::optional<bool> mult, sol::this_state L);

	bool ProjectileMatrix(const unsigned int projID, const sol::optional<bool> mult, sol::this_state L);

	void ScreenViewMatrix(const sol::optional<bool> mult);
	void ScreenProjMatrix(const sol::optional<bool> mult);
	void ScreenViewProjMatrix(const sol::optional<bool> mult);

	void ShadowViewMatrix(const sol::optional<bool> mult);
	void ShadowProjMatrix(const sol::optional<bool> mult);
	void ShadowViewProjMatrix(const sol::optional<bool> mult);

	void Ortho(const float left, const float right, const float bottom, const float top, const float near, const float far, const sol::optional<bool> mult);
	void Frustum(const float left, const float right, const float bottom, const float top, const float near, const float far, const sol::optional<bool> mult);
	void LookAt(const float eyeX, const float eyeY, const float eyeZ, const float atX, const float atY, const float atZ, const float roll, const sol::optional<bool> mult);

	void CameraViewMatrix(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);
	void CameraProjMatrix(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);
	void CameraViewProjMatrix(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);
	void CameraViewInverseMatrix(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);
	void CameraProjInverseMatrix(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);
	void CameraViewProjInverseMatrix(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);
	void CameraBillboardMatrix(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);

	tuple16f GetAsScalar() const
	{
		return std::make_tuple(
			mat[0], mat[1], mat[2], mat[3],
			mat[4], mat[5], mat[6], mat[7],
			mat[8], mat[9], mat[10], mat[11],
			mat[12], mat[13], mat[14], mat[15]
		);
	}

	sol::as_table_t<CMatrix44fProxy> GetAsTable()
	{
		return sol::as_table(static_cast<CMatrix44fProxy&>(mat));
	}

public:
	const CMatrix44f& GetMatRef() const { return  mat; }
	const CMatrix44f* GetMatPtr() const { return &mat; }
public:
	sol::as_table_t<float4Proxy> operator* (const sol::table& tbl) const {
		float4 f4 {0.0f, 0.0f, 0.0f, 1.0f};
		for (int i = 1; i <= 4; ++i) {
			f4[i - 1] = tbl[i].get_or<float>(f4[i - 1]);
		}
		f4 = mat * f4;
		return sol::as_table(static_cast<float4Proxy&>(f4));
	};
	LuaMatrixImpl operator* (const LuaMatrixImpl& lmi) const { return LuaMatrixImpl(mat * lmi.mat); };
	LuaMatrixImpl operator+ (const LuaMatrixImpl& lmi) const { return LuaMatrixImpl(mat + lmi.mat); };
private:
	void AssignOrMultMatImpl(const sol::optional<bool> mult, const bool multDefault, const CMatrix44f* matIn);
	void AssignOrMultMatImpl(const sol::optional<bool> mult, const bool multDefault, const CMatrix44f& matIn);
private:
	//static templated methods
	template<typename TObj>
	static const TObj* ParseObject(int objID, sol::this_state L);

	template<typename TObj>
	bool GetObjectMatImpl(const unsigned int objID, CMatrix44f& outMat, sol::this_state L);
	template <typename  TObj>
	bool ObjectMatImpl(const unsigned int objID, bool mult, sol::this_state L);

	template <typename  TObj>
	bool GetObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, CMatrix44f& outMat, sol::this_state L);
	template <typename  TObj>
	bool ObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, bool mult, sol::this_state L);

	template <typename  TObj>
	static bool IsObjectVisible(const TObj* obj, sol::this_state L);

private:
	//static methods
	static CCamera* GetCamera(const sol::optional<unsigned int> cameraIdOpt);
	static const LocalModelPiece* ParseObjectConstLocalModelPiece(const CSolidObject* obj, const unsigned int pieceNum);
private:
	//static constants
	static constexpr bool VIEWPROJ_MULT_DEFAULT = false;
	static constexpr bool OBJECT_MULT_DEFAULT = true;
private:
	CMatrix44f mat;
};

#endif //LUA_MATRIX_IMPL_H

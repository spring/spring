/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATRIX_H
#define LUA_MATRIX_H

#include <map>
#include <vector>
#include <string>
#include <tuple>

#include "System/Log/ILog.h"

#include "lib/sol2/sol.hpp"
#include "lib/fmt/format.h"

#include "System/Matrix44f.h"
#include "System/MathConstants.h"
#include "System/EventClient.h"
#include "System/float4.h"

#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"

#undef near
#undef far

struct lua_State;


struct CSolidObject;

struct LocalModelPiece;

typedef std::tuple< float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float > tuple16f;
typedef std::tuple< float, float > tuple2f;
typedef std::tuple< float, float, float > tuple3f;
typedef std::tuple< float, float, float, float > tuple4f;

class LuaMatrixImpl {
public:
	LuaMatrixImpl() = default; //matrices should default to identity
	LuaMatrixImpl(CMatrix44f mm) { mat = mm; }

	LuaMatrixImpl(const LuaMatrixImpl& lmi) :
		mat(lmi.mat)
	{};

	//LuaMatrixImpl(LuaMatrixImpl&& lva) = default; //move cons
	//~LuaMatrixImpl() = default;
public:
	void Zero();
	void Identity() { mat = CMatrix44f::Identity(); };

	void SetMatrixElements(
		const float m0, const float m1, const float m2, const float m3,
		const float m4, const float m5, const float m6, const float m7,
		const float m8, const float m9, const float m10, const float m11,
		const float m12, const float m13, const float m14, const float m15);

	LuaMatrixImpl DeepCopy() { return LuaMatrixImpl(*this); }

	/*
	tuple2f Mult(const float x, const float y) { Mult(x, y, 0.0f, 1.0f); }
	tuple3f Mult(const float x, const float y, const float z) { Mult(x, y, z, 1.0f); }
	tuple4f Mult(const float x, const float y, const float z, const float w)
	{
		float4 res = mat.Mul(float4{ x, y, z, w });
		return std::make_tuple(res.x, res.y, res.z, res.w);
	}
	LuaMatrixImpl Mult(const LuaMatrixImpl& mat2) { return mat * mat2.GetMatRef(); }

	LuaMatrixImpl Add(const LuaMatrixImpl& mat2) { return mat + mat2.GetMatRef(); }
	*/

	void InverseAffine() { mat.InvertAffineInPlace(); };
	void Inverse() { mat.InvertInPlace(); };

	void Transpose() { mat.Transpose(); };

	void Translate(const float x, const float y, const float z);
	void Scale(const float x, const float y, const float z);
	void Scale(const float s);

	void RotateRad(const float rad, const float x, const float y, const float z);
	void RotateDeg(const float deg, const float x, const float y, const float z) { RotateRad(deg * math::DEG_TO_RAD, x, y, z); };

	void RotateRadX(const float rad) { RotateRad(rad, 1.0f, 0.0f, 0.0f); };
	void RotateRadY(const float rad) { RotateRad(rad, 0.0f, 1.0f, 0.0f); };
	void RotateRadZ(const float rad) { RotateRad(rad, 0.0f, 0.0f, 1.0f); };

	void RotateDegX(const float deg) { RotateRad(deg * math::DEG_TO_RAD, 1.0f, 0.0f, 0.0f); };
	void RotateDegY(const float deg) { RotateRad(deg * math::DEG_TO_RAD, 0.0f, 1.0f, 0.0f); };
	void RotateDegZ(const float deg) { RotateRad(deg * math::DEG_TO_RAD, 0.0f, 0.0f, 1.0f); };

	bool UnitMatrix(const unsigned int unitID, const sol::optional<bool> mult, sol::this_state L) { return ObjectMatImpl<CUnit>(unitID, mult.value_or(objectMultDefault), L); }
	bool UnitPieceMatrix(const unsigned int unitID, const unsigned int pieceNum, const sol::optional<bool> mult, sol::this_state L) { return ObjectPieceMatImpl<CUnit>(unitID, pieceNum, mult.value_or(objectMultDefault), L); }

	bool FeatureMatrix(const unsigned int featureID, const sol::optional<bool> mult, sol::this_state L) { return ObjectMatImpl<CFeature>(featureID, mult.value_or(objectMultDefault), L); }
	bool FeaturePieceMatrix(const unsigned int featureID, const unsigned int pieceNum, const sol::optional<bool> mult, sol::this_state L) { return ObjectPieceMatImpl<CFeature>(featureID, pieceNum, mult.value_or(objectMultDefault), L); }

	bool ProjectileMatrix(const unsigned int projID, const sol::optional<bool> mult, sol::this_state L) { return ObjectMatImpl<CFeature>(projID, mult.value_or(objectMultDefault), L); }
	bool ProjectilePieceMatrix(const unsigned int projID, const unsigned int pieceNum, const sol::optional<bool> mult, sol::this_state L) { return ObjectPieceMatImpl<CFeature>(projID, pieceNum, mult.value_or(objectMultDefault), L); } //does this exist at all?

	void ScreenViewMatrix(const sol::optional<bool> mult);
	void ScreenProjMatrix(const sol::optional<bool> mult);

	void SunViewMatrix(const sol::optional<bool> mult);
	void SunProjMatrix(const sol::optional<bool> mult);

	void Ortho(const float left, const float right, const float bottom, const float top, const float near, const float far, const sol::optional<bool> mult);
	void Frustum(const float left, const float right, const float bottom, const float top, const float near, const float far, const sol::optional<bool> mult);
	void Billboard(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult);

	tuple16f GetAsScalar()
	{
		return std::make_tuple(
			mat[0], mat[1], mat[2], mat[3],
			mat[4], mat[5], mat[6], mat[7],
			mat[8], mat[9], mat[10], mat[11],
			mat[12], mat[13], mat[14], mat[15]
		);
	}

	sol::as_table_t<std::vector<float>> GetAsTable()
	{
		return sol::as_table(std::vector<float> {
			mat[0], mat[1], mat[2], mat[3],
			mat[4], mat[5], mat[6], mat[7],
			mat[8], mat[9], mat[10], mat[11],
			mat[12], mat[13], mat[14], mat[15]
		});
	}

public:
	const CMatrix44f& GetMatRef() const { return  mat; }
	const CMatrix44f* GetMatPtr() const { return &mat; }
public:
	sol::as_table_t<std::vector<float>> operator* (const sol::table& tbl) const {
		float4 f4 { 0.0f, 0.0f, 0.0f, 1.0f };
		for (auto i = 1; i <= 4; ++i) {
			const sol::optional<float> maybeValue = tbl[i];
			if (!maybeValue)
				continue;

			f4[i - 1] = maybeValue.value();
		}
		f4 = mat * f4;
		return sol::as_table( std::vector<float> { f4.x, f4.y, f4.z, f4.w } );
	};
	LuaMatrixImpl operator* (const LuaMatrixImpl& lmi) const { return LuaMatrixImpl(mat * lmi.GetMatRef()); };

	LuaMatrixImpl operator+ (const LuaMatrixImpl& lmi) const { return LuaMatrixImpl(mat + lmi.GetMatRef()); };
private:
	template<typename TFunc>
	inline void AssignOrMultMatImpl(const sol::optional<bool> mult, const bool multDefault, const TFunc& tfunc);

	template<typename TObj>
	inline static const TObj* ParseObject(int objID, sol::this_state L);

	template<typename TObj>
	inline bool GetObjectMatImpl(const unsigned int objID, CMatrix44f& outMat, sol::this_state L);
	template <typename  TObj>
	bool ObjectMatImpl(const unsigned int objID, bool mult, sol::this_state L);

	template <typename  TObj>
	bool GetObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, CMatrix44f& outMat, sol::this_state L);
	template <typename  TObj>
	bool ObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, bool mult, sol::this_state L);

	template <typename  TObj>
	inline static bool IsObjectVisible(const TObj* obj, sol::this_state L);

private:
	static void SetupScreenMatrices();

	inline static const LocalModelPiece* ParseObjectConstLocalModelPiece(const CSolidObject* obj, const unsigned int pieceNum);
	//static methods
private:
	static constexpr bool viewProjMultDefault = false;
	static constexpr bool objectMultDefault = true;
	//static constants
private:
	CMatrix44f mat;
private:
	static CMatrix44f screenViewMatrix;
	static CMatrix44f screenProjMatrix;
	//private vars
};

class LuaMatrix {
public:
	static int GetMatrix(lua_State* L);
	static bool PushEntries(lua_State* L);
};


#endif //LUA_MATRIX_H
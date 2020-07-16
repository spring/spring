#include "LuaMatrix.hpp"

#include <algorithm>

#include "LuaUtils.h"

#include "System/type2.h"

#include "Sim/Objects/SolidObject.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitHandler.h"

#include "Rendering/Models/3DModel.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GlobalRendering.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"

#undef near
#undef far

CMatrix44f LuaMatrixImpl::screenViewMatrix = CMatrix44f{};
CMatrix44f LuaMatrixImpl::screenProjMatrix = CMatrix44f{};

void LuaMatrixImpl::Zero()
{
#if 1
	std::fill(&mat[0], &mat[0] + 16, 0.0f);
#else
	#define MAT_EQ_0(i) mat[i] = 0.0f
		MAT_EQ_0(0); MAT_EQ_0(1); MAT_EQ_0(2); MAT_EQ_0(3);
		MAT_EQ_0(4); MAT_EQ_0(5); MAT_EQ_0(6); MAT_EQ_0(7);
		MAT_EQ_0(8); MAT_EQ_0(9); MAT_EQ_0(10); MAT_EQ_0(11);
		MAT_EQ_0(12); MAT_EQ_0(13); MAT_EQ_0(14); MAT_EQ_0(15);
	#undef MAT_EQ_0
#endif
}

void LuaMatrixImpl::SetMatrixElements(const float m0, const float m1, const float m2, const float m3, const float m4, const float m5, const float m6, const float m7, const float m8, const float m9, const float m10, const float m11, const float m12, const float m13, const float m14, const float m15)
{
#define MAT_EQ_M(i) mat[i] = m##i
	MAT_EQ_M(0); MAT_EQ_M(1); MAT_EQ_M(2); MAT_EQ_M(3);
	MAT_EQ_M(4); MAT_EQ_M(5); MAT_EQ_M(6); MAT_EQ_M(7);
	MAT_EQ_M(8); MAT_EQ_M(9); MAT_EQ_M(10); MAT_EQ_M(11);
	MAT_EQ_M(12); MAT_EQ_M(13); MAT_EQ_M(14); MAT_EQ_M(15);
#undef MAT_EQ_M
}

void LuaMatrixImpl::Translate(const float x, const float y, const float z)
{
	mat.Translate({ x, y, z });
}

void LuaMatrixImpl::Scale(const float x, const float y, const float z)
{
	mat.Scale({ x, y, z });
}

void LuaMatrixImpl::Scale(const float s)
{
	mat.Scale({ s, s, s });
}

void LuaMatrixImpl::RotateRad(const float rad, const float x, const float y, const float z)
{
	mat.Rotate(-rad , { x, y, z });
}

///////////////////////////////////////////////////////////

template<typename TObj>
inline bool LuaMatrixImpl::IsObjectVisible(const TObj* obj, sol::this_state L)
{
	if (CLuaHandle::GetHandleFullRead(L))
		return true;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if ((readAllyTeam < 0) && (readAllyTeam == CEventClient::NoAccessTeam))
		return false;

	return (obj->IsInLosForAllyTeam(readAllyTeam));
}

template<typename TObj>
inline const TObj* LuaMatrixImpl::ParseObject(int objID, sol::this_state L)
{
	const TObj* obj = nullptr;
	if constexpr(std::is_same<TObj, CUnit>::value) {
		obj = unitHandler.GetUnit(objID);
	}
	else if constexpr(std::is_same<TObj, CFeature>::value) {
		obj = featureHandler.GetFeature(objID);
	}
	else if constexpr(std::is_same<TObj, CProjectile>::value) {
		obj = projectileHandler.GetProjectileBySyncedID(objID);
	}

	if (obj == nullptr)
		return nullptr;

	if (!IsObjectVisible(obj, L))
		return nullptr;

	return obj;
}

inline const LocalModelPiece* LuaMatrixImpl::ParseObjectConstLocalModelPiece(const CSolidObject* obj, const unsigned int pieceNum)
{
	if (!obj->localModel.HasPiece(pieceNum))
		return nullptr;

	return (obj->localModel.GetPiece(pieceNum));
}

///////////////////////////////////////////////////////////

template<typename TFunc>
inline void LuaMatrixImpl::AssignOrMultMatImpl(const sol::optional<bool> mult, const bool multDefault, const TFunc& tfunc)
{
	const bool multVal = mult.value_or(multDefault);
	if (!multVal)
		mat = tfunc();
	else {
		CMatrix44f tmpMat = tfunc();
		mat *= tmpMat;
	}
}

template<typename TObj>
inline bool LuaMatrixImpl::GetObjectMatImpl(const unsigned int objID, CMatrix44f& outMat, sol::this_state L)
{
	const TObj* obj = ParseObject<TObj>(objID, L);

	if (!obj)
		return false;

	outMat = obj->GetTransformMatrix();

	return true;
}

template<typename TObj>
inline bool LuaMatrixImpl::ObjectMatImpl(const unsigned int objID, bool mult, sol::this_state L)
{
	if (!mult)
		return GetObjectMatImpl<TObj>(objID, mat, L);
	else {
		CMatrix44f mat2;
		if (!GetObjectMatImpl<TObj>(objID, mat2, L))
			return false;

		mat *= mat2;
		return true;
	}
}

template<typename TObj>
inline bool LuaMatrixImpl::GetObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, CMatrix44f& outMat, sol::this_state L)
{
	const TObj* obj = ParseObject<TObj>(objID, L);

	if (!obj)
		return false;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(obj, pieceNum);

	if (!lmp)
		return false;

	outMat = lmp->GetModelSpaceMatrix();
	return true;
}

template<typename TObj>
inline bool LuaMatrixImpl::ObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, bool mult, sol::this_state L)
{
	if (!mult)
		return GetObjectPieceMatImpl<TObj>(objID, pieceNum, mat, L);
	else {
		CMatrix44f mat2;
		if (!GetObjectPieceMatImpl<TObj>(objID, pieceNum, mat2, L))
			return false;

		mat *= mat2;
		return true;
	}
}

inline void LuaMatrixImpl::CondSetupScreenMatrices() {
	// .x := screen width (meters), .y := eye-to-screen (meters)
	static float2 screenParameters = { 0.36f, 0.60f };

	const int remScreenSize = globalRendering->screenSizeY - globalRendering->winSizeY; // remaining desktop size (ssy >= wsy)
	const int bottomWinCoor = remScreenSize - globalRendering->winPosY; // *bottom*-left origin

	const float vpx = globalRendering->viewPosX + globalRendering->winPosX;
	const float vpy = globalRendering->viewPosY + bottomWinCoor;
	const float vsx = globalRendering->viewSizeX; // same as winSizeX except in dual-screen mode
	const float vsy = globalRendering->viewSizeY; // same as winSizeY
	const float ssx = globalRendering->screenSizeX;
	const float ssy = globalRendering->screenSizeY;
	const float hssx = 0.5f * ssx;
	const float hssy = 0.5f * ssy;

	const float zplane = screenParameters.y * (ssx / screenParameters.x);
	const float znear = zplane * 0.5f;
	const float zfar = zplane * 2.0f;
	const float zfact = znear / zplane;

	const float left = (vpx - hssx) * zfact;
	const float bottom = (vpy - hssy) * zfact;
	const float right = ((vpx + vsx) - hssx) * zfact;
	const float top = ((vpy + vsy) - hssy) * zfact;

	// translate s.t. (0,0,0) is on the zplane, on the window's bottom-left corner
	LuaMatrixImpl::screenViewMatrix = CMatrix44f{ float3{left / zfact, bottom / zfact, -zplane} };
	LuaMatrixImpl::screenProjMatrix = CMatrix44f::ClipControl(globalRendering->supportClipSpaceControl) * CMatrix44f::PerspProj(left, right, bottom, top, znear, zfar);
}

void LuaMatrixImpl::ScreenViewMatrix(const sol::optional<bool> mult)
{
	CondSetupScreenMatrices();
	const auto lambda = [=]() { return screenViewMatrix; };
	AssignOrMultMatImpl(mult, LuaMatrixImpl::viewProjMultDefault, lambda);
}

void LuaMatrixImpl::ScreenProjMatrix(const sol::optional<bool> mult)
{
	CondSetupScreenMatrices();
	const auto lambda = [=]() { return screenProjMatrix; };
	AssignOrMultMatImpl(mult, LuaMatrixImpl::viewProjMultDefault, lambda);
}

void LuaMatrixImpl::SunViewMatrix(const sol::optional<bool> mult)
{
	const auto lambda = [=]() { return shadowHandler.GetShadowViewMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING); };
	AssignOrMultMatImpl(mult, LuaMatrixImpl::viewProjMultDefault, lambda);
}

void LuaMatrixImpl::SunProjMatrix(const sol::optional<bool> mult)
{
	const auto lambda = [=]() { return shadowHandler.GetShadowProjMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING); };
	AssignOrMultMatImpl(mult, LuaMatrixImpl::viewProjMultDefault, lambda);
}

void LuaMatrixImpl::Ortho(const float left, const float right, const float bottom, const float top, const float near, const float far, const sol::optional<bool> mult)
{
	const auto lambda = [=]() { return CMatrix44f::ClipOrthoProj(left, right, bottom, top, near, far, globalRendering->supportClipSpaceControl * 1.0f); };
	AssignOrMultMatImpl(mult, LuaMatrixImpl::viewProjMultDefault, lambda);
}

void LuaMatrixImpl::Frustum(const float left, const float right, const float bottom, const float top, const float near, const float far, const sol::optional<bool> mult)
{
	const auto lambda = [=]() { return CMatrix44f::ClipPerspProj(left, right, bottom, top, near, far, globalRendering->supportClipSpaceControl * 1.0f); };
	AssignOrMultMatImpl(mult, LuaMatrixImpl::viewProjMultDefault, lambda);
}

void LuaMatrixImpl::Billboard(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult)
{
	constexpr unsigned int minCamType = CCamera::CAMTYPE_PLAYER;
	constexpr unsigned int maxCamType = CCamera::CAMTYPE_ACTIVE;

	const unsigned int camType = std::clamp(cameraIdOpt.value_or(maxCamType), minCamType, maxCamType);
	const auto cam = CCameraHandler::GetCamera(camType);

	const auto lambda = [=]() { return cam->GetBillBoardMatrix(); };
	AssignOrMultMatImpl(mult, LuaMatrixImpl::viewProjMultDefault, lambda);
}

///////////////////////////////////////////////////////////

bool LuaMatrix::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetMatrix);

	sol::state_view lua(L);
	auto gl = sol::stack::get<sol::table>(L, -1);

	gl.new_usertype<LuaMatrixImpl>("LuaMatrixImpl",

		sol::constructors<LuaMatrixImpl()>(),

		"Zero", &LuaMatrixImpl::Zero,
		"LoadZero", &LuaMatrixImpl::Zero,

		"Identity", &LuaMatrixImpl::Identity,
		"LoadIdentity", &LuaMatrixImpl::Identity,

		"SetMatrixElements", &LuaMatrixImpl::SetMatrixElements,

		"DeepCopy", &LuaMatrixImpl::DeepCopy,

		//"Mult", sol::overload(&LuaMatrixImpl::MultMat4, &LuaMatrixImpl::MultVec4, LuaMatrixImpl::MultVec3),

		"InverseAffine", &LuaMatrixImpl::InverseAffine,
		"InvertAffine", &LuaMatrixImpl::InverseAffine,
		"Inverse", &LuaMatrixImpl::Inverse,
		"Invert", &LuaMatrixImpl::Inverse,

		"Transpose", &LuaMatrixImpl::Transpose,

		"Translate", &LuaMatrixImpl::Translate,
		"Scale", sol::overload(
			sol::resolve<void(const float, const float, const float)>(&LuaMatrixImpl::Scale),
			sol::resolve<void(const float)>(&LuaMatrixImpl::Scale)),

		"RotateRad", &LuaMatrixImpl::RotateRad,
		"RotateDeg", &LuaMatrixImpl::RotateDeg,

		"RotateRadX", &LuaMatrixImpl::RotateRadX,
		"RotateRadY", &LuaMatrixImpl::RotateRadY,
		"RotateRadZ", &LuaMatrixImpl::RotateRadZ,

		"RotateDegX", &LuaMatrixImpl::RotateDegX,
		"RotateDegY", &LuaMatrixImpl::RotateDegY,
		"RotateDegZ", &LuaMatrixImpl::RotateDegZ,

		"UnitMatrix", &LuaMatrixImpl::UnitMatrix,
		"UnitPieceMatrix", &LuaMatrixImpl::UnitPieceMatrix,

		"FeatureMatrix", &LuaMatrixImpl::FeatureMatrix,
		"FeaturePieceMatrix", &LuaMatrixImpl::FeaturePieceMatrix,

		"ProjectileMatrix", & LuaMatrixImpl::ProjectileMatrix,
		"ProjectilePieceMatrix", & LuaMatrixImpl::ProjectilePieceMatrix,

		"ScreenViewMatrix", &LuaMatrixImpl::ScreenViewMatrix,
		"ScreenProjMatrix", &LuaMatrixImpl::ScreenProjMatrix,

		"SunViewMatrix", &LuaMatrixImpl::SunViewMatrix,
		"ShadowViewMatrix", &LuaMatrixImpl::SunViewMatrix,
		"SunProjMatrix", & LuaMatrixImpl::SunProjMatrix,
		"ShadowProjMatrix", &LuaMatrixImpl::SunProjMatrix,

		"Ortho", &LuaMatrixImpl::Ortho,
		"Frustum", &LuaMatrixImpl::Frustum,
		"Billboard", &LuaMatrixImpl::Billboard,

		"GetAsScalar", &LuaMatrixImpl::GetAsScalar,
		"GetAsTable", &LuaMatrixImpl::GetAsTable,

		sol::meta_function::multiplication, sol::overload(
			sol::resolve< LuaMatrixImpl(const LuaMatrixImpl&) const >(&LuaMatrixImpl::operator*),
			sol::resolve< sol::as_table_t<std::vector<float>>(const sol::table&) const >(&LuaMatrixImpl::operator*)
		),

		sol::meta_function::addition, &LuaMatrixImpl::operator+
		);

	//gl.set("Matrix", sol::lua_nil); //because :)

	return true;
}

int LuaMatrix::GetMatrix(lua_State* L)
{
	return sol::stack::call_lua(L, 1, [=]() {
		return std::move(LuaMatrixImpl{});
	});
}
#include "LuaMatrixImpl.h"

#include <algorithm>

#include "LuaUtils.h"

#include "Sim/Misc/LosHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"

CMatrix44f LuaMatrixImpl::screenViewMatrix = CMatrix44f{};
CMatrix44f LuaMatrixImpl::screenProjMatrix = CMatrix44f{};

void LuaMatrixImpl::Zero()
{
	std::fill(&mat[0], &mat[0] + 16, 0.0f);
}

void LuaMatrixImpl::SetMatrixElements(
	const float  m0, const float  m1, const float  m2, const float  m3,
	const float  m4, const float  m5, const float  m6, const float  m7,
	const float  m8, const float  m9, const float m10, const float m11,
	const float m12, const float m13, const float m14, const float m15)
{
	const std::array<float, 16> arr {m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15};
	std::copy(arr.cbegin(), arr.cend(), &mat.m[0]);
}

void LuaMatrixImpl::Translate(const float x, const float y, const float z)
{
	mat.Translate({x, y, z});
}

void LuaMatrixImpl::Scale(const float x, const float y, const float z)
{
	mat.Scale({x, y, z});
}

void LuaMatrixImpl::Scale(const float s)
{
	mat.Scale({s, s, s});
}

void LuaMatrixImpl::RotateRad(const float rad, const float x, const float y, const float z)
{
	// TODO: Find out why minus and if it is correct.
	// This code is copy-pasted from develop
	mat.Rotate(-rad , {x, y, z});
}

///////////////////////////////////////////////////////////

bool LuaMatrixImpl::UnitMatrix(const unsigned int unitID, const sol::optional<bool> mult, sol::this_state L)
{
	return ObjectMatImpl<CUnit>(unitID, mult.value_or(OBJECT_MULT_DEFAULT), L);
}

bool LuaMatrixImpl::UnitPieceMatrix(const unsigned int unitID, const unsigned int pieceNum, const sol::optional<bool> mult, sol::this_state L)
{
	return ObjectPieceMatImpl<CUnit>(unitID, pieceNum, mult.value_or(OBJECT_MULT_DEFAULT), L);
}

bool LuaMatrixImpl::FeatureMatrix(const unsigned int featureID, const sol::optional<bool> mult, sol::this_state L)
{
	return ObjectMatImpl<CFeature>(featureID, mult.value_or(OBJECT_MULT_DEFAULT), L);
}

bool LuaMatrixImpl::FeaturePieceMatrix(const unsigned int featureID, const unsigned int pieceNum, const sol::optional<bool> mult, sol::this_state L)
{
	return ObjectPieceMatImpl<CFeature>(featureID, pieceNum, mult.value_or(OBJECT_MULT_DEFAULT), L);
}

bool LuaMatrixImpl::ProjectileMatrix(const unsigned int projID, const sol::optional<bool> mult, sol::this_state L)
{
	return ObjectMatImpl<CProjectile>(projID, mult.value_or(OBJECT_MULT_DEFAULT), L);
}

///////////////////////////////////////////////////////////

template<typename TObj>
bool LuaMatrixImpl::IsObjectVisible(const TObj* obj, sol::this_state L)
{
	if (CLuaHandle::GetHandleFullRead(L))
		return true;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if ((readAllyTeam < 0) && (readAllyTeam == CEventClient::NoAccessTeam))
		return false;

	if constexpr(std::is_same<TObj, CProjectile>::value) //CProjectile has no IsInLosForAllyTeam()
		return losHandler->InLos(obj, readAllyTeam);
	else //else is needed. Otherwise won't compile
		return obj->IsInLosForAllyTeam(readAllyTeam);
}

template<typename TObj>
const TObj* LuaMatrixImpl::ParseObject(int objID, sol::this_state L)
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

const LocalModelPiece* LuaMatrixImpl::ParseObjectConstLocalModelPiece(const CSolidObject* obj, const unsigned int pieceNum)
{
	if (!obj->localModel.HasPiece(pieceNum))
		return nullptr;

	return (obj->localModel.GetPiece(pieceNum));
}

///////////////////////////////////////////////////////////

void LuaMatrixImpl::AssignOrMultMatImpl(const sol::optional<bool> mult, const bool multDefault, const CMatrix44f* matIn)
{
	const bool multVal = mult.value_or(multDefault);

	if (!multVal) {
		mat = *matIn;
		return;
	}

	//TODO: check multiplication order
	CMatrix44f tmpMat = *matIn;
	mat *= tmpMat;
}

void LuaMatrixImpl::AssignOrMultMatImpl(const sol::optional<bool> mult, const bool multDefault, const CMatrix44f& matIn)
{
	AssignOrMultMatImpl(mult, multDefault, &matIn);
}

template<typename TObj>
bool LuaMatrixImpl::GetObjectMatImpl(const unsigned int objID, CMatrix44f& outMat, sol::this_state L)
{
	const TObj* obj = ParseObject<TObj>(objID, L);

	if (obj == nullptr)
		return false;

	outMat = CMatrix44f{};// obj->GetTransformMatrix();

	return true;
}

template<typename TObj>
bool LuaMatrixImpl::ObjectMatImpl(const unsigned int objID, bool mult, sol::this_state L)
{
	if (!mult)
		return GetObjectMatImpl<TObj>(objID, mat, L);

	CMatrix44f mat2;
	if (!GetObjectMatImpl<TObj>(objID, mat2, L))
		return false;

	// TODO: check multiplication order
	mat *= mat2;
	return true;
}

template<typename TObj>
bool LuaMatrixImpl::GetObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, CMatrix44f& outMat, sol::this_state L)
{
	const TObj* obj = ParseObject<TObj>(objID, L);

	if (obj == nullptr)
		return false;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(obj, pieceNum);

	if (lmp == nullptr)
		return false;

	outMat = lmp->GetModelSpaceMatrix();
	return true;
}

template<typename TObj>
bool LuaMatrixImpl::ObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, bool mult, sol::this_state L)
{
	if (!mult)
		return GetObjectPieceMatImpl<TObj>(objID, pieceNum, mat, L);

	CMatrix44f mat2;
	if (!GetObjectPieceMatImpl<TObj>(objID, pieceNum, mat2, L))
		return false;

	mat *= mat2;
	return true;
}

void LuaMatrixImpl::ScreenViewMatrix(const sol::optional<bool> mult)
{
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, globalRendering->screenViewMatrix);
}

void LuaMatrixImpl::ScreenProjMatrix(const sol::optional<bool> mult)
{
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, globalRendering->screenProjMatrix);
}

void LuaMatrixImpl::ScreenViewProjMatrix(const sol::optional<bool> mult)
{
	auto screenViewProjMatrix = (*globalRendering->screenProjMatrix) * (*globalRendering->screenViewMatrix);
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, screenViewProjMatrix);
}

void LuaMatrixImpl::SunViewMatrix(const sol::optional<bool> mult)
{
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, shadowHandler.GetShadowViewMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING));
}

void LuaMatrixImpl::SunProjMatrix(const sol::optional<bool> mult)
{
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, shadowHandler.GetShadowProjMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING));
}

void LuaMatrixImpl::SunViewProjMatrix(const sol::optional<bool> mult)
{
	auto viewProj = shadowHandler.GetShadowProjMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING) * shadowHandler.GetShadowViewMatrix(CShadowHandler::SHADOWMAT_TYPE_DRAWING);
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, viewProj);
}

void LuaMatrixImpl::Ortho(const float left, const float right, const float bottom, const float top, const float _near, const float _far, const sol::optional<bool> mult)
{
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, CMatrix44f::ClipOrthoProj(left, right, bottom, top, _near, _far, globalRendering->supportClipSpaceControl * 1.0f));
}

void LuaMatrixImpl::Frustum(const float left, const float right, const float bottom, const float top, const float _near, const float _far, const sol::optional<bool> mult)
{
	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, CMatrix44f::ClipPerspProj(left, right, bottom, top, _near, _far, globalRendering->supportClipSpaceControl * 1.0f));
}

void LuaMatrixImpl::Billboard(const sol::optional<unsigned int> cameraIdOpt, const sol::optional<bool> mult)
{
	constexpr unsigned int minCamType = CCamera::CAMTYPE_PLAYER;
	constexpr unsigned int maxCamType = CCamera::CAMTYPE_ACTIVE;

	const unsigned int camType = std::clamp(cameraIdOpt.value_or(maxCamType), minCamType, maxCamType);
	const auto cam = CCameraHandler::GetCamera(camType);

	AssignOrMultMatImpl(mult, LuaMatrixImpl::VIEWPROJ_MULT_DEFAULT, cam->GetBillBoardMatrix());
}
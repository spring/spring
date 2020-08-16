#include "LuaMatrix.h"

#include "lib/sol2/sol.hpp"

#include "LuaMatrixImpl.h"
#include "LuaUtils.h"

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

		"ProjectileMatrix", &LuaMatrixImpl::ProjectileMatrix,

		"ScreenViewMatrix", &LuaMatrixImpl::ScreenViewMatrix,
		"ScreenProjMatrix", &LuaMatrixImpl::ScreenProjMatrix,
		"ScreenViewProjMatrix", & LuaMatrixImpl::ScreenViewProjMatrix,

		"SunViewMatrix", &LuaMatrixImpl::SunViewMatrix,
		"ShadowViewMatrix", &LuaMatrixImpl::SunViewMatrix,
		"SunProjMatrix", &LuaMatrixImpl::SunProjMatrix,
		"ShadowProjMatrix", &LuaMatrixImpl::SunProjMatrix,
		"SunViewProjMatrix", & LuaMatrixImpl::SunViewProjMatrix,
		"ShadowViewProjMatrix", & LuaMatrixImpl::SunViewProjMatrix,

		"Ortho", &LuaMatrixImpl::Ortho,
		"Frustum", &LuaMatrixImpl::Frustum,

		"CameraViewMatrix", &LuaMatrixImpl::CameraViewMatrix,
		"CameraProjMatrix", &LuaMatrixImpl::CameraProjMatrix,
		"CameraViewProjMatrix", &LuaMatrixImpl::CameraViewProjMatrix,
		"CameraViewInverseMatrix", &LuaMatrixImpl::CameraViewInverseMatrix,
		"CameraProjInverseMatrix", &LuaMatrixImpl::CameraProjInverseMatrix,
		"CameraViewProjInverseMatrix", &LuaMatrixImpl::CameraViewProjInverseMatrix,
		"CameraBillboardMatrix", &LuaMatrixImpl::CameraBillboardMatrix,

		"GetAsScalar", &LuaMatrixImpl::GetAsScalar,
		"GetAsTable", &LuaMatrixImpl::GetAsTable,

		sol::meta_function::multiplication, sol::overload(
			sol::resolve< LuaMatrixImpl(const LuaMatrixImpl&) const >(&LuaMatrixImpl::operator*),
			sol::resolve< sol::as_table_t<std::array<float, 4>>(const sol::table&) const >(&LuaMatrixImpl::operator*)
		),

		sol::meta_function::addition, &LuaMatrixImpl::operator+
		);

	gl.set("LuaMatrixImpl", sol::lua_nil); //because it's awkward :)

	return true;
}

int LuaMatrix::GetMatrix(lua_State* L)
{
	return sol::stack::call_lua(L, 1, [=]() {
		return std::move(LuaMatrixImpl{});
	});
}
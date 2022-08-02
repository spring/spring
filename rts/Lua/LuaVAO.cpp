#include "LuaVAO.h"

#include "lib/sol2/sol.hpp"

#include "LuaVAOImpl.h"
#include "LuaVBOImpl.h"

#include "LuaUtils.h"

///////////////////////////////////////////////////////////

bool LuaVAOs::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetVAO);

	sol::state_view lua(L);
	auto gl = sol::stack::get<sol::table>(L, -1);

	gl.new_usertype<LuaVAOImpl>("VAO",
		sol::constructors<LuaVAOImpl()>(),
		"Delete", &LuaVAOImpl::Delete,

		"AttachVertexBuffer", &LuaVAOImpl::AttachVertexBuffer,
		"AttachInstanceBuffer", &LuaVAOImpl::AttachInstanceBuffer,
		"AttachIndexBuffer", &LuaVAOImpl::AttachIndexBuffer,

		"DrawArrays", &LuaVAOImpl::DrawArrays,
		"DrawElements", &LuaVAOImpl::DrawElements,

		"ClearSubmission", &LuaVAOImpl::ClearSubmission,
		"AddUnitsToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddUnitsToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddUnitsToSubmission)
		),
		"AddFeaturesToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddFeaturesToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddFeaturesToSubmission)
		),
		"AddUnitDefsToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddUnitDefsToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddUnitDefsToSubmission)
		),
		"AddFeatureDefsToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddFeatureDefsToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddFeatureDefsToSubmission)
		),
		"RemoveFromSubmission", & LuaVAOImpl::RemoveFromSubmission,
		"Submit", &LuaVAOImpl::Submit
	);

	gl.set("VAO", sol::lua_nil); //because :)

	return true;
}

LuaVAOs::~LuaVAOs()
{
	for (auto lva : luaVAOs) {
		if (lva.expired())
			continue; //destroyed already

		lva.lock()->Delete();
	}
	luaVAOs.clear();
}

int LuaVAOs::GetVAO(lua_State* L)
{
	if (!LuaVAOImpl::Supported()) {
		LOG_L(L_ERROR, "[LuaVAOs::%s] Important OpenGL extensions are not supported by the system\n  \tGL_ARB_vertex_buffer_object = %d; GL_ARB_vertex_array_object = %d; GL_ARB_instanced_arrays = %d; GL_ARB_draw_elements_base_vertex = %d; GL_ARB_multi_draw_indirect = %d",
			__func__, (GLEW_ARB_vertex_buffer_object), (GLEW_ARB_vertex_array_object), (GLEW_ARB_instanced_arrays), (GLEW_ARB_draw_elements_base_vertex), (GLEW_ARB_multi_draw_indirect)
		);

		return 0;
	}

	return sol::stack::call_lua(L, 1, [L]() {
		auto& activeVAOs = CLuaHandle::GetActiveVAOs(L);
		return activeVAOs.luaVAOs.emplace_back(std::make_shared<LuaVAOImpl>()).lock();
	});
}
#include "LuaVAO.h"

#include "lib/sol2/sol.hpp"

#include "LuaVAOImpl.h"
#include "LuaVBOImpl.h"

#include "LuaUtils.h"

///////////////////////////////////////////////////////////

bool LuaVAO::PushEntries(lua_State* L)
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
			sol::resolve<void(int)>(&LuaVAOImpl::AddUnitsToSubmission),
			sol::resolve<void(const sol::stack_table&)>(&LuaVAOImpl::AddUnitsToSubmission)
		),
		"AddUnitDefsToSubmission", sol::overload(
			sol::resolve<void(int)>(&LuaVAOImpl::AddUnitDefsToSubmission),
			sol::resolve<void(const sol::stack_table&)>(&LuaVAOImpl::AddUnitDefsToSubmission)
		),
		"Submit", &LuaVAOImpl::Submit
	);

	gl.set("VAO", sol::lua_nil); //because :)

	return true;
}

int LuaVAO::GetVAO(lua_State* L)
{
	if (!LuaVAOImpl::Supported()) {
		LOG_L(L_ERROR, "[LuaVAO::%s] Important OpenGL extensions are not supported by the system\n  GL_ARB_vertex_buffer_object = %d\n GL_ARB_vertex_array_object = %d\n GL_ARB_instanced_arrays = %d\n GL_ARB_draw_elements_base_vertex = %d\n GL_ARB_multi_draw_indirect = %d", \
			__func__, GLEW_ARB_vertex_buffer_object, GLEW_ARB_vertex_array_object, GLEW_ARB_instanced_arrays, GLEW_ARB_draw_elements_base_vertex, GLEW_ARB_multi_draw_indirect);

		return 0;
	}

	return sol::stack::call_lua(L, 1, []() {
		return LuaVAOImpl();
	});
}
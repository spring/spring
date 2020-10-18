#include "LuaVAO.h"

#include "lib/sol2/sol.hpp"

#include "LuaVAOImpl.h"
#include "LuaUtils.h"

///////////////////////////////////////////////////////////

bool LuaVAO::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetVAO);

	sol::state_view lua(L);
	auto gl = sol::stack::get<sol::table>(L, -1);

	gl.new_usertype<LuaVAOImpl>("VAO",
		sol::constructors<LuaVAOImpl(const sol::optional<bool>, sol::this_state)>(),
		"Delete", &LuaVAOImpl::Delete,
		"SetVertexAttributes", &LuaVAOImpl::SetVertexAttributes,
		"SetInstanceAttributes", &LuaVAOImpl::SetInstanceAttributes,
		"SetIndexAttributes", &LuaVAOImpl::SetIndexAttributes,

		"UploadVertexBulk", &LuaVAOImpl::UploadVertexBulk,
		"UploadInstanceBulk", &LuaVAOImpl::UploadInstanceBulk,

		"UploadVertexAttribute", &LuaVAOImpl::UploadVertexAttribute,
		"UploadInstanceAttribute", &LuaVAOImpl::UploadInstanceAttribute,

		"UploadIndices", &LuaVAOImpl::UploadIndices,

		"DrawArrays", &LuaVAOImpl::DrawArrays,
		"DrawElements", &LuaVAOImpl::DrawElements
		);

	gl.set("VAO", sol::lua_nil); //because :)

	return true;
}

int LuaVAO::GetVAO(lua_State* L)
{
	if (!LuaVAOImpl::Supported())
		return 0;

	return sol::stack::call_lua(L, 1, [=](const sol::optional<bool> freqUpdatedOpt) {
		return LuaVAOImpl{freqUpdatedOpt, L};
	});
}
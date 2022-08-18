#include "LuaVBO.h"

#include <unordered_map>
#include <memory>

#include "lib/sol2/sol.hpp"

#include "Rendering/GL/myGL.h"
#include "LuaVBOImpl.h"
#include "LuaHandle.h"
#include "LuaUtils.h"

///////////////////////////////////////////////////////////

bool LuaVBOs::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetVBO);

	sol::state_view lua(L);
	auto gl = sol::stack::get<sol::table>(L, -1);

	gl.new_usertype<LuaVBOImpl>("VBO",
		sol::constructors<LuaVBOImpl(const sol::optional<GLenum>, const sol::optional<bool>)>(),
		"Delete", &LuaVBOImpl::Delete,

		"Define", &LuaVBOImpl::Define,
		"Upload", &LuaVBOImpl::Upload,
		"Download", &LuaVBOImpl::Download,

		"ModelsVBO", &LuaVBOImpl::ModelsVBO,

		"InstanceDataFromUnitDefIDs", sol::overload(
			sol::resolve<size_t(int, int, sol::optional<int>, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromUnitDefIDs),
			sol::resolve<size_t(const sol::stack_table&, int, sol::optional<int>, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromUnitDefIDs)
		),
		"InstanceDataFromFeatureDefIDs", sol::overload(
			sol::resolve<size_t(int, int, sol::optional<int>, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromFeatureDefIDs),
			sol::resolve<size_t(const sol::stack_table&, int, sol::optional<int>, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromFeatureDefIDs)
		),
		"InstanceDataFromUnitIDs", sol::overload(
			sol::resolve<size_t(int, int, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromUnitIDs),
			sol::resolve<size_t(const sol::stack_table&, int, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromUnitIDs)
		),
		"InstanceDataFromFeatureIDs", sol::overload(
			sol::resolve<size_t(int, int, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromFeatureIDs),
			sol::resolve<size_t(const sol::stack_table&, int, sol::optional<int>)>(&LuaVBOImpl::InstanceDataFromFeatureIDs)
		),
		"MatrixDataFromProjectileIDs", sol::overload(
			sol::resolve<size_t(int, int, sol::optional<int>)>(&LuaVBOImpl::MatrixDataFromProjectileIDs),
			sol::resolve<size_t(const sol::stack_table&, int, sol::optional<int>)>(&LuaVBOImpl::MatrixDataFromProjectileIDs)
		),

		"BindBufferRange", &LuaVBOImpl::BindBufferRange,
		"UnbindBufferRange", &LuaVBOImpl::UnbindBufferRange,

		"DumpDefinition", &LuaVBOImpl::DumpDefinition,
		"GetBufferSize", &LuaVBOImpl::GetBufferSize
	);

	gl.set("VBO", sol::lua_nil); //because :)

	return true;
}

bool LuaVBOs::CheckAndReportSupported(lua_State* L, const unsigned int target) {
	#define ValStr(arg) { arg, #arg }
	#define ValStr2(arg1, arg2) { arg1, #arg2 }

	static std::unordered_map<GLenum, std::string> bufferEnumToStr = {
		ValStr(GL_ARRAY_BUFFER),
		ValStr(GL_ELEMENT_ARRAY_BUFFER),
		ValStr(GL_UNIFORM_BUFFER),
		ValStr(GL_SHADER_STORAGE_BUFFER),
	};

	static std::unordered_map<GLenum, std::string> bufferEnumToExtStr = {
		ValStr2(GL_ARRAY_BUFFER, ARB_vertex_buffer_object),
		ValStr2(GL_ELEMENT_ARRAY_BUFFER, ARB_vertex_buffer_object),
		ValStr2(GL_UNIFORM_BUFFER, ARB_uniform_buffer_object),
		ValStr2(GL_SHADER_STORAGE_BUFFER, ARB_shader_storage_buffer_object),
	};

	if (bufferEnumToStr.find(target) == bufferEnumToStr.cend()) {
		LOG_L(L_ERROR, "[LuaVBOs:%s]: Supplied invalid OpenGL buffer type [%u]", __func__, target);
		return false;
	}

	if (!LuaVBOImpl::Supported(target)) {
		LOG_L(L_ERROR, "[LuaVBOs:%s]: important OpenGL extension %s is not supported for buffer type %s", __func__, bufferEnumToExtStr[target].c_str(), bufferEnumToStr[target].c_str());
		return false;
	}

	return true;

	#undef ValStr
	#undef ValStr2
}

LuaVBOs::~LuaVBOs()
{
	for (auto lvb : luaVBOs) {
		if (lvb.expired())
			continue; //destroyed already

		lvb.lock()->Delete();
	}
	luaVBOs.clear();
}

int LuaVBOs::GetVBO(lua_State* L)
{
	unsigned int target = luaL_optint(L, 1, GL_ARRAY_BUFFER);
	if (!LuaVBOs::CheckAndReportSupported(L, target))
		return 0;

	return sol::stack::call_lua(L, 1, [L](const sol::optional<GLenum> defTargetOpt, const sol::optional<bool> freqUpdatedOpt) {
		auto& activeVBOs = CLuaHandle::GetActiveVBOs(L);
		return activeVBOs.luaVBOs.emplace_back(std::make_shared<LuaVBOImpl>(defTargetOpt, freqUpdatedOpt)).lock();
	});
}
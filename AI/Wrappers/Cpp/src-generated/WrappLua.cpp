/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappLua.h"

#include "IncludesSources.h"

	springai::WrappLua::WrappLua(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappLua::~WrappLua() {

	}

	int springai::WrappLua::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappLua::Lua* springai::WrappLua::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Lua* internal_ret = NULL;
		internal_ret = new springai::WrappLua(skirmishAIId);
		return internal_ret;
	}


	std::string springai::WrappLua::CallRules(const char* inData, int inSize) {

		int internal_ret_int;

		char ret_outData[10240];

		internal_ret_int = bridged_Lua_callRules(this->GetSkirmishAIId(), inData, inSize, ret_outData);
	if (internal_ret_int != 0) {
		throw CallbackAIException("callRules", internal_ret_int);
	}
		std::string internal_ret(ret_outData);

		return internal_ret;
	}

	std::string springai::WrappLua::CallUI(const char* inData, int inSize) {

		int internal_ret_int;

		char ret_outData[10240];

		internal_ret_int = bridged_Lua_callUI(this->GetSkirmishAIId(), inData, inSize, ret_outData);
	if (internal_ret_int != 0) {
		throw CallbackAIException("callUI", internal_ret_int);
	}
		std::string internal_ret(ret_outData);

		return internal_ret;
	}

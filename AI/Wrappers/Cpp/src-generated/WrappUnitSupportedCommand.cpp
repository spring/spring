/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappUnitSupportedCommand.h"

#include "IncludesSources.h"

	springai::WrappUnitSupportedCommand::WrappUnitSupportedCommand(int skirmishAIId, int unitId, int supportedCommandId) {

		this->skirmishAIId = skirmishAIId;
		this->unitId = unitId;
		this->supportedCommandId = supportedCommandId;
	}

	springai::WrappUnitSupportedCommand::~WrappUnitSupportedCommand() {

	}

	int springai::WrappUnitSupportedCommand::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappUnitSupportedCommand::GetUnitId() const {

		return unitId;
	}

	int springai::WrappUnitSupportedCommand::GetSupportedCommandId() const {

		return supportedCommandId;
	}

	springai::WrappUnitSupportedCommand::CommandDescription* springai::WrappUnitSupportedCommand::GetInstance(int skirmishAIId, int unitId, int supportedCommandId) {

		if (supportedCommandId < 0) {
			return NULL;
		}

		springai::CommandDescription* internal_ret = NULL;
		internal_ret = new springai::WrappUnitSupportedCommand(skirmishAIId, unitId, supportedCommandId);
		return internal_ret;
	}


	int springai::WrappUnitSupportedCommand::GetId() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_SupportedCommand_getId(this->GetSkirmishAIId(), this->GetUnitId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitSupportedCommand::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Unit_SupportedCommand_getName(this->GetSkirmishAIId(), this->GetUnitId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitSupportedCommand::GetToolTip() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Unit_SupportedCommand_getToolTip(this->GetSkirmishAIId(), this->GetUnitId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	bool springai::WrappUnitSupportedCommand::IsShowUnique() {

		bool internal_ret_int;

		internal_ret_int = bridged_Unit_SupportedCommand_isShowUnique(this->GetSkirmishAIId(), this->GetUnitId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	bool springai::WrappUnitSupportedCommand::IsDisabled() {

		bool internal_ret_int;

		internal_ret_int = bridged_Unit_SupportedCommand_isDisabled(this->GetSkirmishAIId(), this->GetUnitId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	std::vector<const char*> springai::WrappUnitSupportedCommand::GetParams() {

		int params_sizeMax;
		int params_raw_size;
		const char** params;
		std::vector<const char*> params_list;
		int params_size;
		int internal_ret_int;

		params_sizeMax = INT_MAX;
		params = NULL;
		params_size = bridged_Unit_SupportedCommand_getParams(this->GetSkirmishAIId(), this->GetUnitId(), this->GetSupportedCommandId(), params, params_sizeMax);
		params_sizeMax = params_size;
		params_raw_size = params_size;
		params = new const char*[params_raw_size];

		internal_ret_int = bridged_Unit_SupportedCommand_getParams(this->GetSkirmishAIId(), this->GetUnitId(), this->GetSupportedCommandId(), params, params_sizeMax);
		params_list.reserve(params_size);
		for (int i=0; i < params_sizeMax; ++i) {
			params_list.push_back(params[i]);
		}
		delete[] params;

		return params_list;
	}

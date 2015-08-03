/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappCurrentCommand.h"

#include "IncludesSources.h"

	springai::WrappCurrentCommand::WrappCurrentCommand(int skirmishAIId, int unitId, int commandId) {

		this->skirmishAIId = skirmishAIId;
		this->unitId = unitId;
		this->commandId = commandId;
	}

	springai::WrappCurrentCommand::~WrappCurrentCommand() {

	}

	int springai::WrappCurrentCommand::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappCurrentCommand::GetUnitId() const {

		return unitId;
	}

	int springai::WrappCurrentCommand::GetCommandId() const {

		return commandId;
	}

	springai::WrappCurrentCommand::Command* springai::WrappCurrentCommand::GetInstance(int skirmishAIId, int unitId, int commandId) {

		if (commandId < 0) {
			return NULL;
		}

		springai::Command* internal_ret = NULL;
		internal_ret = new springai::WrappCurrentCommand(skirmishAIId, unitId, commandId);
		return internal_ret;
	}


	int springai::WrappCurrentCommand::GetType() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_CurrentCommand_getType(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappCurrentCommand::GetId() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_CurrentCommand_getId(this->GetSkirmishAIId(), this->GetUnitId(), this->GetCommandId());
		return internal_ret_int;
	}

	short springai::WrappCurrentCommand::GetOptions() {

		short internal_ret_int;

		internal_ret_int = bridged_Unit_CurrentCommand_getOptions(this->GetSkirmishAIId(), this->GetUnitId(), this->GetCommandId());
		return internal_ret_int;
	}

	int springai::WrappCurrentCommand::GetTag() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_CurrentCommand_getTag(this->GetSkirmishAIId(), this->GetUnitId(), this->GetCommandId());
		return internal_ret_int;
	}

	int springai::WrappCurrentCommand::GetTimeOut() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_CurrentCommand_getTimeOut(this->GetSkirmishAIId(), this->GetUnitId(), this->GetCommandId());
		return internal_ret_int;
	}

	std::vector<float> springai::WrappCurrentCommand::GetParams() {

		int params_sizeMax;
		int params_raw_size;
		float* params;
		std::vector<float> params_list;
		int params_size;
		int internal_ret_int;

		params_sizeMax = INT_MAX;
		params = NULL;
		params_size = bridged_Unit_CurrentCommand_getParams(this->GetSkirmishAIId(), this->GetUnitId(), this->GetCommandId(), params, params_sizeMax);
		params_sizeMax = params_size;
		params_raw_size = params_size;
		params = new float[params_raw_size];

		internal_ret_int = bridged_Unit_CurrentCommand_getParams(this->GetSkirmishAIId(), this->GetUnitId(), this->GetCommandId(), params, params_sizeMax);
		params_list.reserve(params_size);
		for (int i=0; i < params_sizeMax; ++i) {
			params_list.push_back(params[i]);
		}
		delete[] params;

		return params_list;
	}

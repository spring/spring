/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappGroupSupportedCommand.h"

#include "IncludesSources.h"

	springai::WrappGroupSupportedCommand::WrappGroupSupportedCommand(int skirmishAIId, int groupId, int supportedCommandId) {

		this->skirmishAIId = skirmishAIId;
		this->groupId = groupId;
		this->supportedCommandId = supportedCommandId;
	}

	springai::WrappGroupSupportedCommand::~WrappGroupSupportedCommand() {

	}

	int springai::WrappGroupSupportedCommand::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappGroupSupportedCommand::GetGroupId() const {

		return groupId;
	}

	int springai::WrappGroupSupportedCommand::GetSupportedCommandId() const {

		return supportedCommandId;
	}

	springai::WrappGroupSupportedCommand::CommandDescription* springai::WrappGroupSupportedCommand::GetInstance(int skirmishAIId, int groupId, int supportedCommandId) {

		if (supportedCommandId < 0) {
			return NULL;
		}

		springai::CommandDescription* internal_ret = NULL;
		internal_ret = new springai::WrappGroupSupportedCommand(skirmishAIId, groupId, supportedCommandId);
		return internal_ret;
	}


	int springai::WrappGroupSupportedCommand::GetId() {

		int internal_ret_int;

		internal_ret_int = bridged_Group_SupportedCommand_getId(this->GetSkirmishAIId(), this->GetGroupId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	const char* springai::WrappGroupSupportedCommand::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Group_SupportedCommand_getName(this->GetSkirmishAIId(), this->GetGroupId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	const char* springai::WrappGroupSupportedCommand::GetToolTip() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Group_SupportedCommand_getToolTip(this->GetSkirmishAIId(), this->GetGroupId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	bool springai::WrappGroupSupportedCommand::IsShowUnique() {

		bool internal_ret_int;

		internal_ret_int = bridged_Group_SupportedCommand_isShowUnique(this->GetSkirmishAIId(), this->GetGroupId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	bool springai::WrappGroupSupportedCommand::IsDisabled() {

		bool internal_ret_int;

		internal_ret_int = bridged_Group_SupportedCommand_isDisabled(this->GetSkirmishAIId(), this->GetGroupId(), this->GetSupportedCommandId());
		return internal_ret_int;
	}

	std::vector<const char*> springai::WrappGroupSupportedCommand::GetParams() {

		int params_sizeMax;
		int params_raw_size;
		const char** params;
		std::vector<const char*> params_list;
		int params_size;
		int internal_ret_int;

		params_sizeMax = INT_MAX;
		params = NULL;
		params_size = bridged_Group_SupportedCommand_getParams(this->GetSkirmishAIId(), this->GetGroupId(), this->GetSupportedCommandId(), params, params_sizeMax);
		params_sizeMax = params_size;
		params_raw_size = params_size;
		params = new const char*[params_raw_size];

		internal_ret_int = bridged_Group_SupportedCommand_getParams(this->GetSkirmishAIId(), this->GetGroupId(), this->GetSupportedCommandId(), params, params_sizeMax);
		params_list.reserve(params_size);
		for (int i=0; i < params_sizeMax; ++i) {
			params_list.push_back(params[i]);
		}
		delete[] params;

		return params_list;
	}

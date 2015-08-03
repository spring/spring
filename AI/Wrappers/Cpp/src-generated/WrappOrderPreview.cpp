/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappOrderPreview.h"

#include "IncludesSources.h"

	springai::WrappOrderPreview::WrappOrderPreview(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappOrderPreview::~WrappOrderPreview() {

	}

	int springai::WrappOrderPreview::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappOrderPreview::OrderPreview* springai::WrappOrderPreview::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::OrderPreview* internal_ret = NULL;
		internal_ret = new springai::WrappOrderPreview(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappOrderPreview::GetId(int groupId) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_OrderPreview_getId(this->GetSkirmishAIId(), groupId);
		return internal_ret_int;
	}

	short springai::WrappOrderPreview::GetOptions(int groupId) {

		short internal_ret_int;

		internal_ret_int = bridged_Group_OrderPreview_getOptions(this->GetSkirmishAIId(), groupId);
		return internal_ret_int;
	}

	int springai::WrappOrderPreview::GetTag(int groupId) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_OrderPreview_getTag(this->GetSkirmishAIId(), groupId);
		return internal_ret_int;
	}

	int springai::WrappOrderPreview::GetTimeOut(int groupId) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_OrderPreview_getTimeOut(this->GetSkirmishAIId(), groupId);
		return internal_ret_int;
	}

	std::vector<float> springai::WrappOrderPreview::GetParams(int groupId) {

		int params_sizeMax;
		int params_raw_size;
		float* params;
		std::vector<float> params_list;
		int params_size;
		int internal_ret_int;

		params_sizeMax = INT_MAX;
		params = NULL;
		params_size = bridged_Group_OrderPreview_getParams(this->GetSkirmishAIId(), groupId, params, params_sizeMax);
		params_sizeMax = params_size;
		params_raw_size = params_size;
		params = new float[params_raw_size];

		internal_ret_int = bridged_Group_OrderPreview_getParams(this->GetSkirmishAIId(), groupId, params, params_sizeMax);
		params_list.reserve(params_size);
		for (int i=0; i < params_sizeMax; ++i) {
			params_list.push_back(params[i]);
		}
		delete[] params;

		return params_list;
	}

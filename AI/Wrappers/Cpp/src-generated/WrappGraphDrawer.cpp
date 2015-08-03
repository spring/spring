/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappGraphDrawer.h"

#include "IncludesSources.h"

	springai::WrappGraphDrawer::WrappGraphDrawer(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappGraphDrawer::~WrappGraphDrawer() {

	}

	int springai::WrappGraphDrawer::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappGraphDrawer::GraphDrawer* springai::WrappGraphDrawer::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::GraphDrawer* internal_ret = NULL;
		internal_ret = new springai::WrappGraphDrawer(skirmishAIId);
		return internal_ret;
	}


	bool springai::WrappGraphDrawer::IsEnabled() {

		bool internal_ret_int;

		internal_ret_int = bridged_Debug_GraphDrawer_isEnabled(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	void springai::WrappGraphDrawer::SetPosition(float x, float y) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_GraphDrawer_setPosition(this->GetSkirmishAIId(), x, y);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setPosition", internal_ret_int);
	}

	}

	void springai::WrappGraphDrawer::SetSize(float w, float h) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_GraphDrawer_setSize(this->GetSkirmishAIId(), w, h);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setSize", internal_ret_int);
	}

	}

	springai::GraphLine* springai::WrappGraphDrawer::GetGraphLine() {

		GraphLine* internal_ret_int_out;

		internal_ret_int_out = springai::WrappGraphLine::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

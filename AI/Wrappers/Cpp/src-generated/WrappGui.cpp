/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappGui.h"

#include "IncludesSources.h"

	springai::WrappGui::WrappGui(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappGui::~WrappGui() {

	}

	int springai::WrappGui::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappGui::Gui* springai::WrappGui::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Gui* internal_ret = NULL;
		internal_ret = new springai::WrappGui(skirmishAIId);
		return internal_ret;
	}


	float springai::WrappGui::GetViewRange() {

		float internal_ret_int;

		internal_ret_int = bridged_Gui_getViewRange(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappGui::GetScreenX() {

		float internal_ret_int;

		internal_ret_int = bridged_Gui_getScreenX(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappGui::GetScreenY() {

		float internal_ret_int;

		internal_ret_int = bridged_Gui_getScreenY(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	springai::Camera* springai::WrappGui::GetCamera() {

		Camera* internal_ret_int_out;

		internal_ret_int_out = springai::WrappCamera::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

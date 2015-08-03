/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappCamera.h"

#include "IncludesSources.h"

	springai::WrappCamera::WrappCamera(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappCamera::~WrappCamera() {

	}

	int springai::WrappCamera::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappCamera::Camera* springai::WrappCamera::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Camera* internal_ret = NULL;
		internal_ret = new springai::WrappCamera(skirmishAIId);
		return internal_ret;
	}


	springai::AIFloat3 springai::WrappCamera::GetDirection() {

		float return_posF3_out[3];

		bridged_Gui_Camera_getDirection(this->GetSkirmishAIId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	springai::AIFloat3 springai::WrappCamera::GetPosition() {

		float return_posF3_out[3];

		bridged_Gui_Camera_getPosition(this->GetSkirmishAIId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

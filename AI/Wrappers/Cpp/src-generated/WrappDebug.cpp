/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappDebug.h"

#include "IncludesSources.h"

	springai::WrappDebug::WrappDebug(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappDebug::~WrappDebug() {

	}

	int springai::WrappDebug::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappDebug::Debug* springai::WrappDebug::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Debug* internal_ret = NULL;
		internal_ret = new springai::WrappDebug(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappDebug::AddOverlayTexture(const float* texData, int w, int h) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_addOverlayTexture(this->GetSkirmishAIId(), texData, w, h);
		return internal_ret_int;
	}

	springai::GraphDrawer* springai::WrappDebug::GetGraphDrawer() {

		GraphDrawer* internal_ret_int_out;

		internal_ret_int_out = springai::WrappGraphDrawer::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

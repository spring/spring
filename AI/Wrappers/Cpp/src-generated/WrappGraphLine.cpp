/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappGraphLine.h"

#include "IncludesSources.h"

	springai::WrappGraphLine::WrappGraphLine(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappGraphLine::~WrappGraphLine() {

	}

	int springai::WrappGraphLine::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappGraphLine::GraphLine* springai::WrappGraphLine::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::GraphLine* internal_ret = NULL;
		internal_ret = new springai::WrappGraphLine(skirmishAIId);
		return internal_ret;
	}


	void springai::WrappGraphLine::AddPoint(int lineId, float x, float y) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_GraphDrawer_GraphLine_addPoint(this->GetSkirmishAIId(), lineId, x, y);
	if (internal_ret_int != 0) {
		throw CallbackAIException("addPoint", internal_ret_int);
	}

	}

	void springai::WrappGraphLine::DeletePoints(int lineId, int numPoints) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_GraphDrawer_GraphLine_deletePoints(this->GetSkirmishAIId(), lineId, numPoints);
	if (internal_ret_int != 0) {
		throw CallbackAIException("deletePoints", internal_ret_int);
	}

	}

	void springai::WrappGraphLine::SetColor(int lineId, const springai::AIColor& color) {

		int internal_ret_int;

		short color_colorS3[3];
		color.LoadInto3(color_colorS3);

		internal_ret_int = bridged_Debug_GraphDrawer_GraphLine_setColor(this->GetSkirmishAIId(), lineId, color_colorS3);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setColor", internal_ret_int);
	}

	}

	void springai::WrappGraphLine::SetLabel(int lineId, const char* label) {

		int internal_ret_int;

		internal_ret_int = bridged_Debug_GraphDrawer_GraphLine_setLabel(this->GetSkirmishAIId(), lineId, label);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setLabel", internal_ret_int);
	}

	}

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappFigure.h"

#include "IncludesSources.h"

	springai::WrappFigure::WrappFigure(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappFigure::~WrappFigure() {

	}

	int springai::WrappFigure::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappFigure::Figure* springai::WrappFigure::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Figure* internal_ret = NULL;
		internal_ret = new springai::WrappFigure(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappFigure::DrawSpline(const springai::AIFloat3& pos1, const springai::AIFloat3& pos2, const springai::AIFloat3& pos3, const springai::AIFloat3& pos4, float width, bool arrow, int lifeTime, int figureGroupId) {

		int internal_ret_int;

		float pos1_posF3[3];
		pos1.LoadInto(pos1_posF3);
		float pos2_posF3[3];
		pos2.LoadInto(pos2_posF3);
		float pos3_posF3[3];
		pos3.LoadInto(pos3_posF3);
		float pos4_posF3[3];
		pos4.LoadInto(pos4_posF3);

		internal_ret_int = bridged_Map_Drawer_Figure_drawSpline(this->GetSkirmishAIId(), pos1_posF3, pos2_posF3, pos3_posF3, pos4_posF3, width, arrow, lifeTime, figureGroupId);
		return internal_ret_int;
	}

	int springai::WrappFigure::DrawLine(const springai::AIFloat3& pos1, const springai::AIFloat3& pos2, float width, bool arrow, int lifeTime, int figureGroupId) {

		int internal_ret_int;

		float pos1_posF3[3];
		pos1.LoadInto(pos1_posF3);
		float pos2_posF3[3];
		pos2.LoadInto(pos2_posF3);

		internal_ret_int = bridged_Map_Drawer_Figure_drawLine(this->GetSkirmishAIId(), pos1_posF3, pos2_posF3, width, arrow, lifeTime, figureGroupId);
		return internal_ret_int;
	}

	void springai::WrappFigure::SetColor(int figureGroupId, const springai::AIColor& color, short alpha) {

		int internal_ret_int;

		short color_colorS3[3];
		color.LoadInto3(color_colorS3);

		internal_ret_int = bridged_Map_Drawer_Figure_setColor(this->GetSkirmishAIId(), figureGroupId, color_colorS3, alpha);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setColor", internal_ret_int);
	}

	}

	void springai::WrappFigure::Remove(int figureGroupId) {

		int internal_ret_int;

		internal_ret_int = bridged_Map_Drawer_Figure_remove(this->GetSkirmishAIId(), figureGroupId);
	if (internal_ret_int != 0) {
		throw CallbackAIException("remove", internal_ret_int);
	}

	}

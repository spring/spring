/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappDrawer.h"

#include "IncludesSources.h"

	springai::WrappDrawer::WrappDrawer(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappDrawer::~WrappDrawer() {

	}

	int springai::WrappDrawer::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappDrawer::Drawer* springai::WrappDrawer::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Drawer* internal_ret = NULL;
		internal_ret = new springai::WrappDrawer(skirmishAIId);
		return internal_ret;
	}


	void springai::WrappDrawer::AddNotification(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		short color_colorS3[3];
		color.LoadInto3(color_colorS3);

		internal_ret_int = bridged_Map_Drawer_addNotification(this->GetSkirmishAIId(), pos_posF3, color_colorS3, alpha);
	if (internal_ret_int != 0) {
		throw CallbackAIException("addNotification", internal_ret_int);
	}

	}

	void springai::WrappDrawer::AddPoint(const springai::AIFloat3& pos, const char* label) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Map_Drawer_addPoint(this->GetSkirmishAIId(), pos_posF3, label);
	if (internal_ret_int != 0) {
		throw CallbackAIException("addPoint", internal_ret_int);
	}

	}

	void springai::WrappDrawer::DeletePointsAndLines(const springai::AIFloat3& pos) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Map_Drawer_deletePointsAndLines(this->GetSkirmishAIId(), pos_posF3);
	if (internal_ret_int != 0) {
		throw CallbackAIException("deletePointsAndLines", internal_ret_int);
	}

	}

	void springai::WrappDrawer::AddLine(const springai::AIFloat3& posFrom, const springai::AIFloat3& posTo) {

		int internal_ret_int;

		float posFrom_posF3[3];
		posFrom.LoadInto(posFrom_posF3);
		float posTo_posF3[3];
		posTo.LoadInto(posTo_posF3);

		internal_ret_int = bridged_Map_Drawer_addLine(this->GetSkirmishAIId(), posFrom_posF3, posTo_posF3);
	if (internal_ret_int != 0) {
		throw CallbackAIException("addLine", internal_ret_int);
	}

	}

	void springai::WrappDrawer::DrawUnit(UnitDef* toDrawUnitDef, const springai::AIFloat3& pos, float rotation, int lifeTime, int teamId, bool transparent, bool drawBorder, int facing) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		int toDrawUnitDefId = toDrawUnitDef->GetUnitDefId();

		internal_ret_int = bridged_Map_Drawer_drawUnit(this->GetSkirmishAIId(), toDrawUnitDefId, pos_posF3, rotation, lifeTime, teamId, transparent, drawBorder, facing);
	if (internal_ret_int != 0) {
		throw CallbackAIException("drawUnit", internal_ret_int);
	}

	}

	int springai::WrappDrawer::TraceRay(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags) {

		int internal_ret_int;

		float rayPos_posF3[3];
		rayPos.LoadInto(rayPos_posF3);
		float rayDir_posF3[3];
		rayDir.LoadInto(rayDir_posF3);
		int srcUnitId = srcUnit->GetUnitId();

		internal_ret_int = bridged_Map_Drawer_traceRay(this->GetSkirmishAIId(), rayPos_posF3, rayDir_posF3, rayLen, srcUnitId, flags);
		return internal_ret_int;
	}

	int springai::WrappDrawer::TraceRayFeature(const springai::AIFloat3& rayPos, const springai::AIFloat3& rayDir, float rayLen, Unit* srcUnit, int flags) {

		int internal_ret_int;

		float rayPos_posF3[3];
		rayPos.LoadInto(rayPos_posF3);
		float rayDir_posF3[3];
		rayDir.LoadInto(rayDir_posF3);
		int srcUnitId = srcUnit->GetUnitId();

		internal_ret_int = bridged_Map_Drawer_traceRayFeature(this->GetSkirmishAIId(), rayPos_posF3, rayDir_posF3, rayLen, srcUnitId, flags);
		return internal_ret_int;
	}

	springai::Figure* springai::WrappDrawer::GetFigure() {

		Figure* internal_ret_int_out;

		internal_ret_int_out = springai::WrappFigure::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

	springai::PathDrawer* springai::WrappDrawer::GetPathDrawer() {

		PathDrawer* internal_ret_int_out;

		internal_ret_int_out = springai::WrappPathDrawer::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}

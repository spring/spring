/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappPathDrawer.h"

#include "IncludesSources.h"

	springai::WrappPathDrawer::WrappPathDrawer(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappPathDrawer::~WrappPathDrawer() {

	}

	int springai::WrappPathDrawer::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappPathDrawer::PathDrawer* springai::WrappPathDrawer::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::PathDrawer* internal_ret = NULL;
		internal_ret = new springai::WrappPathDrawer(skirmishAIId);
		return internal_ret;
	}


	void springai::WrappPathDrawer::Start(const springai::AIFloat3& pos, const springai::AIColor& color, short alpha) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		short color_colorS3[3];
		color.LoadInto3(color_colorS3);

		internal_ret_int = bridged_Map_Drawer_PathDrawer_start(this->GetSkirmishAIId(), pos_posF3, color_colorS3, alpha);
	if (internal_ret_int != 0) {
		throw CallbackAIException("start", internal_ret_int);
	}

	}

	void springai::WrappPathDrawer::Finish(bool iAmUseless) {

		int internal_ret_int;

		internal_ret_int = bridged_Map_Drawer_PathDrawer_finish(this->GetSkirmishAIId(), iAmUseless);
	if (internal_ret_int != 0) {
		throw CallbackAIException("finish", internal_ret_int);
	}

	}

	void springai::WrappPathDrawer::DrawLine(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) {

		int internal_ret_int;

		float endPos_posF3[3];
		endPos.LoadInto(endPos_posF3);
		short color_colorS3[3];
		color.LoadInto3(color_colorS3);

		internal_ret_int = bridged_Map_Drawer_PathDrawer_drawLine(this->GetSkirmishAIId(), endPos_posF3, color_colorS3, alpha);
	if (internal_ret_int != 0) {
		throw CallbackAIException("drawLine", internal_ret_int);
	}

	}

	void springai::WrappPathDrawer::DrawLineAndCommandIcon(Command* cmd, const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) {

		int internal_ret_int;

		float endPos_posF3[3];
		endPos.LoadInto(endPos_posF3);
		short color_colorS3[3];
		color.LoadInto3(color_colorS3);
		int cmdId = cmd->GetCommandId();

		internal_ret_int = bridged_Map_Drawer_PathDrawer_drawLineAndCommandIcon(this->GetSkirmishAIId(), cmdId, endPos_posF3, color_colorS3, alpha);
	if (internal_ret_int != 0) {
		throw CallbackAIException("drawLineAndCommandIcon", internal_ret_int);
	}

	}

	void springai::WrappPathDrawer::DrawIcon(Command* cmd) {

		int internal_ret_int;

		int cmdId = cmd->GetCommandId();

		internal_ret_int = bridged_Map_Drawer_PathDrawer_drawIcon(this->GetSkirmishAIId(), cmdId);
	if (internal_ret_int != 0) {
		throw CallbackAIException("drawIcon", internal_ret_int);
	}

	}

	void springai::WrappPathDrawer::Suspend(const springai::AIFloat3& endPos, const springai::AIColor& color, short alpha) {

		int internal_ret_int;

		float endPos_posF3[3];
		endPos.LoadInto(endPos_posF3);
		short color_colorS3[3];
		color.LoadInto3(color_colorS3);

		internal_ret_int = bridged_Map_Drawer_PathDrawer_suspend(this->GetSkirmishAIId(), endPos_posF3, color_colorS3, alpha);
	if (internal_ret_int != 0) {
		throw CallbackAIException("suspend", internal_ret_int);
	}

	}

	void springai::WrappPathDrawer::Restart(bool sameColor) {

		int internal_ret_int;

		internal_ret_int = bridged_Map_Drawer_PathDrawer_restart(this->GetSkirmishAIId(), sameColor);
	if (internal_ret_int != 0) {
		throw CallbackAIException("restart", internal_ret_int);
	}

	}

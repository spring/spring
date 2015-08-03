/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappLine.h"

#include "IncludesSources.h"

	springai::WrappLine::WrappLine(int skirmishAIId, int lineId) {

		this->skirmishAIId = skirmishAIId;
		this->lineId = lineId;
	}

	springai::WrappLine::~WrappLine() {

	}

	int springai::WrappLine::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappLine::GetLineId() const {

		return lineId;
	}

	springai::WrappLine::Line* springai::WrappLine::GetInstance(int skirmishAIId, int lineId) {

		if (lineId < 0) {
			return NULL;
		}

		springai::Line* internal_ret = NULL;
		internal_ret = new springai::WrappLine(skirmishAIId, lineId);
		return internal_ret;
	}


	springai::AIFloat3 springai::WrappLine::GetFirstPosition() {

		float return_posF3_out[3];

		bridged_Map_Line_getFirstPosition(this->GetSkirmishAIId(), this->GetLineId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	springai::AIFloat3 springai::WrappLine::GetSecondPosition() {

		float return_posF3_out[3];

		bridged_Map_Line_getSecondPosition(this->GetSkirmishAIId(), this->GetLineId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	springai::AIColor springai::WrappLine::GetColor() {

		short return_colorS3_out[3];

		bridged_Map_Line_getColor(this->GetSkirmishAIId(), this->GetLineId(), return_colorS3_out);
		springai::AIColor internal_ret((unsigned char) return_colorS3_out[0], (unsigned char) return_colorS3_out[1], (unsigned char) return_colorS3_out[2]);

		return internal_ret;
	}

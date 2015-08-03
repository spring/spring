/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappPoint.h"

#include "IncludesSources.h"

	springai::WrappPoint::WrappPoint(int skirmishAIId, int pointId) {

		this->skirmishAIId = skirmishAIId;
		this->pointId = pointId;
	}

	springai::WrappPoint::~WrappPoint() {

	}

	int springai::WrappPoint::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappPoint::GetPointId() const {

		return pointId;
	}

	springai::WrappPoint::Point* springai::WrappPoint::GetInstance(int skirmishAIId, int pointId) {

		if (pointId < 0) {
			return NULL;
		}

		springai::Point* internal_ret = NULL;
		internal_ret = new springai::WrappPoint(skirmishAIId, pointId);
		return internal_ret;
	}


	springai::AIFloat3 springai::WrappPoint::GetPosition() {

		float return_posF3_out[3];

		bridged_Map_Point_getPosition(this->GetSkirmishAIId(), this->GetPointId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	springai::AIColor springai::WrappPoint::GetColor() {

		short return_colorS3_out[3];

		bridged_Map_Point_getColor(this->GetSkirmishAIId(), this->GetPointId(), return_colorS3_out);
		springai::AIColor internal_ret((unsigned char) return_colorS3_out[0], (unsigned char) return_colorS3_out[1], (unsigned char) return_colorS3_out[2]);

		return internal_ret;
	}

	const char* springai::WrappPoint::GetLabel() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Map_Point_getLabel(this->GetSkirmishAIId(), this->GetPointId());
		return internal_ret_int;
	}

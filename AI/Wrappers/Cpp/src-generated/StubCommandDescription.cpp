/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubCommandDescription.h"

#include "IncludesSources.h"

	springai::StubCommandDescription::~StubCommandDescription() {}
	void springai::StubCommandDescription::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubCommandDescription::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubCommandDescription::SetSupportedCommandId(int supportedCommandId) {
		this->supportedCommandId = supportedCommandId;
	}
	int springai::StubCommandDescription::GetSupportedCommandId() const {
		return supportedCommandId;
	}

	void springai::StubCommandDescription::SetId(int id) {
		this->id = id;
	}
	int springai::StubCommandDescription::GetId() {
		return id;
	}

	void springai::StubCommandDescription::SetName(const char* name) {
		this->name = name;
	}
	const char* springai::StubCommandDescription::GetName() {
		return name;
	}

	void springai::StubCommandDescription::SetToolTip(const char* toolTip) {
		this->toolTip = toolTip;
	}
	const char* springai::StubCommandDescription::GetToolTip() {
		return toolTip;
	}

	void springai::StubCommandDescription::SetShowUnique(bool isShowUnique) {
		this->isShowUnique = isShowUnique;
	}
	bool springai::StubCommandDescription::IsShowUnique() {
		return isShowUnique;
	}

	void springai::StubCommandDescription::SetDisabled(bool isDisabled) {
		this->isDisabled = isDisabled;
	}
	bool springai::StubCommandDescription::IsDisabled() {
		return isDisabled;
	}

	void springai::StubCommandDescription::SetParams(std::vector<const char*> params) {
		this->params = params;
	}
	std::vector<const char*> springai::StubCommandDescription::GetParams() {
		return params;
	}


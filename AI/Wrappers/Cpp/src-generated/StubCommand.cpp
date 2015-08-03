/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubCommand.h"

#include "IncludesSources.h"

	springai::StubCommand::~StubCommand() {}
	void springai::StubCommand::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubCommand::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubCommand::SetCommandId(int commandId) {
		this->commandId = commandId;
	}
	int springai::StubCommand::GetCommandId() const {
		return commandId;
	}

	void springai::StubCommand::SetType(int type) {
		this->type = type;
	}
	int springai::StubCommand::GetType() {
		return type;
	}

	void springai::StubCommand::SetId(int id) {
		this->id = id;
	}
	int springai::StubCommand::GetId() {
		return id;
	}

	void springai::StubCommand::SetOptions(short options) {
		this->options = options;
	}
	short springai::StubCommand::GetOptions() {
		return options;
	}

	void springai::StubCommand::SetTag(int tag) {
		this->tag = tag;
	}
	int springai::StubCommand::GetTag() {
		return tag;
	}

	void springai::StubCommand::SetTimeOut(int timeOut) {
		this->timeOut = timeOut;
	}
	int springai::StubCommand::GetTimeOut() {
		return timeOut;
	}

	void springai::StubCommand::SetParams(std::vector<float> params) {
		this->params = params;
	}
	std::vector<float> springai::StubCommand::GetParams() {
		return params;
	}


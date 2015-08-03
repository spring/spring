/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappEngine.h"

#include "IncludesSources.h"

	springai::WrappEngine::WrappEngine() {

	}

	springai::WrappEngine::~WrappEngine() {

	}

	springai::WrappEngine::Engine* springai::WrappEngine::GetInstance() {

		springai::Engine* internal_ret = NULL;
		internal_ret = new springai::WrappEngine();
		return internal_ret;
	}


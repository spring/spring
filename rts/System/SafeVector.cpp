/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SafeVector.h"

#ifdef USE_SAFE_VECTOR
#include "LogOutput.h"
#include "Platform/CrashHandler.h"

CR_BIND_TEMPLATE(safe_vector<float>, );

template <> const float& safe_vector<float>::safe_element(size_t idx) const {
	static const float def = 0.0f;

	if (showError) {
		showError = false;
		logOutput.Print("[%s const] ERROR: index %u out of bounds! (size %u)", __FUNCTION__, idx, size());
		CrashHandler::OutputStacktrace();
	}

	return def;
}

template <> float& safe_vector<float>::safe_element(size_t idx) {
	static float def = 0.0f;

	if (showError) {
		showError = false;
		logOutput.Print("[%s] ERROR: index %u out of bounds! (size %u)", __FUNCTION__, idx, size());
		CrashHandler::OutputStacktrace();
	}

	return def;
}

#endif

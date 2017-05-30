/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/SafeVector.h"

#ifdef USE_SAFE_VECTOR
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/MainDefines.h"

template <> const float& safe_vector<float>::safe_element(size_type idx) const {
	static const float def = 0.0f;

	if (showError) {
		showError = false;
		LOG_L(L_ERROR, "[%s const] index " _STPF_ " out of bounds! (size " _STPF_ ")", __FUNCTION__, idx, size());
#ifndef UNITSYNC
		CrashHandler::OutputStacktrace();
#endif
	}

	return def;
}

template <> float& safe_vector<float>::safe_element(size_type idx) {
	static float def = 0.0f;

	if (showError) {
		showError = false;
		LOG_L(L_ERROR, "[%s] index " _STPF_ " out of bounds! (size " _STPF_ ")", __FUNCTION__, idx, size());
#ifndef UNITSYNC
		CrashHandler::OutputStacktrace();
#endif
	}

	return def;
}

#endif // USE_SAFE_VECTOR

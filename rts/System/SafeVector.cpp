/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/SafeVector.h"

#ifdef USE_SAFE_VECTOR
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"
#include "System/maindefines.h"

CR_BIND_TEMPLATE(safe_vector<float>, );

template <> const float& safe_vector<float>::safe_element(size_t idx) const {
	static const float def = 0.0f;

	if (showError) {
		showError = false;
		LOG_L(L_ERROR, "[%s const] index "_STPF_" out of bounds! (size "_STPF_")", __FUNCTION__, idx, size());
		CrashHandler::OutputStacktrace();
	}

	return def;
}

template <> float& safe_vector<float>::safe_element(size_t idx) {
	static float def = 0.0f;

	if (showError) {
		showError = false;
		LOG_L(L_ERROR, "[%s] index "_STPF_" out of bounds! (size "_STPF_")", __FUNCTION__, idx, size());
		CrashHandler::OutputStacktrace();
	}

	return def;
}

#endif // USE_SAFE_VECTOR

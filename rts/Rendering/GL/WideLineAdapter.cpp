/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WideLineAdapter.hpp"

std::vector<float>* GL::GetWideLineBuffer() {
	static std::vector<float> vb(sizeof(VA_TYPE_C) / sizeof(float) * 1024 * 64, 0.0f);
	return &vb;
}

GL::WideLineAdapterC* GL::GetWideLineAdapterC() {
	static WideLineAdapterC wlaC;
	return &wlaC;
}

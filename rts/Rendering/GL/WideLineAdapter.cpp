/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WideLineAdapter.hpp"
#include <vector>

std::vector<float>* GL::GetWideLineBuffer() {
	static std::vector<float> vb(65536, 0.0f);
	return &vb;
}

GL::WideLineAdapterC* GL::GetWideLineAdapterC() {
	static WideLineAdapterC wlaC;
	return &wlaC;
}

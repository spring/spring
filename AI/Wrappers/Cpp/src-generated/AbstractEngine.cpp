/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractEngine.h"

#include "IncludesSources.h"

	springai::AbstractEngine::~AbstractEngine() {}
	int springai::AbstractEngine::CompareTo(const Engine& other) {
		static const int EQUAL  =  0;

		if ((Engine*)this == &other) return EQUAL;

		return EQUAL;
	}

bool springai::AbstractEngine::Equals(const Engine& other) {

	return true;
}

int springai::AbstractEngine::HashCode() {

	int internal_ret = 23;


	return internal_ret;
}

std::string springai::AbstractEngine::ToString() {

	std::string internal_ret = "Engine";

	internal_ret = internal_ret + ")";

	return internal_ret;
}


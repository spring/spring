/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlyingPiece.hpp"
#include "Rendering/UnitModels/s3oParser.h"

FlyingPiece::~FlyingPiece() {
	if (verts != NULL) {
		delete[] verts;
	}
}

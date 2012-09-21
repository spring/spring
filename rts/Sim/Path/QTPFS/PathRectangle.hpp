/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_RECTANGLE_HDR
#define QTPFS_RECTANGLE_HDR

#include "System/Rectangle.h"

namespace QTPFS {
	struct PathRectangle: public SRectangle {
		PathRectangle() {
			x1 = 0; x2 = 0;
			y1 = 0; y2 = 0;
			ym = false;
		}
		PathRectangle(int _x1, int _y1, int _x2, int _y2, bool _ym) {
			x1 = _x1; x2 = _x2;
			y1 = _y1; y2 = _y2;
			ym = _ym;
		}

		PathRectangle(const    SRectangle& r) { *this = r; }
		PathRectangle(const PathRectangle& r) { *this = r; }

		PathRectangle& operator = (const SRectangle& r) {
			x1 = r.x1; x2 = r.x2;
			y1 = r.y1; y2 = r.y2;
			ym = false;
			return *this;
		}
		PathRectangle& operator = (const PathRectangle& r) {
			x1 = r.x1; x2 = r.x2;
			y1 = r.y1; y2 = r.y2;
			ym = r.ym;
			return *this;
		}

		// if true, this rectangle covers the yardmap of
		// a CSolidObject* and needs to be tesselated to
		// maximum depth
		bool ym;
	};
};

#endif

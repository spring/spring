/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GL_MARKERS_H
#define _GL_MARKERS_H

#define SCOPED_GMARKER(name) CGraphicsMarker myScopedGraphicsMarker(name)

class CGraphicsMarker
{
public:
	CGraphicsMarker(const char* name) {
		Begin(name);
	}

	~CGraphicsMarker() {
		End();
	}

	static void Begin(const char* name);
	static void End();
	static void Event(const char* name);
};

#endif // _GL_STATE_CHECKER_H

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "glMarkers.h"
#include "myGL.h"

void CGraphicsMarker::Begin(const char* name) {
	if(glPushGroupMarkerEXT && glPopGroupMarkerEXT)
		glPushGroupMarkerEXT(0, name);
}

void CGraphicsMarker::End() {
	if (glPushGroupMarkerEXT && glPopGroupMarkerEXT)
		glPopGroupMarkerEXT();
}

void CGraphicsMarker::Event(const char* name) {
	if(glInsertEventMarkerEXT)
		glInsertEventMarkerEXT(0, name);
}

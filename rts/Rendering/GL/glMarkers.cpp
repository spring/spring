/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "glMarkers.h"
#include "myGL.h"

void CGraphicsMarker::Begin(const char* name) {
#if defined(glPushGroupMarkerEXT) && defined(glPopGroupMarkerEXT)
	if(glPushGroupMarkerEXT && glPopGroupMarkerEXT)
		glPushGroupMarkerEXT(0, name);
#endif
}

void CGraphicsMarker::End() {
#if defined(glPushGroupMarkerEXT) && defined(glPopGroupMarkerEXT)
	if (glPushGroupMarkerEXT && glPopGroupMarkerEXT)
		glPopGroupMarkerEXT();
#endif
}

void CGraphicsMarker::Event(const char* name) {
#if defined(glInsertEventMarkerEXT)
	if(glInsertEventMarkerEXT)
		glInsertEventMarkerEXT(0, name);
#endif
}

#include "StdAfx.h"
#include "glExtra.h"
#include "Sim/Map/Ground.h"
//#include "mmgr.h"


/*
Draws a trigonometric circle in 'resolution' steps.
*/
void glSurfaceCircle(const float3& center, float radius, unsigned int resolution) {
	glBegin(GL_LINE_STRIP);
	for(int i = 0; i <= resolution; ++i) {
		float3 pos = float3(center.x + sin(i*2*PI / resolution)*radius, 0, center.z + cos(i*2*PI / resolution)*radius);
		pos.y = ground->GetHeight(pos.x,pos.z) + 5;
		glVertexf3(pos);
	}
	glEnd();
}


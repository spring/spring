#include "StdAfx.h"
#include "glExtra.h"
#include "Map/Ground.h"
#include "mmgr.h"


/*
 *  Draws a trigonometric circle in 'resolution' steps.
 */
static void defSurfaceCircle(const float3& center, float radius,
                             unsigned int resolution)
{
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < resolution; ++i) {
		const float radians = (2.0f * PI) * (float)i / (float)resolution;
		float3 pos;
		pos.x = center.x + (sin(radians) * radius);
		pos.z = center.z + (cos(radians) * radius);
		pos.y = ground->GetHeight(pos.x, pos.z) + 5.0f;
		glVertexf3(pos);
	}
	glEnd();
}

SurfaceCircleFunc glSurfaceCircle = defSurfaceCircle;

void setSurfaceCircleFunc(SurfaceCircleFunc func)
{
	if (func == NULL) {
		glSurfaceCircle = defSurfaceCircle;
	} else {
		glSurfaceCircle = func;
	} 
}


/*
 *  Draws a trigonometric circle in 'resolution' steps, with a slope modifier
 */
void glBallisticCircle(const float3& center, float radius, float slope,
                       unsigned int resolution)
{
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < resolution; ++i) {
		const float radians = (2.0f * PI) * (float)i / (float)resolution;
		float3 pos;
		pos.x = center.x + (sin(radians) * radius);
		pos.z = center.z + (cos(radians) * radius);
		pos.y = ground->GetHeight(pos.x, pos.z);

		const float heightDiff = (pos.y - center.y);
		const float adjRadius = radius - (heightDiff * slope);		
		pos.x = center.x + (sin(radians) * adjRadius);
		pos.z = center.z + (cos(radians) * adjRadius);
		pos.y = ground->GetHeight(pos.x, pos.z) + 5.0f;
		glVertexf3(pos);
	}
	glEnd();
}



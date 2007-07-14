#include "StdAfx.h"
#include "glExtra.h"
#include "Map/Ground.h"
#include "mmgr.h"
#include "Sim/Weapons/Weapon.h"
#include "LogOutput.h"


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
void glBallisticCircle(const float3& center, const float radius,
                       const CWeapon* weapon,
                       unsigned int resolution, float slope)
{
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < resolution; ++i) {
		const float radians = (2.0f * PI) * (float)i / (float)resolution;
		float rad = radius;
		float3 pos;
		float sinR = sin(radians);
		float cosR = cos(radians);
		pos.x = center.x + (sinR * rad);
		pos.z = center.z + (cosR * rad);
		pos.y = ground->GetHeight(pos.x, pos.z);
		float heightDiff = (pos.y - center.y)/2;
		rad -= heightDiff * slope;
		float adjRadius = weapon ? weapon->GetRange2D(heightDiff) : rad;
		float adjustment = rad/2;
		float ydiff = 0;
		int j;
		for(j = 0; j < 50 && fabs(adjRadius - rad) + ydiff > .01*rad; j++){
			if(adjRadius > rad) {
				rad += adjustment;
			} else {
				rad -= adjustment;
				adjustment /= 2;
			}
			pos.x = center.x + (sinR * rad);
			pos.z = center.z + (cosR * rad);
			float newY = ground->GetHeight(pos.x, pos.z);
			ydiff = fabs(pos.y - newY);
			pos.y = newY;
			heightDiff = (pos.y - center.y);
			adjRadius = weapon ? weapon->GetRange2D(heightDiff) : rad;
		}
		pos.x = center.x + (sinR * adjRadius);
		pos.z = center.z + (cosR * adjRadius);
		pos.y = ground->GetHeight(pos.x, pos.z) + 5.0f;
		glVertexf3(pos);
	}
	glEnd();
}


/******************************************************************************/

void glDrawVolume(DrawVolumeFunc drawFunc, const void* data)
{
	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP_NV);

	glEnable(GL_STENCIL_TEST);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// using zfail method to avoid doing the inside check
	if (GLEW_EXT_stencil_two_side && GL_EXT_stencil_wrap) {
		glDisable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		glActiveStencilFaceEXT(GL_BACK);
		glStencilMask(~0);
		glStencilOp(GL_KEEP, GL_DECR_WRAP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glActiveStencilFaceEXT(GL_FRONT);
		glStencilMask(~0);
		glStencilOp(GL_KEEP, GL_INCR_WRAP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		drawFunc(data); // draw
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	} else {
		glEnable(GL_CULL_FACE);
		glStencilMask(~0);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
		drawFunc(data); // draw
		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
		drawFunc(data); // draw
		glDisable(GL_CULL_FACE);
	}

	glDisable(GL_DEPTH_TEST);

	glStencilFunc(GL_NOTEQUAL, 0, ~0);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO); // clear as we go

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	drawFunc(data);   // draw

	glDisable(GL_DEPTH_CLAMP_NV);

	glDisable(GL_STENCIL_TEST);

	glEnable(GL_DEPTH_TEST);
}


/******************************************************************************/

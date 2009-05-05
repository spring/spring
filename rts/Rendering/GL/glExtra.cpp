#include "StdAfx.h"
#include "mmgr.h"

#include "glExtra.h"
#include "VertexArray.h"
#include "Map/Ground.h"
#include "Sim/Weapons/Weapon.h"
#include "LogOutput.h"


/*
 *  Draws a trigonometric circle in 'resolution' steps.
 */
static void defSurfaceCircle(const float3& center, float radius, unsigned int res)
{
	CVertexArray* va=GetVertexArray();
	va->Initialize();
	for (unsigned int i = 0; i < res; ++i) {
		const float radians = (2.0f * PI) * (float)i / (float)res;
		float3 pos;
		pos.x = center.x + (fastmath::sin(radians) * radius);
		pos.z = center.z + (fastmath::cos(radians) * radius);
		pos.y = ground->GetHeight(pos.x, pos.z) + 5.0f;
		va->AddVertex0(pos);
	}
	va->DrawArray0(GL_LINE_LOOP);
}

static void defSurfaceSquare(const float3& center, float xsize, float zsize)
{
	// FIXME: terrain contouring
	const float3 p0 = center + float3(-xsize, 0.0f, -zsize);
	const float3 p1 = center + float3( xsize, 0.0f, -zsize);
	const float3 p2 = center + float3( xsize, 0.0f,  zsize);
	const float3 p3 = center + float3(-xsize, 0.0f,  zsize);

	CVertexArray* va=GetVertexArray();
	va->Initialize();
		va->AddVertex0(p0.x, ground->GetHeight(p0.x, p0.z), p0.z);
		va->AddVertex0(p1.x, ground->GetHeight(p1.x, p1.z), p1.z);
		va->AddVertex0(p2.x, ground->GetHeight(p2.x, p2.z), p2.z);
		va->AddVertex0(p3.x, ground->GetHeight(p3.x, p3.z), p3.z);
	va->DrawArray0(GL_LINE_LOOP);
}


SurfaceCircleFunc glSurfaceCircle = defSurfaceCircle;
SurfaceSquareFunc glSurfaceSquare = defSurfaceSquare;

void setSurfaceCircleFunc(SurfaceCircleFunc func)
{
	glSurfaceCircle = (func == NULL)? defSurfaceCircle: func;
}

void setSurfaceSquareFunc(SurfaceSquareFunc func)
{
	glSurfaceSquare = (func == NULL)? defSurfaceSquare: func;
}



/*
 *  Draws a trigonometric circle in 'resolution' steps, with a slope modifier
 */
void glBallisticCircle(const float3& center, const float radius,
                       const CWeapon* weapon,
                       unsigned int resolution, float slope)
{
	CVertexArray* va=GetVertexArray();
	va->Initialize();
	for (unsigned int i = 0; i < resolution; ++i) {
		const float radians = (2.0f * PI) * (float)i / (float)resolution;
		float rad = radius;
		float3 pos;
		float sinR = fastmath::sin(radians);
		float cosR = fastmath::cos(radians);
		pos.x = center.x + (sinR * rad);
		pos.z = center.z + (cosR * rad);
		pos.y = ground->GetHeight(pos.x, pos.z);
		float heightDiff = (pos.y - center.y)/2;
		rad -= heightDiff * slope;
		float adjRadius = weapon ? weapon->GetRange2D(heightDiff*weapon->heightMod) : rad;
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
			adjRadius = weapon ? weapon->GetRange2D(heightDiff*weapon->heightMod) : rad;
		}
		pos.x = center.x + (sinR * adjRadius);
		pos.z = center.z + (cosR * adjRadius);
		pos.y = ground->GetHeight(pos.x, pos.z) + 5.0f;
		va->AddVertex0(pos);
	}
	va->DrawArray0(GL_LINE_LOOP);
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
	if (GLEW_EXT_stencil_two_side && GLEW_EXT_stencil_wrap) {
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

// not really the best place to put this
void gluMyCube(float size) {
	const float mid = size * 0.5f;

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glBegin(GL_QUADS);
		// top
		glVertex3f( mid, mid,  mid);
		glVertex3f( mid, mid, -mid);
		glVertex3f(-mid, mid, -mid);
		glVertex3f(-mid, mid,  mid);
		// front
		glVertex3f( mid,  mid, mid);
		glVertex3f(-mid,  mid, mid);
		glVertex3f(-mid, -mid, mid);
		glVertex3f( mid, -mid, mid);
		// right
		glVertex3f(mid,  mid,  mid);
		glVertex3f(mid, -mid,  mid);
		glVertex3f(mid, -mid, -mid);
		glVertex3f(mid,  mid, -mid);
		// left
		glVertex3f(-mid,  mid,  mid);
		glVertex3f(-mid,  mid, -mid);
		glVertex3f(-mid, -mid, -mid);
		glVertex3f(-mid, -mid,  mid);
		// bottom
		glVertex3f( mid, -mid,  mid);
		glVertex3f( mid, -mid, -mid);
		glVertex3f(-mid, -mid, -mid);
		glVertex3f(-mid, -mid,  mid);
		// back
		glVertex3f( mid,  mid, -mid);
		glVertex3f(-mid,  mid, -mid);
		glVertex3f(-mid, -mid, -mid);
		glVertex3f( mid, -mid, -mid);
	glEnd();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

#include "StdAfx.h"
// camera->cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "Camera.h"

#include "Rendering/GL/myGL.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera* camera;
CCamera* cam2;


CCamera::CCamera()
{
	fov    = 45.0f;
	oldFov = 0.0f;

	up      = UpVector;
	forward = float3(1.0f, 0.0f, 0.0f);
	rot     = float3(0.0f, 0.0f, 0.0f);

	rot.y = 0.0f;
	rot.x = 0.0f;
	pos.x = 2000.0f;
	pos.y = 70.0f;
	pos.z = 1800.0f;

	posOffset = float3(0.0f, 0.0f, 0.0f);
	tiltOffset = float3(0.0f, 0.0f, 0.0f);

	lppScale = 0.0f;

	billboard[3]  = billboard[7]  = billboard[11] = 0.0;
	billboard[12] = billboard[13] = billboard[15] = 0.0;
	billboard[15] = 1.0;
}


CCamera::~CCamera()
{
}


static double Calculate4x4Cofactor(const double m[4][4], int ei, int ej)
{
	int ai, bi, ci;
	switch (ei) {
		case 0: { ai = 1; bi = 2; ci = 3; break; }
		case 1: { ai = 0; bi = 2; ci = 3; break; }
		case 2: { ai = 0; bi = 1; ci = 3; break; }
		case 3: { ai = 0; bi = 1; ci = 2; break; }
	}
	int aj, bj, cj;
	switch (ej) {
		case 0: { aj = 1; bj = 2; cj = 3; break; }
		case 1: { aj = 0; bj = 2; cj = 3; break; }
		case 2: { aj = 0; bj = 1; cj = 3; break; }
		case 3: { aj = 0; bj = 1; cj = 2; break; }
	}

	const double val =
	    (m[ai][aj] * ((m[bi][bj] * m[ci][cj]) - (m[ci][bj] * m[bi][cj])))
		- (m[ai][bj] * ((m[bi][aj] * m[ci][cj]) - (m[ci][aj] * m[bi][cj])))
		+ (m[ai][cj] * ((m[bi][aj] * m[ci][bj]) - (m[ci][aj] * m[bi][bj])));
	
	if (((ei + ej) % 2) == 0) {
		return +val;
	} else {
		return -val;
	}
}


static bool CalculateInverse4x4(const double m[4][4], double inv[4][4])
{
	double cofac[4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			cofac[i][j] = Calculate4x4Cofactor(m, i, j);
		}
	}

	const double det = (m[0][0] * cofac[0][0]) +
	                   (m[0][1] * cofac[0][1]) +
	                   (m[0][2] * cofac[0][2]) +
	                   (m[0][3] * cofac[0][3]);

	if (det <= 1.0e-9) {
		// singular matrix, set to identity?
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				inv[i][j] = (i == j) ? 1.0 : 0.0;
			}
		}
		return false;
	}

	const double scale = 1.0 / det;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			inv[i][j] = cofac[j][i] * scale; // (adjoint / determinant)
			                                 // (note the transposition in 'cofac')
		}
	}

	return true;
}


void CCamera::operator=(const CCamera& c)
{
	pos       = c.pos;
	pos2      = pos;
	rot       = c.rot;	
	forward   = c.forward;
	right     = c.right;
	up        = c.up;
	top       = c.top;
	bottom    = c.bottom;
	rightside = c.rightside;
	leftside  = c.leftside;
	fov       = c.fov;
	oldFov    = c.oldFov;
	lppScale  = c.lppScale;
}


void CCamera::Update(bool freeze)
{
	pos2 = pos;
//	fov=60;
	up.Normalize();

	right = forward.cross(up);
	right.Normalize();

	up = right.cross(forward);
	up.Normalize();

	const float viewx = (float)tanf(float(gu->viewSizeX)/gu->viewSizeY * PI / 4 * fov / 90);
	const float viewy = (float)tanf(PI / 4 * fov / 90);

	if (gu->viewSizeY <= 0) {
		lppScale = 0.0f;
	} else {
		const float span = 2.0f * (float)tanf((PI / 180.0) * (fov * 0.5f));
		lppScale = span / (float)gu->viewSizeY;
	}

	top    = (-forward * viewy) + up;
	top.Normalize();
	bottom = (-forward * viewy) - up;
	bottom.Normalize();
	rightside = (-forward * viewx) + right;
	rightside.Normalize();
	leftside  = (-forward * viewx) - right;
	leftside.Normalize();

	if (!freeze) {
		cam2->bottom    = bottom;
		cam2->forward   = forward;
		cam2->leftside  = leftside;
		cam2->pos       = pos;
		cam2->right     = right;
		cam2->rightside = rightside;
		cam2->rot       = rot;
		cam2->top       = top;
		cam2->up        = up;
		cam2->lppScale  = lppScale;
	}

	oldFov = fov;
	const float gndHeight = ground->GetHeight(pos.x, pos.z);
	const float rangemod = 1.0f + max(0.0f, pos.y - gndHeight - 500.0f) * 0.0003f;
	gu->viewRange = MAX_VIEW_RANGE * rangemod;

	glMatrixMode(GL_PROJECTION); // Select the Projection Matrix
	glLoadIdentity();            // Reset  the Projection Matrix
	gluPerspective(fov,
	               (GLfloat)gu->viewSizeX / (GLfloat)gu->viewSizeY,
	               NEAR_PLANE * rangemod, gu->viewRange);

	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	glMatrixMode(GL_MODELVIEW);  // Select the Modelview Matrix
	glLoadIdentity();

	// FIXME: should be applying the offsets to pos/up/right/forward/etc,
	//        but without affecting the real positions (need an intermediary)
	const float3 camPos = pos + posOffset;
	float3 fShake = (forward * (1.0f + tiltOffset.z)) +
	                (right   * tiltOffset.x) +
	                (up      * tiltOffset.y);
	const float3 center = camPos + fShake.Normalize();
	gluLookAt(camPos.x, camPos.y, camPos.z,
	          center.x, center.y, center.z,
	          up.x, up.y, up.z);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

	// the inverse modelview matrix (handy for shaders to have)
	CalculateInverse4x4((double(*)[4])modelview, (double(*)[4])modelviewInverse);

	// transpose the 3x3
	billboard[0]  = modelview[0];
	billboard[1]  = modelview[4];
	billboard[2]  = modelview[8];
	billboard[4]  = modelview[1];
	billboard[5]  = modelview[5];
	billboard[6]  = modelview[9];
	billboard[8]  = modelview[2];
	billboard[9]  = modelview[6];
	billboard[10] = modelview[10];
	
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = gu->viewSizeX;
	viewport[3] = gu->viewSizeY;
}


static inline bool AABBInOriginPlane(const float3& plane, const float3& camPos,
                                     const float3& mins, const float3& maxs)
{
	float3 fp; // far point
	fp.x = (plane.x > 0.0f) ? mins.x : maxs.x;
	fp.y = (plane.y > 0.0f) ? mins.y : maxs.y;
	fp.z = (plane.z > 0.0f) ? mins.z : maxs.z;
	return (plane.dot(fp - camPos) < 0.0f);
}


bool CCamera::InView(const float3& mins, const float3& maxs)
{
	// Axis-aligned bounding box test  (AABB)
	if (AABBInOriginPlane(rightside, pos, mins, maxs) &&
	    AABBInOriginPlane(leftside,  pos, mins, maxs) &&
	    AABBInOriginPlane(bottom,    pos, mins, maxs) &&
	    AABBInOriginPlane(top,       pos, mins, maxs)) {
		return true;
	}
	return false;
}


bool CCamera::InView(const float3 &p, float radius)
{
	const float3 t = (p - pos);
	const float  l = t.Length();

	if (l < 50.0f) {
		return true;
	}
	else if (l > gu->viewRange) {
		return false;
	}
	
	if ((t.dot(rightside) > radius) ||
	    (t.dot(leftside)  > radius) ||
	    (t.dot(bottom)    > radius) ||
	    (t.dot(top)       > radius)) {
		return false;
	}

	return true;
}


void CCamera::UpdateForward()
{
	forward.z = (float)(cos(camera->rot.y) * cos(camera->rot.x));
	forward.x = (float)(sin(camera->rot.y) * cos(camera->rot.x));
	forward.y = (float)(sin(camera->rot.x));
	forward.Normalize();
}


float3 CCamera::CalcPixelDir(int x, int y)
{
	float dx = float(x-gu->viewPosX-gu->viewSizeX/2)/gu->viewSizeY*tanf(fov/180/2*PI)*2;
	float dy = float(y-gu->viewSizeY/2)/gu->viewSizeY*tanf(fov/180/2*PI)*2;
	float3 dir = camera->forward-camera->up*dy+camera->right*dx;
	dir.Normalize();
	return dir;
}


float3 CCamera::CalcWindowCoordinates(const float3& objPos)
{
	double winPos[3];
	gluProject((double)objPos.x, (double)objPos.y, (double)objPos.z,
	           modelview, projection, viewport,
	           &winPos[0], &winPos[1], &winPos[2]);
	return float3((float)winPos[0], (float)winPos[1], (float)winPos[2]);
}

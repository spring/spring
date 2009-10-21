#include "StdAfx.h"
// camera->cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "mmgr.h"

#include "myMath.h"
#include "Matrix44f.h"
#include "GlobalUnsynced.h"
#include "Camera.h"
#include "Map/Ground.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera* camera;
CCamera* cam2;

unsigned int CCamera::billboardList = 0;

CCamera::CCamera() :
	pos(2000.0f, 70.0f, 1800.0f),
	rot(0.0f, 0.0f, 0.0f),
	forward(1.0f, 0.0f, 0.0f),   posOffset(0.0f, 0.0f, 0.0f), tiltOffset(0.0f, 0.0f, 0.0f), lppScale(0.0f)
{
	// stuff that wont change can be initialised here, it doesn't need to be reinitialised every update
	modelview[ 3] =  0.0f;
	modelview[ 7] =  0.0f;
	modelview[11] =  0.0f;
	modelview[15] =  1.0f;

	projection[ 1] = 0.0f;
	projection[ 2] = 0.0f;
	projection[ 3] = 0.0f;

	projection[ 4] = 0.0f;
	projection[ 6] = 0.0f;
	projection[ 7] = 0.0f;

	projection[12] = 0.0f;
	projection[13] = 0.0f;
	projection[15] = 0.0f;

	billboard[3]  = billboard[7]  = billboard[11] = 0.0;
	billboard[12] = billboard[13] = billboard[14] = 0.0;
	billboard[15] = 1.0;

	SetFov(45.0f);

	up      = UpVector;

	if (billboardList == 0) {
		billboardList = glGenLists(1);
	}
}

CCamera::~CCamera()
{
	glDeleteLists(billboardList,1);
}

void CCamera::Roll(float rad)
{
	CMatrix44f rotate;
	rotate.Rotate(rad, forward);
	up = rotate.Mul(up);
}

void CCamera::Pitch(float rad)
{
	CMatrix44f rotate;
	rotate.Rotate(rad, right);
	forward = rotate.Mul(forward);
	forward.Normalize();
	rot.y = atan2(forward.x,forward.z);
	rot.x = asin(forward.y);
	UpdateForward();
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

void CCamera::Update(bool freeze, bool resetUp)
{
	pos2 = pos;

	if (resetUp) {
		up.x = 0.0f;
		up.y = 1.0f;
		up.z = 0.0f;
	}

	right = forward.cross(up);
	right.UnsafeANormalize();

	up = right.cross(forward);
	up.UnsafeANormalize();

	const float aspect = (float) gu->viewSizeX / (float) gu->viewSizeY;
	const float viewx = tan(aspect * halfFov);
	// const float viewx = aspect * tanHalfFov;
	const float viewy = tanHalfFov;

	if (gu->viewSizeY <= 0) {
		lppScale = 0.0f;
	} else {
		const float span = 2.0f * tanHalfFov;
		lppScale = span / (float) gu->viewSizeY;
	}

	const float3 forwardy = (-forward * viewy);
	top    = forwardy + up;
	top.UnsafeANormalize();
	bottom = forwardy - up;
	bottom.UnsafeANormalize();

	const float3 forwardx = (-forward * viewx);
	rightside = forwardx + right;
	rightside.UnsafeANormalize();
	leftside  = forwardx - right;
	leftside.UnsafeANormalize();

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

	const float gndHeight = ground->GetHeight(pos.x, pos.z);
	const float rangemod = 1.0f + std::max(0.0f, pos.y - gndHeight - 500.0f) * 0.0003f;
	const float zNear = (NEAR_PLANE * rangemod);
	gu->viewRange = MAX_VIEW_RANGE * rangemod;

	glMatrixMode(GL_PROJECTION); // Select the Projection Matrix
	glLoadIdentity();            // Reset  the Projection Matrix

	// apply and store the transform, should be faster
	// than calling glGetDoublev(GL_PROJECTION_MATRIX)
	// right after gluPerspective()
	myGluPerspective(aspect, zNear, gu->viewRange);


	glMatrixMode(GL_MODELVIEW);  // Select the Modelview Matrix
	glLoadIdentity();

	// FIXME: should be applying the offsets to pos/up/right/forward/etc,
	//        but without affecting the real positions (need an intermediary)
	float3 fShake = (forward * (1.0f + tiltOffset.z)) +
	                (right   * tiltOffset.x) +
	                (up      * tiltOffset.y);

	const float3 camPos = pos + posOffset;
	const float3 center = camPos + fShake.ANormalize();

	// apply and store the transform, should be faster
	// than calling glGetDoublev(GL_MODELVIEW_MATRIX)
	// right after gluLookAt()
	myGluLookAt(camPos, center, up);


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

	glNewList(billboardList, GL_COMPILE);
	glMultMatrixd(billboard);
	glEndList();

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
	const float3 t   = (p - pos);
	const float  lsq = t.SqLength();

	if (lsq < 2500.0f) {
		return true;
	}
	else if (lsq > Square(gu->viewRange)) {
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
	forward.z = cos(rot.y) * cos(rot.x);
	forward.x = sin(rot.y) * cos(rot.x);
	forward.y = sin(rot.x);
	forward.ANormalize();
}


float3 CCamera::CalcPixelDir(int x, int y)
{
	float dx = float(x-gu->viewPosX-gu->viewSizeX/2)/gu->viewSizeY * tanHalfFov * 2;
	float dy = float(y-gu->viewSizeY/2)/gu->viewSizeY * tanHalfFov * 2;
	float3 dir = forward - up * dy + right * dx;
	dir.ANormalize();
	return dir;
}


float3 CCamera::CalcWindowCoordinates(const float3& objPos)
{
	double winPos[3];
	gluProject((GLdouble)objPos.x, (GLdouble)objPos.y, (GLdouble)objPos.z,
	           modelview, projection, viewport,
	           &winPos[0], &winPos[1], &winPos[2]);
	return float3((float)winPos[0], (float)winPos[1], (float)winPos[2]);
}

inline void CCamera::myGluPerspective(float aspect, float zNear, float zFar) {
	GLdouble t = zNear * tanHalfFov;
	GLdouble b = -t;
	GLdouble l = b * aspect;
	GLdouble r = t * aspect;

	projection[ 0] = (2.0f * zNear) / (r - l);

	projection[ 5] = (2.0f * zNear) / (t - b);

	projection[ 8] = (r + l) / (r - l);
	projection[ 9] = (t + b) / (t - b);
	projection[10] = -(zFar + zNear) / (zFar - zNear);
	projection[11] = -1.0f;

	projection[14] = -(2.0f * zFar * zNear) / (zFar - zNear);

	glMultMatrixd(projection);
}

inline void CCamera::myGluLookAt(const float3& eye, const float3& center, const float3& up) {
	float3 f = (center - eye).ANormalize();
	float3 s = f.cross(up);
	float3 u = s.cross(f);

	modelview[ 0] =  s.x;
	modelview[ 1] =  u.x;
	modelview[ 2] = -f.x;


	modelview[ 4] =  s.y;
	modelview[ 5] =  u.y;
	modelview[ 6] = -f.y;

	modelview[ 8] =  s.z;
	modelview[ 9] =  u.z;
	modelview[10] = -f.z;

	// save a glTranslated(-eye.x, -eye.y, -eye.z) call
	modelview[12] = ( s.x * -eye.x) + ( s.y * -eye.y) + ( s.z * -eye.z);
	modelview[13] = ( u.x * -eye.x) + ( u.y * -eye.y) + ( u.z * -eye.z);
	modelview[14] = (-f.x * -eye.x) + (-f.y * -eye.y) + (-f.z * -eye.z);

	glMultMatrixd(modelview);
}

const GLdouble* CCamera::GetProjection() const { return projection; }
const GLdouble* CCamera::GetModelview() const { return modelview; }
const GLdouble* CCamera::GetBillboard() const { return billboard; }

float CCamera::GetFov() const { return fov; }
float CCamera::GetHalfFov() const { return halfFov; }
float CCamera::GetTanHalfFov() const { return tanHalfFov; }

void CCamera::SetFov(float myfov)
{
	fov = myfov;
	halfFov = fov * 0.008726646f;
	tanHalfFov = tan(halfFov);
}

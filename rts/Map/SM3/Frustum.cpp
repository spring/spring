/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Frustum.h"

#include "Rendering/GL/myGL.h"
#undef far // avoid collision with windef.h

void Frustum::InversePlanes ()
{
	std::vector<Plane>::iterator p;
	for(p=planes.begin(); p!=planes.end(); ++p) {
		p->Inverse ();
	}
}

// find the box vertices to compare against the plane
static void BoxPlaneVerts (const Vector3& min, const Vector3& max, const Vector3& plane,
                           Vector3& close, Vector3& far)
{
	if(plane.x > 0) { close.x = min.x; far.x = max.x; }
	else { close.x = max.x; far.x = min.x; }
	if(plane.y > 0) { close.y = min.y; far.y = max.y; }
	else { close.y = max.y; far.y = min.y; }
	if(plane.z > 0) { close.z = min.z; far.z = max.z; }
	else { close.z = max.z; far.z = min.z; }
}

void Frustum::CalcCameraPlanes (Vector3 *cbase, Vector3 *cright, Vector3* cup, Vector3* cfront, float tanHalfFov, float aspect)
{
	planes.resize(5);
	planes[0].SetVec (*cfront);
	planes[0].CalcDist (*cbase + *cfront);

	float m = 200.0f;
	base = *cbase + *cfront * m;
	up = *cup , right = *cright;
	up *= tanHalfFov * m;
	right *= tanHalfFov * m * aspect;
	front = *cfront;

	pos [0] = base + right + up; // rightup
	pos [1] = base + right - up; // rightdown
	pos [2] = base - right - up; // leftdown
	pos [3] = base - right + up; // leftup

	base = *cbase;

	planes[1].MakePlane (base, pos[2], pos[3]); // left
	planes[2].MakePlane (base, pos[3], pos[0]); // up
	planes[3].MakePlane (base, pos[0], pos[1]); // right
	planes[4].MakePlane (base, pos[1], pos[2]); // down

	right.ANormalize();
	up.ANormalize();
	front.ANormalize();
}

void Frustum::Draw ()
{
	if (base.x==0.0f) return;

	glDisable(GL_CULL_FACE);

/*	if (keys[SDLK_t]) {
		glBegin(GL_LINES);
		glColor3ub (255,0,0);
		glVertex3fv((float*)&base);
		glVertex3fv((float*)&(base+front*100));
		glEnd();
		glBegin(GL_LINES);
		glColor3ub (0,255,0);
		glVertex3fv((float*)&base);
		glVertex3fv((float*)&(base+right*100));
		glEnd();
		glBegin(GL_LINES);
		glColor3ub (0,0,255);
		glVertex3fv((float*)&base);
		glVertex3fv((float*)&(base+up*100));
		glEnd();
	}else{*/
		glBegin (GL_LINES);
		for (int a=0;a<4;a++) {
			glVertex3f (base.x, base.y, base.z);
			glVertex3f (pos[a].x, pos[a].y, pos[a].z);
		}
		glEnd();
		glBegin (GL_LINE_LOOP);
		for (int a=0;a<4;a++) {
			glVertex3fv ((float*)&pos[a]);
		}
		glEnd();
	//}
	glColor3ub(255,255,255);
}

Frustum::VisType Frustum::IsBoxVisible (const Vector3& min, const Vector3& max)
{
	bool full = true;
	Vector3 c,f;

	std::vector<Plane>::iterator p;
	for(p=planes.begin(); p!=planes.end(); ++p)
	{
		BoxPlaneVerts (min, max, p->GetVector(), c, f);
		
		const float dc = p->Dist(&c);
		const float df = p->Dist(&f);
		if(dc < 0.0f || df < 0.0f) full=false;
		if(dc < 0.0f && df < 0.0f) return Outside;
	}

	return full ? Inside : Partial;
}

Frustum::VisType Frustum::IsPointVisible (const Vector3& pt)
{
	std::vector<Plane>::const_iterator p;
	for(p=planes.begin(); p!=planes.end(); ++p) {
		float d = p->Dist (&pt);

		if (d < 0.0f) return Outside;
	}
	return Inside;
}

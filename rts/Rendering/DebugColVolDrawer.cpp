/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugColVolDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"

static const float3 DEFAULT_VOLUME_COLOR = float3(0.45f, 0.0f, 0.45f);
static unsigned int volumeDisplayListIDs[3] = {0, 0, 0};

static inline void DrawCollisionVolume(const CollisionVolume* vol)
{
	glPushMatrix();

	switch (vol->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// scaled sphere is special case of ellipsoid: radius, slices, stacks
			glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			glScalef(vol->GetHScale(0), vol->GetHScale(1), vol->GetHScale(2));
			glWireSphere(&volumeDisplayListIDs[0], 20, 20);
		} break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER: {
			// scaled cylinder: base-radius, top-radius, height, slices, stacks
			//
			// (cylinder base is drawn at unit center by default so add offset
			// by half major axis to visually match the mathematical situation,
			// height of the cylinder equals the unit's full major axis)
			switch (vol->GetPrimaryAxis()) {
				case CollisionVolume::COLVOL_AXIS_X: {
					glTranslatef(-(vol->GetHScale(0)), 0.0f, 0.0f);
					glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					glScalef(vol->GetScale(0), vol->GetHScale(1), vol->GetHScale(2));
					glRotatef( 90.0f, 0.0f, 1.0f, 0.0f);
				} break;
				case CollisionVolume::COLVOL_AXIS_Y: {
					glTranslatef(0.0f, -(vol->GetHScale(1)), 0.0f);
					glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					glScalef(vol->GetHScale(0), vol->GetScale(1), vol->GetHScale(2));
					glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
				} break;
				case CollisionVolume::COLVOL_AXIS_Z: {
					glTranslatef(0.0f, 0.0f, -(vol->GetHScale(2)));
					glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					glScalef(vol->GetHScale(0), vol->GetHScale(1), vol->GetScale(2));
				} break;
			}

			glWireCylinder(&volumeDisplayListIDs[1], 20, 1.0f);
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			// scaled cube: length, width, height
			glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			glScalef(vol->GetScale(0), vol->GetScale(1), vol->GetScale(2));
			glWireCube(&volumeDisplayListIDs[2]);
		} break;
	}

	glPopMatrix();
}



static inline void DrawObjectMidAndAimPos(const CSolidObject* o)
{
	GLUquadricObj* q = gluNewQuadric();
	glDisable(GL_DEPTH_TEST);

	if (o->aimPos != o->midPos) {
		// draw the aim-point
		glPushMatrix();
		glTranslatef3(o->relAimPos * WORLD_TO_OBJECT_SPACE);
		glColor3f(1.0f, 0.0f, 0.0f);
		gluQuadricDrawStyle(q, GLU_FILL);
		gluSphere(q, 2.0f, 5, 5);
		glPopMatrix();
	}

	{
		// draw the mid-point, keep this transform on the stack
		glTranslatef3(o->relMidPos * WORLD_TO_OBJECT_SPACE);
		glColor3f(1.0f, 0.0f, 1.0f);
		gluQuadricDrawStyle(q, GLU_FILL);
		gluSphere(q, 2.0f, 5, 5);
		glColorf3(DEFAULT_VOLUME_COLOR);
	}

	glEnable(GL_DEPTH_TEST);
	gluDeleteQuadric(q);
}


static inline void DrawFeatureColVol(const CFeature* f)
{
	const CollisionVolume* v = f->collisionVolume;

	if (!f->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView) return;
	if (!camera->InView(f->pos, f->drawRadius)) return;

	const bool vCustomType = (v->GetVolumeType() < CollisionVolume::COLVOL_TYPE_SPHERE);
	const bool vCustomDims = ((v->GetOffsets()).SqLength() >= 1.0f || math::fabs(v->GetBoundingRadius() - f->radius) >= 1.0f);

	glPushMatrix();
		glMultMatrixf(f->GetTransformMatrixRef());
		DrawObjectMidAndAimPos(f);

		if (!v->IgnoreHits()) {
			DrawCollisionVolume(v);
		}

		if (vCustomType || vCustomDims) {
			// assume this is a custom volume
			glColor3f(0.5f, 0.5f, 0.5f);
			glScalef(f->radius, f->radius, f->radius);
			glWireSphere(&volumeDisplayListIDs[0], 20, 20);
		}
	glPopMatrix();
}


static void DrawUnitDebugPieceTree(const LocalModelPiece* p, const LocalModelPiece* lap, int lapf, CMatrix44f mat)
{
	const float3& rot = p->GetRotation();
	mat.Translate(p->GetPosition());
	mat.RotateY(-rot[1]);
	mat.RotateX(-rot[0]);
	mat.RotateZ(-rot[2]);

	glPushMatrix();
		glMultMatrixf(mat.m);

		if (p->scriptSetVisible && !p->GetCollisionVolume()->IgnoreHits()) {
			if ((p == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150))) {
				glColor3f((1.0f - ((gs->frameNum - lapf) / 150.0f)), 0.0f, 0.0f);
			}

			DrawCollisionVolume(p->GetCollisionVolume());

			if ((p == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150))) {
				glColorf3(DEFAULT_VOLUME_COLOR);
			}
		}
	glPopMatrix();

	for (unsigned int i = 0; i < p->children.size(); i++) {
		DrawUnitDebugPieceTree(p->children[i], lap, lapf, mat);
	}
}


static inline void DrawUnitColVol(const CUnit* u)
{
	if (!(u->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView) return;
	if (!camera->InView(u->drawMidPos, u->drawRadius)) return;

	const CollisionVolume* v = u->collisionVolume;
	const bool vCustomType = (v->GetVolumeType() < CollisionVolume::COLVOL_TYPE_SPHERE);
	const bool vCustomDims = ((v->GetOffsets()).SqLength() >= 1.0f || math::fabs(v->GetBoundingRadius() - u->radius) >= 1.0f);

	glPushMatrix();
		glMultMatrixf(u->GetTransformMatrix());
		DrawObjectMidAndAimPos(u);

		if (v->DefaultToPieceTree()) {
			// draw only the piece volumes for less clutter
			CMatrix44f mat(u->relMidPos * float3(0.0f, -1.0f, 0.0f));
			DrawUnitDebugPieceTree(u->localModel->GetRoot(), u->lastAttackedPiece, u->lastAttackedPieceFrame, mat);
		} else {
			if (!v->IgnoreHits()) {
				// make it fade red under attack
				if (u->lastAttackFrame > 0 && ((gs->frameNum - u->lastAttackFrame) < 150)) {
					glColor3f((1.0f - ((gs->frameNum - u->lastAttackFrame) / 150.0f)), 0.0f, 0.0f);
				}

				DrawCollisionVolume(v);

				if (u->lastAttackFrame > 0 && ((gs->frameNum - u->lastAttackFrame) < 150)) {
					glColorf3(DEFAULT_VOLUME_COLOR);
				}
			}
		}

		if (vCustomType || vCustomDims) {
			// assume this is a custom volume
			glColor3f(0.5f, 0.5f, 0.5f);
			glScalef(u->radius, u->radius, u->radius);
			glWireSphere(&volumeDisplayListIDs[0], 20, 20);
		}

	glPopMatrix();
}


class CDebugColVolQuadDrawer : public CReadMap::IQuadDrawer {
public:
	void DrawQuad(int x, int y)
	{
		const CQuadField::Quad& q = quadField->GetQuadAt(x, y);

		for (std::list<CFeature*>::const_iterator fi = q.features.begin(); fi != q.features.end(); ++fi) {
			DrawFeatureColVol(*fi);
		}

		for (std::list<CUnit*>::const_iterator ui = q.units.begin(); ui != q.units.end(); ++ui) {
			DrawUnitColVol(*ui);
		}

		// TODO show colvols of synced projectiles
	}
};



namespace DebugColVolDrawer
{
	bool enable = false;

	void Draw()
	{
		// DebugColVolDrawer is not thread-safe
		if (GML::Enabled() || !enable)
			return;

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHT1);
			glDisable(GL_CULL_FACE);
			glDisable(GL_TEXTURE_2D);
			// glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_FOG);
			glDisable(GL_CLIP_PLANE0);
			glDisable(GL_CLIP_PLANE1);

			glLineWidth(2.0f);
			glDepthMask(GL_TRUE);

			static CDebugColVolQuadDrawer drawer;
			readmap->GridVisibility(camera, CQuadField::QUAD_SIZE / SQUARE_SIZE, 1e9, &drawer);

			glLineWidth(1.0f);
		glPopAttrib();
	}
}

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include "DebugColVolDrawer.h"

#include "Game/Camera.h"
#include "Map/ReadMap.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"

static float3 defaultColVolColor(0.45f, 0.0f, 0.45f);

static inline void DrawCollisionVolume(const CollisionVolume* vol)
{
	static unsigned int listIDs[3] = {0, 0, 0};

	switch (vol->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_FOOTPRINT:
			// fall through, this is too hard to render correctly so just render sphere :)
		case CollisionVolume::COLVOL_TYPE_SPHERE:
			// fall through, sphere is special case of ellipsoid
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID: {
			// scaled sphere: radius, slices, stacks
			glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			glScalef(vol->GetHScale(0), vol->GetHScale(1), vol->GetHScale(2));
			glWireSphere(&listIDs[0], 20, 20);
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

			glWireCylinder(&listIDs[1], 20, 1.0f);
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			// scaled cube: length, width, height
			glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			glScalef(vol->GetScale(0), vol->GetScale(1), vol->GetScale(2));
			glWireCube(&listIDs[2]);
		} break;
	}
}


static inline void DrawFeatureColVol(const CFeature* f)
{
	if (f->collisionVolume && !f->collisionVolume->IsDisabled()) {
		if (!f->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView) return;
		if (!camera->InView(f->pos, f->drawRadius)) return;

		glPushMatrix();
			glMultMatrixf(f->transMatrix.m);
			glTranslatef3(f->relMidPos * float3(-1.0f, 1.0f, 1.0f));

			GLUquadricObj* q = gluNewQuadric();
			{
				// draw the centerpos
				glColor3f(1.0f, 0.0f, 1.0f);
				gluQuadricDrawStyle(q, GLU_FILL);
				gluSphere(q, 2.0f, 5, 5);
				glColorf3(defaultColVolColor);
			}
			gluDeleteQuadric(q);

			DrawCollisionVolume(f->collisionVolume);
		glPopMatrix();
	}
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

		if (p->visible && !p->GetCollisionVolume()->IsDisabled()) {
			if ((p == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150))) {
				glColor3f((1.0f - ((gs->frameNum - lapf) / 150.0f)), 0.0f, 0.0f);
			}

			DrawCollisionVolume(p->GetCollisionVolume());

			if ((p == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150))) {
				glColorf3(defaultColVolColor);
			}
		}
	glPopMatrix();

	for (unsigned int i = 0; i < p->childs.size(); i++) {
		DrawUnitDebugPieceTree(p->childs[i], lap, lapf, mat);
	}
}


static inline void DrawUnitColVol(const CUnit* unit)
{
	if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView) return;
	if (!camera->InView(unit->drawMidPos, unit->drawRadius)) return;

	glPushMatrix();
		glMultMatrixf(unit->GetTransformMatrix());
		glTranslatef3(unit->relMidPos * float3(-1.0f, 1.0f, 1.0f));

		GLUquadricObj* q = gluNewQuadric();
		{
			// draw the aimpoint
			glColor3f(1.0f, 0.0f, 1.0f);
			gluQuadricDrawStyle(q, GLU_FILL);
			gluSphere(q, 2.0f, 5, 5);
			glColorf3(defaultColVolColor);
		}
		gluDeleteQuadric(q);

		if (unit->unitDef->usePieceCollisionVolumes) {
			//! draw only the piece volumes for less clutter
			CMatrix44f mat(unit->relMidPos * float3(0.0f, -1.0f, 0.0f));
			DrawUnitDebugPieceTree(unit->localmodel->GetRoot(), unit->lastAttackedPiece, unit->lastAttackedPieceFrame, mat);
		} else {
			if (!unit->collisionVolume->IsDisabled()) {
				//! make it fade red under attack
				if (unit->lastAttack > 0 && ((gs->frameNum - unit->lastAttack) < 150)) {
					glColor3f((1.0f - ((gs->frameNum - unit->lastAttack) / 150.0f)), 0.0f, 0.0f);
				}

				DrawCollisionVolume(unit->collisionVolume);

				if (unit->lastAttack > 0 && ((gs->frameNum - unit->lastAttack) < 150)) {
					glColorf3(defaultColVolColor);
				}
			}
		}
	glPopMatrix();
}


class CDebugColVolQuadDrawer : public CReadMap::IQuadDrawer {
public:
	void DrawQuad(int x, int y)
	{
		const CQuadField::Quad& q = qf->GetQuadAt(x,y);

		for (std::list<CFeature*>::const_iterator fi = q.features.begin(); fi != q.features.end(); ++fi) {
			DrawFeatureColVol(*fi);
		}

		for (std::list<CUnit*>::const_iterator ui = q.units.begin(); ui != q.units.end(); ++ui) {
			DrawUnitColVol(*ui);
		}

		//todo: show colvols of synced projectiles
	}
};



namespace DebugColVolDrawer
{
	void Draw()
	{
		if (!globalRendering->drawdebug)
			return;

		GML_RECMUTEX_LOCK(unit);
		GML_RECMUTEX_LOCK(feat);

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

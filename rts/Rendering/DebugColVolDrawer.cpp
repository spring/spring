/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugColVolDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/MatrixState.hpp"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "System/UnorderedSet.hpp"




static constexpr float4 DEFAULT_VOLUME_COLOR = float4(0.45f, 0.0f, 0.45f, 0.35f);
// [0] := VBO, [1] := IBO, [2 + i, 3 + i] := {#verts, #indcs}
static unsigned int MESH_COLVOL_BUFFERS[2 + 3 * 2] = {0, 0, 0, 0, 0, 0, 0, 0};

static inline void DrawCollisionVolume(const CollisionVolume* vol)
{
	GL::PushMatrix();

	switch (vol->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// scaled sphere is special case of ellipsoid: radius, slices, stacks
			GL::Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
			GL::Scale({vol->GetHScale(0), vol->GetHScale(1), vol->GetHScale(2)});

			gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);
		} break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER: {
			// scaled cylinder: base-radius, top-radius, height, slices, stacks
			//
			// (cylinder base is drawn at unit center by default so add offset
			// by half major axis to visually match the mathematical situation,
			// height of the cylinder equals the unit's full major axis)
			switch (vol->GetPrimaryAxis()) {
				case CollisionVolume::COLVOL_AXIS_X: {
					GL::Translate({-(vol->GetHScale(0)), 0.0f, 0.0f});
					GL::Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
					GL::Scale({vol->GetScale(0), vol->GetHScale(1), vol->GetHScale(2)});
					GL::RotateY(90.0f);
				} break;
				case CollisionVolume::COLVOL_AXIS_Y: {
					GL::Translate({0.0f, -(vol->GetHScale(1)), 0.0f});
					GL::Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
					GL::Scale({vol->GetHScale(0), vol->GetScale(1), vol->GetHScale(2)});
					GL::RotateX(-90.0f);
				} break;
				case CollisionVolume::COLVOL_AXIS_Z: {
					GL::Translate({0.0f, 0.0f, -(vol->GetHScale(2))});
					GL::Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
					GL::Scale({vol->GetHScale(0), vol->GetHScale(1), vol->GetScale(2)});
				} break;
			}

			gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 1);
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			// scaled cube: length, width, height
			GL::Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
			GL::Scale({vol->GetScale(0), vol->GetScale(1), vol->GetScale(2)});

			gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 0);
		} break;
	}

	GL::PopMatrix();
}



/*
static void DrawUnitDebugPieceTree(const LocalModelPiece* lmp, const LocalModelPiece* lap, int lapf)
{
	GL::PushMatrix();
	GL::MultMatrix(lmp->GetModelSpaceMatrix());

		if (lmp->scriptSetVisible && !lmp->GetCollisionVolume()->IgnoreHits()) {
			if ((lmp == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150)))
				glColor3f((1.0f - ((gs->frameNum - lapf) / 150.0f)), 0.0f, 0.0f);

			// factors in the volume offsets
			DrawCollisionVolume(lmp->GetCollisionVolume());

			if ((lmp == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150)))
				glColorf3(DEFAULT_VOLUME_COLOR);
		}

	GL::PopMatrix();

	for (unsigned int i = 0; i < lmp->children.size(); i++) {
		DrawUnitDebugPieceTree(lmp->children[i], lap, lapf);
	}
}
*/

static void DrawObjectDebugPieces(const CSolidObject* o)
{
	const int hitDeltaTime = (gs->frameNum - o->lastHitPieceFrame);

	for (unsigned int n = 0; n < o->localModel.pieces.size(); n++) {
		const LocalModelPiece* lmp = o->localModel.GetPiece(n);
		const CollisionVolume* lmpVol = lmp->GetCollisionVolume();

		const bool b0 = ((o->lastHitPieceFrame > 0) && (hitDeltaTime < 150));
		const bool b1 = (lmp == o->lastHitPiece);

		if (!lmp->scriptSetVisible || lmpVol->IgnoreHits())
			continue;

		if (b0 && b1)
			glColor3f((1.0f - (hitDeltaTime / 150.0f)), 0.0f, 0.0f);

		GL::PushMatrix();
		GL::MultMatrix(lmp->GetModelSpaceMatrix());

		// factors in the volume offsets
		DrawCollisionVolume(lmpVol);
		GL::PopMatrix();

		if (b0 && b1)
			glColorf4(DEFAULT_VOLUME_COLOR);
	}
}



static inline void DrawObjectMidAndAimPos(const CSolidObject* o)
{
	glDisable(GL_DEPTH_TEST);

	if (o->aimPos != o->midPos) {
		// draw the aim-point
		GL::PushMatrix();
		GL::Translate(o->relAimPos);

		glColor4f(1.0f, 0.0f, 0.0f, 0.35f);
		gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);

		GL::PopMatrix();
	}

	{
		// draw the mid-point, keep this transform on the stack
		GL::Translate(o->relMidPos);

		glColor4f(1.0f, 0.0f, 1.0f, 0.35f);
		gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);
		glColorf4(DEFAULT_VOLUME_COLOR);
	}

	glEnable(GL_DEPTH_TEST);
}



static inline void DrawFeatureColVol(const CFeature* f)
{
	const CollisionVolume* v = &f->collisionVolume;

	if (f->IsInVoid())
		return;
	if (!f->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
		return;
	if (!camera->InView(f->pos, f->GetDrawRadius()))
		return;

	const bool customType = (v->GetVolumeType() < CollisionVolume::COLVOL_TYPE_SPHERE);
	const bool customSize = ((v->GetOffsets()).SqLength() >= 1.0f || std::fabs(v->GetBoundingRadius() - f->radius) >= 1.0f);

	GL::PushMatrix();
	GL::MultMatrix(f->GetTransformMatrixRef(false));

		DrawObjectMidAndAimPos(f);

		if (v->DefaultToPieceTree()) {
			// draw only the piece volumes for less clutter
			// note: relMidPos transform is on the stack at this
			// point but all piece-positions are relative to pos
			// --> undo it
			GL::Translate(-f->relMidPos);
			DrawObjectDebugPieces(f);
			GL::Translate(f->relMidPos);
		} else {
			if (!v->IgnoreHits())
				DrawCollisionVolume(v);
		}

		if (customType || customSize) {
			GL::Scale({f->radius, f->radius, f->radius});

			// assume this is a custom volume; draw radius-sphere next to it
			glColor4f(0.5f, 0.5f, 0.5f, 0.35f);
			gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);
		}

	GL::PopMatrix();
}

static inline void DrawUnitColVol(const CUnit* u)
{
	if (u->IsInVoid())
		return;
	if (!(u->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
		return;
	if (!camera->InView(u->drawMidPos, u->GetDrawRadius()))
		return;

	const CollisionVolume* v = &u->collisionVolume;
	const bool customType = (v->GetVolumeType() < CollisionVolume::COLVOL_TYPE_SPHERE);
	const bool customSize = ((v->GetOffsets()).SqLength() >= 1.0f || std::fabs(v->GetBoundingRadius() - u->radius) >= 1.0f);

	glDisable(GL_DEPTH_TEST);

	for (const CWeapon* w: u->weapons) {
		GL::PushMatrix();
		GL::Translate(w->aimFromPos);

		glColor4f(1.0f, 1.0f, 0.0f, 0.4f);
		gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);

		GL::PopMatrix();
		GL::PushMatrix();
		GL::Translate(w->weaponMuzzlePos);

		if (w->HaveTarget()) {
			glColor4f(1.0f, 0.8f, 0.0f, 0.4f);
		} else {
			glColor4f(1.0f, 0.0f, 0.0f, 0.4f);
		}

		gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);
		GL::PopMatrix();

		if (w->HaveTarget()) {
			GL::PushMatrix();
			GL::Translate(w->GetCurrentTargetPos());

			glColor4f(1.0f, 0.8f, 0.0f, 0.4f);
			gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);

			GL::PopMatrix();
		}
	}

	glColorf4(DEFAULT_VOLUME_COLOR);
	glEnable(GL_DEPTH_TEST);


	GL::PushMatrix();
	GL::MultMatrix(u->GetTransformMatrix(false));

		DrawObjectMidAndAimPos(u);

		if (v->DefaultToPieceTree()) {
			// draw only the piece volumes for less clutter
			// note: relMidPos transform is on the stack at this
			// point but all piece-positions are relative to pos
			// --> undo it
			GL::Translate(-u->relMidPos);
			DrawObjectDebugPieces(u);
			GL::Translate(u->relMidPos);
		} else {
			if (!v->IgnoreHits()) {
				// make it fade red under attack
				if (u->lastAttackFrame > 0 && ((gs->frameNum - u->lastAttackFrame) < 150))
					glColor3f((1.0f - ((gs->frameNum - u->lastAttackFrame) / 150.0f)), 0.0f, 0.0f);

				// if drawing this, disable the DrawObjectMidAndAimPos call
				// DrawCollisionVolume((u->localModel).GetBoundingVolume());
				DrawCollisionVolume(v);

				if (u->lastAttackFrame > 0 && ((gs->frameNum - u->lastAttackFrame) < 150))
					glColorf4(DEFAULT_VOLUME_COLOR);
			}
		}
		if (u->shieldWeapon != nullptr) {
			const CPlasmaRepulser* shield = static_cast<const CPlasmaRepulser*>(u->shieldWeapon);

			glColor4f(0.0f, 0.0f, 0.6f, 0.35f);
			DrawCollisionVolume(&shield->collisionVolume);
		}

		if (customType || customSize) {
			GL::Scale({u->radius, u->radius, u->radius});

			// assume this is a custom volume; draw radius-sphere next to it
			glColor4f(0.5f, 0.5f, 0.5f, 0.35f);
			gleDrawColVolMeshSubBuffer(&MESH_COLVOL_BUFFERS[0], 2);
		}

	GL::PopMatrix();
}


class CDebugColVolQuadDrawer : public CReadMap::IQuadDrawer {
public:
	void ResetState() {
		unitIDs.clear();
		unitIDs.reserve(32);
		featureIDs.clear();
		featureIDs.reserve(32);
	}

	void Enable() const {
		GL::PushMatrix();
		GL::LoadIdentity();
		GL::MultMatrix(camera->GetViewMatrix());

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		glDisable(GL_CULL_FACE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		gleBindColVolMeshBuffers(&MESH_COLVOL_BUFFERS[0]);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(2.0f);
		glDepthMask(GL_TRUE);
	}
	void Disable() const {
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		gleBindColVolMeshBuffers(nullptr);
		glPopAttrib();

		GL::PopMatrix();
	}

	void DrawQuad(int x, int y) {
		const CQuadField::Quad& q = quadField->GetQuadAt(x, y);

		for (const CUnit* u: q.units) {
			const auto& p = unitIDs.insert(u->id);

			if (!p.second)
				continue;

			DrawUnitColVol(u);
		}

		for (const CFeature* f: q.features) {
			const auto& p = featureIDs.insert(f->id);

			if (!p.second)
				continue;

			DrawFeatureColVol(f);
		}
	}

private:
	spring::unordered_set<int>    unitIDs;
	spring::unordered_set<int> featureIDs;
};



namespace DebugColVolDrawer
{
	bool enable = false;

	static CDebugColVolQuadDrawer cvDrawer;


	void Init() {
		MESH_COLVOL_BUFFERS[0] = 20; // cylinder divs
		MESH_COLVOL_BUFFERS[1] = 20; // sphere rows
		MESH_COLVOL_BUFFERS[2] = 20; // sphere cols

		gleGenColVolMeshBuffers(&MESH_COLVOL_BUFFERS[0]);
	}
	void Kill() {
		gleDelColVolMeshBuffers(&MESH_COLVOL_BUFFERS[0]);
	}

	void Draw() {
		if (!enable)
			return;

		cvDrawer.ResetState();
		cvDrawer.Enable();
		readMap->GridVisibility(nullptr, &cvDrawer, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);
		cvDrawer.Disable();
	}
}


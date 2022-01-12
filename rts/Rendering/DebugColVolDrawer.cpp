/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugColVolDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "System/UnorderedSet.hpp"


static constexpr float4 DEFAULT_VOLUME_COLOR = float4(0.45f, 0.0f, 0.45f, 0.35f);


static inline void DrawCollisionVolume(const CollisionVolume* vol, Shader::IProgramObject* s, CMatrix44f m)
{
	switch (vol->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// scaled sphere is special case of ellipsoid: radius, slices, stacks
			m.Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
			m.Scale({vol->GetHScale(0), vol->GetHScale(1), vol->GetHScale(2)});

			s->SetUniformMatrix4fv(0, false, m);

			gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);
		} break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER: {
			// scaled cylinder: base-radius, top-radius, height, slices, stacks
			//
			// (cylinder base is drawn at unit center by default so add offset
			// by half major axis to visually match the mathematical situation,
			// height of the cylinder equals the unit's full major axis)
			switch (vol->GetPrimaryAxis()) {
				case CollisionVolume::COLVOL_AXIS_X: {
					m.Translate({-(vol->GetHScale(0)), 0.0f, 0.0f});
					m.Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
					m.Scale({vol->GetScale(0), vol->GetHScale(1), vol->GetHScale(2)});
					m.RotateY(-90.0f * math::DEG_TO_RAD);
				} break;
				case CollisionVolume::COLVOL_AXIS_Y: {
					m.Translate({0.0f, -(vol->GetHScale(1)), 0.0f});
					m.Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
					m.Scale({vol->GetHScale(0), vol->GetScale(1), vol->GetHScale(2)});
					m.RotateX(90.0f * math::DEG_TO_RAD);
				} break;
				case CollisionVolume::COLVOL_AXIS_Z: {
					m.Translate({0.0f, 0.0f, -(vol->GetHScale(2))});
					m.Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
					m.Scale({vol->GetHScale(0), vol->GetHScale(1), vol->GetScale(2)});
				} break;
			}

			s->SetUniformMatrix4fv(0, false, m);

			gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_CYL);
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			// scaled cube: length, width, height
			m.Translate({vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2)});
			m.Scale({vol->GetScale(0), vol->GetScale(1), vol->GetScale(2)});

			s->SetUniformMatrix4fv(0, false, m);

			gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_BOX);
		} break;
	}
}



/*
static void DrawUnitDebugPieceTree(CMatrix44f m, const LocalModelPiece* lmp, const LocalModelPiece* lap, int lapf)
{
	CMatrix44f pm = m * lmp->GetModelSpaceMatrix();

		if (lmp->scriptSetVisible && !lmp->GetCollisionVolume()->IgnoreHits()) {
			if ((lmp == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150)))
				shader->SetUniform("u_color_rgba", (1.0f - ((gs->frameNum - lapf) / 150.0f)), 0.0f, 0.0f);

			// factors in the volume offsets
			DrawCollisionVolume(lmp->GetCollisionVolume(), s, pm);

			if ((lmp == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150)))
				shader->SetUniform4fv("u_color_rgba", DEFAULT_VOLUME_COLOR);
		}

	for (unsigned int i = 0; i < lmp->children.size(); i++) {
		DrawUnitDebugPieceTree(pm, lmp->children[i], lap, lapf);
	}
}
*/

static void DrawObjectDebugPieces(const CSolidObject* o, Shader::IProgramObject* s, CMatrix44f m)
{
	const int hitDeltaTime = gs->frameNum - o->pieceHitFrames[true];
	const int setFadeColor = (o->pieceHitFrames[true] > 0 && hitDeltaTime < 150);

	for (unsigned int n = 0; n < o->localModel.pieces.size(); n++) {
		const LocalModelPiece* lmp = o->localModel.GetPiece(n);
		const CollisionVolume* lmpVol = lmp->GetCollisionVolume();

		if (!lmp->scriptSetVisible || lmpVol->IgnoreHits())
			continue;

		if (setFadeColor && lmp == o->hitModelPieces[true])
			s->SetUniform4f(3, 1.0f - (hitDeltaTime / 150.0f), 0.0f, 0.0f, 1.0f);

		// factors in the volume offsets
		DrawCollisionVolume(lmpVol, s, m * lmp->GetModelSpaceMatrix());

		if (setFadeColor && lmp == o->hitModelPieces[true])
			s->SetUniform4fv(3, DEFAULT_VOLUME_COLOR);
	}
}



static inline void DrawObjectMidAndAimPos(const CSolidObject* o, Shader::IProgramObject* s, CMatrix44f& m)
{
	glAttribStatePtr->DisableDepthTest();

	if (o->aimPos != o->midPos) {
		// draw the aim-point
		m.Translate(o->relAimPos);

		s->SetUniform4f(3, 1.0f, 0.0f, 0.0f, 0.35f);
		s->SetUniformMatrix4fv(0, false, m);

		gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);

		m.Translate(-o->relAimPos);
	}

	{
		// draw the mid-point, keep this transform on the stack
		m.Translate(o->relMidPos);

		s->SetUniform4f(3, 1.0f, 0.0f, 1.0f, 0.35f);
		s->SetUniformMatrix4fv(0, false, m);

		gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);

		s->SetUniform4fv(3, DEFAULT_VOLUME_COLOR);
	}

	glAttribStatePtr->EnableDepthTest();
}



static inline void DrawFeatureColVol(const CFeature* f, Shader::IProgramObject* s)
{
	if (f->IsInVoid())
		return;
	if (!f->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
		return;
	if (!camera->InView(f->pos, f->GetDrawRadius()))
		return;

	CMatrix44f m = f->GetTransformMatrixRef(false);
	const CollisionVolume* v = f->GetCollisionVolume(nullptr);

	{
		DrawObjectMidAndAimPos(f, s, m);

		if (v->DefaultToPieceTree()) {
			// draw only the piece volumes for less clutter
			// note: relMidPos transform is on the stack at this
			// point but all piece-positions are relative to pos
			// --> undo it
			m.Translate(-f->relMidPos);
			DrawObjectDebugPieces(f, s, m);
			m.Translate(f->relMidPos);
		} else {
			if (!v->IgnoreHits())
				DrawCollisionVolume(v, s, m);
		}

		if (v->HasCustomType() || v->HasCustomProp(f->radius)) {
			m.Scale({f->radius, f->radius, f->radius});

			// assume this is a custom volume; draw radius-sphere next to it
			s->SetUniform4f(3, 0.5f, 0.5f, 0.5f, 0.35f);
			s->SetUniformMatrix4fv(0, false, m);

			gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);
		}
	}
}

static inline void DrawUnitColVol(const CUnit* u, Shader::IProgramObject* s)
{
	if (u->IsInVoid())
		return;
	if (!(u->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
		return;
	if (!camera->InView(u->drawMidPos, u->GetDrawRadius()))
		return;

	CMatrix44f m;
	const CollisionVolume* v = u->GetCollisionVolume(nullptr);

	glAttribStatePtr->DisableDepthTest();

	for (const CWeapon* w: u->weapons) {
		if (!w->HaveTarget())
			continue;

		m.LoadIdentity();
		m.Translate(w->aimFromPos);

		s->SetUniform4f(3, 1.0f, 1.0f, 0.0f, 0.4f);
		s->SetUniformMatrix4fv(0, false, m);

		gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);


		m.Translate(-w->aimFromPos);
		m.Translate(w->weaponMuzzlePos);

		s->SetUniform4f(3, 1.0f, 0.0f, 1.0f, 0.4f);
		s->SetUniformMatrix4fv(0, false, m);

		gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);

		m.Translate(-w->weaponMuzzlePos);
		m.Translate(w->GetCurrentTargetPos());


		s->SetUniform4f(3, 0.0f, 1.0f, 1.0f, 0.4f);
		s->SetUniformMatrix4fv(0, false, m);
		gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);

		m.Translate(-w->GetCurrentTargetPos());
	}

	glAttribStatePtr->EnableDepthTest();

	{
		DrawObjectMidAndAimPos(u, s, m = u->GetTransformMatrix(false));

		if (v->DefaultToPieceTree()) {
			// draw only the piece volumes for less clutter
			// note: relMidPos transform is on the stack at this
			// point but all piece-positions are relative to pos
			// --> undo it
			m.Translate(-u->relMidPos);
			DrawObjectDebugPieces(u, s, m);
			m.Translate(u->relMidPos);
		} else {
			if (!v->IgnoreHits()) {
				const int hitDeltaTime = gs->frameNum - u->lastAttackFrame;
				const int setFadeColor = (u->lastAttackFrame > 0 && hitDeltaTime < 150);

				// make it fade red under attack
				if (setFadeColor)
					s->SetUniform4f(3, 1.0f - (hitDeltaTime / 150.0f), 0.0f, 0.0f, 1.0f);

				// if drawing this, disable the DrawObjectMidAndAimPos call
				// DrawCollisionVolume((u->localModel).GetBoundingVolume(), s, m);
				DrawCollisionVolume(v, s, m);

				if (setFadeColor)
					s->SetUniform4fv(3, DEFAULT_VOLUME_COLOR);
			}
		}

		if (u->shieldWeapon != nullptr) {
			const CPlasmaRepulser* shieldWeapon = static_cast<const CPlasmaRepulser*>(u->shieldWeapon);
			const CollisionVolume* shieldColVol = &shieldWeapon->collisionVolume;

			s->SetUniform4f(3, 0.0f, 0.0f, 0.6f, 0.35f);

			DrawCollisionVolume(shieldColVol, s, CMatrix44f{shieldWeapon->weaponMuzzlePos});
		}

		if (v->HasCustomType() || v->HasCustomProp(u->radius)) {
			m.Scale({u->radius, u->radius, u->radius});

			// assume this is a custom volume; draw radius-sphere next to it
			s->SetUniform4f(3, 0.5f, 0.5f, 0.5f, 0.35f);
			s->SetUniformMatrix4fv(0, false, m);

			gleDrawMeshSubBuffer(&COLVOL_MESH_BUFFERS[0], GLE_MESH_TYPE_SPH);
		}
	}
}


class CDebugColVolQuadDrawer : public CReadMap::IQuadDrawer {
public:
	void ResetState() override {
		unitIDs.clear();
		unitIDs.reserve(32);
		featureIDs.clear();
		featureIDs.reserve(32);
	}

	void Init(Shader::GLSLProgramObject* shader) { ipo = shader; }
	void Kill() { ipo = nullptr; }

	void Enable() {
		ipo->Enable();
		ipo->SetUniformMatrix4fv(1, false, camera->GetViewMatrix());
		ipo->SetUniformMatrix4fv(2, false, camera->GetProjectionMatrix());

		glAttribStatePtr->PushBits(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);
		glAttribStatePtr->DisableCullFace();

		glAttribStatePtr->EnableBlendMask();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glAttribStatePtr->EnableDepthMask();

		gleBindMeshBuffers(&COLVOL_MESH_BUFFERS[0]);
	}
	void Disable() {
		gleBindMeshBuffers(nullptr);

		glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glAttribStatePtr->PopBits();

		ipo->Disable();
	}

	void DrawQuad(int x, int y) override {
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);

		for (const CUnit* u: q.units) {
			const auto& p = unitIDs.insert(u->id);

			if (!p.second)
				continue;

			DrawUnitColVol(u, ipo);
		}

		for (const CFeature* f: q.features) {
			const auto& p = featureIDs.insert(f->id);

			if (!p.second)
				continue;

			DrawFeatureColVol(f, ipo);
		}
	}

private:
	spring::unordered_set<int>    unitIDs;
	spring::unordered_set<int> featureIDs;

	Shader::IProgramObject* ipo = nullptr;
};



namespace DebugColVolDrawer
{
	bool enable = false;

	static CDebugColVolQuadDrawer cvDrawer;
	static Shader::GLSLProgramObject cvShader;

	void InitShader() {
		const std::string& vsText = Shader::GetShaderSource("GLSL/ColVolDebugVertProg.glsl");
		const std::string& fsText = Shader::GetShaderSource("GLSL/ColVolDebugFragProg.glsl");

		Shader::GLSLShaderObject vsShaderObj = {GL_VERTEX_SHADER, vsText, ""};
		Shader::GLSLShaderObject fsShaderObj = {GL_FRAGMENT_SHADER, fsText, ""};

		cvShader.AttachShaderObject(&vsShaderObj);
		cvShader.AttachShaderObject(&fsShaderObj);
		cvShader.ReloadShaderObjects();
		cvShader.CreateAndLink();
		cvShader.RecalculateShaderHash();
		cvShader.ClearAttachedShaderObjects();
		cvShader.Validate();
		cvShader.SetUniformLocation("u_mesh_mat"  ); // idx 0
		cvShader.SetUniformLocation("u_view_mat"  ); // idx 1
		cvShader.SetUniformLocation("u_proj_mat"  ); // idx 2
		cvShader.SetUniformLocation("u_color_rgba"); // idx 3
	}
	void InitBuffers() {
		memcpy(&COLVOL_MESH_BUFFERS[0], &COLVOL_MESH_PARAMS[0], GLE_MESH_TYPE_CNT * sizeof(COLVOL_MESH_PARAMS[0]));
		gleGenMeshBuffers(&COLVOL_MESH_BUFFERS[0]);
	}

	void KillShader() { cvShader.Release(false); }
	void KillBuffers() {
		gleDelMeshBuffers(&COLVOL_MESH_BUFFERS[0]);
		memcpy(&COLVOL_MESH_BUFFERS[0], &COLVOL_MESH_PARAMS[0], GLE_MESH_TYPE_CNT * sizeof(COLVOL_MESH_PARAMS[0]));
	}

	void Init() {
		InitShader();
		InitBuffers();

		cvDrawer.Init(&cvShader);
		shaderHandler->InsertExtProgramObject("[DebugColVolDrawer]", &cvShader);
	}
	void Kill() {
		KillShader();
		KillBuffers();

		shaderHandler->RemoveExtProgramObject("[DebugColVolDrawer]", &cvShader);
		cvDrawer.Kill();
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


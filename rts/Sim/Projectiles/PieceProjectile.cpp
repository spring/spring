/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Colors.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Models/3DOParser.h"
#include "Rendering/Models/S3OParser.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/Sync/SyncTracer.h"
#include "System/Util.h"

#define SMOKE_TIME 40
#define NUM_TRAIL_PARTS (sizeof(fireTrailPoints) / sizeof(fireTrailPoints[0]))

CR_BIND_DERIVED(CPieceProjectile, CProjectile, (NULL, NULL, ZeroVector, ZeroVector, 0, 0));

CR_REG_METADATA(CPieceProjectile,(
	CR_SETFLAG(CF_Synced),

	CR_MEMBER(age),
	CR_MEMBER(explFlags),
	CR_MEMBER(dispList),
	// NOTE: what about this?
	// CR_MEMBER(omp),
	CR_MEMBER(curCallback),
	CR_MEMBER(spinVec),
	CR_MEMBER(spinSpeed),
	CR_MEMBER(spinAngle),
	CR_MEMBER(oldSmokePos),
	CR_MEMBER(oldSmokeDir),
	CR_MEMBER(alphaThreshold),
	// CR_MEMBER(target),
	CR_MEMBER(drawTrail),

	CR_RESERVED(36)
));

CPieceProjectile::CPieceProjectile(
	CUnit* owner,
	LocalModelPiece* lmp,
	const float3& pos,
	const float3& speed,
	int flags,
	float radius
):
	CProjectile(pos, speed, owner, true, false, true),

	age(0),
	explFlags(flags),
	dispList((lmp != NULL)? lmp->dispListID: 0),

	omp(NULL),
	curCallback(NULL),

	spinSpeed(0.0f),
	spinAngle(0.0f),
	alphaThreshold(0.1f),

	oldSmokePos(pos),
	oldSmokeDir(FwdVector),

	drawTrail(true)
{
	checkCol = false;

	if (owner != NULL) {
		if ((explFlags & PF_NoCEGTrail) == 0) {
			explGenHandler->GenExplosion((cegID = owner->unitDef->GetPieceExplosionGeneratorID(gs->randInt())), pos, speed, 100, 0.0f, 0.0f, NULL, NULL);
		}

		model = owner->model;
		alphaThreshold = owner->alphaThreshold;
		explFlags |= (PF_NoCEGTrail * (cegID == -1u));
	}

	if (lmp) {
		omp = lmp->original;
	} else {
		omp = NULL;
	}

	castShadow = true;

	if (pos.y - ground->GetApproximateHeight(pos.x, pos.z) > 10) {
		useAirLos = true;
	}

	oldSmokeDir = speed;
	oldSmokeDir.Normalize();


	const float3 camVec = pos - camera->GetPos();
	const float  camDst = camVec.Length() + 0.01f;
	const float3 camDir = camVec / camDst;

	drawTrail = ((camDst + (1.0f - math::fabs(camDir.dot(oldSmokeDir))) * 3000.0f) >= 200.0f);

	// neither spinVec nor spinSpeed technically
	// needs to be synced, but since instances of
	// this class are themselves synced and have
	// LuaSynced{Ctrl, Read} exposure we treat
	// them that way for consistency
	spinVec = gs->randVector();
	spinVec.Normalize();
	spinSpeed = gs->randFloat() * 20;

	for (unsigned int a = 0; a < NUM_TRAIL_PARTS; ++a) {
		fireTrailPoints[a] = new FireTrailPoint();
		fireTrailPoints[a]->pos = pos;
		fireTrailPoints[a]->size = gu->RandFloat() * 2 + 2;
	}

	SetRadiusAndHeight(radius, 0.0f);
	drawRadius = 32.0f;

#ifdef TRACE_SYNC
	tracefile << "New CPieceProjectile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	projectileHandler->AddProjectile(this);
}

void CPieceProjectile::Detach()
{
	// SYNCED
	if (curCallback) {
		// this is unsynced, but it prevents some callback crash on exit
		curCallback->drawCallbacker = NULL;
	}

	CProjectile::Detach();
}

CPieceProjectile::~CPieceProjectile()
{
	// UNSYNCED
	for (unsigned int a = 0; a < NUM_TRAIL_PARTS; ++a) {
		delete fireTrailPoints[a];
	}
}


void CPieceProjectile::Collision()
{
	const float3& norm = ground->GetNormal(pos.x, pos.z);
	const float ns = speed.dot(norm);

	SetVelocityAndSpeed(speed - (norm * ns * 1.6f));
	SetPosition(pos + (norm * 0.1f));

	if (explFlags & PF_Explode) {
		const DamageArray damageArray(50.0f);
		const CGameHelper::ExplosionParams params = {
			pos,
			ZeroVector,
			damageArray,
			NULL,              // weaponDef
			owner(),
			NULL,              // hitUnit
			NULL,              // hitFeature
			5.0f,              // craterAreaOfEffect
			5.0f,              // damageAreaOfEffect
			0.0f,              // edgeEffectiveness
			10.0f,             // explosionSpeed
			1.0f,              // gfxMod
			false,             // impactOnly
			false,             // ignoreOwner
			true,              // damageGround
			static_cast<unsigned int>(id)
		};

		helper->Explosion(params);
	}
	if (explFlags & PF_Smoke) {
		if (explFlags & PF_NoCEGTrail) {
			CSmokeTrailProjectile* tp = new CSmokeTrailProjectile(
				owner(),
				pos, oldSmokePos,
				dir, oldSmokeDir,
				false,
				true,
				(NUM_TRAIL_PARTS - 1),
				SMOKE_TIME,
				0.5f,
				drawTrail,
				NULL,
				projectileDrawer->smoketrailtex);

			tp->creationTime += (NUM_TRAIL_PARTS - (age & (NUM_TRAIL_PARTS - 1)));
		}
	}

	// keep flying with some small probability
	if (gs->randFloat() < 0.666f) {
		CProjectile::Collision();
	}

	oldSmokePos = pos;
}

void CPieceProjectile::Collision(CUnit* unit)
{
	if (unit == owner())
		return;

	if (explFlags & PF_Explode) {
		const DamageArray damageArray(50.0f);
		const CGameHelper::ExplosionParams params = {
			pos,
			ZeroVector,
			damageArray,
			NULL,              // weaponDef
			owner(),
			unit,              // hitUnit
			NULL,              // hitFeature
			5.0f,              // craterAreaOfEffect
			5.0f,              // damageAreaOfEffect
			0.0f,              // edgeEffectiveness
			10.0f,             // explosionSpeed
			1.0f,              // gfxMod
			false,             // impactOnly
			false,             // ignoreOwner
			true,              // damageGround
			static_cast<unsigned int>(id)
		};

		helper->Explosion(params);
	}
	if (explFlags & PF_Smoke) {
		if (explFlags & PF_NoCEGTrail) {
			CSmokeTrailProjectile* tp = new CSmokeTrailProjectile(
				owner(),
				pos, oldSmokePos,
				dir, oldSmokeDir,
				false,
				true,
				NUM_TRAIL_PARTS - 1,
				SMOKE_TIME,
				0.5f,
				drawTrail,
				NULL,
				projectileDrawer->smoketrailtex
			);

			tp->creationTime += (NUM_TRAIL_PARTS - (age & (NUM_TRAIL_PARTS - 1)));
		}
	}

	CProjectile::Collision(unit);
	oldSmokePos = pos;
}


bool CPieceProjectile::HasVertices() const
{
	if (omp == NULL)
		return false;

	return (omp->GetVertexCount() > 0);
}

float3 CPieceProjectile::RandomVertexPos()
{
	if (!HasVertices())
		return ZeroVector;

	const int vertexNum = (int) (gu->RandFloat() * 0.99f * omp->GetVertexCount());
	const float3& pos = omp->GetVertexPos(vertexNum);

	return pos;
}


void CPieceProjectile::Update()
{
	if (!luaMoveCtrl) {
		if (explFlags & PF_Fall)
			speed.y += mygravity;

		SetVelocityAndSpeed(speed * 0.997f);
		SetPosition(pos + speed);
	}

	spinAngle += spinSpeed;
	age += 1;
	checkCol |= (age > 10);

	if ((explFlags & PF_Fire) != 0 && HasVertices()) {
		FireTrailPoint* tempTrailPoint = fireTrailPoints[NUM_TRAIL_PARTS - 1];

		for (int a = NUM_TRAIL_PARTS - 2; a >= 0; --a) {
			fireTrailPoints[a + 1] = fireTrailPoints[a];
		}

		CMatrix44f m(pos);
		m.Rotate(spinAngle * PI / 180.0f, spinVec);
		m.Translate(RandomVertexPos());

		fireTrailPoints[0] = tempTrailPoint;
		fireTrailPoints[0]->pos = m.GetPos();
		fireTrailPoints[0]->size = gu->RandFloat() * 1 + 1;
	}

	if ((explFlags & PF_NoCEGTrail) == 0) {
		// TODO: pass a more sensible ttl to the CEG (age-related?)
		explGenHandler->GenExplosion(cegID, pos, speed, 100, 0.0f, 0.0f, NULL, NULL);
		return;
	}

	if ((explFlags & PF_Smoke) != 0) {
		if ((age & (NUM_TRAIL_PARTS - 1)) == 0) {
			if (curCallback != NULL) {
				curCallback->drawCallbacker = NULL;
			}

			curCallback = new CSmokeTrailProjectile(
				owner(),
				pos, oldSmokePos,
				dir, oldSmokeDir,
				age == (NUM_TRAIL_PARTS - 1),
				false,
				14,
				SMOKE_TIME,
				0.5f,
				drawTrail,
				this,
				projectileDrawer->smoketrailtex
			);

			useAirLos = curCallback->useAirLos;

			oldSmokePos = pos;
			oldSmokeDir = dir;

			if (!drawTrail) {
				float3 camDir = pos - camera->GetPos();

				const float camPieceDist = camDir.LengthNormalize();
				const float camAngleScale = (1.0f - math::fabs(camDir.dot(dir))) * 3000.0f;

				drawTrail = ((camPieceDist + camAngleScale) >= 300.0f);
			}
		}
	}
}



void CPieceProjectile::Draw()
{
	if ((explFlags & PF_NoCEGTrail) == 0)
		return;

	if ((explFlags & PF_Smoke) != 0) {
		// this piece leaves a default (non-CEG) smoketrail
		inArray = true;

		const int numParts = age & (NUM_TRAIL_PARTS - 1);
		float age2 = (age & (NUM_TRAIL_PARTS - 1)) + globalRendering->timeOffset;
		float color = 0.5f;
		unsigned char col[4];

		va->EnlargeArrays(4+4*numParts,0,VA_SIZE_TC);

		if (drawTrail) {
			// draw the trail as a single quad if camera close enough
			const float3 dif  = (drawPos - camera->GetPos()).Normalize();
			const float3 dir1 = (dif.cross(dir)).Normalize();
			const float3 dif2 = (oldSmokePos - camera->GetPos()).Normalize();
			const float3 dir2 = (dif2.cross(oldSmokeDir)).Normalize();

			float a1 = ((1 - 0.0f / SMOKE_TIME) * 255) * (0.7f + math::fabs(dif.dot(dir)));
			float alpha = std::min(255.0f, std::max(0.0f, a1));
			col[0] = (unsigned char) (color * alpha);
			col[1] = (unsigned char) (color * alpha);
			col[2] = (unsigned char) (color * alpha);
			col[3] = (unsigned char) (alpha);

			unsigned char col2[4];
			float a2 = ((1 - float(age2) / SMOKE_TIME) * 255) * (0.7f + math::fabs(dif2.dot(oldSmokeDir)));

			if (age < NUM_TRAIL_PARTS)
				a2 = 0.0f;

			alpha = std::min(255.0f, std::max(0.0f, a2));
			col2[0] = (unsigned char) (color * alpha);
			col2[1] = (unsigned char) (color * alpha);
			col2[2] = (unsigned char) (color * alpha);
			col2[3] = (unsigned char) (alpha);

			const float size = 1.0f;
			const float size2 = 1 + (age2 * (1.0f / SMOKE_TIME)) * 14;
			const float txs =
				projectileDrawer->smoketrailtex->xstart -
				(projectileDrawer->smoketrailtex->xend - projectileDrawer->smoketrailtex->xstart) *
				(age2 / NUM_TRAIL_PARTS);

			va->AddVertexQTC(drawPos - dir1 * size, txs, projectileDrawer->smoketrailtex->ystart, col);
			va->AddVertexQTC(drawPos + dir1 * size, txs, projectileDrawer->smoketrailtex->yend,   col);
			va->AddVertexQTC(oldSmokePos + dir2 * size2, projectileDrawer->smoketrailtex->xend, projectileDrawer->smoketrailtex->yend,   col2);
			va->AddVertexQTC(oldSmokePos - dir2 * size2, projectileDrawer->smoketrailtex->xend, projectileDrawer->smoketrailtex->ystart, col2);
		} else {
			// draw the trail as particles
			const float dist = pos.distance(oldSmokePos);
			const float3 dirpos1 = pos - dir * dist * 0.33f;
			const float3 dirpos2 = oldSmokePos + oldSmokeDir * dist * 0.33f;

			// CAUTION: loop count must match EnlargeArrays above
			for (int a = 0; a < numParts; ++a) {
				float alpha = 255.0f;
				col[0] = (unsigned char) (color * alpha);
				col[1] = (unsigned char) (color * alpha);
				col[2] = (unsigned char) (color * alpha);
				col[3] = (unsigned char) (alpha);

				const float size = 1.0f + ((a) * (1.0f / SMOKE_TIME)) * 14.0f;
				const float3 pos1 = CalcBeizer(float(a) / (numParts), pos, dirpos1, dirpos2, oldSmokePos);

				#define st projectileDrawer->smoketex[0]
				va->AddVertexQTC(pos1 + ( camera->up+camera->right) * size, st->xstart, st->ystart, col);
				va->AddVertexQTC(pos1 + ( camera->up-camera->right) * size, st->xend,   st->ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up-camera->right) * size, st->xend,   st->ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up+camera->right) * size, st->xstart, st->ystart, col);
				#undef st
			}
		}
	}

	DrawCallback();
}

void CPieceProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::red);
}

void CPieceProjectile::DrawCallback()
{
	inArray = true;

	// fireTrailPoints contains nothing useful if piece is empty
	if ((explFlags & PF_Fire) != 0 && HasVertices()) {
		va->EnlargeArrays(NUM_TRAIL_PARTS * 4, 0, VA_SIZE_TC);

		for (unsigned int age = 0; age < NUM_TRAIL_PARTS; ++age) {
			const float3 interPos = fireTrailPoints[age]->pos;
			const float size = fireTrailPoints[age]->size;
			const float modage = age * 1.0f;

			const float alpha = (7.5f - modage) * (1.0f / NUM_TRAIL_PARTS);
			const float drawsize = (0.5f + modage) * size;

			unsigned char col[4];

			col[0] = (unsigned char) (255 * alpha);
			col[1] = (unsigned char) (200 * alpha);
			col[2] = (unsigned char) (150 * alpha);
			col[3] = (unsigned char) (alpha * 50);

			#define eft projectileDrawer->explofadetex
			va->AddVertexQTC(interPos - camera->right * drawsize-camera->up * drawsize, eft->xstart, eft->ystart, col);
			va->AddVertexQTC(interPos + camera->right * drawsize-camera->up * drawsize, eft->xend,   eft->ystart, col);
			va->AddVertexQTC(interPos + camera->right * drawsize+camera->up * drawsize, eft->xend,   eft->yend,   col);
			va->AddVertexQTC(interPos - camera->right * drawsize+camera->up * drawsize, eft->xstart, eft->yend,   col);
			#undef eft
		}
	}
}

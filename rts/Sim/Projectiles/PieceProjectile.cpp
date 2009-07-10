#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Colors.h"
#include "Rendering/UnitModels/IModelParser.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/Unit.h"
#include "Sync/SyncTracer.h"
#include "Util.h"

static const float Smoke_Time = 40;

CR_BIND_DERIVED(CPieceProjectile, CProjectile, (float3(0, 0, 0), float3(0, 0, 0), NULL, 0, NULL, 0));

CR_REG_METADATA(CPieceProjectile,(
	CR_SERIALIZER(creg_Serialize), // numCallback, oldInfos
	CR_MEMBER(flags),
	CR_MEMBER(dispList),
	// TODO what to do with the next three fields
	// CR_MEMBER(omp),
	// CR_MEMBER(piece3do),
	// CR_MEMBER(pieces3o),
	CR_MEMBER(spinVec),
	CR_MEMBER(spinSpeed),
	CR_MEMBER(spinAngle),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldSmokeDir),
	CR_MEMBER(alphaThreshold),
	// CR_MEMBER(target),
	CR_MEMBER(drawTrail),
	CR_MEMBER(curCallback),
	CR_MEMBER(age),
	CR_MEMBER(colorTeam),
	CR_MEMBER(ceg),
	CR_RESERVED(36)
	));

void CPieceProjectile::creg_Serialize(creg::ISerializer& s)
{
	s.Serialize(numCallback, sizeof(int));
	for (int i = 0; i < 8; i++) {
		s.Serialize(oldInfos[i], sizeof(CPieceProjectile::OldInfo));
	}
}

CPieceProjectile::CPieceProjectile(const float3& pos, const float3& speed, LocalModelPiece* piece, int f, CUnit* owner, float radius GML_PARG_C):
	CProjectile(pos, speed, owner, true, false, true GML_PARG_P),
	flags(f),
	dispList(piece? piece->displist: 0),
	omp(NULL),
	spinAngle(0.0f),
	alphaThreshold(0.1f),
	oldSmoke(pos),
	drawTrail(true),
	curCallback(0),
	age(0)
{
	checkCol = false;

	if (owner) {
		// choose a (synced) random tag-postfix string k from the
		// range given in UnitDef and stick it onto pieceTrailCEGTag
		// (assumes all possible "tag + k" CEG identifiers are valid)
		// if this piece does not override the FBI and wants a trail
		if ((flags & PF_NoCEGTrail) == 0) {
			const int size = owner->unitDef->pieceTrailCEGTag.size();
			const int range = owner->unitDef->pieceTrailCEGRange;
			const int num = gs->randInt() % range;
			const char* tag = owner->unitDef->pieceTrailCEGTag.c_str();

			if (size > 0) {
				char cegTag[1024];
				SNPRINTF(cegTag, sizeof(cegTag) - 1, "%s%d", tag, num);
				cegTag[1023] = 0;
				ceg.Load(explGenHandler, cegTag);
			} else {
				flags |= PF_NoCEGTrail;
			}
		}

		/* If we're an S3O unit, this is where ProjectileHandler
		   fetches our texture from. */
		s3domodel = owner->model;
		/* If we're part of an S3O unit, save this so we can
		   draw with the right teamcolour. */
		colorTeam = owner->team;
		// copy the owner's alphaThreshold value
		alphaThreshold = owner->alphaThreshold;
	}

	/* Don't store piece; owner may be a dying unit, so piece could be freed. */

	/* //if (piece->texturetype == 0) {
	   Great. LocalPiece doesn't carry the texture name.

	   HACK TODO PieceProjectile shouldn't need to know about
	   different model formats; push it into Rendering/UnitModels.

	   If this needs to change, also modify Sim/Units/COB/CobInstance.cpp::Explosion.

	   Nothing else wants to draw just one part without PieceInfo, so this
	   polymorphism can stay put for the moment.
	   */
	if (piece) {
		if (piece->type == MODELTYPE_3DO) {
			piece3do = (S3DOPiece*) piece->original;
			pieces3o = NULL;
		} else if (piece->type == MODELTYPE_S3O) {
			piece3do = NULL;
			pieces3o = (SS3OPiece*) piece->original;
		}
		omp = piece->original;
	} else {
		piece3do = NULL;
		pieces3o = NULL;
	}

	castShadow = true;

	if (pos.y - ground->GetApproximateHeight(pos.x, pos.z) > 10) {
		useAirLos = true;
	}

	numCallback = new int;
	*numCallback = 0;
	oldSmokeDir = speed;
	oldSmokeDir.Normalize();
	float3 camDir = (pos-camera->pos).Normalize();

	if (camera->pos.distance(pos) + (1 - fabs(camDir.dot(oldSmokeDir))) * 3000 < 200) {
		drawTrail = false;
	}

	//! neither spinVec nor spinSpeed technically
	//! needs to be synced, but since instances of
	//! this class are themselves synced and have
	//! LuaSynced{Ctrl, Read} exposure we treat
	//! them that way for consistency
	spinVec = gs->randVector();
	spinVec.Normalize();
	spinSpeed = gs->randFloat() * 20;

	for (int a = 0; a < 8; ++a) {
		oldInfos[a] = new OldInfo;
		oldInfos[a]->pos = pos;
		oldInfos[a]->size = gu->usRandFloat() * 2 + 2;
	}

	SetRadius(radius);
	drawRadius = 32;

#ifdef TRACE_SYNC
	tracefile << "New CPieceProjectile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	ph->AddProjectile(this);
}


CPieceProjectile::~CPieceProjectile(void)
{
	delete numCallback;

	if (curCallback)
		curCallback->drawCallbacker = 0;

	for (int a = 0; a < 8; ++a) {
		delete oldInfos[a];
	}
}


void CPieceProjectile::Collision()
{
	if (speed.SqLength() > Square(gs->randFloat() * 5 + 1) && pos.y > radius + 2) {
		float3 norm = ground->GetNormal(pos.x, pos.z);
		float ns = speed.dot(norm);
		speed -= norm * ns * 1.6f;
		pos += norm * 0.1f;
	} else {
		if (flags & PF_Explode) {
			helper->Explosion(pos, DamageArray() * 50, 5, 0, 10, owner(), false, 1.0f, false, false, 0, 0, ZeroVector, -1);
		}
		if (flags & PF_Smoke) {
			if (flags & PF_NoCEGTrail) {
				float3 dir = speed;
				dir.Normalize();

				CSmokeTrailProjectile* tp =
					new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir, owner(),
					false, true, 7, Smoke_Time, 0.5f, drawTrail, 0, &ph->smoketrailtex);
				tp->creationTime += (8 - ((age) & 7));
			}
		}

		CProjectile::Collision();
		oldSmoke = pos;
	}
}

void CPieceProjectile::Collision(CUnit* unit)
{
	if (unit == owner())
		return;
	if (flags & PF_Explode) {
		helper->Explosion(pos, DamageArray() * 50, 5, 0, 10, owner(), false, 1.0f, false, false, 0, unit, ZeroVector, -1);
	}
	if (flags & PF_Smoke) {
		if (flags & PF_NoCEGTrail) {
			float3 dir = speed;
			dir.Normalize();

			CSmokeTrailProjectile* tp =
				new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir, owner(),
				false, true, 7, Smoke_Time, 0.5f, drawTrail, 0, &ph->smoketrailtex);
			tp->creationTime += (8 - ((age) & 7));
		}
	}

	CProjectile::Collision(unit);
	oldSmoke = pos;
}


bool CPieceProjectile::HasVertices(void)
{
	if (!piece3do && !pieces3o) return false;
	if (piece3do != NULL) {
		/* 3DO */
		return !piece3do->vertices.empty();
	}
	else {
		/* S3O */
		return !pieces3o->vertexDrawOrder.empty();
	}
}

float3 CPieceProjectile::RandomVertexPos(void)
{
	if (!piece3do && !pieces3o) return float3(0, 0, 0);
	float3 pos;

	if (piece3do != NULL) {
		/* 3DO */
		int vertexNum = (int) (gu->usRandFloat() * 0.99f * piece3do->vertices.size());
		pos = piece3do->vertices[vertexNum].pos;
	}
	else {
		/* S3O */
		int vertexNum = (int) (gu->usRandFloat() * 0.99f * pieces3o->vertexDrawOrder.size());
		pos = pieces3o->vertices[pieces3o->vertexDrawOrder[vertexNum]].pos;
	}

	return pos;
}


void CPieceProjectile::Update()
{
	if (flags & PF_Fall) {
		speed.y += gravity;
	}

	speed *= 0.997f;
	pos += speed;
	spinAngle += spinSpeed;

	if (flags & PF_Fire && HasVertices()) {
		OldInfo* tempOldInfo = oldInfos[7];
		for (int a = 6; a >= 0; --a) {
			oldInfos[a + 1] = oldInfos[a];
		}

		CMatrix44f m(pos);
		m.Rotate(spinAngle * PI / 180.0f, spinVec);
		float3 pos = RandomVertexPos();
		m.Translate(pos.x, pos.y, pos.z);

		oldInfos[0] = tempOldInfo;
		oldInfos[0]->pos = m.GetPos();
		oldInfos[0]->size = gu->usRandFloat() * 1 + 1;
	}

	age++;

	if (flags & PF_NoCEGTrail) {
		if (!(age & 7) && (flags & PF_Smoke)) {
			float3 dir = speed;
			dir.Normalize();

			if (curCallback)
				curCallback->drawCallbacker = 0;

			curCallback =
				new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir, owner(),
				age == 8, false, 14, Smoke_Time, 0.5f, drawTrail, this, &ph->smoketrailtex);
			useAirLos = curCallback->useAirLos;

			oldSmoke = pos;
			oldSmokeDir = dir;

			if (!drawTrail) {
				float3 camDir = (pos - camera->pos).Normalize();
				if (camera->pos.distance(pos) + (1 - fabs(camDir.dot(dir))) * 3000 > 300) {
					drawTrail = true;
				}
			}
		}
	} else {
		// TODO: pass a more sensible ttl to the CEG (age-related?)
		ceg.Explosion(pos, 100, 0.0f, 0x0, 0.0f, 0x0, speed);
	}

	if (age > 10)
		checkCol = true;
}



void CPieceProjectile::Draw()
{
	if (flags & PF_NoCEGTrail) {
		if (flags & PF_Smoke) {
			// this piece leaves a default (non-CEG) smoketrail
			inArray = true;
			float age2 = (age & 7) + gu->timeOffset;
			float color = 0.5f;
			unsigned char col[4];

			float3 dir = speed;
			dir.Normalize();

			int numParts = age & 7;
			va->EnlargeArrays(4+4*numParts,0,VA_SIZE_TC);
			if (drawTrail) {
				// draw the trail as a single quad if camera close enough
				float3 dif(drawPos - camera->pos);
				dif.Normalize();
				float3 dir1(dif.cross(dir));
				dir1.Normalize();
				float3 dif2(oldSmoke - camera->pos);
				dif2.Normalize();
				float3 dir2(dif2.cross(oldSmokeDir));
				dir2.Normalize();

				float a1 = ((1 - 0.0f / (Smoke_Time)) * 255) * (0.7f + fabs(dif.dot(dir)));
				float alpha = std::min(255.0f, std::max(0.f, a1));
				col[0] = (unsigned char) (color * alpha);
				col[1] = (unsigned char) (color * alpha);
				col[2] = (unsigned char) (color * alpha);
				col[3] = (unsigned char) (alpha);

				unsigned char col2[4];
				float a2 = ((1 - float(age2) / (Smoke_Time)) * 255) * (0.7f + fabs(dif2.dot(oldSmokeDir)));

				if (age < 8)
					a2 = 0;

				alpha = std::min(255.0f, std::max(0.f, a2));
				col2[0] = (unsigned char) (color * alpha);
				col2[1] = (unsigned char) (color * alpha);
				col2[2] = (unsigned char) (color * alpha);
				col2[3] = (unsigned char) (alpha);

				float size = 1.0f;
				float size2 = 1 + (age2 * (1 / Smoke_Time)) * 14;
				float txs = ph->smoketrailtex.xstart - (ph->smoketrailtex.xend - ph->smoketrailtex.xstart) * (age2 / 8.0f);

				va->AddVertexQTC(drawPos - dir1 * size, txs, ph->smoketrailtex.ystart, col);
				va->AddVertexQTC(drawPos + dir1 * size, txs, ph->smoketrailtex.yend,   col);
				va->AddVertexQTC(oldSmoke + dir2 * size2, ph->smoketrailtex.xend, ph->smoketrailtex.yend,   col2);
				va->AddVertexQTC(oldSmoke - dir2 * size2, ph->smoketrailtex.xend, ph->smoketrailtex.ystart, col2);
			} else {
				// draw the trail as particles
				float dist = pos.distance(oldSmoke);
				float3 dirpos1 = pos - dir * dist * 0.33f;
				float3 dirpos2 = oldSmoke + oldSmokeDir * dist * 0.33f;

				for (int a = 0; a < numParts; ++a) { //! CAUTION: loop count must match EnlargeArrays above
					float alpha = 255;
					col[0] = (unsigned char) (color * alpha);
					col[1] = (unsigned char) (color * alpha);
					col[2] = (unsigned char) (color * alpha);
					col[3] = (unsigned char) (alpha);
					float size = 1 + ((a) * (1 / Smoke_Time)) * 14;
					float3 pos1 = CalcBeizer(float(a) / (numParts), pos, dirpos1, dirpos2, oldSmoke);

					va->AddVertexQTC(pos1 + ( camera->up+camera->right) * size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
					va->AddVertexQTC(pos1 + ( camera->up-camera->right) * size, ph->smoketex[0].xend,   ph->smoketex[0].ystart, col);
					va->AddVertexQTC(pos1 + (-camera->up-camera->right) * size, ph->smoketex[0].xend,   ph->smoketex[0].ystart, col);
					va->AddVertexQTC(pos1 + (-camera->up+camera->right) * size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
				}
			}
		}
	}

	DrawCallback();
	if (curCallback == 0) {
		DrawCallback();
	}
}

void CPieceProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::red);
}

void CPieceProjectile::DrawCallback(void)
{
	if (*numCallback != gu->drawFrame) {
		*numCallback = gu->drawFrame;
		return;
	}
	*numCallback = 0;

	inArray = true;
	unsigned char col[4];

	if (flags & PF_Fire) {
		va->EnlargeArrays(8*4,0,VA_SIZE_TC);
		for (int age = 0; age < 8; ++age) {
			float modage = age;
			float3 interPos = oldInfos[age]->pos;
			float size = oldInfos[age]->size;

			float alpha = (7.5f - modage) * (1.0f / 8);
			col[0] = (unsigned char) (255 * alpha);
			col[1] = (unsigned char) (200 * alpha);
			col[2] = (unsigned char) (150 * alpha);
			col[3] = (unsigned char) (alpha * 50);
			float drawsize = (0.5f + modage) * size;

			va->AddVertexQTC(interPos - camera->right * drawsize-camera->up * drawsize, ph->explofadetex.xstart, ph->explofadetex.ystart, col);
			va->AddVertexQTC(interPos + camera->right * drawsize-camera->up * drawsize, ph->explofadetex.xend,   ph->explofadetex.ystart, col);
			va->AddVertexQTC(interPos + camera->right * drawsize+camera->up * drawsize, ph->explofadetex.xend,   ph->explofadetex.yend,   col);
			va->AddVertexQTC(interPos - camera->right * drawsize+camera->up * drawsize, ph->explofadetex.xstart, ph->explofadetex.yend,   col);
		}
	}
}


void CPieceProjectile::DrawUnitPart(void)
{
	unitDrawer->SetTeamColour(colorTeam);

	if (alphaThreshold != 0.1f) {
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glAlphaFunc(GL_GEQUAL, alphaThreshold);
	}
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glRotatef(spinAngle, spinVec.x, spinVec.y, spinVec.z);
	glCallList(dispList);
	glPopMatrix();

	if (alphaThreshold != 0.1f) {
		glPopAttrib();
	}

	*numCallback = 0;
}

void CPieceProjectile::DrawS3O(void)
{
	// copy of CWeaponProjectile::::DrawS3O()
	DrawUnitPart();
}

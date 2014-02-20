/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FireProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "System/creg/STL_List.h"

CR_BIND_DERIVED(CFireProjectile, CProjectile, (ZeroVector,ZeroVector,NULL,0,0,0,0));
CR_BIND(CFireProjectile::SubParticle, );

CR_REG_METADATA(CFireProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(ttl),
	CR_MEMBER(emitPos),
	CR_MEMBER(emitRadius),
	CR_MEMBER(particleTime),
	CR_MEMBER(particleSize),
	CR_MEMBER(ageSpeed),
	CR_MEMBER(subParticles2),
	CR_MEMBER(subParticles),
	CR_RESERVED(16)
	));

CR_REG_METADATA_SUB(CFireProjectile, SubParticle, (
	CR_MEMBER(pos),
	CR_MEMBER(posDif),
	CR_MEMBER(age),
	CR_MEMBER(maxSize),
	CR_MEMBER(rotSpeed),
	CR_MEMBER(smokeType),
	CR_RESERVED(8)
	));

CFireProjectile::CFireProjectile(
	const float3& pos,
	const float3& spd,
	CUnit* owner,
	int emitTtl,
	int particleTtl,
	float emitRadius,
	float particleSize
):
	// these are synced, but neither weapon nor piece
	// (only burning features create instances of them)
	CProjectile(pos, speed, owner, true, false, false),
	ttl(emitTtl),
	emitPos(pos),
	emitRadius(emitRadius),
	particleTime(particleTtl),
	particleSize(particleSize)
{
	drawRadius = emitRadius + particleTime * speed.w;
	checkCol = false;
	ageSpeed = 1.0f / particleTime;

	SetPosition(pos + (UpVector * particleTime * speed.w * 0.5f));

	alwaysVisible = true;
	castShadow = true;
}

void CFireProjectile::StopFire()
{
	ttl = 0;
}

void CFireProjectile::Update()
{
	ttl--;
	if (ttl > 0) {
		if (projectileHandler->particleSaturation < 0.8f || (projectileHandler->particleSaturation < 1 && (gs->frameNum & 1))) {
			// unsynced code
			SubParticle sub;
			sub.age = 0;
			sub.maxSize = (0.7f + gu->RandFloat()*0.3f) * particleSize;
			sub.posDif = gu->RandVector() * emitRadius;
			sub.pos = emitPos;
			sub.pos.y += sub.posDif.y;
			sub.posDif.y = 0;
			sub.rotSpeed = (gu->RandFloat() - 0.5f) * 4;
			sub.smokeType = gu->RandInt() % projectileDrawer->smoketex.size();
			subParticles.push_front(sub);

			sub.maxSize = (0.7f + gu->RandFloat()*0.3f) * particleSize;
			sub.posDif = gu->RandVector() * emitRadius;
			sub.pos = emitPos;
			sub.pos.y += sub.posDif.y - radius*0.3f;
			sub.posDif.y = 0;
			sub.rotSpeed=(gu->RandFloat() - 0.5f) * 4;
			subParticles2.push_front(sub);
		}
		if (!(ttl & 31)) {
			// synced code
			const std::vector<CFeature*>& features = quadField->GetFeaturesExact(emitPos + wind.GetCurrentWind() * 0.7f, emitRadius * 2);
			const std::vector<CUnit*>& units = quadField->GetUnitsExact(emitPos + wind.GetCurrentWind() * 0.7f, emitRadius * 2);

			for (std::vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
				if (gs->randFloat() > 0.8f) {
					(*fi)->StartFire();
				}
			}

			for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
				(*ui)->DoDamage(DamageArray(30), ZeroVector, NULL, -CSolidObject::DAMAGE_EXTSOURCE_FIRE, -1);
			}
		}
	}

	for(part_list_type::iterator pi=subParticles.begin();pi!=subParticles.end();++pi){
		pi->age+=ageSpeed;
		if(pi->age>1){
			subParticles.pop_back();
			break;
		}
		pi->pos+=speed + wind.GetCurrentWind()*pi->age*0.05f + pi->posDif*0.1f;
		pi->posDif*=0.9f;
	}
	for(part_list_type::iterator pi=subParticles2.begin();pi!=subParticles2.end();++pi){
		pi->age+=ageSpeed*1.5f;
		if(pi->age>1){
			subParticles2.pop_back();
			break;
		}
		pi->pos+=speed*0.7f+pi->posDif*0.1f;
		pi->posDif*=0.9f;
	}

	if (subParticles.empty() && (ttl <= 0)) {
		deleteMe = true;
	}
}

void CFireProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[3] = 1;
	unsigned char col2[4];
	size_t sz2 = subParticles2.size();
	size_t sz = subParticles.size();
	va->EnlargeArrays(sz2 * 4 + sz * 8, 0, VA_SIZE_TC);
	for(part_list_type::iterator pi = subParticles2.begin(); pi != subParticles2.end(); ++pi) {
		float age = pi->age+ageSpeed*globalRendering->timeOffset;
		float size = pi->maxSize*(age);
		float rot = pi->rotSpeed*age;

		float sinRot = fastmath::sin(rot);
		float cosRot = fastmath::cos(rot);
		float3 dir1 = (camera->right*cosRot + camera->up*sinRot) * size;
		float3 dir2 = (camera->right*sinRot - camera->up*cosRot) * size;

		float3 interPos=pi->pos;

		col[0] = (unsigned char) ((1 - age) * 255);
		col[1] = (unsigned char) ((1 - age) * 255);
		col[2] = (unsigned char) ((1 - age) * 255);

		va->AddVertexQTC(interPos - dir1 - dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->ystart, col);
		va->AddVertexQTC(interPos + dir1 - dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->ystart, col);
		va->AddVertexQTC(interPos + dir1 + dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->yend,   col);
		va->AddVertexQTC(interPos - dir1 + dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->yend,   col);
	}
	for (part_list_type::iterator pi = subParticles.begin(); pi != subParticles.end(); ++pi) {
		const AtlasedTexture* at = projectileDrawer->smoketex[pi->smokeType];
		float age = pi->age+ageSpeed * globalRendering->timeOffset;
		float size = pi->maxSize * fastmath::apxsqrt(age);
		float rot = pi->rotSpeed * age;

		float sinRot = fastmath::sin(rot);
		float cosRot = fastmath::cos(rot);
		float3 dir1 = (camera->right*cosRot + camera->up*sinRot) * size;
		float3 dir2 = (camera->right*sinRot - camera->up*cosRot) * size;

		float3 interPos = pi->pos;

		if (age < 1/1.31f) {
			col[0] = (unsigned char) ((1 - age * 1.3f) * 255);
			col[1] = (unsigned char) ((1 - age * 1.3f) * 255);
			col[2] = (unsigned char) ((1 - age * 1.3f) * 255);
			col[3] = 1;

			va->AddVertexQTC(interPos - dir1 - dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->ystart, col);
			va->AddVertexQTC(interPos + dir1 - dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->ystart, col);
			va->AddVertexQTC(interPos + dir1 + dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->yend,   col);
			va->AddVertexQTC(interPos - dir1 + dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->yend,   col);
		}

		unsigned char c;
		if (age < 0.5f) {
			c = (unsigned char)        (age * 510);
		} else {
			c = (unsigned char) (510 - (age * 510));
		}
		col2[0] = (unsigned char) (c * 0.6f);
		col2[1] = (unsigned char) (c * 0.6f);
		col2[2] = (unsigned char) (c * 0.6f);
		col2[3] = c;

		va->AddVertexQTC(interPos - dir1 - dir2, at->xstart, at->ystart, col2);
		va->AddVertexQTC(interPos + dir1 - dir2, at->xend,   at->ystart, col2);
		va->AddVertexQTC(interPos + dir1 + dir2, at->xend,   at->yend,   col2);
		va->AddVertexQTC(interPos - dir1 + dir2, at->xstart, at->yend,   col2);
	}
}


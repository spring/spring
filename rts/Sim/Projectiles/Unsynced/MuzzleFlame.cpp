/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "MuzzleFlame.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/GlobalUnsynced.h"


CR_BIND_DERIVED(CMuzzleFlame, CProjectile, (ZeroVector, ZeroVector, ZeroVector, 0));

CR_REG_METADATA(CMuzzleFlame,(
	CR_SERIALIZER(creg_Serialize), // randSmokeDir
	CR_MEMBER(dir),
	CR_MEMBER(size),
	CR_MEMBER(age),
	CR_MEMBER(numFlame),
	CR_MEMBER(numSmoke),
	CR_MEMBER(randSmokeDir),
	CR_RESERVED(8)
	));

void CMuzzleFlame::creg_Serialize(creg::ISerializer& s)
{
//	s.Serialize(randSmokeDir, numSmoke*sizeof(float3));
}

CMuzzleFlame::CMuzzleFlame(const float3& pos, const float3& speed, const float3& dir, float size):
	CProjectile(pos, speed, 0, false, false, false),
	dir(dir),
	size(size),
	age(0)
{
	this->pos -= dir * size * 0.2f;
	checkCol = false;
	castShadow = true;
	numFlame = 1 + (int)(size * 3);
	numSmoke = 1 + (int)(size * 5);
//	randSmokeDir=new float3[numSmoke];
	randSmokeDir.resize(numSmoke);

	for (int a = 0; a < numSmoke; ++a) {
		randSmokeDir[a] = dir + gu->usRandFloat() * 0.4f;
	}
}

CMuzzleFlame::~CMuzzleFlame()
{
//	delete[] randSmokeDir;
}

void CMuzzleFlame::Update()
{
	age++;
	if (age > (4 + size * 30)) {
		deleteMe = true;
	}
	pos += speed;
}

void CMuzzleFlame::Draw()
{
	inArray = true;
	unsigned char col[4];
	float alpha = std::max(0.0f, 1 - (age / (4 + size * 30)));
	float modAge = fastmath::apxsqrt(static_cast<float>(age + 2));

	va->EnlargeArrays(numSmoke * 8, 0, VA_SIZE_TC);

	for (int a = 0; a < numSmoke; ++a) { //! CAUTION: loop count must match EnlargeArrays above
		int tex = a % projectileDrawer->smoketex.size();
		//float xmod=0.125f+(float(int(tex%6)))/16;
		//float ymod=(int(tex/6))/16.0f;

		float drawsize = modAge * 3;
		float3 interPos(pos+randSmokeDir[a]*(a+2)*modAge*0.4f);
		float fade = std::max(0.0f, std::min(1.0f, (1 - alpha) * (20 + a) * 0.1f));

		col[0] = (unsigned char) (180 * alpha * fade);
		col[1] = (unsigned char) (180 * alpha * fade);
		col[2] = (unsigned char) (180 * alpha * fade);
		col[3] = (unsigned char) (255 * alpha * fade);

		#define st projectileDrawer->smoketex[tex]
		va->AddVertexQTC(interPos - camera->right * drawsize - camera->up * drawsize, st->xstart, st->ystart, col);
		va->AddVertexQTC(interPos + camera->right * drawsize - camera->up * drawsize, st->xend,   st->ystart, col);
		va->AddVertexQTC(interPos + camera->right * drawsize + camera->up * drawsize, st->xend,   st->yend,   col);
		va->AddVertexQTC(interPos - camera->right * drawsize + camera->up * drawsize, st->xstart, st->yend,   col);
		#undef st

		if (fade < 1.0f) {
			float ifade = 1.0f - fade;
			col[0] = (unsigned char) (ifade * 255);
			col[1] = (unsigned char) (ifade * 255);
			col[2] = (unsigned char) (ifade * 255);
			col[3] = (unsigned char) (1);

			#define mft projectileDrawer->muzzleflametex
			va->AddVertexQTC(interPos - camera->right * drawsize - camera->up * drawsize, mft->xstart, mft->ystart, col);
			va->AddVertexQTC(interPos + camera->right * drawsize - camera->up * drawsize, mft->xend,   mft->ystart, col);
			va->AddVertexQTC(interPos + camera->right * drawsize + camera->up * drawsize, mft->xend,   mft->yend,   col);
			va->AddVertexQTC(interPos - camera->right * drawsize + camera->up * drawsize, mft->xstart, mft->yend,   col);
			#undef mft
		}
	}
}



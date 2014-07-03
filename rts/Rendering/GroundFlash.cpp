/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GroundFlash.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/ProjectileDrawer.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CGroundFlash, CExpGenSpawnable, );
CR_REG_METADATA(CGroundFlash, (
 	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(size),
		CR_MEMBER(depthTest),
		CR_MEMBER(depthMask),
	CR_MEMBER_ENDFLAG(CM_Config)
));

CR_BIND_DERIVED(CStandardGroundFlash, CGroundFlash, );
CR_REG_METADATA(CStandardGroundFlash, (
 	CR_MEMBER_BEGINFLAG(CM_Config),
 		CR_MEMBER(flashSize),
		CR_MEMBER(flashAlpha),
		CR_MEMBER(circleGrowth),
		CR_MEMBER(circleAlpha),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config),

	CR_MEMBER(side1),
	CR_MEMBER(side2),
	CR_MEMBER(circleSize),
	CR_MEMBER(flashAge),
	CR_MEMBER(flashAgeSpeed),
	CR_MEMBER(circleAlphaDec),
	CR_MEMBER(color),
	CR_MEMBER(ttl)
));

CR_BIND_DERIVED(CSeismicGroundFlash, CGroundFlash, (ZeroVector, 1, 0, 1, 1, 1, ZeroVector));
CR_REG_METADATA(CSeismicGroundFlash, (
	CR_MEMBER(side1),
	CR_MEMBER(side2),
	CR_MEMBER(texture),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(alpha),
	CR_MEMBER(fade),
	CR_MEMBER(ttl),
	CR_MEMBER(color)
));

CR_BIND_DERIVED(CSimpleGroundFlash, CGroundFlash, );
CR_REG_METADATA(CSimpleGroundFlash, (
	CR_MEMBER(side1),
	CR_MEMBER(side2),
	CR_MEMBER(age),
	CR_MEMBER(agerate),
 	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(sizeGrowth),
		CR_MEMBER(ttl),
		CR_MEMBER(colorMap),
		CR_MEMBER(texture),
	CR_MEMBER_ENDFLAG(CM_Config)
));



CVertexArray* CGroundFlash::va = NULL;

// CREG-only
CGroundFlash::CGroundFlash()
{
	size = 0.0f;
	depthTest = true;
	depthMask = false;
	alwaysVisible = false;
}

CGroundFlash::CGroundFlash(const float3& p): CExpGenSpawnable()
{
	size = 0.0f;
	depthTest = true;
	depthMask = false;
	alwaysVisible = false;
	pos = p;
}



CStandardGroundFlash::CStandardGroundFlash()
{
	ttl = 0;
	color[0] = 0;
	color[1] = 0;
	color[2] = 0;

	circleAlphaDec = 0.0f;
	flashAgeSpeed = 0.0f;
	flashAge = 0.0f;
	flashAlpha = 0.0f;
	circleAlpha = 0.0f;
	circleGrowth = 0.0f;
	circleSize = 0.0f;
	flashSize = 0.0f;
}

CStandardGroundFlash::CStandardGroundFlash(const float3& p, float circleAlpha, float flashAlpha, float flashSize, float circleSpeed, float ttl, const float3& col)
	: CGroundFlash(p)
	, flashSize(flashSize)
	, circleSize(circleSpeed)
	, circleGrowth(circleSpeed)
	, circleAlpha(circleAlpha)
	, flashAlpha(flashAlpha)
	, flashAge(0)
	, flashAgeSpeed(ttl? 1.0f / ttl : 0)
	, circleAlphaDec(ttl? circleAlpha / ttl : 0)
	, ttl((int)ttl)
{
	for (size_t a = 0; a < 3; ++a) {
		color[a] = (col[a] * 255.0f);
	}

	const float3 fw = camera->forward * -1000.0f;
	this->pos.y = CGround::GetHeightReal(p.x, p.z, false) + 1.0f;

	float3 p1(p.x + flashSize, 0, p.z);
		p1.y = CGround::GetApproximateHeight(p1.x, p1.z, false);
		p1  += fw;
	float3 p2(p.x - flashSize, 0, p.z);
		p2.y = CGround::GetApproximateHeight(p2.x, p2.z, false);
		p2  += fw;
	float3 p3(p.x, 0, p.z + flashSize);
		p3.y = CGround::GetApproximateHeight(p3.x, p3.z, false);
		p3  += fw;
	float3 p4(p.x, 0, p.z - flashSize);
		p4.y = CGround::GetApproximateHeight(p4.x, p4.z, false);
		p4  += fw;

	// else ANormalize() fails!
	assert(flashSize > 1);

	const float3 n1 = ((p3 - p1).cross(p4 - p1)).ANormalize();
	const float3 n2 = ((p4 - p2).cross(p3 - p2)).ANormalize();
	const float3 normal = (n1 + n2).ANormalize();

	size = flashSize; // flashSize is just backward compability

	side1 = normal.cross(RgtVector);
	side1.ANormalize();
	side2 = side1.cross(normal);

	projectileHandler->AddGroundFlash(this);
}

bool CStandardGroundFlash::Update()
{
	circleSize += circleGrowth;
	circleAlpha -= circleAlphaDec;
	flashAge += flashAgeSpeed;
	return ttl? (--ttl > 0) : false;
}

void CStandardGroundFlash::Draw()
{
	float iAlpha = circleAlpha - (circleAlphaDec * globalRendering->timeOffset);
	if (iAlpha > 1.0f) iAlpha = 1.0f;
	if (iAlpha < 0.0f) iAlpha = 0.0f;

	unsigned char col[4] = {
		color[0],
		color[1],
		color[2],
		(unsigned char)(iAlpha * 255),
	};

	const float iSize = circleSize + circleGrowth * globalRendering->timeOffset;
	const float iAge = flashAge + flashAgeSpeed * globalRendering->timeOffset;

	if (iAlpha > 0.0f) {
		const float3 p1 = pos + (-side1 - side2) * iSize;
		const float3 p2 = pos + ( side1 - side2) * iSize;
		const float3 p3 = pos + ( side1 + side2) * iSize;
		const float3 p4 = pos + (-side1 + side2) * iSize;

		va->AddVertexQTC(p1, projectileDrawer->groundringtex->xstart, projectileDrawer->groundringtex->ystart, col);
		va->AddVertexQTC(p2, projectileDrawer->groundringtex->xend,   projectileDrawer->groundringtex->ystart, col);
		va->AddVertexQTC(p3, projectileDrawer->groundringtex->xend,   projectileDrawer->groundringtex->yend,   col);
		va->AddVertexQTC(p4, projectileDrawer->groundringtex->xstart, projectileDrawer->groundringtex->yend,   col);
	}

	if (iAge < 1.0f) {
		if (iAge < 0.091f) {
			iAlpha = flashAlpha * iAge * 10.0f;
		} else {
			iAlpha = flashAlpha * (1.0f - iAge);
		}

		if (iAlpha > 1.0f) iAlpha = 1.0f;
		if (iAlpha < 0.0f) iAlpha = 0.0f;

		col[3] = (iAlpha * 255);

		const float3 p1 = pos + (-side1 - side2) * size;
		const float3 p2 = pos + ( side1 - side2) * size;
		const float3 p3 = pos + ( side1 + side2) * size;
		const float3 p4 = pos + (-side1 + side2) * size;

		va->AddVertexQTC(p1, projectileDrawer->groundflashtex->xstart, projectileDrawer->groundflashtex->yend,   col);
		va->AddVertexQTC(p2, projectileDrawer->groundflashtex->xend,   projectileDrawer->groundflashtex->yend,   col);
		va->AddVertexQTC(p3, projectileDrawer->groundflashtex->xend,   projectileDrawer->groundflashtex->ystart, col);
		va->AddVertexQTC(p4, projectileDrawer->groundflashtex->xstart, projectileDrawer->groundflashtex->ystart, col);
	}
}



CSimpleGroundFlash::CSimpleGroundFlash()
{
	texture = NULL;
	colorMap = NULL;
	agerate = 0.0f;
	age = 0.0f;
	ttl = 0;
	sizeGrowth = 0.0f;
}

void CSimpleGroundFlash::Init(const CUnit* owner, const float3& offset)
{
	pos += offset;
	age = ttl ? 0.0f : 1.0f;
	agerate = ttl ? 1.0f / ttl : 1.0f;

	const float flashsize = size + (sizeGrowth * ttl);
	const float3 fw = camera->forward * -1000.0f;

	this->pos.y = CGround::GetHeightReal(pos.x, pos.z, false) + 1.0f;

	float3 p1(pos.x + flashsize, 0.0f, pos.z);
		p1.y = CGround::GetApproximateHeight(p1.x, p1.z, false);
		p1 += fw;
	float3 p2(pos.x - flashsize, 0.0f, pos.z);
		p2.y = CGround::GetApproximateHeight(p2.x, p2.z, false);
		p2 += fw;
	float3 p3(pos.x, 0.0f, pos.z + flashsize);
		p3.y = CGround::GetApproximateHeight(p3.x, p3.z, false);
		p3 += fw;
	float3 p4(pos.x, 0.0f, pos.z - flashsize);
		p4.y = CGround::GetApproximateHeight(p4.x, p4.z, false);
		p4 += fw;

	const float3 n1 = ((p3 - p1).cross(p4 - p1)).ANormalize();
	const float3 n2 = ((p4 - p2).cross(p3 - p2)).ANormalize();
	const float3 normal = (n1 + n2).ANormalize();

	side1 = normal.cross(RgtVector);
	side1.ANormalize();
	side2 = side1.cross(normal);

	projectileHandler->AddGroundFlash(this);
}

void CSimpleGroundFlash::Draw()
{
	unsigned char color[4] = {0, 0, 0, 0};
	colorMap->GetColor(color, age);

	const float3 p1 = pos + (-side1 - side2) * size;
	const float3 p2 = pos + ( side1 - side2) * size;
	const float3 p3 = pos + ( side1 + side2) * size;
	const float3 p4 = pos + (-side1 + side2) * size;

	va->AddVertexQTC(p1, texture->xstart, texture->ystart, color);
	va->AddVertexQTC(p2, texture->xend,   texture->ystart, color);
	va->AddVertexQTC(p3, texture->xend,   texture->yend,   color);
	va->AddVertexQTC(p4, texture->xstart, texture->yend,   color);
}

bool CSimpleGroundFlash::Update()
{
	age += agerate;
	size += sizeGrowth;

	return (age < 1);
}



CSeismicGroundFlash::CSeismicGroundFlash(const float3& p, int ttl, int fade, float size, float sizeGrowth, float alpha, const float3& col)
	: CGroundFlash(p)
	, texture(projectileDrawer->seismictex)
	, sizeGrowth(sizeGrowth)
	, alpha(alpha)
	, fade(fade)
	, ttl(ttl)
{
	this->size = size;
	alwaysVisible = true;

	for (size_t a = 0; a < 3; ++a) {
		color[a] = (col[a] * 255.0f);
	}

	const float flashsize = size + sizeGrowth * ttl;
	const float3 fw = camera->forward * -1000.0f;

	this->pos.y = CGround::GetHeightReal(p.x, p.z, false) + 1.0f;

	float3 p1(p.x + flashsize, 0.0f, p.z);
		p1.y = CGround::GetApproximateHeight(p1.x, p1.z, false);
		p1 += fw;
	float3 p2(p.x - flashsize, 0.0f, p.z);
		p2.y = CGround::GetApproximateHeight(p2.x, p2.z, false);
		p2 += fw;
	float3 p3(p.x, 0.0f, p.z + flashsize);
		p3.y = CGround::GetApproximateHeight(p3.x, p3.z, false);
		p3 += fw;
	float3 p4(p.x, 0.0f, p.z - flashsize);
		p4.y = CGround::GetApproximateHeight(p4.x, p4.z, false);
		p4 += fw;

	const float3 n1 = ((p3 - p1).cross(p4 - p1)).SafeANormalize();
	const float3 n2 = ((p4 - p2).cross(p3 - p2)).SafeANormalize();
	const float3 normal = (n1 + n2).SafeANormalize();

	side1 = normal.cross(RgtVector);
	side1.SafeANormalize();
	side2 = side1.cross(normal);

	projectileHandler->AddGroundFlash(this);
}

void CSeismicGroundFlash::Draw()
{
	color[3] = ttl<fade ? int(((ttl) / (float)(fade)) * 255) : 255;

	const float3 p1 = pos + (-side1 - side2) * size;
	const float3 p2 = pos + ( side1 - side2) * size;
	const float3 p3 = pos + ( side1 + side2) * size;
	const float3 p4 = pos + (-side1 + side2) * size;

	va->AddVertexQTC(p1, texture->xstart, texture->ystart, color);
	va->AddVertexQTC(p2, texture->xend,   texture->ystart, color);
	va->AddVertexQTC(p3, texture->xend,   texture->yend,   color);
	va->AddVertexQTC(p4, texture->xstart, texture->yend,   color);
}

bool CSeismicGroundFlash::Update()
{
	size += sizeGrowth;
	return (--ttl > 0);
}

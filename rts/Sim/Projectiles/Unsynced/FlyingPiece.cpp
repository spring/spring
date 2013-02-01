/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlyingPiece.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Models/3DOParser.h"
#include "Rendering/Models/S3OParser.h"

SS3OFlyingPiece::~SS3OFlyingPiece() {
	delete[] chunk;
}



bool FlyingPiece::Update() {
	pos      += speed;
	speed    *= 0.996f;
	speed.y  += mapInfo->map.gravity; // fp's are not projectiles
	rotAngle += rotSpeed;

	transMat.LoadIdentity();
	transMat.Rotate(rotAngle, rotAxis);

	return (pos.y >= ground->GetApproximateHeight(pos.x, pos.z - 10.0f, false));
}

void FlyingPiece::InitCommon(const float3& _pos, const float3& _speed, int _team)
{
	pos   = _pos;
	speed = _speed;

	texture = 0;
	team    = _team;

	rotAxis  = gu->RandVector().ANormalize();
	rotSpeed = gu->RandFloat() * 0.1f;
	rotAngle = 0.0f;
}

void FlyingPiece::DrawCommon(size_t* lastTeam, CVertexArray* va) {

	if (team != *lastTeam) {
		*lastTeam = team;

		va->DrawArrayTN(GL_QUADS); //switch to GL_TRIANGLES?
		va->Initialize();
		unitDrawer->SetTeamColour(team);
	}
}

void S3DOFlyingPiece::Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* va) {
	DrawCommon(lastTeam, va);

	const float3& interPos = pos + speed * globalRendering->timeOffset;

	const C3DOTextureHandler::UnitTexture* tex = chunk->texture;

	const std::vector<S3DOVertex>& vertices = piece->vertices;
	const std::vector<int>&         indices = chunk->vertices;

	const float uvCoords[8] = {
		tex->xstart, tex->ystart,
		tex->xend,   tex->ystart,
		tex->xend,   tex->yend,
		tex->xstart, tex->yend
	};

	for (int i = 0; i < 4; i++) {
		const S3DOVertex& v = vertices[indices[i]];
		const float3 tp = transMat.Mul(v.pos) + interPos;
		const float3 tn = transMat.Mul(v.normal);
		va->AddVertexQTN(tp, uvCoords[i << 1], uvCoords[(i << 1) + 1], tn);
	}
}

void SS3OFlyingPiece::Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* va) {
	DrawCommon(lastTeam, va);

	const float3& interPos = pos + speed * globalRendering->timeOffset;

	if (texture != *lastTex) {
		*lastTex = texture;

		if (*lastTex == 0) {
			return;
		}

		va->DrawArrayTN(GL_QUADS);
		va->Initialize();
		texturehandlerS3O->SetS3oTexture(texture);
	}

	for (int i = 0; i < 4; i++) {
		const SS3OVertex& v = chunk[i];
		const float3 tp = transMat.Mul(v.pos) + interPos;
		const float3 tn = transMat.Mul(v.normal);
		va->AddVertexQTN(tp, v.texCoord.x, v.texCoord.y, tn);
	}
}


/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlyingPiece.h"

#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/S3OTextureHandler.h"


static const float EXPLOSION_SPEED = 2.f;
static const int   MAX_SPLITTER_PARTS = 10;


/////////////////////////////////////////////////////////////////////
/// NEW S3O,OBJ,ASSIMP,... IMPLEMENTATION
///

FlyingPiece::FlyingPiece(
	const S3DModelPiece* p,
	const std::vector<unsigned int>& inds,
	const float3 pos,
	const float3 speed,
	const CMatrix44f& _pieceMatrix,
	const float pieceChance,
	const int2 _renderParams // (.x=texType, .y=team)
)
: piece(p)
, indices(inds)
, pos0(pos)
, age(0)
, pieceMatrix(_pieceMatrix)
{
	assert(piece->HasGeometryData()); // else binding the vbos cause gl errors
	assert(piece->GetVertexDrawIndexCount() % 3 == 0); // only triangles
	pieceRadius = float3::max(float3::fabs(piece->maxs), float3::fabs(piece->mins)).Length();
	InitCommon(pos, speed, pieceRadius, _renderParams.y, _renderParams.x);

	// initialize splitter parts
	splitterParts.resize(MAX_SPLITTER_PARTS);
	for (auto& cp: splitterParts) {
		cp.dir = gu->RandVector().ANormalize();
		cp.speed = speed + cp.dir * EXPLOSION_SPEED * gu->RandFloat();
		cp.rotationAxisAndSpeed = float4(gu->RandVector().ANormalize(), gu->RandFloat() * 0.1f);
	}

	// add vertices to splitter parts
	for (size_t i = 0; i < indices.size(); i += 3) {
		auto dir = GetPolygonDir(i);

		float md = -2.f;
		SplitterData* mcp = nullptr;
		for (auto& cp: splitterParts) {
			if (cp.dir.dot(dir) < md)
				continue;

			md = cp.dir.dot(dir);
			mcp = &cp;
		}

		mcp->indices.push_back(indices[i + 0]);
		mcp->indices.push_back(indices[i + 1]);
		mcp->indices.push_back(indices[i + 2]);
	}

	// randomly remove pieces depending on the pieceChance
	for (auto& cp: splitterParts) {
		if (gu->RandFloat() > pieceChance) {
			cp.indices.clear();
		}
	}

	// calculate needed vbo size for the vertex indices
	size_t neededVboSize = 0;
	for (auto& cp: splitterParts) {
		cp.indexCount = cp.indices.size();
		cp.vboOffset = neededVboSize;
		neededVboSize += cp.indexCount * sizeof(unsigned int);
	}

	// fill the vertex index vbo
	indexVBO.Bind(GL_ELEMENT_ARRAY_BUFFER);
	indexVBO.Resize(neededVboSize);
	auto* memVBO = reinterpret_cast<unsigned char*>(indexVBO.MapBuffer(GL_WRITE_ONLY));
	for (auto& cp: splitterParts) {
		memcpy(memVBO + cp.vboOffset, &cp.indices[0], cp.indexCount * sizeof(unsigned int));

		// free memory
		cp.indices.clear();
		cp.indices.shrink_to_fit();
	}
	indexVBO.UnmapBuffer();
	indexVBO.Unbind();

	// delete empty splitter parts
	for (int i = splitterParts.size() - 1;i >= 0;--i) {
		auto& cp = splitterParts[i];
		if (cp.indexCount == 0) {
			cp = splitterParts.back();
			splitterParts.pop_back();
		}
	}
}


void FlyingPiece::InitCommon(const float3 _pos, const float3 _speed, const float _radius, int _team, int _texture)
{
	pos   = _pos;
	speed = _speed;
	radius = _radius + 10.f;

	texture = _texture;
	team    = _team;
}


unsigned FlyingPiece::GetDrawCallCount() const
{
	return splitterParts.size();
}


bool FlyingPiece::Update()
{
	if (splitterParts.empty())
		return false;

	++age;

	const float3 dragFactors = GetDragFactors();

	// used for `in camera frustum checks`
	pos    = pos0 + (speed * dragFactors.x) + UpVector * (mapInfo->map.gravity * dragFactors.y);
	radius = pieceRadius + EXPLOSION_SPEED * dragFactors.x + 10.f;

	// check visibility (if all particles are underground -> kill)
	if (age % GAME_SPEED != 0) {
		for (auto& cp: splitterParts) {
			const CMatrix44f& m = GetMatrixOf(cp, dragFactors);
			const float3 p = m.GetPos();
			if ((p.y + pieceRadius * 2.f) >= CGround::GetApproximateHeight(p.x, p.z, false)) {
				return true;
			}
		}
		return false;
	}

	return true;
}


const float3& FlyingPiece::GetVertexPos(const size_t i) const
{
	assert(i < indices.size());
	return piece->GetVertexPos( indices[i] );
}


float3 FlyingPiece::GetPolygonDir(const size_t idx) const
{
	float3 midPos;
	for (int j: {0,1,2}) {
		midPos += GetVertexPos(idx + j);
	}
	midPos *= 0.333f;
	return midPos.ANormalize();
}


float3 FlyingPiece::GetDragFactors() const
{
	// We started with a naive (iterative) method like this:
	//  pos   += speed;
	//  speed *= airDrag;
	//  speed += gravity;
	// The problem is that pos & speed need to be saved for this (-> memory) and
	// need to be updated each frame (-> cpu usage). Doing so for each polygon is massively slow.
	// So I wanted to replace it with a stateless system, computing the current
	// position just from the params t & individual speed (start pos is 0!).
	//
	// To do so we split the computations in 2 parts: explosion speed & gravity.
	//
	// 1.
	// So the positional offset caused by the explosion speed becomes:
	//  d := drag   & s := explosion start speed
	//  xs(t=1) := s
	//  xs(t=2) := s + s * d
	//  xs(t=3) := s + s * d + s * d^2
	//  xs(t)   := s + s * d + s * d^2 + ... + s * d^t
	//           = s * sum(i=0,t){d^i}
	// That sum is a `geometric series`, the solution is `(1-d^(t+1))/(1-d)`.
	//  => xs(t) = s * (1 - d^(t+1)) / (1 - d)
	//           = s * speedDrag
	//
	// 2.
	// The positional offset caused by gravity is similar but a bit more complicated:
	//  xg(t=1) := g
	//  xg(t=2) := g + (g * d + g)                         = 2g + 1g*d
	//  xg(t=3) := g + (g * d + g) + (g * d^2 + g * d + g) = 3g + 2g*d + 1g*d^2
	//  xg(t)   := g * t + g * (t - 1) * d^2 + ... + g * (t - t) * d^t
	//           = g * sum(i=0,t){(t - i) * d^i}
	//           = g * (t * sum(i=0,t){d^i} - sum(i=0,t){i * d^i})
	//           = g * gravityDrag
	// The first sum is again a geometric series as above, the 2nd one is very similar.
	// The solution can be found here: https://de.wikipedia.org/w/index.php?title=Geometrische_Reihe&oldid=149159222#Verwandte_Summenformel_1
	//
	// The advantage of those 2 drag factors are that they only depend on the time (which even can be interpolated),
	// so we only have to compute them once per frame. And can then get the position of each particle with a simple multiplication,
	// saving memory & cpu time.

	static const float airDrag = 0.995f;
	static const float invAirDrag = 1.f / (1.f - airDrag);

	const float interAge = age + globalRendering->timeOffset;
	const float airDragPowOne = std::pow(airDrag, interAge + 1.f);
	const float airDragPowTwo = airDragPowOne * airDrag; // := std::pow(airDrag, interAge + 2.f);

	float3 dragFactors;

	// Speed Drag
	dragFactors.x = (1.f - airDragPowOne) * invAirDrag;

	// Gravity Drag
	dragFactors.y  = interAge * dragFactors.x; // 1st sum
	dragFactors.y -= (interAge * (airDragPowTwo - airDragPowOne) - airDragPowOne + airDrag) * invAirDrag * invAirDrag; // 2nd sum

	// time interpolation factor
	dragFactors.z = interAge;

	return dragFactors;
}


CMatrix44f FlyingPiece::GetMatrixOf(const SplitterData& cp, const float3 dragFactors) const
{
	float3 interPos = cp.speed * dragFactors.x;
	interPos.y += mapInfo->map.gravity * dragFactors.y;

	const float4& rot = cp.rotationAxisAndSpeed;
	CMatrix44f m = pieceMatrix;
	m.GetPos() += interPos; //note: not the same as .Translate(pos) which does `m = m * translate(pos)`, but we want `m = translate(pos) * m`
	m.Rotate(rot.w * dragFactors.z, rot.xyz);

	return m;
}


void FlyingPiece::CheckDrawStateChange(FlyingPiece* prev) const
{
	if (prev == nullptr) {
		unitDrawer->SetTeamColour(team);
		if (texture != -1) texturehandlerS3O->SetS3oTexture(texture);
		piece->BindVertexAttribVBOs();
		return;
	}

	if (team != prev->GetTeam()) {
		unitDrawer->SetTeamColour(team);
	}

	if (texture != prev->GetTexture()) {
		if (texture != -1) texturehandlerS3O->SetS3oTexture(texture);
	}

	if (piece != prev->piece) {
		prev->piece->UnbindVertexAttribVBOs();
		piece->BindVertexAttribVBOs();
	}
}


void FlyingPiece::EndDraw() const
{
	piece->UnbindVertexAttribVBOs();
}


void FlyingPiece::Draw(FlyingPiece* prev)
{
	CheckDrawStateChange(prev);
	const float3 dragFactors = GetDragFactors(); // speedDrag, gravityDrag, interAge

	indexVBO.Bind(GL_ELEMENT_ARRAY_BUFFER);
	for (auto& cp: splitterParts) {
		glPushMatrix();
		glMultMatrixf(GetMatrixOf(cp, dragFactors));
		glDrawRangeElements(GL_TRIANGLES, 0, piece->GetVertexCount() - 1, cp.indexCount, GL_UNSIGNED_INT, indexVBO.GetPtr(cp.vboOffset));
		glPopMatrix();
	}
	indexVBO.Unbind();
}

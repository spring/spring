/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlyingPiece.h"

#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/SpringMath.h"


static constexpr float EXPLOSION_SPEED = 2.0f;

//TODO: add to creg

/////////////////////////////////////////////////////////////////////
/// NEW S3O,ASSIMP,... IMPLEMENTATION
///

FlyingPiece::FlyingPiece(
	const S3DModel* _model,
	const S3DModelPiece* _piece,
	const CMatrix44f& _pieceMatrix,
	const float3 pos,
	const float3 speed,
	const float2 _pieceParams, // (.x=radius, .y=chance)
	const int2 _renderParams // (.x=texType, .y=team)
)
: pos0(pos)
, pieceMatrix(_pieceMatrix)
, age(0)
, model(_model)
, piece(_piece)
{
	// only allow triangles
	assert(piece->GetVertexDrawIndexCount() % 3 == 0);

	InitCommon(pos, speed, _pieceParams.x, _renderParams.y, _renderParams.x);

	const S3DModelPiecePart& shatterPiecePart = piece->shatterParts[guRNG.NextInt(piece->shatterParts.size())];
	const auto& shatterPieceData = shatterPiecePart.renderData;

	splitterParts.reserve(shatterPieceData.size());

	for (const auto& cp: shatterPieceData) {
		if (guRNG.NextFloat() > _pieceParams.x)
			continue;

		const float3 rndVec = guRNG.NextVector() * 0.3f;
		const float3 flyDir = (cp.dir + rndVec).ANormalize();

		splitterParts.emplace_back();
		splitterParts.back().speed                = speed + flyDir * mix<float>(1.0f, EXPLOSION_SPEED, guRNG.NextFloat());
		splitterParts.back().rotationAxisAndSpeed = float4(guRNG.NextVector().ANormalize(), guRNG.NextFloat() * 0.1f);
		splitterParts.back().indexCount           = cp.indexCount;
		splitterParts.back().vboOffset            = cp.vboOffset;
	}
}


void FlyingPiece::InitCommon(const float3 _pos, const float3 _speed, const float _radius, int _team, int _texture)
{
	pos   = _pos;
	speed = _speed;

	pieceRadius = _radius;
	drawRadius = _radius + 10.f;

	texture = _texture;
	team    = _team;
}


bool FlyingPiece::Update()
{
	if (splitterParts.empty())
		return false;

	++age;

	const float3 dragFactors = GetDragFactors();

	// used for camera frustum checks
	pos        = pos0 + (speed * dragFactors.x) + UpVector * (mapInfo->map.gravity * dragFactors.y);
	drawRadius = pieceRadius + EXPLOSION_SPEED * dragFactors.x + 10.f;

	// check visibility (if all particles are underground -> kill)
	if ((age % GAME_SPEED) == 0)
		return true;

	for (const SplitterData& sp: splitterParts) {
		const CMatrix44f& m = GetMatrix(sp, dragFactors);
		const float3 p = m.GetPos();

		if ((p.y + pieceRadius * 2.0f) >= CGround::GetApproximateHeight(p.x, p.z, false))
			return true;
	}

	return false;
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
	static const float invAirDrag = 1.0f / (1.0f - airDrag);

	const float interAge = age + globalRendering->timeOffset;
	const float airDragPowOne = std::pow(airDrag, interAge + 1.0f);
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


CMatrix44f FlyingPiece::GetMatrix(const SplitterData& cp, const float3 dragFactors) const
{
	const float3 interPos = cp.speed * dragFactors.x + UpVector * mapInfo->map.gravity * dragFactors.y;
	const float4& rot = cp.rotationAxisAndSpeed;

	CMatrix44f m = pieceMatrix;
	m.GetPos() += interPos; //note: not the same as .Translate(pos) which does `m = m * translate(pos)`, but we want `m = translate(pos) * m`
	m.Rotate(rot.w * dragFactors.z, rot.xyz);

	return m;
}


void FlyingPiece::CheckDrawStateChange(const FlyingPiece* prev) const
{
	if (prev == nullptr) {
		unitDrawer->SetTeamColour(team);

		if (texture != -1)
			CUnitDrawer::BindModelTypeTexture(MODELTYPE_S3O, texture);

		model->BindElemsBuffer();
		piece->BindShatterIndexBuffer();
		model->EnableAttribs();
		return;
	}

	if (team != prev->team)
		unitDrawer->SetTeamColour(team);

	if (texture != prev->texture && texture != -1)
		CUnitDrawer::BindModelTypeTexture(MODELTYPE_S3O, texture);


	if (piece == prev->piece)
		return;

	if (model != prev->model) {
		prev->model->UnbindElemsBuffer();
		model->BindElemsBuffer();
	}

	prev->piece->UnbindShatterIndexBuffer();
	piece->BindShatterIndexBuffer();
}


void FlyingPiece::EndDraw() const
{
	piece->UnbindShatterIndexBuffer();
	model->UnbindElemsBuffer();
	model->DisableAttribs();
}


void FlyingPiece::Draw(const FlyingPiece* prev) const
{
	CheckDrawStateChange(prev);

	// speedDrag, gravityDrag, interAge
	const float3 dragFactors = GetDragFactors();

	const auto& pieceMatrices = model->GetPieceMatrices();
	const VBO& shatterIndices = piece->GetShatterIndexBuffer();

	const IUnitDrawerState* udState = unitDrawer->GetDrawerState(DRAWER_STATE_SEL);

	// set piece-matrices only once; shared by all parts
	udState->SetMatrices(CMatrix44f::Identity(), pieceMatrices);

	for (const SplitterData& sp: splitterParts) {
		udState->SetMatrices(GetMatrix(sp, dragFactors), {});

		glDrawElements(GL_TRIANGLES, sp.indexCount, GL_UNSIGNED_INT, shatterIndices.GetPtr(sp.vboOffset));
	}
}


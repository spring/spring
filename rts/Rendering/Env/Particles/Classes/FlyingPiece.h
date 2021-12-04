/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLYING_PIECE_H
#define FLYING_PIECE_H

#include <vector>

#include "System/float3.h"
#include "System/type2.h"
#include "System/Matrix44f.h"
#include "Rendering/GL/VBO.h"

struct S3DModel;
struct S3DModelPiece;


struct FlyingPiece {
public:
	FlyingPiece(
		const S3DModel* model,
		const S3DModelPiece* _piece,
		const CMatrix44f& _pieceMatrix,
		const float3 pos,
		const float3 speed,
		const float2 _pieceParams,
		const int2 _renderParams
	);

	bool Update();
	void Draw(const FlyingPiece* prev) const;
	void EndDraw() const;
	unsigned GetDrawCallCount() const { return splitterParts.size(); }

public:
	const int& GetTeam() const { return team; }
	const float3& GetPos() const { return pos; }
	const float&  GetRadius() const { return drawRadius; }

	// used for sorting to reduce gl state changes
	bool operator< (const FlyingPiece& fp) const {
		if (texture != fp.texture)
			return (texture < fp.texture);
		if (piece != fp.piece)
			return (piece < fp.piece);
		if (team != fp.team)
			return (team < fp.team);
		return false;
	}

private:
	struct SplitterData {
		float3 speed;
		float4 rotationAxisAndSpeed;
		size_t vboOffset;
		size_t indexCount;
	};

private:
	inline void InitCommon(const float3 _pos, const float3 _speed, const float _radius, int _team, int _texture);
	void CheckDrawStateChange(const FlyingPiece* prev) const;
	float3 GetDragFactors() const;
	CMatrix44f GetMatrix(const SplitterData& cp, const float3 dragFactors) const;

private:
	float3 pos0;
	float3 pos;
	float3 speed;

	CMatrix44f pieceMatrix;

	int team;
	int texture;
	unsigned age;

	float pieceRadius;
	float drawRadius;

	const S3DModel* model;
	const S3DModelPiece* piece;

	std::vector<SplitterData> splitterParts;
};

#endif // FLYING_PIECE_H


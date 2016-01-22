/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLYING_PIECE_H
#define FLYING_PIECE_H


#include "System/float3.h"
#include "System/type2.h"
#include "System/Matrix44f.h"
#include "Rendering/GL/VBO.h"


struct S3DModelPiece;


struct FlyingPiece {
public:
	FlyingPiece(
		const S3DModelPiece* p,
		const std::vector<unsigned int>& indices,
		const float3 pos,
		const float3 speed,
		const CMatrix44f& _pieceMatrix,
		const float pieceChance,
		const int2 _renderParams
	);

	bool Update();
	void Draw(FlyingPiece* prev);
	void EndDraw() const;
	unsigned GetDrawCallCount() const;

public:
	int GetTeam() const { return team; }
	int GetTexture() const { return texture; }

	float3 GetPos() const { return pos; }
	float GetRadius() const { return radius; }

private:
	struct SplitterData {
		float3 dir;
		float3 speed;
		float4 rotationAxisAndSpeed;

		std::vector<unsigned int> indices; // just as a buffer, is cleared once the vbo has been created

		size_t vboOffset;
		size_t indexCount;
	};

private:
	inline void InitCommon(const float3 _pos, const float3 _speed, const float _radius, int _team, int _texture);
	void CheckDrawStateChange(FlyingPiece* prev) const;
	float3 GetDragFactors() const;
	CMatrix44f GetMatrixOf(const SplitterData& cp, const float3 dragFactors) const;
	const float3& GetVertexPos(const size_t i) const;
	float3 GetPolygonDir(const size_t idx) const;

private:
	float3 pos;
	float3 speed;
	float radius;
	int team;
	int texture;

	const S3DModelPiece* piece;
	const std::vector<unsigned int>& indices;

	float3 pos0;
	unsigned age;
	float pieceRadius;
	const CMatrix44f pieceMatrix;

	std::vector<SplitterData> splitterParts;
	VBO indexVBO;
};

#endif // FLYING_PIECE_H


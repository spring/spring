/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLYING_PIECE_H
#define FLYING_PIECE_H

#include "lib/gml/gmlcnf.h"

#if !(defined(USE_GML) && GML_ENABLE_SIM)
#include "System/MemPool.h"
#endif

#include "System/float3.h"

class CVertexArray;
struct S3DOPrimitive;
struct S3DOPiece;
struct SS3OVertex;

struct FlyingPiece {
	#if !(defined(USE_GML) && GML_ENABLE_SIM)
	inline void* operator new(size_t size) { return mempool.Alloc(size); }
	inline void operator delete(void* p, size_t size) { mempool.Free(p, size); }
	#endif

public:
	FlyingPiece(int team, const float3& pos, const float3& speed, const S3DOPiece* _object, const S3DOPrimitive* piece)
	{
		Init(team, pos, speed);

		//! 3D0
		prim = piece;
		object = _object;
	}

	FlyingPiece(int team, const float3& pos, const float3& speed, int textureType, SS3OVertex* _verts)
	{
		Init(team, pos, speed);

		//! S30
		verts = _verts;
		texture = textureType;
	}

	~FlyingPiece();

	void Draw(int modelType, size_t* lastTeam, size_t* lastTex, CVertexArray* va);

public:
	const S3DOPrimitive* prim;
	const S3DOPiece* object;

	SS3OVertex* verts;
	size_t texture;

	float3 pos;
	float3 speed;
	float3 rotAxis;
	float rot;
	float rotSpeed;

	size_t team;

private:
	void Init(int _team, const float3& _pos, const float3& _speed);
};

#endif // FLYING_PIECE_H

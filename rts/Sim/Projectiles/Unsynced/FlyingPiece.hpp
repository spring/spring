/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLYING_PIECE_HDR
#define FLYING_PIECE_HDR

#if !defined(USE_MMGR) && !(defined(USE_GML) && GML_ENABLE_SIM)
#include "mmgr.h"
#include "MemPool.h"
#endif

#include "System/float3.h"

struct S3DOPrimitive;
struct S3DOPiece;
struct SS3OVertex;

struct FlyingPiece {
	#if !defined(USE_MMGR) && !(defined(USE_GML) && GML_ENABLE_SIM)
	inline void* operator new(size_t size) { return mempool.Alloc(size); }
	inline void operator delete(void* p, size_t size) { mempool.Free(p, size); }
	#endif

	FlyingPiece(): prim(0), object(0), verts(0) {}
	~FlyingPiece();

	const S3DOPrimitive* prim;
	const S3DOPiece* object;

	SS3OVertex* verts; /* SS3OVertex[4], our deletion. */

	float3 pos;
	float3 speed;
	float3 rotAxis;
	float rot;
	float rotSpeed;

	size_t texture;
	size_t team;
};

#endif

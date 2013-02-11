/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLYING_PIECE_H
#define FLYING_PIECE_H

#include "lib/gml/gmlcnf.h"

#if !(defined(USE_GML) && GML_ENABLE_SIM)
#include "System/MemPool.h"
#endif

#include "System/float3.h"
#include "System/Matrix44f.h"

class CVertexArray;
struct S3DOPrimitive;
struct S3DOPiece;
struct SS3OVertex;

struct FlyingPiece {
public:
	virtual ~FlyingPiece() {}
	virtual void Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* va) {}

	bool Update();

	size_t GetTeam() const { return team; }
	size_t GetTexture() const { return texture; }

	#if !(defined(USE_GML) && GML_ENABLE_SIM)
	inline void* operator new(size_t size) { return mempool.Alloc(size); }
	inline void operator delete(void* p, size_t size) { mempool.Free(p, size); }
	#endif

protected:
	void InitCommon(const float3& _pos, const float3& _speed, int _team);
	void DrawCommon(size_t* lastTeam, CVertexArray* va);

protected:
	CMatrix44f transMat;

	float3 pos;
	float3 speed;
	float3 rotAxis;

	float rotAngle;
	float rotSpeed;

	size_t team;
	size_t texture;
};



struct S3DOFlyingPiece: public FlyingPiece {
public:
	S3DOFlyingPiece(const float3& pos, const float3& speed, int team, const S3DOPiece* _piece, const S3DOPrimitive* _chunk)
	{
		InitCommon(pos, speed, team);

		piece = _piece;
		chunk = _chunk;
	}

	void Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* va);

private:
	const S3DOPiece* piece;
	const S3DOPrimitive* chunk;
};

struct SS3OFlyingPiece: public FlyingPiece {
public:
	~SS3OFlyingPiece();
	SS3OFlyingPiece(const float3& pos, const float3& speed, int team, int textureType, const SS3OVertex* _chunk)
	{
		InitCommon(pos, speed, team);

		chunk = _chunk;
		texture = textureType;
	}

	void Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* va);

private:
	const SS3OVertex* chunk;
};

#endif // FLYING_PIECE_H


/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLYING_PIECE_H
#define FLYING_PIECE_H


#include "System/float3.h"
#include "System/type2.h"
#include "System/Matrix44f.h"

class CVertexArray;
struct S3DOPrimitive;
struct S3DOPiece;
struct SVertexData;
struct S3DModelPiece;
struct SS3OPiece;
struct SOBJPiece;
struct SAssPiece;


struct FlyingPiece {
public:
	virtual ~FlyingPiece() {}
	virtual void Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* const va) = 0;
	virtual bool Update() = 0;
	virtual unsigned GetTriangleCount() const = 0;

	size_t GetTeam() const { return team; }
	size_t GetTexture() const { return texture; }

	float3 GetPos() const { return pos; }
	float GetRadius() const { return radius + speed.Length() * 2.0f; }


protected:
	inline void InitCommon(const float3 _pos, const float3 _speed, const float _radius, int _team, int _texture);
	inline void DrawCommon(size_t* lastTeam, size_t* lastTex, CVertexArray* const va);

protected:
	float3 pos;
	float3 speed;
	float radius;

	size_t team;
	size_t texture;
};



struct S3DOFlyingPiece: public FlyingPiece {
public:
	S3DOFlyingPiece(const float3& pos, const float3& speed, int team, const S3DOPiece* _piece, const S3DOPrimitive* _chunk);

	void Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* const va);
	bool Update();
	unsigned GetTriangleCount() const;

private:
	const S3DOPiece* piece;
	const S3DOPrimitive* chunk;

	CMatrix44f transMat;
	float3 rotAxis;
	float rotAngle;
	float rotSpeed;
};



struct SNewFlyingPiece: public FlyingPiece {
public:
	SNewFlyingPiece(
		const std::vector<SVertexData>& vertices,
		const std::vector<unsigned int>& indices,
		const float3 pos,
		const float3 speed,
		const CMatrix44f& _pieceMatrix,
		const float2 _pieceParams,
		const int2 _renderParams
	);

	bool Update() override;
	void Draw(size_t* lastTeam, size_t* lastTex, CVertexArray* const va) override;
	unsigned GetTriangleCount() const;

private:
	inline float3 GetDragFactors() const;
	inline CMatrix44f GetMatrixOf(const size_t i, const float3 dragFactors) const;
	inline const SVertexData& GetVertexData(unsigned short i) const;
	inline float3 GetPolygonDir(unsigned short idx) const;

private:
	const std::vector<SVertexData>& vertices;
	const std::vector<unsigned int>& indices;

	float3 pos0;
	unsigned age;
	float pieceRadius;
	const CMatrix44f pieceMatrix;

	std::vector<unsigned short> polygon;
	std::vector<float4> rotationAxisAndSpeed;
	std::vector<float3> speeds;
};

#endif // FLYING_PIECE_H


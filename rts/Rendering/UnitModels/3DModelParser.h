#ifndef SPRING_3DMODELPARSER_H
#define SPRING_3DMODELPARSER_H

#include <vector>
#include <string>
#include "Matrix44f.h"
#include "System/GlobalStuff.h"
#include "Sim/Units/Unit.h"

struct S3DO;
struct SS3O;
class C3DOParser;
class CS3OParser;
struct S3DOModel;
struct LocalS3DOModel;


class C3DModelParser
{
public:
	C3DModelParser(void);
	~C3DModelParser(void);

	void Update();
	S3DOModel* Load3DModel(std::string name, float scale = 1.0f, int side = 1);
	// S3DOModel* Load3DO(string name,float scale,int side,const float3& offsets);
	std::set<CUnit *> fixLocalModels;
	std::vector<LocalS3DOModel *> deleteLocalModels;
	void DeleteLocalModel(CUnit *unit);
	void CreateLocalModel(CUnit *unit);
	void FixLocalModel(CUnit *unit);
	LocalS3DOModel *CreateLocalModel(S3DOModel *model, std::vector<struct PieceInfo> *pieces);

	C3DOParser* unit3doparser;
	CS3OParser* units3oparser;
};

extern C3DModelParser* modelParser;


struct S3DOModel
{
	S3DO* rootobject3do;
	SS3O* rootobjects3o;
	int numobjects;
	float radius;
	float height;
	std::string name;
	int farTextureNum;
	float maxx,maxy,maxz;
	float minx,miny,minz;
	float3 relMidPos;
	int textureType;		//0=3do, otherwise s3o
	std::string tex1;
	std::string tex2;
	void DrawStatic();
};


struct PieceInfo;

struct LocalS3DO
{
	float3 offset;
	unsigned int displist;
	std::vector<unsigned int> lodDispLists;
	std::string name;
	std::vector<LocalS3DO*> childs;
	LocalS3DO *parent;
	S3DO *original3do;
	SS3O *originals3o;
	PieceInfo *anim;
	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);
	void ApplyTransform() const;
	void GetPiecePosIter(CMatrix44f* mat) const;
};


struct LocalS3DOModel
{	
	int numpieces;
	//LocalS3DO *rootobject;
	LocalS3DO *pieces;
	int *scritoa;  //scipt index to local array index
	unsigned int lodCount;

	LocalS3DOModel() : lodCount(0) {};
	~LocalS3DOModel();

	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);

	int ScriptToArray(int piecenum) const;

	bool PieceExists(int scriptnum) const;

	void ApplyPieceTransform(int scriptnum) const;
	float3 GetPiecePos(int scriptnum) const;
	CMatrix44f GetPieceMatrix(int scriptnum) const;
	float3 GetPieceDirection(int scriptnum) const;
	int GetPieceVertCount(int scriptnum) const;
	void GetEmitDirPos(int scriptnum, float3 &pos, float3 &dir) const;

	// raw forms, the piecenum must be valid
	void ApplyRawPieceTransform(int piecenum) const;
	float3 GetRawPiecePos(int piecenum) const;
	CMatrix44f GetRawPieceMatrix(int piecenum) const;
	float3 GetRawPieceDirection(int piecenum) const;
	int GetRawPieceVertCount(int piecenum) const;
	void GetRawEmitDirPos(int piecenum, float3 &pos, float3 &dir) const;
};


inline int LocalS3DOModel::ScriptToArray(int scriptnum) const
{
	if ((scriptnum < 0) || (scriptnum >= numpieces)) {
		return -1;
	}
	return scritoa[scriptnum];
}


inline bool LocalS3DOModel::PieceExists(int scriptnum) const
{
	const int p = ScriptToArray(scriptnum);
	if (p < 0) {
		return false;
	}
	return true;
}


inline void LocalS3DOModel::ApplyPieceTransform(int scriptnum) const
{
	const int p = ScriptToArray(scriptnum);
	if (p < 0) {
		return;
	}
	ApplyRawPieceTransform(p);
}


inline float3 LocalS3DOModel::GetPiecePos(int scriptnum) const
{
	const int p = ScriptToArray(scriptnum);
	if (p < 0) {
		return ZeroVector;
	}
	return GetRawPiecePos(p);
}


inline CMatrix44f LocalS3DOModel::GetPieceMatrix(int scriptnum) const
{
	const int p = ScriptToArray(scriptnum);
	if (p < 0) {
		return CMatrix44f();
	}
	return GetRawPieceMatrix(p);
}


inline float3 LocalS3DOModel::GetPieceDirection(int scriptnum) const
{
	const int p = ScriptToArray(scriptnum);
	if (p < 0) {
		return float3(1.0f, 1.0f, 1.0f);
	}
	return GetRawPieceDirection(p);
}


inline int LocalS3DOModel::GetPieceVertCount(int scriptnum) const
{
	const int p = ScriptToArray(scriptnum);
	if (p < 0) {
		return 0;
	}
	return GetRawPieceVertCount(p);
}


inline void LocalS3DOModel::GetEmitDirPos(int scriptnum, float3 &pos, float3 &dir) const
{
	const int p = ScriptToArray(scriptnum);
	if (p < 0) {
		return;
	}
	return GetRawEmitDirPos(p, pos, dir);
}


#endif /* SPRING_3DMODELPARSER_H */

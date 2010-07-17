/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <map>
#include <vector>
#include <string>
#include <set>

#include "Matrix44f.h"
#include "3DModel.h"


class CUnit;
class C3DOParser;
class CS3OParser;
class C3DModelLoader;
extern C3DModelLoader* modelParser;


class IModelParser
{
public:
	virtual S3DModel* Load(const std::string& name) = 0;
};


class C3DModelLoader
{
public:
	C3DModelLoader(void);
	~C3DModelLoader(void);

	void Update();
	S3DModel* Load3DModel(std::string name, const float3& centerOffset = ZeroVector);

	void AddParser(const std::string& ext, IModelParser* parser);

	void DeleteLocalModel(CUnit* unit);
	void CreateLocalModel(CUnit* unit);
	void FixLocalModel(CUnit* unit);

private:
//FIXME make some static?
	std::map<std::string, S3DModel*> cache;
	std::map<std::string, IModelParser*> parsers;

#if defined(USE_GML) && GML_ENABLE_SIM
	std::vector<S3DModelPiece*> createLists;

	std::set<CUnit*> fixLocalModels;
	std::vector<LocalModel*> deleteLocalModels;
#endif

	void CreateLists(S3DModelPiece* o);
	void CreateListsNow(S3DModelPiece* o);

	void DeleteChilds(S3DModelPiece* o);

	LocalModel* CreateLocalModel(S3DModel* model);
	void CreateLocalModelPieces(S3DModelPiece* model, LocalModel* lmodel, int* piecenum);

	void FixLocalModel(S3DModelPiece* model, LocalModel* lmodel, int* piecenum);
};

#endif /* IMODELPARSER_H */

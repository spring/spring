/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <map>
#include <vector>
#include <string>
#include <set>

#include "System/Matrix44f.h"
#include "3DModel.h"


class IModelParser
{
public:
	virtual S3DModel* Load(const std::string& name) = 0;
};


class CUnit;
class CAssParser;

class C3DModelLoader
{
public:
	C3DModelLoader();
	~C3DModelLoader();

	void Update();
	S3DModel* Load3DModel(std::string name, const float3& centerOffset = ZeroVector);

	void DeleteLocalModel(CUnit* unit);
	void CreateLocalModel(CUnit* unit);

	typedef std::map<std::string, S3DModel*> ModelMap;
	typedef std::map<std::string, IModelParser*> ParserMap;

private:
	// FIXME make some static?
	ModelMap cache;
	ParserMap parsers;

#if defined(USE_GML) && GML_ENABLE_SIM && !GML_SHARE_LISTS
	std::vector<S3DModelPiece*> createLists;

	std::set<CUnit*> fixLocalModels;
	std::vector<LocalModel*> deleteLocalModels;
#endif

	void CreateLists(S3DModelPiece* o);
	void CreateListsNow(S3DModelPiece* o);

	void DeleteChilds(S3DModelPiece* o);

	void SetLocalModelPieceDisplayLists(CUnit* unit);
	void SetLocalModelPieceDisplayLists(S3DModelPiece* model, LocalModel* lmodel, int* piecenum);
};

extern C3DModelLoader* modelParser;

#endif /* IMODELPARSER_H */

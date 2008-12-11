#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <vector>
#include <string>
#include <set>
#include "Matrix44f.h"
#include "Sim/Units/Unit.h"
#include "3DModel.h"


class C3DOParser;
class CS3OParser;
class C3DModelParser;
extern C3DModelParser* modelParser;


class IModelParser
{
public:
	virtual S3DModel* Load(std::string name) = 0;
	virtual void Draw(S3DModelPiece* o) = 0;
};


class C3DModelParser
{
public:
	C3DModelParser(void);
	~C3DModelParser(void);

	void Update();
	S3DModel* Load3DModel(std::string name);

	void AddParser(const std::string ext, IModelParser* parser);

	void DeleteLocalModel(CUnit* unit);
	void CreateLocalModel(CUnit* unit);
	void FixLocalModel(CUnit* unit);

private:
//FIXME make some static?
	std::map<std::string,S3DModel*> cache;
	std::map<std::string,IModelParser*> parsers;

#if defined(USE_GML) && GML_ENABLE_SIM
	struct ModelParserPair {
		ModelParserPair(S3DModelPiece* o,IModelParser* p) : model(o),parser(p) {};
		S3DModelPiece* model;
		IModelParser* parser;
	};
	std::vector<ModelParserPair> createLists;

	std::set<CUnit*> fixLocalModels;
	std::vector<LocalModel*> deleteLocalModels;
#endif

	void CreateLists(IModelParser* parser, S3DModelPiece* o);
	void CreateListsNow(IModelParser* parser, S3DModelPiece* o);

	void DeleteChilds(S3DModelPiece* o);

	LocalModel* CreateLocalModel(S3DModel *model);
	void CreateLocalModelPieces(S3DModelPiece* model, LocalModel* lmodel, int* piecenum);

	void FixLocalModel(S3DModelPiece* model, LocalModel* lmodel, int* piecenum);

/*
	//FIXME: abstract this too
	//s3o
	void FindMinMax(SS3O *object);
	//3do
	void FindCenter(S3DO* object);
	float FindRadius(S3DO* object,float3 offset);
	float FindHeight(S3DO* object,float3 offset);
*/
};

#endif /* IMODELPARSER_H */

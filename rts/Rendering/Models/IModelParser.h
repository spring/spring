/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <map>
#include <string>
#include <list>

#include "System/Matrix44f.h"
#include "3DModel.h"


class IModelParser
{
public:
	virtual S3DModel* Load(const std::string& name) = 0;
	virtual ~IModelParser() {}
};

class C3DModelLoader
{
public:
	C3DModelLoader();
	~C3DModelLoader();

	void Update();
	void CreateLocalModel(LocalModel* model);
	void DeleteLocalModel(LocalModel* model);

	std::string FindModelPath(std::string name) const;
	S3DModel* Load3DModel(std::string modelName);

	typedef std::map<std::string, unsigned int> ModelMap;
	typedef std::map<std::string, IModelParser*> ParserMap;

private:
	void AddModelToCache(S3DModel* model, const std::string& modelName, const std::string& modelPath);
	void CreateLists(S3DModelPiece* o);
	void CreateListsNow(S3DModelPiece* o);

	ModelMap cache;
	ParserMap parsers;

	std::vector<S3DModel*> models;
	std::list<S3DModelPiece*> createLists;

	std::list<LocalModel*> fixLocalModels;
	std::list<LocalModel*> deleteLocalModels;
};

extern C3DModelLoader* modelParser;

#endif /* IMODELPARSER_H */

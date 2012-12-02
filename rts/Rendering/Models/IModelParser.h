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

	std::string Find(std::string name) const;
	S3DModel* Load3DModel(std::string name);

	typedef std::map<std::string, S3DModel*> ModelMap;
	typedef std::map<std::string, IModelParser*> ParserMap;

private:
	ModelMap cache;
	ParserMap parsers;
	std::list<S3DModel*> models;

	std::list<S3DModelPiece*> createLists;

	std::list<LocalModel*> fixLocalModels;
	std::list<LocalModel*> deleteLocalModels;

	void CreateLists(S3DModelPiece* o);
	void CreateListsNow(S3DModelPiece* o);

	void DeleteChildren(S3DModelPiece* o);
};

extern C3DModelLoader* modelParser;

#endif /* IMODELPARSER_H */

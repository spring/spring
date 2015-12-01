/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <unordered_map>
#include <string>
#include <list>

#include "System/Matrix44f.h"
#include "3DModel.h"


class IModelParser
{
public:
	virtual ~IModelParser() {}
	virtual S3DModel* Load(const std::string& name) = 0;
	virtual ModelType GetType() const = 0;
};

class C3DModelLoader
{
public:
	C3DModelLoader();
	~C3DModelLoader();

	std::string FindModelPath(std::string name) const;
	S3DModel* Load3DModel(std::string modelName);

	typedef std::unordered_map<std::string, unsigned int> ModelMap; // "armflash.3do" --> id
	typedef std::unordered_map<std::string, unsigned int> FormatMap; // "3do" --> MODELTYPE_3DO
	typedef std::unordered_map<unsigned int, IModelParser*> ParserMap; // MODELTYPE_3DO --> parser

private:
	void AddModelToCache(S3DModel* model, const std::string& modelName, const std::string& modelPath);
	void CreateLists(S3DModelPiece* o);
	void CreateListsNow(S3DModelPiece* o);

	ModelMap cache;
	FormatMap formats;
	ParserMap parsers;

	// all unique models loaded so far
	std::vector<S3DModel*> models;
};

extern C3DModelLoader* modelParser;

#endif /* IMODELPARSER_H */

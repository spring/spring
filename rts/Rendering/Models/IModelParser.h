/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <deque>
#include <string>

#include "3DModel.h"
#include "System/UnorderedMap.hpp"
#include "System/Threading/SpringThreading.h"


class IModelParser
{
public:
	virtual ~IModelParser() {}
	virtual S3DModel Load(const std::string& name) = 0;
};





class CModelLoader
{
public:
	static CModelLoader& GetInstance();

	void Init();
	void Kill();

	S3DModel* LoadModel(std::string name, bool preload = false);
	std::string FindModelPath(std::string name) const;

	bool IsValid() const { return (!formats.empty()); }
	void PreloadModel(const std::string& name);
	void LogErrors();

public:
	typedef spring::unordered_map<std::string, unsigned int> ModelMap; // "armflash.3do" --> id
	typedef spring::unordered_map<std::string, unsigned int> FormatMap; // "3do" --> MODELTYPE_3DO
	typedef spring::unordered_map<unsigned int, IModelParser*> ParserMap; // MODELTYPE_3DO --> parser

private:
	S3DModel ParseModel(const std::string& name, const std::string& path);
	S3DModel* CreateModel(const std::string& name, const std::string& path, bool preload);
	S3DModel* LoadCachedModel(const std::string& name, bool preload);

	IModelParser* GetFormatParser(const std::string& pathExt);

	void KillModels();
	void KillParsers();

	void UploadRenderData(S3DModel* o);

private:
	ModelMap cache;
	FormatMap formats;
	ParserMap parsers;

	spring::mutex mutex;

	// all unique models loaded so far
	std::deque<S3DModel> models;
	std::deque< std::pair<std::string, std::string> > errors;
};

#define modelLoader (CModelLoader::GetInstance())

#endif /* IMODELPARSER_H */

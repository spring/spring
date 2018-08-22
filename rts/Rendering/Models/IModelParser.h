/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <vector>
#include <string>

#include "3DModel.h"
#include "System/UnorderedMap.hpp"
#include "System/Threading/SpringThreading.h"


class IModelParser
{
public:
	virtual ~IModelParser() {}
	virtual void Init() {}
	virtual void Kill() {}
	virtual S3DModel Load(const std::string& name) = 0;
};


class CModelLoader
{
public:
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
	typedef std::array<IModelParser*, MODELTYPE_OTHER> ParserMap; // MODELTYPE_3DO --> parser

private:
	S3DModel ParseModel(const std::string& name, const std::string& path);
	S3DModel* CreateModel(const std::string& name, const std::string& path, bool preload);
	S3DModel* LoadCachedModel(const std::string& name, bool preload);

	IModelParser* GetFormatParser(const std::string& pathExt);

	void InitParsers();
	void KillModels();
	void KillParsers();

	void UploadRenderData(S3DModel* o);

private:
	ModelMap cache;
	FormatMap formats;
	ParserMap parsers;

	spring::mutex mutex;

	std::vector<S3DModel> models;
	std::vector< std::pair<std::string, std::string> > errors;

	// all unique models loaded so far
	unsigned int numModels = 0;
};

extern CModelLoader modelLoader;

#endif /* IMODELPARSER_H */

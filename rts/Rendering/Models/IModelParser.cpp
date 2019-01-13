/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IModelParser.h"
#include "3DOParser.h"
#include "S3OParser.h"
#include "AssParser.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Net/Protocol/NetProtocol.h" // NETLOG
#include "Sim/Misc/CollisionVolume.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Exceptions.h"
#include "System/MainDefines.h" // SNPRINTF
#include "System/SafeUtil.h"
#include "System/Threading/ThreadPool.h"
#include "lib/assimp/include/assimp/Importer.hpp"


CModelLoader modelLoader;

static C3DOParser g3DOParser;
static CS3OParser gS3OParser;
static CAssParser gAssParser;


static bool CheckAssimpWhitelist(const char* aiExt) {
	constexpr std::array<const char*, 5> whitelist = {
		"3ds"  , // 3DSMax
		"dae"  , // Collada
		"lwo"  , // LightWave
		"obj"  ,
		"blend", // Blender
	};

	const auto pred = [&aiExt](const char* wlExt) { return (strcmp(aiExt, wlExt) == 0); };
	const auto iter = std::find_if(whitelist.begin(), whitelist.end(), pred);

	return (iter != whitelist.end());
}

static void RegisterModelFormats(CModelLoader::FormatMap& formats) {
	// file-extension should be lowercase
	formats.insert("3do", MODELTYPE_3DO);
	formats.insert("s3o", MODELTYPE_S3O);

	std::string extension;
	std::string extensions;
	std::string enabledExtensions;

	Assimp::Importer importer;
	// get a ";" separated list of format extensions ("*.3ds;*.lwo;*.mesh.xml;...")
	importer.GetExtensionList(extensions);
	// do not ignore the last extension
	extensions.append(";");

	size_t curIdx = 0;
	size_t nxtIdx = 0;

	// split the list, strip off the "*." extension prefixes
	while ((nxtIdx = extensions.find(';', curIdx)) != std::string::npos) {
		extension = extensions.substr(curIdx, nxtIdx - curIdx);
		extension = extension.substr(extension.find("*.") + 2);
		extension = StringToLower(extension);

		curIdx = nxtIdx + 1;

		if (!CheckAssimpWhitelist(extension.c_str()))
			continue;
		if (formats.find(extension) != formats.end())
			continue;

		formats.insert(extension, MODELTYPE_ASS);
		enabledExtensions.append("*." + extension + ";");
	}

	LOG("[%s] supported (Assimp) model formats: %s", __func__, enabledExtensions.c_str());
}

static S3DModel CreateDummyModel(unsigned int id)
{
	// create a crash-dummy
	S3DModel model;
	model.id = id;
	model.type = MODELTYPE_3DO;
	model.numPieces = 1;
	// give it one empty piece
	model.AddPiece(g3DOParser.AllocPiece());
	model.GetRootPiece()->SetCollisionVolume(CollisionVolume('b', 'z', -UpVector, ZeroVector));
	return model;
}

static void CheckPieceNormals(const S3DModel* model, const S3DModelPiece* modelPiece)
{
	if (modelPiece->GetVertexCount() >= 3) {
		// do not check pseudo-pieces
		unsigned int numNullNormals = 0;

		for (unsigned int n = 0; n < modelPiece->GetVertexCount(); n++) {
			numNullNormals += (modelPiece->GetNormal(n).SqLength() < 0.5f);
		}

		if (numNullNormals > 0) {
			const char* formatStr =
				"[%s] piece \"%s\" of model \"%s\" has %u (of %u) null-normals! "
				"It will either be rendered fully black or with black splotches!";

			const char* modelName = model->name.c_str();
			const char* pieceName = modelPiece->name.c_str();

			LOG_L(L_DEBUG, formatStr, __func__, pieceName, modelName, numNullNormals, modelPiece->GetVertexCount());
		}
	}

	for (const S3DModelPiece* childPiece: modelPiece->children) {
		CheckPieceNormals(model, childPiece);
	}
}



void CModelLoader::Init()
{
	RegisterModelFormats(formats);
	InitParsers();

	models.clear();
	models.resize(MAX_MODEL_OBJECTS);

	// dummy first model, legitimate model IDs start at 1
	models[0] = std::move(CreateDummyModel(numModels = 0));
}

void CModelLoader::InitParsers()
{
	parsers[MODELTYPE_3DO] = &g3DOParser;
	parsers[MODELTYPE_S3O] = &gS3OParser;
	parsers[MODELTYPE_ASS] = &gAssParser;

	for (IModelParser* p: parsers) {
		p->Init();
	}
}

void CModelLoader::Kill()
{
	LogErrors();
	KillModels();
	KillParsers();

	cache.clear();
	formats.clear();
}

void CModelLoader::KillModels()
{
	for (unsigned int i = 0; i < numModels; i++) {
		models[i].DeletePieces();
	}
}

void CModelLoader::KillParsers()
{
	for (IModelParser* p: parsers) {
		p->Kill();
	}

	parsers[MODELTYPE_3DO] = nullptr;
	parsers[MODELTYPE_S3O] = nullptr;
	parsers[MODELTYPE_ASS] = nullptr;
}



std::string CModelLoader::FindModelPath(std::string name) const
{
	// check for empty string because we can be called
	// from Lua*Defs and certain features have no models
	if (name.empty())
		return name;

	const std::string vfsPath = "objects3d/";
	const std::string& fileExt = FileSystem::GetExtension(name);

	if (fileExt.empty()) {
		for (const auto& format: formats) {
			const std::string& formatExt = format.first;

			if (CFileHandler::FileExists(name + "." + formatExt, SPRING_VFS_ZIP)) {
				name.append("." + formatExt);
				break;
			}
		}
	}

	if (CFileHandler::FileExists(name, SPRING_VFS_ZIP))
		return name;

	if (name.find(vfsPath) != std::string::npos)
		return name;

	return (FindModelPath(vfsPath + name));
}


void CModelLoader::PreloadModel(const std::string& modelName)
{
	assert(Threading::IsMainThread());

	if (modelName.empty())
		return;
	if (!ThreadPool::HasThreads())
		return;

	// if already in cache, thread just returns early
	// not spawning the thread at all would be better but still
	// requires locking around cache.find(...) since some other
	// preload worker might be down in CreateModel modifying it
	// at the same time
	ThreadPool::Enqueue([modelName]() {
		modelLoader.LoadModel(modelName, true);
	});
}

void CModelLoader::LogErrors()
{
	assert(Threading::IsMainThread());

	if (errors.empty())
		return;

	// block any preload threads from modifying <errors>
	// doing the empty-check outside lock should be fine
	std::lock_guard<spring::mutex> lock(mutex);

	for (const auto& pair: errors) {
		char buf[1024];

		SNPRINTF(buf, sizeof(buf), "could not load model \"%s\" (reason: %s)", pair.first.c_str(), pair.second.c_str());
		LOG_L(L_ERROR, "%s", buf);
		CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, buf);
	}

	errors.clear();
}


S3DModel* CModelLoader::LoadModel(std::string name, bool preload)
{
	// cannot happen except through SpawnProjectile
	if (name.empty())
		return nullptr;

	std::string  path;
	std::string* refs[2] = {&name, &path};

	StringToLowerInPlace(name);

	{
		std::lock_guard<spring::mutex> lock(mutex);

		// search in cache first
		for (const auto& ref: refs) {
			S3DModel* cachedModel = LoadCachedModel(*ref, preload);

			if (cachedModel != nullptr)
				return cachedModel;

			// expensive, delay until needed
			path = FindModelPath(name);
		}
	}

	// not found in cache, create the model and cache it
	return (CreateModel(name, path, preload));
}

S3DModel* CModelLoader::LoadCachedModel(const std::string& name, bool preload)
{
	// caller has lock
	const auto ci = cache.find(name);

	if (ci == cache.end())
		return nullptr;

	S3DModel* cachedModel = &models[ci->second];

	if (!preload)
		UploadRenderData(cachedModel);

	return cachedModel;
}



S3DModel* CModelLoader::CreateModel(
	const std::string& name,
	const std::string& path,
	bool preload
) {
	S3DModel model = std::move(ParseModel(name, path));
	S3DModel* pmodel = &models[0];

	{
		assert(model.numPieces != 0);
		assert(model.GetRootPiece() != nullptr);

		model.SetPieceMatrices();

		if (!preload)
			UploadRenderData(&model);
	}
	{
		std::lock_guard<spring::mutex> lock(mutex);

		// discard loaded model and return dummy if at limit
		if (numModels >= MAX_MODEL_OBJECTS) {
			errors.emplace_back(name, "numModels >= MAX_MODEL_OBJECTS");
			return pmodel;
		}

		// NB: id depends on thread order, can not be used in synced code
		pmodel = &models[model.id = ++numModels];

		// add (parsed or dummy) model to cache
		cache[name] = model.id;
		cache[path] = model.id;

		*pmodel = std::move(model);
	}

	return pmodel;
}



IModelParser* CModelLoader::GetFormatParser(const std::string& pathExt)
{
	const auto fi = formats.find(StringToLower(pathExt));

	if (fi == formats.end())
		return nullptr;

	return parsers[fi->second];
}

S3DModel CModelLoader::ParseModel(const std::string& name, const std::string& path)
{
	S3DModel model;
	IModelParser* parser = GetFormatParser(FileSystem::GetExtension(path));

	if (parser == nullptr) {
		LOG_L(L_ERROR, "could not find a parser for model \"%s\" (unknown format?)", name.c_str());
		return (CreateDummyModel(0));
	}

	try {
		model = std::move(parser->Load(path));
	} catch (const content_error& ex) {
		{
			std::lock_guard<spring::mutex> lock(mutex);
			errors.emplace_back(name, ex.what());
		}

		model = std::move(CreateDummyModel(0));
	}

	return model;
}



void CModelLoader::UploadRenderData(S3DModel* model) {
	if (model->UploadedBuffers())
		return;

	model->UploadBuffers();

	if (model->type == MODELTYPE_3DO)
		return;

	// make sure textures (already preloaded) are fully loaded
	textureHandlerS3O.LoadTexture(model);

	// warn about models with bad normals, they break lighting
	// skip for 3DO's since those have auto-calculated normals
	CheckPieceNormals(model, model->GetRootPiece());
}


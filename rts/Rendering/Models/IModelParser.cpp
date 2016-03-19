/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <mutex>
#include "IModelParser.h"
#include "3DModel.h"
#include "3DModelLog.h"
#include "3DOParser.h"
#include "S3OParser.h"
#include "OBJParser.h"
#include "AssParser.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/maindefines.h"
#include "System/ThreadPool.h"
#include "lib/assimp/include/assimp/Importer.hpp"


static void RegisterAssimpModelFormats(CModelLoader::FormatMap& formats) {
	std::set<std::string> whitelist;
	std::string extension;
	std::string extensions;
	std::string enabledExtensions;

	whitelist.insert("3ds"  ); // 3DSMax
	whitelist.insert("dae"  ); // Collada
	whitelist.insert("lwo"  ); // LightWave
	whitelist.insert("blend"); // Blender

	Assimp::Importer importer;
	// get a ";" separated list of format extensions ("*.3ds;*.lwo;*.mesh.xml;...")
	importer.GetExtensionList(extensions);

	// do not ignore the last extension
	extensions += ";";

	size_t curIdx = 0;
	size_t nxtIdx = 0;

	// split the list, strip off the "*." extension prefixes
	while ((nxtIdx = extensions.find(";", curIdx)) != std::string::npos) {
		extension = extensions.substr(curIdx, nxtIdx - curIdx);
		extension = extension.substr(extension.find("*.") + 2);
		extension = StringToLower(extension);

		curIdx = nxtIdx + 1;

		if (whitelist.find(extension) == whitelist.end())
			continue;
		if (formats.find(extension) != formats.end())
			continue;

		formats[extension] = MODELTYPE_ASS;
		enabledExtensions += "*." + extension + ";";
	}

	LOG("[%s] supported Assimp model formats: %s", __FUNCTION__, enabledExtensions.c_str());
}

static S3DModel* CreateDummyModel()
{
	// create a crash-dummy
	S3DModel* model = new S3DModel();
	model->type = MODELTYPE_3DO;
	model->numPieces = 1;
	// give it one empty piece
	model->SetRootPiece(new S3DOPiece());
	model->GetRootPiece()->SetCollisionVolume(CollisionVolume("box", -UpVector, ZeroVector));
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

			LOG_L(L_WARNING, formatStr, __FUNCTION__, pieceName, modelName, numNullNormals, modelPiece->GetVertexCount());
		}
	}

	for (const S3DModelPiece* childPiece: modelPiece->children) {
		CheckPieceNormals(model, childPiece);
	}
}


void CModelLoader::Init()
{
	// file-extension should be lowercase
	formats["3do"] = MODELTYPE_3DO;
	formats["s3o"] = MODELTYPE_S3O;
	formats["obj"] = MODELTYPE_OBJ;

	parsers[MODELTYPE_3DO] = new C3DOParser();
	parsers[MODELTYPE_S3O] = new CS3OParser();
	parsers[MODELTYPE_OBJ] = new COBJParser();
	parsers[MODELTYPE_ASS] = new CAssParser();

	// FIXME: unify the metadata formats of CAssParser and COBJParser
	RegisterAssimpModelFormats(formats);

	// dummy first model, model IDs start at 1
	models.reserve(32);
	models.push_back(nullptr);
}

void CModelLoader::Kill()
{
	KillModels();
	KillParsers();

	cache.clear();
	formats.clear();
}

void CModelLoader::KillModels()
{
	for (unsigned int n = 1; n < models.size(); n++) {
		S3DModel* model = models[n];

		assert(model != nullptr);
		assert(model->GetRootPiece() != nullptr);

		model->DeletePieces(model->GetRootPiece());
		model->SetRootPiece(nullptr);

		SafeDelete(model);
	}

	models.clear();
}

void CModelLoader::KillParsers()
{
	for (auto it = parsers.cbegin(); it != parsers.cend(); ++it) {
		delete (it->second);
	}

	parsers.clear();
}

CModelLoader& CModelLoader::GetInstance()
{
	static CModelLoader instance;
	return instance;
}



std::string CModelLoader::FindModelPath(std::string name) const
{
	// check for empty string because we can be called
	// from Lua*Defs and certain features have no models
	if (!name.empty()) {
		const std::string vfsPath = "objects3d/";
		const std::string& fileExt = FileSystem::GetExtension(name);

		if (fileExt.empty()) {
			for (auto it = formats.cbegin(); it != formats.cend(); ++it) {
				const std::string& formatExt = it->first;

				if (CFileHandler::FileExists(name + "." + formatExt, SPRING_VFS_ZIP)) {
					name += ("." + formatExt); break;
				}
			}
		}

		if (!CFileHandler::FileExists(name, SPRING_VFS_ZIP)) {
			if (name.find(vfsPath) == std::string::npos) {
				return (FindModelPath(vfsPath + name));
			}
		}
	}

	return name;
}


void CModelLoader::PreloadModel(const std::string& modelName)
{
	if (!ThreadPool::HasThreads())
		return;

	if (cache.find(StringToLower(modelName)) != cache.end())
		return;

	ThreadPool::enqueue([modelName](){
		modelLoader.LoadModel(modelName, true);
	});
}


S3DModel* CModelLoader::LoadModel(std::string name, bool preload)
{
	// cannot happen except through SpawnProjectile
	if (name.empty())
		return nullptr;

	std::lock_guard<spring::mutex> lock(mutex);

	StringToLowerInPlace(name);

	std::string  path;
	std::string* refs[2] = {&name, &path};

	// search in cache first
	for (unsigned int n = 0; n < 2; n++) {
		S3DModel* cachedModel = LoadCachedModel(*refs[n], preload);

		if (cachedModel != nullptr)
			return cachedModel;

		// expensive, delay until needed
		path = FindModelPath(name);
	}

	// not found in cache, create the model and cache it
	return (CreateModel(name, path, preload));
}

S3DModel* CModelLoader::LoadCachedModel(const std::string& name, bool preload)
{
	S3DModel* cachedModel = nullptr;

	const auto ci = cache.find(name);
	if (ci != cache.end()) {
		cachedModel = models[ci->second];

		if (!preload) {
			CreateLists(cachedModel);
		}
	}

	return cachedModel;
}



S3DModel* CModelLoader::CreateModel(
	const std::string& name,
	const std::string& path,
	bool preload
) {
	S3DModel* model = ParseModel(name, path);

	if (model == nullptr)
		model = CreateDummyModel();

	assert(model->GetRootPiece() != nullptr);

	if (!preload)
		CreateLists(model);

	AddModelToCache(model, name, path);
	return model;
}



IModelParser* CModelLoader::GetFormatParser(const std::string& pathExt)
{
	const auto fi = formats.find(StringToLower(pathExt));

	if (fi == formats.end())
		return nullptr;

	return parsers[fi->second];
}

S3DModel* CModelLoader::ParseModel(const std::string& name, const std::string& path)
{
	S3DModel* model = nullptr;
	IModelParser* parser = GetFormatParser(FileSystem::GetExtension(path));

	if (parser != nullptr) {
		try {
			model = parser->Load(path);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "could not load model \"%s\" (reason: %s)", name.c_str(), ex.what());
		}
	} else {
		LOG_L(L_ERROR, "could not find a parser for model \"%s\" (unknown format?)", name.c_str());
	}

	return model;
}



void CModelLoader::AddModelToCache(
	S3DModel* model,
	const std::string& name,
	const std::string& path
) {
	model->id = models.size(); // IDs start at 1
	models.push_back(model);

	assert(models[model->id] == model);

	cache[name] = model->id;
	cache[path] = model->id;
}



void CModelLoader::CreateListsNow(S3DModelPiece* o)
{
	o->UploadGeometryVBOs();
	o->CreateShatterPieces();
	o->CreateDispList();

	for (unsigned int n = 0; n < o->GetChildCount(); n++) {
		CreateListsNow(o->GetChild(n));
	}
}


void CModelLoader::CreateLists(S3DModel* model) {
	S3DModelPiece* rootPiece = model->GetRootPiece();

	if (rootPiece->GetDisplayListID() != 0)
		return;

	CreateListsNow(rootPiece);

	if (model->type == MODELTYPE_3DO)
		return;

	// make sure textures (already preloaded) are fully loaded
	texturehandlerS3O->LoadTexture(model);

	// warn about models with bad normals (they break lighting)
	// skip for 3DO's since they are auto-calculated there
	CheckPieceNormals(model, model->GetRootPiece());
}

/******************************************************************************/
/******************************************************************************/

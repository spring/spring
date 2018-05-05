/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AssParser.h"
#include "3DModel.h"
#include "3DModelLog.h"
#include "AssIO.h"

#include "Lua/LuaParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/ScopedFPUSettings.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include "lib/assimp/include/assimp/config.h"
#include "lib/assimp/include/assimp/defs.h"
#include "lib/assimp/include/assimp/types.h"
#include "lib/assimp/include/assimp/scene.h"
#include "lib/assimp/include/assimp/postprocess.h"
#include "lib/assimp/include/assimp/Importer.hpp"
#include "lib/assimp/include/assimp/DefaultLogger.hpp"



#define IS_QNAN(f) (f != f)

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
static const unsigned int ASS_POSTPROCESS_OPTIONS =
	  aiProcess_RemoveComponent
	| aiProcess_FindInvalidData
	| aiProcess_CalcTangentSpace
	| aiProcess_GenSmoothNormals
	| aiProcess_Triangulate
	| aiProcess_GenUVCoords
	| aiProcess_SortByPType
	| aiProcess_JoinIdenticalVertices
	//| aiProcess_ImproveCacheLocality // FIXME crashes in an assert in VertexTriangleAdjancency.h (date 04/2011)
	| aiProcess_SplitLargeMeshes;

static const unsigned int ASS_IMPORTER_OPTIONS =
	aiComponent_CAMERAS |
	aiComponent_LIGHTS |
	aiComponent_TEXTURES |
	aiComponent_ANIMATIONS;
static const unsigned int ASS_LOGGING_OPTIONS =
	Assimp::Logger::Debugging |
	Assimp::Logger::Info |
	Assimp::Logger::Err |
	Assimp::Logger::Warn;



static inline float3 aiVectorToFloat3(const aiVector3D v)
{
	// no-op; AssImp's internal coordinate-system matches Spring's modulo handedness
	return float3(v.x, v.y, v.z);

	// Blender --> Spring
	// return float3(v.x, v.z, -v.y);
}

static inline CMatrix44f aiMatrixToMatrix(const aiMatrix4x4t<float>& m)
{
	CMatrix44f n;

	n[ 0] = m.a1; n[ 1] = m.a2; n[ 2] = m.a3; n[ 3] = m.a4; // 1st column
	n[ 4] = m.b1; n[ 5] = m.b2; n[ 6] = m.b3; n[ 7] = m.b4; // 2nd column
	n[ 8] = m.c1; n[ 9] = m.c2; n[10] = m.c3; n[11] = m.c4; // 3rd column
	n[12] = m.d1; n[13] = m.d2; n[14] = m.d3; n[15] = m.d4; // 4th column

	// AssImp (row-major, RH) --> Spring (column-major, LH)
	return (n.Transpose());

	// Blender --> Spring
	// return (CMatrix44f(n.GetPos(), n.GetX(), n.GetZ(), -n.GetY()));
}

/*
static float3 aiQuaternionToRadianAngles(const aiQuaternion q1)
{
	const float sqw = q1.w * q1.w;
	const float sqx = q1.x * q1.x;
	const float sqy = q1.y * q1.y;
	const float sqz = q1.z * q1.z;
	// <unit> is 1 if normalised, otherwise correction factor
	const float unit = sqx + sqy + sqz + sqw;
	const float test = q1.x * q1.y + q1.z * q1.w;

	aiVector3D angles;

	if (test > (0.499f * unit)) {
		// singularity at north pole
		angles.x = 2.0f * math::atan2(q1.x, q1.w);
		angles.y = PI * 0.5f;
	} else if (test < (-0.499f * unit)) {
		// singularity at south pole
		angles.x = -2.0f * math::atan2(q1.x, q1.w);
		angles.y = -PI * 0.5f;
	} else {
		angles.x = math::atan2(2.0f * q1.y * q1.w - 2.0f * q1.x * q1.z,  sqx - sqy - sqz + sqw);
		angles.y = math::asin((2.0f * test) / unit);
		angles.z = math::atan2(2.0f * q1.x * q1.w - 2.0f * q1.y * q1.z, -sqx + sqy - sqz + sqw);
	}

	return (aiVectorToFloat3(angles));
}
*/




class AssLogStream : public Assimp::LogStream
{
public:
	void write(const char* message) {
		LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "Assimp: %s", message);
	}
};


CAssParser::CAssParser()
{
	// FIXME: non-optimal, maybe compute these ourselves (pre-TL cache size!)
	maxIndices = std::max(globalRendering->glslMaxRecommendedIndices, 1024);
	maxVertices = std::max(globalRendering->glslMaxRecommendedVertices, 1024);

	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	// Create a logger for debugging model loading issues
	Assimp::DefaultLogger::get()->attachStream(new AssLogStream(), ASS_LOGGING_OPTIONS);
}

CAssParser::~CAssParser()
{
	Assimp::DefaultLogger::kill();
}


S3DModel CAssParser::Load(const std::string& modelFilePath)
{
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading model: %s", modelFilePath.c_str());

	const std::string& modelPath = FileSystem::GetDirectory(modelFilePath);
	const std::string& modelName = FileSystem::GetBasename(modelFilePath);

	// load the lua metafile containing properties unique to Spring models (must return a table)
	std::string metaFileName = modelFilePath + ".lua";

	// try again without the model file extension
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP))
		metaFileName = modelPath + modelName + ".lua";

	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP))
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No meta-file '%s'. Using defaults.", metaFileName.c_str());

	LuaParser metaFileParser(metaFileName, SPRING_VFS_ZIP, SPRING_VFS_ZIP);

	if (!metaFileParser.Execute())
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "'%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());

	// get the (root-level) model table
	const LuaTable& modelTable = metaFileParser.GetRoot();

	if (!modelTable.IsValid())
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No valid model metadata in '%s' or no meta-file", metaFileName.c_str());


	// create a model importer instance
	Assimp::Importer importer;


	// give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler(new AssVFSSystem());
	// speed-up processing by skipping things we don't need
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, ASS_IMPORTER_OPTIONS);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT,   maxVertices);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices / 3);

	// Read the model file to build a scene object
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Importing model file: %s", modelFilePath.c_str());

	const aiScene* scene = nullptr;

	{
		// ASSIMP spams many SIGFPEs atm in normal & tangent generation
		ScopedDisableFpuExceptions fe;
		scene = importer.ReadFile(modelFilePath, ASS_POSTPROCESS_OPTIONS);
	}

	if (scene != nullptr) {
		LOG_SL(LOG_SECTION_MODEL, L_INFO,
			"Processing scene for model: %s (%d meshes / %d materials / %d textures)",
			modelFilePath.c_str(), scene->mNumMeshes, scene->mNumMaterials,
			scene->mNumTextures);
	} else {
		throw content_error("[AssimpParser] Model Import: " + std::string(importer.GetErrorString()));
	}

	ModelPieceMap pieceMap;
	ParentNameMap parentMap;

	S3DModel model;
	model.name = modelFilePath;
	model.type = MODELTYPE_ASS;

	// Load textures
	FindTextures(&model, scene, modelTable, modelPath, modelName);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading textures. Tex1: '%s' Tex2: '%s'", model.texs[0].c_str(), model.texs[1].c_str());

	textureHandlerS3O.PreloadTexture(&model, modelTable.GetBool("fliptextures", true), modelTable.GetBool("invertteamcolor", true));

	// Load all pieces in the model
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading pieces from root node '%s'", scene->mRootNode->mName.data);
	LoadPiece(&model, scene->mRootNode, scene, modelTable, pieceMap, parentMap);

	// Update piece hierarchy based on metadata
	BuildPieceHierarchy(&model, pieceMap, parentMap);
	CalculateModelProperties(&model, modelTable);

	// Verbose logging of model properties
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->name: %s", model.name.c_str());
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->numobjects: %d", model.numPieces);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->radius: %f", model.radius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->height: %f", model.height);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->mins: (%f,%f,%f)", model.mins[0], model.mins[1], model.mins[2]);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->maxs: (%f,%f,%f)", model.maxs[0], model.maxs[1], model.maxs[2]);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Model %s Imported.", model.name.c_str());
	return model;
}


/*
void CAssParser::CalculateModelMeshBounds(S3DModel* model, const aiScene* scene)
{
	model->meshBounds.resize(scene->mNumMeshes * 2);

	// calculate bounds for each individual mesh of
	// the model; currently we have no use for this
	// and S3DModel has only one pair of bounds
	//
	for (size_t i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];

		float3& mins = model->meshBounds[i*2 + 0];
		float3& maxs = model->meshBounds[i*2 + 1];

		mins = DEF_MIN_SIZE;
		maxs = DEF_MAX_SIZE;

		for (size_t vertexIndex= 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
			const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			mins = std::min(mins, aiVectorToFloat3(aiVertex));
			maxs = std::max(maxs, aiVectorToFloat3(aiVertex));
		}

		if (mins == DEF_MIN_SIZE) { mins = ZeroVector; }
		if (maxs == DEF_MAX_SIZE) { maxs = ZeroVector; }
	}
}
*/


void CAssParser::LoadPieceTransformations(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable
) {
	aiVector3D aiScaleVec;
	aiVector3D aiTransVec;
	aiQuaternion aiRotateQuat;

	// process transforms
	pieceNode->mTransformation.Decompose(aiScaleVec, aiRotateQuat, aiTransVec);

	const aiMatrix3x3t<float> aiBakedRotMatrix = aiRotateQuat.GetMatrix();
	const aiMatrix4x4t<float> aiBakedMatrix = aiMatrix4x4t<float>(aiBakedRotMatrix);
	CMatrix44f bakedMatrix = aiMatrixToMatrix(aiBakedMatrix);

	// metadata-scaling
	piece->scales   = pieceTable.GetFloat3("scale", aiVectorToFloat3(aiScaleVec));
	piece->scales.x = pieceTable.GetFloat("scalex", piece->scales.x);
	piece->scales.y = pieceTable.GetFloat("scaley", piece->scales.y);
	piece->scales.z = pieceTable.GetFloat("scalez", piece->scales.z);

	if (piece->scales.x != piece->scales.y || piece->scales.y != piece->scales.z) {
		// LOG_SL(LOG_SECTION_MODEL, L_WARNING, "Spring doesn't support non-uniform scaling");
		piece->scales.y = piece->scales.x;
		piece->scales.z = piece->scales.x;
	}

	// metadata-translation
	piece->offset   = pieceTable.GetFloat3("offset", aiVectorToFloat3(aiTransVec));
	piece->offset.x = pieceTable.GetFloat("offsetx", piece->offset.x);
	piece->offset.y = pieceTable.GetFloat("offsety", piece->offset.y);
	piece->offset.z = pieceTable.GetFloat("offsetz", piece->offset.z);

	// metadata-rotation
	// NOTE:
	//   these rotations are "pre-scripting" but "post-modelling"
	//   together with the (baked) aiRotateQuad they determine the
	//   model's pose *before* any animations execute
	//
	// float3 bakedRotAngles = pieceTable.GetFloat3("rotate", aiQuaternionToRadianAngles(aiRotateQuat) * math::RAD_TO_DEG);
	float3 bakedRotAngles = pieceTable.GetFloat3("rotate", ZeroVector);

	bakedRotAngles.x = pieceTable.GetFloat("rotatex", bakedRotAngles.x);
	bakedRotAngles.y = pieceTable.GetFloat("rotatey", bakedRotAngles.y);
	bakedRotAngles.z = pieceTable.GetFloat("rotatez", bakedRotAngles.z);
	bakedRotAngles  *= math::DEG_TO_RAD;

	LOG_SL(LOG_SECTION_PIECE, L_INFO,
		"(%d:%s) Assimp offset (%f,%f,%f), rotate (%f,%f,%f,%f), scale (%f,%f,%f)",
		model->numPieces, piece->name.c_str(),
		aiTransVec.x, aiTransVec.y, aiTransVec.z,
		aiRotateQuat.w, aiRotateQuat.x, aiRotateQuat.y, aiRotateQuat.z,
		aiScaleVec.x, aiScaleVec.y, aiScaleVec.z
	);
	LOG_SL(LOG_SECTION_PIECE, L_INFO,
		"(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)",
		model->numPieces, piece->name.c_str(),
		piece->offset.x, piece->offset.y, piece->offset.z,
		bakedRotAngles.x, bakedRotAngles.y, bakedRotAngles.z,
		piece->scales.x, piece->scales.y, piece->scales.z
	);

	// construct 'baked' piece-space transform
	//
	// AssImp order is Translate * Rotate * Scale * v; the
	// translation and scale parts are split into <offset>
	// and <scales> so the baked part reduces to R
	//
	// note: for all non-AssImp models this is identity!
	piece->SetBakedMatrix(bakedMatrix.RotateEulerYXZ(-bakedRotAngles));
}

void CAssParser::SetPieceName(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	ModelPieceMap& pieceMap
) {
	assert(piece->name.empty());
	piece->name = std::string(pieceNode->mName.data);

	if (piece->name.empty()) {
		if (piece == model->GetRootPiece()) {
			// root is always the first piece created, so safe to assign this
			piece->name = "$$root$$";
			return;
		}

		piece->name = "$$piece$$";
	}

	// find a new name if none given or if a piece with the same name already exists
	ModelPieceMap::const_iterator it = pieceMap.find(piece->name);

	for (unsigned int i = 0; it != pieceMap.end(); i++) {
		const std::string newPieceName = piece->name + IntToString(i, "%02i");

		if ((it = pieceMap.find(newPieceName)) == pieceMap.end()) {
			piece->name = newPieceName; break;
		}
	}

	assert(piece->name != "SpringHeight");
	assert(piece->name != "SpringRadius");
}

void CAssParser::SetPieceParentName(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable,
	ParentNameMap& parentMap
) {
	// parent was updated in GetPieceTableRecursively
	if (parentMap.find(piece->name) != parentMap.end())
		return;

	// Get parent name from metadata or model
	if (pieceTable.KeyExists("parent")) {
		parentMap[piece->name] = pieceTable.GetString("parent", "");
		return;
	}

	if (pieceNode->mParent == nullptr)
		return;

	if (pieceNode->mParent->mParent != nullptr) {
		// parent is not the root
		parentMap[piece->name] = std::string(pieceNode->mParent->mName.data);
	} else {
		// parent is the root (which must already exist)
		assert(model->GetRootPiece() != nullptr);
		parentMap[piece->name] = (model->GetRootPiece())->name;
	}
}

void CAssParser::LoadPieceGeometry(SAssPiece* piece, const S3DModel* model, const aiNode* pieceNode, const aiScene* scene)
{
	// Get vertex data from node meshes
	for (unsigned meshListIndex = 0; meshListIndex < pieceNode->mNumMeshes; ++meshListIndex) {
		const unsigned int meshIndex = pieceNode->mMeshes[meshListIndex];
		const aiMesh* mesh = scene->mMeshes[meshIndex];

		LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching mesh %d from scene", meshIndex);
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
			"Processing vertices for mesh %d (%d vertices)",
			meshIndex, mesh->mNumVertices);
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
			"Normals: %s Tangents/Bitangents: %s TexCoords: %s",
			(mesh->HasNormals() ? "Y" : "N"),
			(mesh->HasTangentsAndBitangents() ? "Y" : "N"),
			(mesh->HasTextureCoords(0) ? "Y" : "N"));

		piece->vertices.reserve(piece->vertices.size() + mesh->mNumVertices);
		piece->indices.reserve(piece->indices.size() + mesh->mNumFaces * 3);

		std::vector<unsigned> meshVertexMapping;

		// extract vertex data per mesh
		for (unsigned vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];

			SAssVertex vertex;

			// vertex coordinates
			vertex.pos = aiVectorToFloat3(aiVertex);
			vertex.pieceIndex = model->numPieces - 1;

			// update piece min/max extents
			piece->mins = float3::min(piece->mins, vertex.pos);
			piece->maxs = float3::max(piece->maxs, vertex.pos);

			// vertex normal
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching normal for vertex %d", vertexIndex);

			const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];

			if (!IS_QNAN(aiNormal))
				vertex.normal = (aiVectorToFloat3(aiNormal)).SafeANormalize();

			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching tangent for vertex %d", vertexIndex);

				const aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				const aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];

				vertex.sTangent = (aiVectorToFloat3(aiTangent)).SafeANormalize();
				vertex.tTangent = (aiVectorToFloat3(aiBitangent)).SafeANormalize();
			}

			// vertex tex-coords per channel
			for (unsigned int uvChanIndex = 0; uvChanIndex < NUM_MODEL_UVCHANNS; uvChanIndex++) {
				if (!mesh->HasTextureCoords(uvChanIndex))
					break;

				piece->SetNumTexCoorChannels(uvChanIndex + 1);

				vertex.texCoords[uvChanIndex].x = mesh->mTextureCoords[uvChanIndex][vertexIndex].x;
				vertex.texCoords[uvChanIndex].y = mesh->mTextureCoords[uvChanIndex][vertexIndex].y;
			}

			meshVertexMapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
		}

		// extract face data
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);

		/*
		 * since aiProcess_SortByPType is being used,
		 * we're sure we'll get only 1 type here,
		 * so combination check isn't needed, also
		 * anything more complex than triangles is
		 * being split thanks to aiProcess_Triangulate
		 */
		for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			const aiFace& face = mesh->mFaces[faceIndex];

			// some models contain lines (mNumIndices == 2) which
			// we cannot render and they would need a 2nd drawcall)
			if (face.mNumIndices != 3)
				continue;

			for (unsigned vertexListID = 0; vertexListID < face.mNumIndices; ++vertexListID) {
				const unsigned int vertexFaceIdx = face.mIndices[vertexListID];
				const unsigned int vertexDrawIdx = meshVertexMapping[vertexFaceIdx];
				piece->indices.push_back(vertexDrawIdx);
			}
		}
	}
}

// Not efficient, but there aren't that many pieces
// So fast anyway
static LuaTable GetPieceTableRecursively(
	const LuaTable& table,
	const std::string& name,
	const std::string& parentName,
	CAssParser::ParentNameMap& parentMap
) {
	LuaTable ret = table.SubTable(name);
	if (ret.IsValid()) {
		if (parentName.size() > 0)
			parentMap[name] = parentName;
		return ret;
	}

	std::vector<std::string> keys;
	table.GetKeys(keys);
	for (const std::string& key: keys) {
		ret = GetPieceTableRecursively(table.SubTable(key), name, key, parentMap);
		if (ret.IsValid())
			break;
	}
	return ret;
}


SAssPiece* CAssParser::LoadPiece(
	S3DModel* model,
	const aiNode* pieceNode,
	const aiScene* scene,
	const LuaTable& modelTable,
	ModelPieceMap& pieceMap,
	ParentNameMap& parentMap
) {
	++model->numPieces;

	SAssPiece* piece = new SAssPiece();

	if (pieceNode->mParent == nullptr) {
		// set the model's root piece ASAP, needed in SetPiece*Name
		assert(pieceNode == scene->mRootNode);
		model->AddPiece(piece);
	}

	SetPieceName(piece, model, pieceNode, pieceMap);

	LOG_SL(LOG_SECTION_PIECE, L_INFO, "Converting node '%s' to piece '%s' (%d meshes).", pieceNode->mName.data, piece->name.c_str(), pieceNode->mNumMeshes);

	// Load additional piece properties from metadata
	const LuaTable& pieceTable = GetPieceTableRecursively(modelTable.SubTable("pieces"), piece->name, "", parentMap);

	if (pieceTable.IsValid())
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "Found metadata for piece '%s'", piece->name.c_str());


	LoadPieceTransformations(piece, model, pieceNode, pieceTable);
	LoadPieceGeometry(piece, model, pieceNode, scene);
	SetPieceParentName(piece, model, pieceNode, pieceTable, parentMap);

	{
		// operator[] creates an empty string if piece is not in map
		const auto parentNameIt = parentMap.find(piece->name);
		const std::string& parentName = (parentNameIt != parentMap.end())? (parentNameIt->second).c_str(): "[null]";

		// Verbose logging of piece properties
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "Loaded model piece: %s with %d meshes", piece->name.c_str(), pieceNode->mNumMeshes);
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "piece->name: %s", piece->name.c_str());
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "piece->parent: %s", parentName.c_str());
	}

	// Recursively process all child pieces
	for (unsigned int i = 0; i < pieceNode->mNumChildren; ++i) {
		LoadPiece(model, pieceNode->mChildren[i], scene, modelTable, pieceMap, parentMap);
	}

	pieceMap[piece->name] = piece;
	return piece;
}


// Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy(S3DModel* model, ModelPieceMap& pieceMap, const ParentNameMap& parentMap)
{
	const char* fmt1 = "Missing piece '%s' declared as parent of '%s'.";
	const char* fmt2 = "Missing root piece (parent of orphan '%s')";

	// loop through all pieces and create missing hierarchy info
	for (auto it = pieceMap.cbegin(); it != pieceMap.cend(); ++it) {
		SAssPiece* piece = static_cast<SAssPiece*>(it->second);

		if (piece == model->GetRootPiece()) {
			assert(!piece->HasParent());
			assert(model->GetRootPiece() == piece);
			continue;
		}

		const auto parentNameIt = parentMap.find(piece->name);

		if (parentNameIt != parentMap.end()) {
			const std::string& parentName = parentNameIt->second;
			const auto pieceIt = pieceMap.find(parentName);

			// re-assign this piece to a different parent
			if (pieceIt != pieceMap.end()) {
				piece->parent = pieceIt->second;
				piece->parent->children.push_back(piece);
			} else {
				LOG_SL(LOG_SECTION_PIECE, L_ERROR, fmt1, parentName.c_str(), piece->name.c_str());
			}

			continue;
		}

		// piece with no named parent that isn't the root (orphaned)
		// link it to the root piece which has already been pre-added
		if ((piece->parent = model->GetRootPiece()) == nullptr) {
			LOG_SL(LOG_SECTION_PIECE, L_ERROR, fmt2, piece->name.c_str());
		} else {
			piece->parent->children.push_back(piece);
		}
	}

	model->FlattenPieceTree(model->GetRootPiece());
}


// Iterate over the model and calculate its overall dimensions
void CAssParser::CalculateModelDimensions(S3DModel* model, S3DModelPiece* piece)
{
	const CMatrix44f scaleRotMat = piece->ComposeTransform(ZeroVector, ZeroVector, piece->scales);

	// cannot set this until parent relations are known, so either here or in BuildPieceHierarchy()
	piece->SetGlobalOffset(scaleRotMat);
	piece->SetCollisionVolume(CollisionVolume('b', 'z', piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	// update model min/max extents
	model->mins = float3::min(piece->goffset + piece->mins, model->mins);
	model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

	// Repeat with children
	for (S3DModelPiece* childPiece: piece->children) {
		CalculateModelDimensions(model, childPiece);
	}
}

// Calculate model radius from the min/max extents
void CAssParser::CalculateModelProperties(S3DModel* model, const LuaTable& modelTable)
{
	CalculateModelDimensions(model, model->GetRootPiece());

	model->mins = modelTable.GetFloat3("mins", model->mins);
	model->maxs = modelTable.GetFloat3("maxs", model->maxs);

	model->radius = modelTable.GetFloat("radius", model->CalcDrawRadius());
	model->height = modelTable.GetFloat("height", model->CalcDrawHeight());

	model->relMidPos = modelTable.GetFloat3("midpos", model->CalcDrawMidPos());
}


static std::string FindTexture(std::string testTextureFile, const std::string& modelPath, const std::string& fallback)
{
	if (testTextureFile.empty())
		return fallback;

	// blender denotes relative paths with "//..", remove it
	if (testTextureFile.find("//..") == 0)
		testTextureFile = testTextureFile.substr(4);

	if (CFileHandler::FileExists(testTextureFile, SPRING_VFS_ZIP_FIRST))
		return testTextureFile;

	if (CFileHandler::FileExists("unittextures/" + testTextureFile, SPRING_VFS_ZIP_FIRST))
		return "unittextures/" + testTextureFile;

	if (CFileHandler::FileExists(modelPath + testTextureFile, SPRING_VFS_ZIP_FIRST))
		return modelPath + testTextureFile;

	return fallback;
}


static std::string FindTextureByRegex(const std::string& regex_path, const std::string& regex)
{
	//FIXME instead of ".*" only check imagetypes!
	const std::vector<std::string>& files = CFileHandler::FindFiles(regex_path, regex + ".*");

	if (!files.empty())
		return FindTexture(FileSystem::GetFilename(files[0]), "", "");

	return "";
}


void CAssParser::FindTextures(
	S3DModel* model,
	const aiScene* scene,
	const LuaTable& modelTable,
	const std::string& modelPath,
	const std::string& modelName
) {
	// 1. try to find by name (lowest priority)
	model->texs[0] = FindTextureByRegex("unittextures/", modelName);

	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex("unittextures/", modelName + "1");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex("unittextures/", modelName + "2");
	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex(modelPath, "tex1");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex(modelPath, "tex2");
	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex(modelPath, "diffuse");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex(modelPath, "glow"); // lowest-priority name

	// 2. gather model-defined textures of first material (medium priority)
	if (scene->mNumMaterials > 0) {
		constexpr unsigned int texTypes[] = {
			aiTextureType_SPECULAR,
			aiTextureType_UNKNOWN,
			aiTextureType_DIFFUSE,
			/*
			// TODO: support these too (we need to allow constructing tex1 & tex2 from several sources)
			aiTextureType_EMISSIVE,
			aiTextureType_HEIGHT,
			aiTextureType_NORMALS,
			aiTextureType_SHININESS,
			aiTextureType_OPACITY,
			*/
		};
		for (unsigned int texType: texTypes) {
			aiString textureFile;
			if (scene->mMaterials[0]->Get(AI_MATKEY_TEXTURE(texType, 0), textureFile) != aiReturn_SUCCESS)
				continue;

			assert(textureFile.length > 0);
			model->texs[0] = FindTexture(textureFile.data, modelPath, model->texs[0]);
		}
	}

	// 3. try to load from metafile (highest priority)
	model->texs[0] = FindTexture(modelTable.GetString("tex1", ""), modelPath, model->texs[0]);
	model->texs[1] = FindTexture(modelTable.GetString("tex2", ""), modelPath, model->texs[1]);
}


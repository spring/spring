/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AssParser.h"

#include "3DModel.h"
#include "3DModelLog.h"
#include "S3OParser.h"
#include "AssIO.h"

#include "Lua/LuaParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include "lib/assimp/include/assimp/config.h"
#include "lib/assimp/include/assimp/defs.h"
#include "lib/assimp/include/assimp/types.h"
#include "lib/assimp/include/assimp/scene.h"
#include "lib/assimp/include/assimp/postprocess.h"
#include "lib/assimp/include/assimp/Importer.hpp"
#include "lib/assimp/include/assimp/DefaultLogger.hpp"
#include "Rendering/Textures/S3OTextureHandler.h"
#ifndef BITMAP_NO_OPENGL
	#include "Rendering/GL/myGL.h"
#endif


#define IS_QNAN(f) (f != f)
static const float DEGTORAD = PI / 180.f;
static const float RADTODEG = 180.f / PI;

static const float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static const float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
static const int ASS_POSTPROCESS_OPTIONS =
	  aiProcess_RemoveComponent
	| aiProcess_FindInvalidData
	| aiProcess_CalcTangentSpace
	| aiProcess_GenSmoothNormals
	| aiProcess_Triangulate
	| aiProcess_GenUVCoords
	| aiProcess_SortByPType
	| aiProcess_JoinIdenticalVertices
	//| aiProcess_ImproveCacheLocality FIXME crashes in an assert in VertexTriangleAdjancency.h (date 04/2011)
	| aiProcess_SplitLargeMeshes;


static inline float3 aiVectorToFloat3(const aiVector3D v)
{
	return float3(v.x, v.y, v.z);
}


static inline CMatrix44f aiMatrixToMatrix(const aiMatrix4x4t<float>& m)
{
	CMatrix44f n;

	// assimp uses row-major, spring column-major, transform it
	n[0] = m.a1; n[4] = m.a2; n[8 ] = m.a3; n[12] = m.a4;
	n[1] = m.b1; n[5] = m.b2; n[9 ] = m.b3; n[13] = m.b4;
	n[2] = m.c1; n[6] = m.c2; n[10] = m.c3; n[14] = m.c4;
	n[3] = m.d1; n[7] = m.d2; n[11] = m.d3; n[15] = m.d4;

	return n;
}


class AssLogStream : public Assimp::LogStream
{
public:
	AssLogStream() {}
	~AssLogStream() {}
	void write(const char* message)
	{
		LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "Assimp: %s", message);
	}
};



S3DModel* CAssParser::Load(const std::string& modelFilePath)
{
	LOG_S(LOG_SECTION_MODEL, "Loading model: %s", modelFilePath.c_str() );
	const std::string modelPath  = FileSystem::GetDirectory(modelFilePath);
	const std::string modelName  = FileSystem::GetBasename(modelFilePath);

	// LOAD METADATA
	// Load the lua metafile. This contains properties unique to Spring models and must return a table
	std::string metaFileName = modelFilePath + ".lua";
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP)) {
		// Try again without the model file extension
		metaFileName = modelPath + '/' + modelName + ".lua";
	}
	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP)) {
		LOG_S(LOG_SECTION_MODEL, "No meta-file '%s'. Using defaults.", metaFileName.c_str());
	} else if (!metaFileParser.Execute()) {
		LOG_SL(LOG_SECTION_MODEL, L_ERROR, "'%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());
	}

	// Get the (root-level) model table
	const LuaTable& metaTable = metaFileParser.GetRoot();
	if (metaTable.IsValid()) {
		LOG_S(LOG_SECTION_MODEL, "Found valid model metadata in '%s'", metaFileName.c_str());
	}


	// LOAD MODEL DATA
	// Create a model importer instance
	Assimp::Importer importer;

	// Create a logger for debugging model loading issues
	Assimp::DefaultLogger::create("",Assimp::Logger::VERBOSE);
	const unsigned int severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
	Assimp::DefaultLogger::get()->attachStream( new AssLogStream(), severity );

	// Give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler( new AssVFSSystem() );

	// Speed-up processing by skipping things we don't need
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS|aiComponent_LIGHTS|aiComponent_TEXTURES|aiComponent_ANIMATIONS);

#ifndef BITMAP_NO_OPENGL
	// Optimize VBO-Mesh sizes/ranges
	GLint maxIndices  = 1024;
	GLint maxVertices = 1024;
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,  &maxIndices);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxVertices); //FIXME returns not optimal data, at best compute it ourself! (pre-TL cache size!)
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT,   maxVertices);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices/3);
#endif

	// Read the model file to build a scene object
	LOG_S(LOG_SECTION_MODEL, "Importing model file: %s", modelFilePath.c_str() );
	const aiScene* scene = importer.ReadFile( modelFilePath, ASS_POSTPROCESS_OPTIONS );
	if (scene != NULL) {
		LOG_S(LOG_SECTION_MODEL,
				"Processing scene for model: %s (%d meshes / %d materials / %d textures)",
				modelFilePath.c_str(), scene->mNumMeshes, scene->mNumMaterials,
				scene->mNumTextures );
	} else {
		throw content_error("[AssimpParser] Model Import: " + std::string(importer.GetErrorString()));
	}

	SAssModel* model = new SAssModel;
	model->name = modelFilePath;
	model->type = MODELTYPE_ASS;
	model->scene = scene;

	// Gather per mesh info
	CalculatePerMeshMinMax(model);

	// Load textures
	FindTextures(model, scene, metaTable, modelName);
	LOG_S(LOG_SECTION_MODEL, "Loading textures. Tex1: '%s' Tex2: '%s'",
			model->tex1.c_str(), model->tex2.c_str());
	texturehandlerS3O->LoadS3OTexture(model);

	// Load all pieces in the model
	LOG_S(LOG_SECTION_MODEL, "Loading pieces from root node '%s'",
			scene->mRootNode->mName.data);
	LoadPiece(model, scene->mRootNode, metaTable);

	// Update piece hierarchy based on metadata
	BuildPieceHierarchy(model);
	CalculateModelProperties(model, metaTable);

	// Verbose logging of model properties
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->name: %s", model->name.c_str());
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->numobjects: %d", model->numPieces);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->radius: %f", model->radius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->height: %f", model->height);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->drawRadius: %f", model->drawRadius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->mins: (%f,%f,%f)", model->mins[0], model->mins[1], model->mins[2]);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->maxs: (%f,%f,%f)", model->maxs[0], model->maxs[1], model->maxs[2]);

	LOG_S(LOG_SECTION_MODEL, "Model %s Imported.", model->name.c_str());
	return model;
}


void CAssParser::CalculatePerMeshMinMax(SAssModel* model)
{
	const aiScene* scene = model->scene;

	model->mesh_minmax.resize(scene->mNumMeshes);
	for (size_t i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh& mesh = *scene->mMeshes[i];

		SAssModel::MinMax& minmax = model->mesh_minmax[i];
		minmax.mins = DEF_MIN_SIZE;
		minmax.maxs = DEF_MAX_SIZE;

		for (size_t vertexIndex= 0; vertexIndex < mesh.mNumVertices; vertexIndex++) {
			const aiVector3D& aiVertex = mesh.mVertices[vertexIndex];
			minmax.mins = std::min(minmax.mins, aiVectorToFloat3(aiVertex));
			minmax.maxs = std::max(minmax.maxs, aiVectorToFloat3(aiVertex));
		}

		if (minmax.mins == DEF_MIN_SIZE)
			minmax.mins = ZeroVector;
		if (minmax.maxs == DEF_MAX_SIZE)
			minmax.maxs = ZeroVector;
	}
}


void CAssParser::LoadPieceTransformations(const S3DModel* model, SAssPiece* piece, const LuaTable& pieceMetaTable)
{
	// Process transforms
	float3 rotate, offset;
	float3 scale(1.0,1.0,1.0);
	aiVector3D _scale, _offset;
	aiQuaternion _rotate;
	piece->node->mTransformation.Decompose(_scale,_rotate,_offset);
	const aiMatrix4x4t<float> aiRotMatrix = aiMatrix4x4t<float>(_rotate.GetMatrix());

	LOG_S(LOG_SECTION_PIECE,
		"(%d:%s) Assimp offset (%f,%f,%f), rotate (%f,%f,%f,%f), scale (%f,%f,%f)",
		model->numPieces, piece->name.c_str(),
		_offset.x, _offset.y, _offset.z,
		_rotate.w, _rotate.x, _rotate.y, _rotate.z,
		_scale.x, _scale.y, _scale.z
	);

	// Scale
	scale   = pieceMetaTable.GetFloat3("scale", float3(_scale.x, _scale.z, _scale.y));
	scale.x = pieceMetaTable.GetFloat("scalex", scale.x);
	scale.y = pieceMetaTable.GetFloat("scaley", scale.y);
	scale.z = pieceMetaTable.GetFloat("scalez", scale.z);

	if (scale.x != scale.y || scale.y != scale.z) {
		//LOG_SL(LOG_SECTION_MODEL, L_WARNING, "Spring doesn't support non-uniform scaling");
		scale.y = scale.x;
		scale.z = scale.x;
	}

	// Rotate
	// Note these rotations are put into the `Spring rotations` and are not baked into the Assimp matrix!
	rotate   = pieceMetaTable.GetFloat3("rotate", float3(0,0,0));
	rotate.x = pieceMetaTable.GetFloat("rotatex", rotate.x);
	rotate.y = pieceMetaTable.GetFloat("rotatey", rotate.y);
	rotate.z = pieceMetaTable.GetFloat("rotatez", rotate.z);
	rotate  *= DEGTORAD;

	// Translate
	offset   = pieceMetaTable.GetFloat3("offset", float3(_offset.x, _offset.y, _offset.z));
	offset.x = pieceMetaTable.GetFloat("offsetx", offset.x);
	offset.y = pieceMetaTable.GetFloat("offsety", offset.y);
	offset.z = pieceMetaTable.GetFloat("offsetz", offset.z);

	LOG_S(LOG_SECTION_PIECE,
		"(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)",
		model->numPieces, piece->name.c_str(),
		offset.x, offset.y, offset.z,
		rotate.x, rotate.y, rotate.z,
		scale.x, scale.y, scale.z
	);

	// construct 'baked' part of the modelpiece matrix
	// Assimp order is: translate * rotate * scale * v
	piece->scaleRotMatrix.LoadIdentity();
	piece->scaleRotMatrix.Scale(scale);
	piece->scaleRotMatrix *= aiMatrixToMatrix(aiRotMatrix);
	// piece->scaleRotMatrix.Translate(offset);

	piece->offset = offset;
	piece->rot = rotate;
	piece->mIsIdentity = (scale.x == 1.0f) && aiRotMatrix.IsIdentity();
}


SAssPiece* CAssParser::LoadPiece(SAssModel* model, aiNode* node, const LuaTable& metaTable)
{
	// Create new piece
	++model->numPieces;
	SAssPiece* piece = new SAssPiece;
	piece->type = MODELTYPE_OTHER;
	piece->node = node;
	piece->isEmpty = (node->mNumMeshes == 0);

	if (node->mParent) {
		piece->name = std::string(node->mName.data);
	} else {
		//FIXME is this really smart?
		piece->name = "root"; //! The real model root
	}

	// find a new name if none given or if a piece with the same name already exists
	if (piece->name.empty()) {
		piece->name = "piece";
	}

	ModelPieceMap::const_iterator it = model->pieces.find(piece->name);
	if (it != model->pieces.end()) {
		char buf[64];
		int i = 0;
		while (it != model->pieces.end()) {
			SNPRINTF(buf, 64, "%s%02i", piece->name.c_str(), i++);
			it = model->pieces.find(buf);
		}
		piece->name = buf;
	}
	LOG_S(LOG_SECTION_PIECE, "Converting node '%s' to piece '%s' (%d meshes).",
			node->mName.data, piece->name.c_str(), node->mNumMeshes);

	// Load additional piece properties from metadata
	const LuaTable& pieceTable = metaTable.SubTable("pieces").SubTable(piece->name);
	if (pieceTable.IsValid()) {
		LOG_S(LOG_SECTION_PIECE, "Found metadata for piece '%s'",
				piece->name.c_str());
	}

	// Load transforms
	LoadPieceTransformations(model, piece, pieceTable);

	// Update piece min/max extents
	for (unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; meshListIndex++) {
		const unsigned int meshIndex = node->mMeshes[meshListIndex];
		const SAssModel::MinMax& minmax = model->mesh_minmax[meshIndex];

		piece->mins = std::min(piece->mins, minmax.mins);
		piece->maxs = std::max(piece->maxs, minmax.maxs);
	}


	// Check if piece is special (ie, used to set Spring model properties)
	if (strcmp(node->mName.data, "SpringHeight") == 0) {
		// Set the model height to this nodes Z value
		if (!metaTable.KeyExists("height")) {
			model->height = piece->offset.z;
			LOG_S(LOG_SECTION_MODEL,
					"Model height of %f set by special node 'SpringHeight'",
					model->height);
		}
		--model->numPieces;
		delete piece;
		return NULL;
	}
	if (strcmp(node->mName.data, "SpringRadius") == 0) {
		if (!metaTable.KeyExists("midpos")) {
			model->relMidPos = piece->scaleRotMatrix.Mul(piece->offset);
			LOG_S(LOG_SECTION_MODEL,
					"Model midpos of (%f,%f,%f) set by special node 'SpringRadius'",
					model->relMidPos.x, model->relMidPos.y, model->relMidPos.z);
		}
		if (!metaTable.KeyExists("radius")) {
			if (piece->maxs.x <= 0.00001f) {
				aiVector3D _scale, _offset;
				aiQuaternion _rotate;
				piece->node->mTransformation.Decompose(_scale,_rotate,_offset);
				model->radius = aiVectorToFloat3(_scale).x; // the blender import script only sets the scale property
			} else {
				model->radius = piece->maxs.x; // use the transformed mesh extents
			}
			LOG_S(LOG_SECTION_MODEL,
					"Model radius of %f set by special node 'SpringRadius'",
					model->radius);
		}
		--model->numPieces;
		delete piece;
		return NULL;
	}



	//! Get vertex data from node meshes
	for (unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; ++meshListIndex) {
		unsigned int meshIndex = node->mMeshes[meshListIndex];
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching mesh %d from scene",
				meshIndex);
		const aiMesh* mesh = model->scene->mMeshes[meshIndex];
		std::vector<unsigned> mesh_vertex_mapping;
		// extract vertex data
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Processing vertices for mesh %d (%d vertices)",
				meshIndex, mesh->mNumVertices);
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals() ? "Y" : "N"),
				(mesh->HasTangentsAndBitangents() ? "Y" : "N"),
				(mesh->HasTextureCoords(0) ? "Y" : "N"));

		piece->vertices.reserve(piece->vertices.size() + mesh->mNumVertices);
		for (unsigned vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			SAssVertex vertex;

			// vertex coordinates
			const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			vertex.pos = aiVectorToFloat3(aiVertex);

			// vertex normal
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching normal for vertex %d",
					vertexIndex);
			const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];
			if (!IS_QNAN(aiNormal)) {
				vertex.normal = aiVectorToFloat3(aiNormal);
			}

			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
						"Fetching tangent for vertex %d", vertexIndex );
				const aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				const aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];

				vertex.sTangent = aiVectorToFloat3(aiTangent);
				vertex.tTangent = aiVectorToFloat3(aiBitangent);
			}

			// vertex texcoords
			if (mesh->HasTextureCoords(0)) {
				vertex.texCoord.x = mesh->mTextureCoords[0][vertexIndex].x;
				vertex.texCoord.y = mesh->mTextureCoords[0][vertexIndex].y;
			}

			if (mesh->HasTextureCoords(1)) {
				piece->hasTexCoord2 = true,
				vertex.texCoord2.x = mesh->mTextureCoords[1][vertexIndex].x;
				vertex.texCoord2.y = mesh->mTextureCoords[1][vertexIndex].y;
			}

			mesh_vertex_mapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
		}

		// extract face data
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Processing faces for mesh %d (%d faces)",
				meshIndex, mesh->mNumFaces);
		/*
		 * since aiProcess_SortByPType is being used,
		 * we're sure we'll get only 1 type here,
		 * so combination check isn't needed, also
		 * anything more complex than triangles is
		 * being split thanks to aiProcess_Triangulate
		 */
		piece->vertexDrawIndices.reserve(piece->vertexDrawIndices.size() + mesh->mNumFaces * 3);
		for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			const aiFace& face = mesh->mFaces[faceIndex];

			// some models contain lines (mNumIndices == 2)
			// we cannot render those (esp. they would need to be called in a 2nd drawcall)
			if (face.mNumIndices != 3)
				continue;

			for (unsigned vertexListID = 0; vertexListID < face.mNumIndices; ++vertexListID) {
				const unsigned int vertexFaceIdx = face.mIndices[vertexListID];
				const unsigned int vertexDrawIdx = mesh_vertex_mapping[vertexFaceIdx];
				piece->vertexDrawIndices.push_back(vertexDrawIdx);
			}
		}
	}

	piece->isEmpty = piece->vertices.empty();

	//! Get parent name from metadata or model
	if (pieceTable.KeyExists("parent")) {
		piece->parentName = pieceTable.GetString("parent", "");
	} else if (node->mParent) {
		if (node->mParent->mParent) {
			piece->parentName = std::string(node->mParent->mName.data);
		} else { // my parent is the root, which gets renamed
			piece->parentName = "root";
		}
	} else {
		piece->parentName = "";
	}

	LOG_S(LOG_SECTION_PIECE, "Loaded model piece: %s with %d meshes",
			piece->name.c_str(), node->mNumMeshes);

	// Verbose logging of piece properties
	LOG_S(LOG_SECTION_PIECE, "piece->name: %s", piece->name.c_str());
	LOG_S(LOG_SECTION_PIECE, "piece->parent: %s", piece->parentName.c_str());

	// Recursively process all child pieces
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		LoadPiece(model, node->mChildren[i], metaTable);
	}

	model->pieces[piece->name] = piece;
	return piece;
}


//! Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy(S3DModel* model)
{
	//! Loop through all pieces and create missing hierarchy info
	for (ModelPieceMap::const_iterator it = model->pieces.begin(); it != model->pieces.end(); ++it) {
		S3DModelPiece* piece = it->second;

		if (piece->name == "root") {
			piece->parent = NULL;
			assert(model->GetRootPiece() == NULL);
			model->SetRootPiece(piece); // FIXME what if called multiple times?
			continue;
		}

		if (piece->parentName != "") {
			if ((piece->parent = model->FindPiece(piece->parentName)) == NULL) {
				LOG_SL(LOG_SECTION_PIECE, L_ERROR,
						"Missing piece '%s' declared as parent of '%s'.",
						piece->parentName.c_str(), piece->name.c_str());
			} else {
				piece->parent->children.push_back(piece);
			}

			continue;
		}

		//! A piece with no parent that isn't the root (orphan)
		if ((piece->parent = model->FindPiece("root")) == NULL) {
			LOG_SL(LOG_SECTION_PIECE, L_ERROR, "Missing root piece");
		} else {
			piece->parent->children.push_back(piece);
		}
	}
}


// Iterate over the model and calculate its overall dimensions
void CAssParser::CalculateModelDimensions(S3DModel* model, S3DModelPiece* piece)
{
	// cannot set this until parent relations are known, so either here or in BuildPieceHierarchy()
	piece->goffset = piece->scaleRotMatrix.Mul(piece->offset) + ((piece->parent != NULL)? piece->parent->goffset: ZeroVector);

	// update model min/max extents
	model->mins = std::min(piece->goffset + piece->mins, model->mins);
	model->maxs = std::max(piece->goffset + piece->maxs, model->maxs);

	const float3 cvScales = piece->maxs - piece->mins;
	const float3 cvOffset =
		(piece->maxs - piece->goffset) +
		(piece->mins - piece->goffset);

	piece->SetCollisionVolume(new CollisionVolume("box", cvScales, cvOffset * 0.5f));

	// Repeat with children
	for (unsigned int i = 0; i < piece->children.size(); i++) {
		CalculateModelDimensions(model, piece->children[i]);
	}
}

//! Calculate model radius from the min/max extents
void CAssParser::CalculateModelProperties(S3DModel* model, const LuaTable& metaTable)
{
	CalculateModelDimensions(model, model->rootPiece);

	// note: overrides default midpos of the SpringRadius piece
	model->relMidPos.y = (model->maxs.y - model->mins.y) * 0.5f;

	// Simplified dimensions used for rough calculations
	model->radius = metaTable.GetFloat("radius", std::max(std::fabs(model->maxs), std::fabs(model->mins)).Length());
	model->height = metaTable.GetFloat("height", model->maxs.z);
	model->relMidPos = metaTable.GetFloat3("midpos", model->relMidPos);
	model->mins = metaTable.GetFloat3("mins", model->mins);
	model->maxs = metaTable.GetFloat3("maxs", model->maxs);

	model->drawRadius = model->radius;
}


void CAssParser::FindTextures(S3DModel* model, const aiScene* scene, const LuaTable& metaTable, const std::string& modelFilePath)
{
	const std::string modelPath = FileSystem::GetDirectory(modelFilePath);
	const std::string modelName = FileSystem::GetBasename(modelFilePath);

	// Assign textures
	// The S3O texture handler uses two textures.
	// The first contains diffuse color (RGB) and teamcolor (A)
	// The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).

	// gather model defined textures
	if (scene->mNumMaterials > 0) {
		const aiMaterial* mat = scene->mMaterials[0]; //only check first material

		//FIXME support these too (we need to allow to construct tex1 & tex2 from several sources)
		/*aiTextureType_EMISSIVE
		aiTextureType_HEIGHT
		aiTextureType_NORMALS
		aiTextureType_SHININESS
		aiTextureType_OPACITY*/

		aiString textureFile;

		mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE,  0), textureFile);
		if (strcmp(textureFile.C_Str(), "") == 0) model->tex1 = textureFile.C_Str();
		textureFile.Clear();

		mat->Get(AI_MATKEY_TEXTURE(aiTextureType_UNKNOWN,  0), textureFile);
		if (strcmp(textureFile.C_Str(), "") == 0) model->tex1 = textureFile.C_Str();
		textureFile.Clear();

		mat->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), textureFile);
		if (strcmp(textureFile.C_Str(), "") == 0) model->tex1 = textureFile.C_Str();
		textureFile.Clear();
	}

	// try to load from metafile
	model->tex1 = metaTable.GetString("tex1", model->tex1);
	model->tex2 = metaTable.GetString("tex2", model->tex2);

	// try to find by name
	if (model->tex1.empty()) {
		const std::vector<std::string>& files = CFileHandler::FindFiles("unittextures/", modelName + ".*");

		if (!files.empty()) {
			model->tex1 = FileSystem::GetFilename(files[0]);
		}
	}
	if (model->tex2.empty()) {
		const std::vector<std::string>& files = CFileHandler::FindFiles("unittextures/", modelName + "2.*");

		if (!files.empty()) {
			model->tex2 = FileSystem::GetFilename(files[0]);
		}
	}

	// last chance for primary texture
	if (model->tex1.empty()) {
		const std::vector<std::string>& files = CFileHandler::FindFiles(modelPath, "diffuse.*");

		if (!files.empty()) {
			model->tex1 = FileSystem::GetFilename(files[0]);
		}
	}

	// correct filepath?
	if (!CFileHandler::FileExists(model->tex1, SPRING_VFS_ZIP)) {
		if (CFileHandler::FileExists("unittextures/" + model->tex1, SPRING_VFS_ZIP)) {
			model->tex1 = "unittextures/" + model->tex1;
		} else if (CFileHandler::FileExists(modelPath + model->tex1, SPRING_VFS_ZIP)) {
			model->tex1 = modelPath + model->tex1;
		}
	}
	if (!CFileHandler::FileExists(model->tex2, SPRING_VFS_ZIP)) {
		if (CFileHandler::FileExists("unittextures/" + model->tex2, SPRING_VFS_ZIP)) {
			model->tex2 = "unittextures/" + model->tex2;
		} else if (CFileHandler::FileExists(modelPath + model->tex2, SPRING_VFS_ZIP)) {
			model->tex2 = modelPath + model->tex2;
		}
	}

	model->flipTexY = metaTable.GetBool("fliptextures", true); // Flip texture upside down
	model->invertTexAlpha = metaTable.GetBool("invertteamcolor", true); // Reverse teamcolor levels
}


void SAssPiece::UploadGeometryVBOs()
{
	if (isEmpty)
		return;

	//FIXME share 1 VBO for ALL models
	vboAttributes.Bind(GL_ARRAY_BUFFER);
	vboAttributes.Resize(vertices.size() * sizeof(SAssVertex), GL_STATIC_DRAW, &vertices[0]);
	vboAttributes.Unbind();

	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	vboIndices.Resize(vertexDrawIndices.size() * sizeof(unsigned int), GL_STATIC_DRAW, &vertexDrawIndices[0]);
	vboIndices.Unbind();

	// NOTE: wasteful to keep these around, but still needed (eg. for Shatter())
	// vertices.clear();
	// vertexDrawIndices.clear();
}


void SAssPiece::DrawForList() const
{
	if (isEmpty)
		return;

	vboAttributes.Bind(GL_ARRAY_BUFFER);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(SAssVertex), vboAttributes.GetPtr(offsetof(SAssVertex, pos)));

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(SAssVertex), vboAttributes.GetPtr(offsetof(SAssVertex, normal)));

		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(SAssVertex), vboAttributes.GetPtr(offsetof(SAssVertex, texCoord)));

		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(SAssVertex), vboAttributes.GetPtr(offsetof(SAssVertex, texCoord)));

		if (hasTexCoord2) {
			glClientActiveTexture(GL_TEXTURE2);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, sizeof(SAssVertex), vboAttributes.GetPtr(offsetof(SAssVertex, texCoord2)));
		}

		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(SAssVertex), vboAttributes.GetPtr(offsetof(SAssVertex, sTangent)));

		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(SAssVertex), vboAttributes.GetPtr(offsetof(SAssVertex, tTangent)));
	vboAttributes.Unbind();

	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
		/*
		* since aiProcess_SortByPType is being used,
		* we're sure we'll get only 1 type here,
		* so combination check isn't needed, also
		* anything more complex than triangles is
		* being split thanks to aiProcess_Triangulate
		*/
		glDrawRangeElements(GL_TRIANGLES, 0, vertices.size() - 1, vertexDrawIndices.size(), GL_UNSIGNED_INT, vboIndices.GetPtr());
	vboIndices.Unbind();

	glClientActiveTexture(GL_TEXTURE6);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE5);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE2);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}


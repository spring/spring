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
#include "Rendering/Textures/S3OTextureHandler.h"
#ifndef BITMAP_NO_OPENGL
	#include "Rendering/GL/myGL.h"
#endif


#define IS_QNAN(f) (f != f)
static const float DEGTORAD = PI / 180.0f;
static const float RADTODEG = 180.0f / PI;

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
	// default
	return float3(v.x, v.y, v.z);

	// Blender --> Spring
	// return float3(v.x, v.z, -v.y);
}

static inline CMatrix44f aiMatrixToMatrix(const aiMatrix4x4t<float>& m)
{
	CMatrix44f n;

	n[ 0] = m.a1; n[ 1] = m.a2; n[ 2] = m.a3; n[ 3] = m.a4;
	n[ 4] = m.b1; n[ 5] = m.b2; n[ 6] = m.b3; n[ 7] = m.b4;
	n[ 8] = m.c1; n[ 9] = m.c2; n[10] = m.c3; n[11] = m.c4;
	n[12] = m.d1; n[13] = m.d2; n[14] = m.d3; n[15] = m.d4;

	// AssImp (row-major) --> Spring (column-major)
	n.Transpose();

	// default
	return (CMatrix44f(n.GetPos(), n.GetX(), n.GetY(), n.GetZ()));

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
	const float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
	const float test = q1.x * q1.y + q1.z * q1.w;

	float3 result;

	if (test > (0.499f * unit)) {
		// singularity at north pole
		result.x = 2.0f * math::atan2(q1.x, q1.w);
		result.y = PI * 0.5f;
	} else if (test < (-0.499f * unit)) {
		// singularity at south pole
		result.x = -2.0f * math::atan2(q1.x, q1.w);
		result.y = -PI * 0.5f;
	} else {
		result.x = math::atan2(2.0f * q1.y * q1.w - 2.0f * q1.x * q1.z,  sqx - sqy - sqz + sqw);
		result.y = math::asin((2.0f * test) / unit);
		result.z = math::atan2(2.0f * q1.x * q1.w - 2.0f * q1.y * q1.z, -sqx + sqy - sqz + sqw);
	}

	return result;
	// Blender --> Spring
	// return (float3(result.x, result.z, -result.y));
}
*/





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
	LOG_S(LOG_SECTION_MODEL, "Loading model: %s", modelFilePath.c_str());

	const std::string& modelPath = FileSystem::GetDirectory(modelFilePath);
	const std::string& modelName = FileSystem::GetBasename(modelFilePath);

	// LOAD METADATA
	// Load the lua metafile. This contains properties unique to Spring models and must return a table
	std::string metaFileName = modelFilePath + ".lua";

	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP)) {
		// Try again without the model file extension
		metaFileName = modelPath + '/' + modelName + ".lua";
	}
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP)) {
		LOG_S(LOG_SECTION_MODEL, "No meta-file '%s'. Using defaults.", metaFileName.c_str());
	}

	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);

	if (!metaFileParser.Execute()) {
		LOG_SL(LOG_SECTION_MODEL, L_ERROR, "'%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());
	}

	// Get the (root-level) model table
	const LuaTable& modelTable = metaFileParser.GetRoot();

	if (!modelTable.IsValid()) {
		LOG_S(LOG_SECTION_MODEL, "No valid model metadata in '%s' or no meta-file", metaFileName.c_str());
	}


	// LOAD MODEL DATA
	// Create a model importer instance
	Assimp::Importer importer;

	// Create a logger for debugging model loading issues
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	Assimp::DefaultLogger::get()->attachStream(new AssLogStream(), ASS_LOGGING_OPTIONS);

	// Give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler(new AssVFSSystem());
	// Speed-up processing by skipping things we don't need
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, ASS_IMPORTER_OPTIONS);

#ifndef BITMAP_NO_OPENGL
	{
		// Optimize VBO-Mesh sizes/ranges
		GLint maxIndices  = 1024;
		GLint maxVertices = 1024;
		glGetIntegerv(GL_MAX_ELEMENTS_INDICES,  &maxIndices);
		glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxVertices); //FIXME returns not optimal data, at best compute it ourself! (pre-TL cache size!)
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT,   maxVertices);
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices / 3);
	}
#endif

	// Read the model file to build a scene object
	LOG_S(LOG_SECTION_MODEL, "Importing model file: %s", modelFilePath.c_str());

	const aiScene* scene;
	{
		// ASSIMP spams many SIGFPEs atm in normal & tangent generation
		ScopedDisableFpuExceptions fe;
		scene = importer.ReadFile(modelFilePath, ASS_POSTPROCESS_OPTIONS);
	}

	if (scene != NULL) {
		LOG_S(LOG_SECTION_MODEL,
			"Processing scene for model: %s (%d meshes / %d materials / %d textures)",
			modelFilePath.c_str(), scene->mNumMeshes, scene->mNumMaterials,
			scene->mNumTextures);
	} else {
		throw content_error("[AssimpParser] Model Import: " + std::string(importer.GetErrorString()));
	}

	S3DModel* model = new S3DModel();
	model->name = modelFilePath;
	model->type = MODELTYPE_ASS;

	// Load textures
	FindTextures(model, scene, modelTable, modelPath, modelName);
	LOG_S(LOG_SECTION_MODEL, "Loading textures. Tex1: '%s' Tex2: '%s'", model->tex1.c_str(), model->tex2.c_str());
	texturehandlerS3O->LoadS3OTexture(model);

	// Load all pieces in the model
	LOG_S(LOG_SECTION_MODEL, "Loading pieces from root node '%s'", scene->mRootNode->mName.data);
	LoadPiece(model, scene->mRootNode, scene, modelTable);

	// Update piece hierarchy based on metadata
	BuildPieceHierarchy(model);
	CalculateModelProperties(model, modelTable);

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


/*
void CAssParser::CalculatePerMeshMinMax(SAssModel* model, const aiScene* scene)
{
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

		if (minmax.mins == DEF_MIN_SIZE) { minmax.mins = ZeroVector; }
		if (minmax.maxs == DEF_MAX_SIZE) { minmax.maxs = ZeroVector; }
	}
}
*/


void CAssParser::LoadPieceTransformations(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable
) {
	float3 spRotateVec, spTransVec;
	float3 spScaleVec = OnesVector;

	aiVector3D aiScaleVec, aiTransVec;
	aiQuaternion aiRotateQuat;

	// process transforms
	pieceNode->mTransformation.Decompose(aiScaleVec, aiRotateQuat, aiTransVec);

	LOG_S(LOG_SECTION_PIECE,
		"(%d:%s) Assimp offset (%f,%f,%f), rotate (%f,%f,%f,%f), scale (%f,%f,%f)",
		model->numPieces, piece->name.c_str(),
		aiTransVec.x, aiTransVec.y, aiTransVec.z,
		aiRotateQuat.w, aiRotateQuat.x, aiRotateQuat.y, aiRotateQuat.z,
		aiScaleVec.x, aiScaleVec.y, aiScaleVec.z
	);

	// metadata-scaling
	spScaleVec   = pieceTable.GetFloat3("scale", float3(aiScaleVec.x, aiScaleVec.y, aiScaleVec.z));
	spScaleVec.x = pieceTable.GetFloat("scalex", spScaleVec.x);
	spScaleVec.y = pieceTable.GetFloat("scaley", spScaleVec.y);
	spScaleVec.z = pieceTable.GetFloat("scalez", spScaleVec.z);

	if (spScaleVec.x != spScaleVec.y || spScaleVec.y != spScaleVec.z) {
		// LOG_SL(LOG_SECTION_MODEL, L_WARNING, "Spring doesn't support non-uniform scaling");
		spScaleVec.y = spScaleVec.x;
		spScaleVec.z = spScaleVec.x;
	}

	// metadata-rotation
	// NOTE:
	//   these rotations are "pre-scripting" but "post-modelling"
	//   together with the (baked) aiRotateQuad they determine the
	//   model's pose before any animations execute
	//   
	// spRotateVec   = pieceTable.GetFloat3("rotate", aiQuaternionToRadianAngles(aiRotateQuat) * RADTODEG);
	spRotateVec   = pieceTable.GetFloat3("rotate", ZeroVector);
	spRotateVec.x = pieceTable.GetFloat("rotatex", spRotateVec.x);
	spRotateVec.y = pieceTable.GetFloat("rotatey", spRotateVec.y);
	spRotateVec.z = pieceTable.GetFloat("rotatez", spRotateVec.z);
	spRotateVec  *= DEGTORAD;

	// metadata-translation
	spTransVec   = pieceTable.GetFloat3("offset", aiVectorToFloat3(aiTransVec));
	spTransVec.x = pieceTable.GetFloat("offsetx", spTransVec.x);
	spTransVec.y = pieceTable.GetFloat("offsety", spTransVec.y);
	spTransVec.z = pieceTable.GetFloat("offsetz", spTransVec.z);

	LOG_S(LOG_SECTION_PIECE,
		"(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)",
		model->numPieces, piece->name.c_str(),
		spTransVec.x, spTransVec.y, spTransVec.z,
		spRotateVec.x, spRotateVec.y, spRotateVec.z,
		spScaleVec.x, spScaleVec.y, spScaleVec.z
	);

	// NOTE:
	//   at least collada (.dae) files generated by Blender
	//   represent a coordinate-system that differs from the
	//   standard formats (3DO, S3O, ...) where the existing
	//   tools have prior knowledge of Spring's expectations
	//   --> let the user override the root SR transform and
	//   the rotation-axis mapping used by animation scripts
	//   (but re-modelling/re-exporting is always preferred!)
	//
	//   .dae  : x=Rgt, y=-Fwd, z= Up, as=(-1, -1, 1), am=AXIS_XZY (if Z_UP)
	//   .dae  : x=Rgt, y=-Fwd, z= Up, as=(-1, -1, 1), am=AXIS_XZY (if Y_UP) [!?]
	//   .blend: ????
	piece->scaleRotMatrix = aiMatrixToMatrix(aiMatrix4x4t<float>(aiRotateQuat.GetMatrix()));

	if (piece->isRoot) {
		const float3 xaxis = pieceTable.GetFloat3("xaxis", piece->scaleRotMatrix.GetX());
		const float3 yaxis = pieceTable.GetFloat3("yaxis", piece->scaleRotMatrix.GetY());
		const float3 zaxis = pieceTable.GetFloat3("zaxis", piece->scaleRotMatrix.GetZ());

		if (math::fabs(xaxis.SqLength() - yaxis.SqLength()) < 0.01f && math::fabs(yaxis.SqLength() - zaxis.SqLength()) < 0.01f) {
			piece->scaleRotMatrix = CMatrix44f(ZeroVector, xaxis, yaxis, zaxis);
		}
	}

	piece->offset = spTransVec;
	piece->raSigns = pieceTable.GetFloat3("rotAxisSigns", float3(-OnesVector));
	piece->ramType = AxisMappingType(pieceTable.GetInt("rotAxisMap", AXIS_MAPPING_XYZ));

	// construct 'baked' part of the modelpiece matrix
	// (AssImp order is translate * rotate * scale * v)
	// we leave the translation part out and put it in
	// <offset> so SRM = R * S instead of T * R * S
	//
	// note: for all non-AssImp models this is identity!
	//
	piece->mIsIdentity |= (spScaleVec == OnesVector);
	piece->mIsIdentity &= piece->ComposeTransform(piece->scaleRotMatrix.Scale(spScaleVec), ZeroVector, spRotateVec);
}

bool CAssParser::SetModelRadiusAndHeight(
	S3DModel* model,
	const SAssPiece* piece,
	const aiNode* pieceNode,
	const LuaTable& pieceTable
) {
	// check if this piece is "special" (ie, used to set Spring model properties)
	// if so, extract them and then remove the piece from the hierarchy entirely
	//
	if (piece->name == "SpringHeight") {
		// set the model height to this node's Z-value (FIXME: 'z' is Blender-specific)
		if (!pieceTable.KeyExists("height")) {
			model->height = piece->offset.z;

			LOG_S(LOG_SECTION_MODEL, "Model height of %f set by special node 'SpringHeight'", model->height);
		}

		--model->numPieces;
		delete piece;
		return true;
	}

	if (piece->name == "SpringRadius") {
		if (!pieceTable.KeyExists("midpos")) {
			model->relMidPos = piece->scaleRotMatrix.Mul(piece->offset);

			LOG_S(LOG_SECTION_MODEL,
				"Model midpos of (%f,%f,%f) set by special node 'SpringRadius'",
				model->relMidPos.x, model->relMidPos.y, model->relMidPos.z);
		}
		if (!pieceTable.KeyExists("radius")) {
			if (true || piece->maxs.x <= 0.00001f) {
				// FIXME: geometry bounds are not calculated yet at this point
				aiVector3D _scale, _offset;
				aiQuaternion _rotate;
				pieceNode->mTransformation.Decompose(_scale, _rotate, _offset);

				// the blender import script only sets the scale property
				model->radius = aiVectorToFloat3(_scale).x;
			} else {
				// use the transformed mesh extents (FIXME: bounds are NOT transformed!)
				model->radius = piece->maxs.x;
			}

			LOG_S(LOG_SECTION_MODEL, "Model radius of %f set by special node 'SpringRadius'", model->radius);
		}

		--model->numPieces;
		delete piece;
		return true;
	}

	return false;
}

void CAssParser::SetPieceName(SAssPiece* piece, const S3DModel* model, const aiNode* pieceNode)
{
	assert(piece->name.empty());
	piece->name = std::string(pieceNode->mName.data);

	if (piece->name.empty()) {
		if (piece->isRoot) {
			// root is always the first piece created, so safe to assign this
			piece->name = "$$root$$";
			return;
		} else {
			piece->name = "$$piece$$";
		}
	}

	// find a new name if none given or if a piece with the same name already exists
	ModelPieceMap::const_iterator it = model->pieces.find(piece->name);

	for (unsigned int i = 0; it != model->pieces.end(); i++) {
		const std::string newPieceName = piece->name + IntToString(i, "%02i");

		if ((it = model->pieces.find(newPieceName)) == model->pieces.end()) {
			piece->name = newPieceName; break;
		}
	}
}

void CAssParser::SetPieceParentName(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable
) {
	// Get parent name from metadata or model
	if (pieceTable.KeyExists("parent")) {
		piece->parentName = pieceTable.GetString("parent", "");
	} else if (pieceNode->mParent != NULL) {
		if (pieceNode->mParent->mParent != NULL) {
			piece->parentName = std::string(pieceNode->mParent->mName.data);
		} else {
			// my parent is the root (which must already exist)
			assert(model->GetRootPiece() != NULL);
			piece->parentName = model->GetRootPiece()->name;
		}
	}
}

void CAssParser::LoadPieceGeometry(SAssPiece* piece, const aiNode* pieceNode, const aiScene* scene)
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
		piece->vertexDrawIndices.reserve(piece->vertexDrawIndices.size() + mesh->mNumFaces * 3);

		std::vector<unsigned> mesh_vertex_mapping;

		// extract vertex data per mesh
		for (unsigned vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];

			SAssVertex vertex;

			// vertex coordinates
			vertex.pos = aiVectorToFloat3(aiVertex);

			// update piece min/max extents
			piece->mins = std::min(piece->mins, vertex.pos);
			piece->maxs = std::max(piece->maxs, vertex.pos);

			// vertex normal
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching normal for vertex %d", vertexIndex);

			const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];

			if (!IS_QNAN(aiNormal)) {
				vertex.normal = aiVectorToFloat3(aiNormal);
			}

			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching tangent for vertex %d", vertexIndex);

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
				piece->extraUVs = true,
				vertex.texCoord2.x = mesh->mTextureCoords[1][vertexIndex].x;
				vertex.texCoord2.y = mesh->mTextureCoords[1][vertexIndex].y;
			}

			mesh_vertex_mapping.push_back(piece->vertices.size());
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
				const unsigned int vertexDrawIdx = mesh_vertex_mapping[vertexFaceIdx];
				piece->vertexDrawIndices.push_back(vertexDrawIdx);
			}
		}
	}

	piece->isEmpty = (piece->vertices.empty());
}

SAssPiece* CAssParser::LoadPiece(
	S3DModel* model,
	const aiNode* pieceNode,
	const aiScene* scene,
	const LuaTable& modelTable
) {
	++model->numPieces;

	SAssPiece* piece = new SAssPiece();
	piece->isEmpty = (pieceNode->mNumMeshes == 0);
	piece->isRoot = (pieceNode->mParent == NULL);

	if (piece->isRoot) {
		// set the root piece ASAP, needed later
		assert(pieceNode == scene->mRootNode);
		model->SetRootPiece(piece);
	}

	SetPieceName(piece, model, pieceNode);

	LOG_S(LOG_SECTION_PIECE, "Converting node '%s' to piece '%s' (%d meshes).", pieceNode->mName.data, piece->name.c_str(), pieceNode->mNumMeshes);

	// Load additional piece properties from metadata
	const LuaTable& pieceTable = modelTable.SubTable("pieces").SubTable(piece->name);

	if (pieceTable.IsValid()) {
		LOG_S(LOG_SECTION_PIECE, "Found metadata for piece '%s'", piece->name.c_str());
	}

	// Load transforms
	LoadPieceTransformations(piece, model, pieceNode, pieceTable);

	if (SetModelRadiusAndHeight(model, piece, pieceNode, pieceTable))
		return NULL;

	LoadPieceGeometry(piece, pieceNode, scene);
	SetPieceParentName(piece, model, pieceNode, pieceTable);

	// Verbose logging of piece properties
	LOG_S(LOG_SECTION_PIECE, "Loaded model piece: %s with %d meshes", piece->name.c_str(), pieceNode->mNumMeshes);
	LOG_S(LOG_SECTION_PIECE, "piece->name: %s", piece->name.c_str());
	LOG_S(LOG_SECTION_PIECE, "piece->parent: %s", piece->parentName.c_str());

	// Recursively process all child pieces
	for (unsigned int i = 0; i < pieceNode->mNumChildren; ++i) {
		LoadPiece(model, pieceNode->mChildren[i], scene, modelTable);
	}

	model->pieces[piece->name] = piece;
	return piece;
}


// Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy(S3DModel* model)
{
	// Loop through all pieces and create missing hierarchy info
	for (ModelPieceMap::const_iterator it = model->pieces.begin(); it != model->pieces.end(); ++it) {
		SAssPiece* piece = static_cast<SAssPiece*>(it->second);

		if (piece->isRoot) {
			assert(piece->parent == NULL);
			assert(model->GetRootPiece() == piece);
			continue;
		}

		if (!piece->parentName.empty()) {
			if ((piece->parent = model->FindPiece(piece->parentName)) == NULL) {
				LOG_SL(LOG_SECTION_PIECE, L_ERROR,
					"Missing piece '%s' declared as parent of '%s'.",
					piece->parentName.c_str(), piece->name.c_str());
			} else {
				piece->parent->children.push_back(piece);
			}

			continue;
		}

		// a piece with no named parent that isn't the root (orphan)
		// link these to the root piece if it exists (which it should)
		if ((piece->parent = model->GetRootPiece()) == NULL) {
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

	piece->SetCollisionVolume(new CollisionVolume("box", piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	// Repeat with children
	for (unsigned int i = 0; i < piece->children.size(); i++) {
		CalculateModelDimensions(model, piece->children[i]);
	}
}

// Calculate model radius from the min/max extents
void CAssParser::CalculateModelProperties(S3DModel* model, const LuaTable& modelTable)
{
	CalculateModelDimensions(model, model->rootPiece);

	// note: overrides default midpos of the SpringRadius piece
	model->relMidPos.y = (model->maxs.y - model->mins.y) * 0.5f;

	// Simplified dimensions used for rough calculations
	model->radius = modelTable.GetFloat("radius", std::max(std::fabs(model->maxs), std::fabs(model->mins)).Length());
	model->height = modelTable.GetFloat("height", model->maxs.z);
	model->relMidPos = modelTable.GetFloat3("midpos", model->relMidPos);
	model->mins = modelTable.GetFloat3("mins", model->mins);
	model->maxs = modelTable.GetFloat3("maxs", model->maxs);

	model->drawRadius = model->radius;
}


void CAssParser::FindTextures(
	S3DModel* model,
	const aiScene* scene,
	const LuaTable& modelTable,
	const std::string& modelPath,
	const std::string& modelName
) {
	// Assign textures
	// The S3O texture handler uses two textures.
	// The first contains diffuse color (RGB) and teamcolor (A)
	// The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).

	const unsigned int texTypes[] = {
		aiTextureType_DIFFUSE,
		aiTextureType_UNKNOWN,
		aiTextureType_SPECULAR,
		/*
		// TODO: support these too (we need to allow constructing tex1 & tex2 from several sources)
		aiTextureType_EMISSIVE,
		aiTextureType_HEIGHT,
		aiTextureType_NORMALS,
		aiTextureType_SHININESS,
		aiTextureType_OPACITY,
		*/
	};

	// gather model-defined textures (only check first material)
	if (scene->mNumMaterials > 0) {
		const aiMaterial* mat = scene->mMaterials[0];

		for (unsigned int n = 0; n < (sizeof(texTypes) / sizeof(texTypes[0])); n++) {
			aiString textureFile;

			if (mat->Get(AI_MATKEY_TEXTURE(texTypes[n], 0), textureFile) != aiReturn_SUCCESS)
				continue;

			assert(textureFile.length > 0);
			model->tex1 = std::string(textureFile.data);
			break;
		}
	}

	// try to load from metafile
	model->tex1 = modelTable.GetString("tex1", model->tex1);
	model->tex2 = modelTable.GetString("tex2", model->tex2);

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

	model->flipTexY = modelTable.GetBool("fliptextures", true); // Flip texture upside down
	model->invertTexAlpha = modelTable.GetBool("invertteamcolor", true); // Reverse teamcolor levels
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

		if (extraUVs) {
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


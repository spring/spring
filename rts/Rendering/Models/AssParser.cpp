// AssParser.cpp: implementation of the CAssParser class. Reads 3D formats supported by Assimp lib.
//
//////////////////////////////////////////////////////////////////////
//#include "StdAfx.h"

#include "Util.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "System/Exceptions.h"
#include "Sim/Misc/CollisionVolume.h"
#include "FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "3DModel.h"
#include "3DModelLog.h"
#include "S3OParser.h"
#include "AssIO.h"
#include "AssParser.h"

#include "assimp.hpp"
#include "aiDefines.h"
#include "aiTypes.h"
#include "aiScene.h"
#include "aiPostProcess.h"
#include "DefaultLogger.h"
#include "Rendering/Textures/S3OTextureHandler.h"

#define IS_QNAN(f) (f != f)
#define DEGTORAD 0.0174532925

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
#define ASS_POSTPROCESS_OPTIONS \
    aiProcess_RemoveComponent               | \
	aiProcess_FindInvalidData				| \
	aiProcess_CalcTangentSpace				| \
	aiProcess_GenSmoothNormals				| \
	aiProcess_SplitLargeMeshes				| \
	aiProcess_Triangulate					| \
	aiProcess_GenUVCoords             		| \
	aiProcess_SortByPType					| \
	aiProcess_JoinIdenticalVertices

//aiProcess_ImproveCacheLocality

// Convert Assimp quaternion to radians around x, y and z
float3 QuaternionToRadianAngles( aiQuaternion q1 )
{
    float sqw = q1.w*q1.w;
    float sqx = q1.x*q1.x;
    float sqy = q1.y*q1.y;
    float sqz = q1.z*q1.z;
	float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
	float test = q1.x*q1.y + q1.z*q1.w;

	float3 result(0.0f, 0.0f, 0.0f);

	if (test > 0.499f * unit) { // singularity at north pole
		result.x = 2 * atan2(q1.x,q1.w);
		result.y = PI/2;
	} else if (test < -0.499f * unit) { // singularity at south pole
		result.x = -2 * atan2(q1.x,q1.w);
		result.y = -PI/2;
	} else {
        result.x = atan2(2*q1.y*q1.w-2*q1.x*q1.z , sqx - sqy - sqz + sqw);
        result.y = asin(2*test/unit);
        result.z = atan2(2*q1.x*q1.w-2*q1.y*q1.z , -sqx + sqy - sqz + sqw);
	}
	return result;
}

// Convert float3 rotations in degrees to radians
void DegreesToRadianAngles( float3& angles )
{
    angles.x *= DEGTORAD;
    angles.y *= DEGTORAD;
    angles.z *= DEGTORAD;
}

class AssLogStream : public Assimp::LogStream
{
public:
	AssLogStream() {}
	~AssLogStream() {}
	void write(const char* message)
	{
		logOutput.Print (LOG_MODEL_DETAIL, "Assimp: %s", message);
	}
};



S3DModel* CAssParser::Load(const std::string& modelFileName)
{
	logOutput.Print (LOG_MODEL, "Loading model: %s\n", modelFileName.c_str() );
	std::string modelPath = modelFileName.substr(0, modelFileName.find_last_of('/'));
    std::string modelFileNameNoPath = modelFileName.substr(modelPath.length()+1, modelFileName.length());
    std::string modelName = modelFileNameNoPath.substr(0, modelFileNameNoPath.find_last_of('.'));
    std::string modelExt = modelFileNameNoPath.substr(modelFileNameNoPath.find_last_of('.'), modelFileName.length());

    // LOAD METADATA
	// Load the lua metafile. This contains properties unique to Spring models and must return a table
	std::string metaFileName = modelFileName + ".lua";
	CFileHandler* metaFile = new CFileHandler(metaFileName);
	if (!metaFile->FileExists()) {
	    // Try again without the model file extension
        metaFileName = modelPath + '/' + modelName + ".lua";
        metaFile = new CFileHandler(metaFileName);
	}
	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!metaFileParser.Execute()) {
		if (!metaFile->FileExists()) {
			logOutput.Print(LOG_MODEL, "No meta-file '%s'. Using defaults.", metaFileName.c_str());
		} else {
			logOutput.Print(LOG_MODEL, "ERROR in '%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());
		}
	}
	// Get the (root-level) model table
	const LuaTable& metaTable = metaFileParser.GetRoot();
    if (metaTable.IsValid()) logOutput.Print(LOG_MODEL, "Found valid model metadata in '%s'", metaFileName.c_str());


    // LOAD MODEL DATA
	// Create a model importer instance
 	Assimp::Importer importer;

	// Create a logger for debugging model loading issues
	Assimp::DefaultLogger::create("",Assimp::Logger::VERBOSE);
	const unsigned int severity = Assimp::Logger::DEBUGGING|Assimp::Logger::INFO|Assimp::Logger::ERR|Assimp::Logger::WARN;
	Assimp::DefaultLogger::get()->attachStream( new AssLogStream(), severity );

	// Give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler( new AssVFSSystem() );

    // Speed-up processing by skipping things we don't need
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS|aiComponent_LIGHTS|aiComponent_TEXTURES|aiComponent_ANIMATIONS);

	// Read the model file to build a scene object
	logOutput.Print(LOG_MODEL, "Importing model file: %s\n", modelFileName.c_str() );
	const aiScene* scene = importer.ReadFile( modelFileName, ASS_POSTPROCESS_OPTIONS );
	if (scene != NULL) {
		logOutput.Print(LOG_MODEL, "Processing scene for model: %s (%d meshes / %d materials / %d textures)", modelFileName.c_str(), scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures );
    } else {
		logOutput.Print (LOG_MODEL, "Model Import Error: %s\n",  importer.GetErrorString());
	}

    S3DModel* model = new S3DModel;
    model->name = modelFileName;
	model->type = MODELTYPE_OTHER;
	model->numPieces = 0;
    model->scene = scene;
    //model->meta = &metaTable;

    // Assign textures
    // The S3O texture handler uses two textures.
    // The first contains diffuse color (RGB) and teamcolor (A)
    // The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).
    if (metaTable.KeyExists("tex1")) {
        model->tex1 = metaTable.GetString("tex1", "default.png");
    } else {
        // Search for a texture
        std::vector<std::string> files = CFileHandler::FindFiles("unittextures/", modelName + ".*");
        for(std::vector<std::string>::iterator fi = files.begin(); fi != files.end(); ++fi) {
            std::string texPath = std::string(*fi);
            model->tex1 = texPath.substr(texPath.find('/')+1, texPath.length());
            break; // there can be only one!
        }
    }
    if (metaTable.KeyExists("tex2")) {
        model->tex2 = metaTable.GetString("tex2", "");
    } else {
        // Search for a texture
        std::vector<std::string> files = CFileHandler::FindFiles("unittextures/", modelName + "2.*");
        for(std::vector<std::string>::iterator fi = files.begin(); fi != files.end(); ++fi) {
            std::string texPath = std::string(*fi);
            model->tex2 = texPath.substr(texPath.find('/')+1, texPath.length());
            break; // there can be only one!
        }
    }
    model->flipTexY = metaTable.GetBool("fliptextures", true); // Flip texture upside down
    model->invertTexAlpha = metaTable.GetBool("invertteamcolor", true); // Reverse teamcolor levels

    // Load textures
    logOutput.Print(LOG_MODEL, "Loading textures. Tex1: '%s' Tex2: '%s'", model->tex1.c_str(), model->tex2.c_str());
    texturehandlerS3O->LoadS3OTexture(model);

    // Load all pieces in the model
    logOutput.Print(LOG_MODEL, "Loading pieces from root node '%s'", scene->mRootNode->mName.data);
    LoadPiece( model, scene->mRootNode, metaTable );

    // Update piece hierarchy based on metadata
    BuildPieceHierarchy( model );

    // Simplified dimensions used for rough calculations
    model->radius = metaTable.GetFloat("radius", model->radius);
    model->height = metaTable.GetFloat("height", model->height);
    model->relMidPos = metaTable.GetFloat3("midpos", model->relMidPos);
    model->mins = metaTable.GetFloat3("mins", model->mins);
    model->maxs = metaTable.GetFloat3("maxs", model->maxs);

    // Calculate model dimensions if not set
    if (!metaTable.KeyExists("mins") || !metaTable.KeyExists("maxs")) CalculateMinMax( model->rootPiece );
    if (model->radius < 0.0001f) CalculateRadius( model );
    if (model->height < 0.0001f) CalculateHeight( model );

    // Verbose logging of model properties
    logOutput.Print(LOG_MODEL_DETAIL, "model->name: %s", model->name.c_str());
    logOutput.Print(LOG_MODEL_DETAIL, "model->numobjects: %d", model->numPieces);
    logOutput.Print(LOG_MODEL_DETAIL, "model->radius: %f", model->radius);
    logOutput.Print(LOG_MODEL_DETAIL, "model->height: %f", model->height);
    logOutput.Print(LOG_MODEL_DETAIL, "model->mins: (%f,%f,%f)", model->mins[0], model->mins[1], model->mins[2]);
    logOutput.Print(LOG_MODEL_DETAIL, "model->maxs: (%f,%f,%f)", model->maxs[0], model->maxs[1], model->maxs[2]);

    logOutput.Print (LOG_MODEL, "Model %s Imported.", model->name.c_str());
    return model;
}

SAssPiece* CAssParser::LoadPiece(S3DModel* model, aiNode* node, const LuaTable& metaTable)
{
	// Create new piece
	model->numPieces++;
	SAssPiece* piece = new SAssPiece;
	piece->type = MODELTYPE_OTHER;
	piece->node = node;
	piece->model = model;
	piece->isEmpty = node->mNumMeshes == 0;

    if (node->mParent) {
        piece->name = std::string(node->mName.data);
	} else {
        piece->name = "root"; // The real model root
	}
	logOutput.Print(LOG_PIECE, "Converting node '%s' to piece '%s' (%d meshes).", node->mName.data, piece->name.c_str(), node->mNumMeshes);

    // Load additional piece properties from metadata
    const LuaTable& pieceTable = metaTable.SubTable("pieces").SubTable(piece->name);
    if (pieceTable.IsValid()) logOutput.Print(LOG_PIECE, "Found metadata for piece '%s'", piece->name.c_str());

    // Process transforms
    float3 rotate, scale, offset;
	aiVector3D _scale, _offset;
 	aiQuaternion _rotate;
	node->mTransformation.Decompose(_scale,_rotate,_offset);

	logOutput.Print(LOG_PIECE, "(%d:%s) Assimp offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)", model->numPieces, piece->name.c_str(),
		_offset.x, _offset.y, _offset.z,
		_rotate.x, _rotate.y, _rotate.z,
		_scale.x, _scale.y, _scale.z
	);

	offset = pieceTable.GetFloat3("offset", float3(_offset.x, _offset.y, _offset.z));
    offset.x = pieceTable.GetFloat("offsetx", offset.x);
    offset.y = pieceTable.GetFloat("offsety", offset.y);
    offset.z = pieceTable.GetFloat("offsetz", offset.z);

    if (pieceTable.KeyExists("rotate")) {
        rotate = pieceTable.GetFloat3("rotate", float3(0.0f, 0.0f, 0.0f));
        DegreesToRadianAngles(rotate);
    } else {
        rotate = QuaternionToRadianAngles(_rotate);
        rotate = float3(rotate.z, rotate.x, rotate.y);
    }
    if (pieceTable.KeyExists("rotatex")) rotate.x = pieceTable.GetFloat("rotatex", 0.0f) * DEGTORAD;
    if (pieceTable.KeyExists("rotatey")) rotate.y = pieceTable.GetFloat("rotatey", 0.0f) * DEGTORAD;
    if (pieceTable.KeyExists("rotatez")) rotate.z = pieceTable.GetFloat("rotatez", 0.0f) * DEGTORAD;

	scale = pieceTable.GetFloat3("scale", float3(_scale.x, _scale.z, _scale.y));
    scale.x = pieceTable.GetFloat("scalex", scale.x);
    scale.y = pieceTable.GetFloat("scaley", scale.y);
    scale.z = pieceTable.GetFloat("scalez", scale.z);

	logOutput.Print(LOG_PIECE, "(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)", model->numPieces, piece->name.c_str(),
		offset.x, offset.y, offset.z,
		rotate.x, rotate.y, rotate.z,
		scale.x, scale.y, scale.z
	);
	piece->offset = offset;
	piece->rot = rotate;
	piece->scale = scale;

	// Get vertex data from node meshes
	for ( unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; meshListIndex++ ) {
		unsigned int meshIndex = node->mMeshes[meshListIndex];
		logOutput.Print(LOG_PIECE_DETAIL, "Fetching mesh %d from scene", meshIndex );
		aiMesh* mesh = model->scene->mMeshes[meshIndex];
		std::vector<unsigned> mesh_vertex_mapping;
		// extract vertex data
		logOutput.Print(LOG_PIECE_DETAIL, "Processing vertices for mesh %d (%d vertices)", meshIndex, mesh->mNumVertices );
		logOutput.Print(LOG_PIECE_DETAIL, "Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals())?"Y":"N",
				(mesh->HasTangentsAndBitangents())?"Y":"N",
				(mesh->HasTextureCoords(0)?"Y":"N")
		);
		for ( unsigned vertexIndex= 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
			SAssVertex vertex;

			// vertex coordinates
			logOutput.Print(LOG_PIECE_DETAIL, "Fetching vertex %d from mesh", vertexIndex );
			aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			vertex.pos.x = aiVertex.x;
			vertex.pos.y = aiVertex.y;
			vertex.pos.z = aiVertex.z;

			// update piece min/max extents
			piece->mins.x = std::min(piece->mins.x, aiVertex.x);
			piece->mins.y = std::min(piece->mins.y, aiVertex.y);
			piece->mins.z = std::min(piece->mins.z, aiVertex.z);
			piece->maxs.x = std::max(piece->maxs.x, aiVertex.x);
			piece->maxs.y = std::max(piece->maxs.y, aiVertex.y);
			piece->maxs.z = std::max(piece->maxs.z, aiVertex.z);

			logOutput.Print(LOG_PIECE_DETAIL, "vertex position %d: %f %f %f", vertexIndex, vertex.pos.x, vertex.pos.y, vertex.pos.z );

			// vertex normal
			logOutput.Print(LOG_PIECE_DETAIL, "Fetching normal for vertex %d", vertexIndex );
			aiVector3D& aiNormal = mesh->mNormals[vertexIndex];
			vertex.hasNormal = !IS_QNAN(aiNormal);
			if ( vertex.hasNormal ) {
				vertex.normal.x = aiNormal.x;
				vertex.normal.y = aiNormal.y;
				vertex.normal.z = aiNormal.z;
				logOutput.Print(LOG_PIECE_DETAIL, "vertex normal %d: %f %f %f",vertexIndex, vertex.normal.x, vertex.normal.y,vertex.normal.z );
			}

			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				logOutput.Print(LOG_PIECE_DETAIL, "Fetching tangent for vertex %d", vertexIndex );
				aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				vertex.hasTangent = !IS_QNAN(aiTangent);
				if ( vertex.hasTangent ) {
					float3 tangent;
					tangent.x = aiTangent.x;
					tangent.y = aiTangent.y;
					tangent.z = aiTangent.z;
					logOutput.Print(LOG_PIECE_DETAIL, "vertex tangent %d: %f %f %f",vertexIndex, tangent.x, tangent.y,tangent.z );
					piece->sTangents.push_back(tangent);
					// bitangent is cross product of tangent and normal
					float3 bitangent;
					if ( vertex.hasNormal ) {
						bitangent = tangent.cross(vertex.normal);
						logOutput.Print(LOG_PIECE_DETAIL, "vertex bitangent %d: %f %f %f",vertexIndex, bitangent.x, bitangent.y,bitangent.z );
						piece->tTangents.push_back(bitangent);
					}
				}
			} else {
				vertex.hasTangent = false;
			}

			// vertex texcoords
			if (mesh->HasTextureCoords(0)) {
				vertex.textureX = mesh->mTextureCoords[0][vertexIndex].x;
				vertex.textureY = mesh->mTextureCoords[0][vertexIndex].y;
				logOutput.Print(LOG_PIECE_DETAIL, "vertex texcoords %d: %f %f", vertexIndex, vertex.textureX, vertex.textureY );
			}

			mesh_vertex_mapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
		}

		// extract face data
		if ( mesh->HasFaces() ) {
			logOutput.Print(LOG_PIECE_DETAIL, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);
			for ( unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++ ) {
                aiFace& face = mesh->mFaces[faceIndex];
				// get the vertex belonging to the mesh
				for ( unsigned vertexListID = 0; vertexListID < face.mNumIndices; vertexListID++ ) {
					unsigned int vertexID = mesh_vertex_mapping[face.mIndices[vertexListID]];
					logOutput.Print(LOG_PIECE_DETAIL, "face %d vertex %d", faceIndex, vertexID );
					piece->vertexDrawOrder.push_back(vertexID);
				}
			}
		}
	}

    // Check if piece is special (ie, used to set Spring model properties)
    if (strcmp(node->mName.data, "SpringHeight") == 0) {
        // Set the model height to this nodes Z value
        if (!metaTable.KeyExists("height")) {
            model->height = piece->offset.z;
            logOutput.Print (LOG_MODEL, "Model height of %f set by special node 'SpringHeight'", model->height);
        }
        return NULL;
    }
    if (strcmp(node->mName.data, "SpringRadius") == 0) {
        if (!metaTable.KeyExists("midpos")) {
            model->relMidPos = float3(piece->offset.x, piece->offset.z, piece->offset.y); // Y and Z are swapped because this piece isn't rotated
            logOutput.Print (LOG_MODEL, "Model midpos of (%f,%f,%f) set by special node 'SpringRadius'", model->relMidPos.x, model->relMidPos.y, model->relMidPos.z);
        }
        if (!metaTable.KeyExists("radius")) {
            if (piece->maxs.x <= 0.00001f) {
                model->radius = piece->scale.x; // the blender import script only sets the scale property
            } else {
                model->radius = piece->maxs.x; // use the transformed mesh extents
            }
            logOutput.Print (LOG_MODEL, "Model radius of %f set by special node 'SpringRadius'", model->radius);
        }
        return NULL;
    }

	// collision volume for piece (not sure about these coords)
	const float3 cvScales = (piece->maxs) - (piece->mins);
	const float3 cvOffset = (piece->maxs - piece->offset) + (piece->mins - piece->offset);
	//const float3 cvOffset(piece->offset.x, piece->offset.y, piece->offset.z);
	piece->colvol = new CollisionVolume("box", cvScales, cvOffset, CollisionVolume::COLVOL_HITTEST_CONT);

    // Get parent name from metadata or model
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

    logOutput.Print(LOG_PIECE, "Loaded model piece: %s with %d meshes\n", piece->name.c_str(), node->mNumMeshes );

    // Verbose logging of piece properties
    logOutput.Print(LOG_PIECE, "piece->name: %s", piece->name.c_str());
    logOutput.Print(LOG_PIECE, "piece->parent: %s", piece->parentName.c_str());

	// Recursively process all child pieces
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		LoadPiece(model, node->mChildren[i], metaTable);
	}

	model->pieces[piece->name] = piece;
	return piece;
}

// Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy( S3DModel* model )
{
    // Loop through all pieces and create missing hierarchy info
    PieceMap::const_iterator end = model->pieces.end();
    for (PieceMap::const_iterator it = model->pieces.begin(); it != end; ++it)
    {
        S3DModelPiece* piece = it->second;
        if (piece->name == "root") {
            piece->parent = NULL;
            model->SetRootPiece(piece);
        } else if (piece->parentName != "") {
            piece->parent = model->FindPiece(piece->parentName);
            if (piece->parent == NULL) {
                logOutput.Print( LOG_PIECE, "Error: Missing piece '%s' declared as parent of '%s'.\n", piece->parentName.c_str(), piece->name.c_str() );
            } else {
                piece->parent->childs.push_back(piece);
            }
        } else {
            // A piece with no parent that isn't the root (orphan)
            piece->parent = model->FindPiece("root");
            if (piece->parent == NULL) {
                logOutput.Print( LOG_PIECE, "Error: Missing root piece.\n" );
            } else {
                piece->parent->childs.push_back(piece);
            }
        }
    }
}

// Iterate over the model and calculate its overall dimensions
void CAssParser::CalculateMinMax( S3DModelPiece* piece )
{
    piece->goffset = piece->parent ? piece->parent->goffset + piece->offset : piece->offset;

	// update model min/max extents
	piece->model->mins.x = std::min(piece->goffset.x + piece->mins.x, piece->model->mins.x);
	piece->model->mins.y = std::min(piece->goffset.y + piece->mins.y, piece->model->mins.y);
	piece->model->mins.z = std::min(piece->goffset.z + piece->mins.z, piece->model->mins.z);
	piece->model->maxs.x = std::max(piece->goffset.x + piece->maxs.x, piece->model->maxs.x);
	piece->model->maxs.y = std::max(piece->goffset.y + piece->maxs.y, piece->model->maxs.y);
	piece->model->maxs.z = std::max(piece->goffset.z + piece->maxs.z, piece->model->maxs.z);

	// Repeat with childs
	for (unsigned int i = 0; i < piece->childs.size(); i++) {
		CalculateMinMax(piece->childs[i]);
	}
}

// Calculate model radius from the min/max extents
void CAssParser::CalculateRadius( S3DModel* model )
{
    model->radius = std::max(model->radius, model->maxs.x);
    model->radius = std::max(model->radius, model->maxs.y);
    model->radius = std::max(model->radius, model->maxs.z);
}

// Calculate model height from the min/max extents
void CAssParser::CalculateHeight( S3DModel* model )
{
    model->height = model->maxs.z;
}

void DrawPiecePrimitive( const S3DModelPiece* o)
{
	if (o->isEmpty) {
		return;
	}
	logOutput.Print(LOG_PIECE_DETAIL, "Compiling piece %s", o->name.c_str());
	// Add GL commands to the pieces displaylist

	const SAssPiece* so = static_cast<const SAssPiece*>(o);
	const SAssVertex* sAssV = static_cast<const SAssVertex*>(&so->vertices[0]);


	// pass the tangents as 3D texture coordinates
	// (array elements are float3's, which are 12
	// bytes in size and each represent a single
	// xyz triple)
	// TODO: test if we have this many texunits
	// (if not, could only send the s-tangents)?

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->sTangents[0].x);
	}
	if (!so->tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->tTangents[0].x);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SAssVertex), &sAssV->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SAssVertex), &sAssV->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SAssVertex), &sAssV->normal.x);

	// since aiProcess_SortByPType is being used, we're sure we'll get only 1 type here, so combination check isn't needed, also anything more complex than triangles is being split thanks to aiProcess_Triangulate
	glDrawElements(GL_TRIANGLES, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (!so->tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	logOutput.Print(LOG_PIECE_DETAIL, "Completed compiling piece %s", o->name.c_str());
}

void CAssParser::Draw( const S3DModelPiece* o) const
{
	DrawPiecePrimitive( o );
}

void SAssPiece::DrawList() const
{
	DrawPiecePrimitive(this);
}

/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_OBJ_IMPORTER

#include "DefaultIOSystem.h"
#include "ObjFileImporter.h"
#include "ObjFileParser.h"
#include "ObjFileData.h"

namespace Assimp	{

using namespace std;

// ------------------------------------------------------------------------------------------------
//	Default constructor
ObjFileImporter::ObjFileImporter() :
	m_Buffer(),	
	m_pRootObject( NULL ),
	m_strAbsPath( "" )
{
    DefaultIOSystem io;
	m_strAbsPath = io.getOsSeparator();
}

// ------------------------------------------------------------------------------------------------
//	Destructor.
ObjFileImporter::~ObjFileImporter()
{
	// Release root object instance
	if (NULL != m_pRootObject)
	{
		delete m_pRootObject;
		m_pRootObject = NULL;
	}
}

// ------------------------------------------------------------------------------------------------
//	Returns true, if file is an obj file.
bool ObjFileImporter::CanRead( const std::string& pFile, IOSystem*  pIOHandler , bool checkSig ) const
{
	if(!checkSig) //Check File Extension
	{
		return SimpleExtensionCheck(pFile,"obj");
	}
	else //Check file Header
	{
		const char* tokens[] = {"mtllib","usemtl","vt ","vn ","o "};
		return BaseImporter::SearchFileHeaderForToken(pIOHandler, pFile, tokens, 5);
	}
}

// ------------------------------------------------------------------------------------------------
//	Obj-file import implementation
void ObjFileImporter::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
    DefaultIOSystem io;
    
	// Read file into memory
	const std::string mode  = "rb";
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, mode));
	if (NULL == file.get())
		throw DeadlyImportError( "Failed to open file " + pFile + ".");

	// Get the filesize and vaslidate it, throwing an exception when failes
	size_t fileSize = file->FileSize();
	if( fileSize < 16)
		throw DeadlyImportError( "OBJ-file is too small.");

	// Allocate buffer and read file into it
	TextFileToBuffer(file.get(),m_Buffer);

	// Get the model name
	std::string  strModelName;
	std::string::size_type pos = pFile.find_last_of( "\\/" );
	if ( pos != std::string::npos )	
	{
		strModelName = pFile.substr(pos+1, pFile.size() - pos - 1);
	}
	else
	{
		strModelName = pFile;
	}
	
	// parse the file into a temporary representation
	ObjFileParser parser(m_Buffer, strModelName, pIOHandler);

	// And create the proper return structures out of it
	CreateDataFromImport(parser.GetModel(), pScene);

	// Clean up allocated storage for the next import 
	m_Buffer.clear();
}

// ------------------------------------------------------------------------------------------------
//	Create the data from parsed obj-file
void ObjFileImporter::CreateDataFromImport(const ObjFile::Model* pModel, aiScene* pScene)
{
	if (0L == pModel)
		return;
		
	// Create the root node of the scene
	pScene->mRootNode = new aiNode;
	if ( !pModel->m_ModelName.empty() )
	{
		// Set the name of the scene
		pScene->mRootNode->mName.Set(pModel->m_ModelName);
	}
	else
	{
		// This is an error, so break down the application
		ai_assert(false);
	}

	// Create nodes for the whole scene	
	std::vector<aiMesh*> MeshArray;
	for (size_t index = 0; index < pModel->m_Objects.size(); index++)
	{
		createNodes(pModel, pModel->m_Objects[ index ], index, pScene->mRootNode, pScene, MeshArray);
	}

	// Create mesh pointer buffer for this scene
	if (pScene->mNumMeshes > 0)
	{
		pScene->mMeshes = new aiMesh*[ MeshArray.size() ];
		for (size_t index =0; index < MeshArray.size(); index++)
		{
			pScene->mMeshes [ index ] = MeshArray[ index ];
		}
	}

	// Create all materials
	createMaterials( pModel, pScene );
}

// ------------------------------------------------------------------------------------------------
//	Creates all nodes of the model
aiNode *ObjFileImporter::createNodes(const ObjFile::Model* pModel, const ObjFile::Object* pObject, 
									 unsigned int uiMeshIndex,
									 aiNode *pParent, aiScene* pScene, 
									 std::vector<aiMesh*> &MeshArray )
{
	ai_assert( NULL != pModel );
	if ( NULL == pObject )
		return NULL;
	
	// Store older mesh size to be able to computate mesh offsets for new mesh instances
	const size_t oldMeshSize = MeshArray.size();
	aiNode *pNode = new aiNode;
	
	if (pParent != NULL)
		appendChildToParentNode(pParent, pNode);

	
	for ( unsigned int i=0; i< pObject->m_Meshes.size(); i++ )
	{
		unsigned int meshId = pObject->m_Meshes[ i ];
		aiMesh *pMesh = new aiMesh;
		createTopology( pModel, pObject, meshId, pMesh );	
		if ( pMesh->mNumVertices > 0 ) 
		{
			MeshArray.push_back( pMesh );
		}
		else
		{
			delete pMesh;
		}
	}

	// Create all nodes from the sub-objects stored in the current object
	if ( !pObject->m_SubObjects.empty() )
	{
		size_t numChilds = pObject->m_SubObjects.size();
		pNode->mNumChildren = static_cast<unsigned int>( numChilds );
		pNode->mChildren = new aiNode*[ numChilds ];
		pNode->mNumMeshes = 1;
		pNode->mMeshes = new unsigned int[ 1 ];
	}

	// Set mesh instances into scene- and node-instances
	const size_t meshSizeDiff = MeshArray.size()- oldMeshSize;
	if ( meshSizeDiff > 0 )
	{
		pNode->mMeshes = new unsigned int[ meshSizeDiff ];
		pNode->mNumMeshes = static_cast<unsigned int>( meshSizeDiff );
		size_t index = 0;
		for (size_t i = oldMeshSize; i < MeshArray.size(); i++)
		{
			pNode->mMeshes[ index ] = pScene->mNumMeshes;
			pScene->mNumMeshes++;
			index++;
		}
	}
	
	return pNode;
}

// ------------------------------------------------------------------------------------------------
//	Create topology data
void ObjFileImporter::createTopology(const ObjFile::Model* pModel, 
									 const ObjFile::Object* pData, 
									 unsigned int uiMeshIndex,
									 aiMesh* pMesh )
{
	// Checking preconditions
	ai_assert( NULL != pModel );
	if (NULL == pData)
		return;

	// Create faces
	ObjFile::Mesh *pObjMesh = pModel->m_Meshes[ uiMeshIndex ];
	ai_assert( NULL != pObjMesh );
	pMesh->mNumFaces = static_cast<unsigned int>( pObjMesh->m_Faces.size() );
	if ( pMesh->mNumFaces > 0 )
	{
		pMesh->mFaces = new aiFace[ pMesh->mNumFaces ];
		if ( pObjMesh->m_uiMaterialIndex != ObjFile::Mesh::NoMaterial )
		{
			pMesh->mMaterialIndex = pObjMesh->m_uiMaterialIndex;
		}

		// Copy all data from all stored meshes
		for (size_t index = 0; index < pObjMesh->m_Faces.size(); index++)
		{
			aiFace *pFace = &pMesh->mFaces[ index ];
			const unsigned int uiNumIndices = (unsigned int) pObjMesh->m_Faces[ index ]->m_pVertices->size();
			pFace->mNumIndices = (unsigned int) uiNumIndices;
			if (pFace->mNumIndices > 0)
			{
				pFace->mIndices = new unsigned int[ uiNumIndices ];
				ObjFile::Face::IndexArray *pIndexArray = pObjMesh->m_Faces[ index ]->m_pVertices;
				ai_assert ( NULL != pIndexArray );
				for ( size_t a=0; a<pFace->mNumIndices; a++ )
				{
					pFace->mIndices[ a ] = pIndexArray->at( a );
				}
			}
			else
			{
				pFace->mIndices = NULL;
			}
		}
	}

	// Create mesh vertices
	createVertexArray(pModel, pData, uiMeshIndex, pMesh);
}

// ------------------------------------------------------------------------------------------------
//	Creates a vertex array
void ObjFileImporter::createVertexArray(const ObjFile::Model* pModel, 
										const ObjFile::Object* pCurrentObject, 
										unsigned int uiMeshIndex,
										aiMesh* pMesh)
{
	// Checking preconditions
	ai_assert( NULL != pCurrentObject );
	
	// Break, if no faces are stored in object
	if ( pCurrentObject->m_Meshes.empty() )
		return;

	// Get current mesh
	ObjFile::Mesh *pObjMesh = pModel->m_Meshes[ uiMeshIndex ];
	if ( NULL == pObjMesh || pObjMesh->m_uiNumIndices < 1)
		return;

	// Copy vertices of this mesh instance
	pMesh->mNumVertices = (unsigned int) pObjMesh->m_uiNumIndices;
	pMesh->mVertices = new aiVector3D[ pMesh->mNumVertices ];
	
	// Allocate buffer for normal vectors
	if ( !pModel->m_Normals.empty() && pObjMesh->m_hasNormals )
		pMesh->mNormals = new aiVector3D[ pMesh->mNumVertices ];
	
	// Allocate buffer for texture coordinates
	if ( !pModel->m_TextureCoord.empty() && pObjMesh->m_uiUVCoordinates[0] )
	{
		pMesh->mNumUVComponents[ 0 ] = 2;
		pMesh->mTextureCoords[ 0 ]   = new aiVector3D[ pMesh->mNumVertices ];
	}
	
	// Copy vertices, normals and textures into aiMesh instance
	unsigned int newIndex = 0;
	for ( size_t index=0; index < pObjMesh->m_Faces.size(); index++ )
	{
		// Get destination face
		aiFace *pDestFace = &pMesh->mFaces[ index ];
		
		// Get source face
		ObjFile::Face *pSourceFace = pObjMesh->m_Faces[ index ]; 

		// Copy all index arrays
		for ( size_t vertexIndex = 0; vertexIndex < pSourceFace->m_pVertices->size(); vertexIndex++ )
		{
			const unsigned int vertex = pSourceFace->m_pVertices->at( vertexIndex );
			ai_assert( vertex < pModel->m_Vertices.size() );
			pMesh->mVertices[ newIndex ] = pModel->m_Vertices[ vertex ];
			
			// Copy all normals 
			if ( !pSourceFace->m_pNormals->empty() )
			{
				const unsigned int normal = pSourceFace->m_pNormals->at( vertexIndex );
				ai_assert( normal < pModel->m_Normals.size() );
				pMesh->mNormals[ newIndex ] = pModel->m_Normals[ normal ];
			}
			
			// Copy all texture coordinates
			if ( !pModel->m_TextureCoord.empty() )
			{
				if ( !pSourceFace->m_pTexturCoords->empty() )
				{
					const unsigned int tex = pSourceFace->m_pTexturCoords->at( vertexIndex );
					ai_assert( tex < pModel->m_TextureCoord.size() );
					for ( size_t i=0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; i++ )
					{
						if ( pMesh->mNumUVComponents[ i ] > 0 )
						{
							aiVector2D coord2d = pModel->m_TextureCoord[ tex ];
							pMesh->mTextureCoords[ i ][ newIndex ] = aiVector3D( coord2d.x, coord2d.y, 0.0 );
						}
					}
				}
			}

			ai_assert( pMesh->mNumVertices > newIndex );
			pDestFace->mIndices[ vertexIndex ] = newIndex;
			++newIndex;
		}
	}	
}

// ------------------------------------------------------------------------------------------------
//	Counts all stored meshes 
void ObjFileImporter::countObjects(const std::vector<ObjFile::Object*> &rObjects, int &iNumMeshes)
{
	iNumMeshes = 0;
	if ( rObjects.empty() )	
		return;

	iNumMeshes += static_cast<unsigned int>( rObjects.size() );
	for (std::vector<ObjFile::Object*>::const_iterator it = rObjects.begin();
		it != rObjects.end(); 
		++it)
	{
		if (!(*it)->m_SubObjects.empty())
		{
			countObjects((*it)->m_SubObjects, iNumMeshes);
		}
	}
}

// ------------------------------------------------------------------------------------------------
//	Creates the material 
void ObjFileImporter::createMaterials(const ObjFile::Model* pModel, aiScene* pScene )
{
	ai_assert( NULL != pScene );
	if ( NULL == pScene )
		return;

	const unsigned int numMaterials = (unsigned int) pModel->m_MaterialLib.size();
	pScene->mNumMaterials = 0;
	if ( pModel->m_MaterialLib.empty() )
		return;
	
	pScene->mMaterials = new aiMaterial*[ numMaterials ];
	for ( unsigned int matIndex = 0; matIndex < numMaterials; matIndex++ )
	{
		Assimp::MaterialHelper* mat = new Assimp::MaterialHelper;
		
		// Store material name
		std::map<std::string, ObjFile::Material*>::const_iterator it;
		it = pModel->m_MaterialMap.find( pModel->m_MaterialLib[ matIndex ] );
		
		// No material found, use the default material
		if ( pModel->m_MaterialMap.end() == it )
			continue;

		ObjFile::Material *pCurrentMaterial = (*it).second;
		mat->AddProperty( &pCurrentMaterial->MaterialName, AI_MATKEY_NAME );

		// convert illumination model
		int sm = 0;
		switch (pCurrentMaterial->illumination_model) 
		{
		case 0:
			sm = aiShadingMode_NoShading;
			break;
		case 1:
			sm = aiShadingMode_Gouraud;
			break;
		case 2:
			sm = aiShadingMode_Phong;
			break;
		default:
			sm = aiShadingMode_Gouraud;
			DefaultLogger::get()->error("OBJ/MTL: Unexpected illumination model (0-2 recognized)");
		}
		mat->AddProperty<int>( &sm, 1, AI_MATKEY_SHADING_MODEL);

		// multiplying the specular exponent with 2 seems to yield better results
		pCurrentMaterial->shineness *= 4.f;

		// Adding material colors
		mat->AddProperty( &pCurrentMaterial->ambient, 1, AI_MATKEY_COLOR_AMBIENT );
		mat->AddProperty( &pCurrentMaterial->diffuse, 1, AI_MATKEY_COLOR_DIFFUSE );
		mat->AddProperty( &pCurrentMaterial->specular, 1, AI_MATKEY_COLOR_SPECULAR );
		mat->AddProperty( &pCurrentMaterial->shineness, 1, AI_MATKEY_SHININESS );
		mat->AddProperty( &pCurrentMaterial->alpha, 1, AI_MATKEY_OPACITY );

		// Adding refraction index
		mat->AddProperty( &pCurrentMaterial->ior, 1, AI_MATKEY_REFRACTI );

		// Adding textures
		if ( 0 != pCurrentMaterial->texture.length )
			mat->AddProperty( &pCurrentMaterial->texture, AI_MATKEY_TEXTURE_DIFFUSE(0));

		if ( 0 != pCurrentMaterial->textureAmbient.length )
			mat->AddProperty( &pCurrentMaterial->textureAmbient, AI_MATKEY_TEXTURE_AMBIENT(0));

		if ( 0 != pCurrentMaterial->textureSpecular.length )
			mat->AddProperty( &pCurrentMaterial->textureSpecular, AI_MATKEY_TEXTURE_SPECULAR(0));

		if ( 0 != pCurrentMaterial->textureBump.length )
			mat->AddProperty( &pCurrentMaterial->textureBump, AI_MATKEY_TEXTURE_HEIGHT(0));

		if ( 0 != pCurrentMaterial->textureOpacity.length )
			mat->AddProperty( &pCurrentMaterial->textureOpacity, AI_MATKEY_TEXTURE_OPACITY(0));

		if ( 0 != pCurrentMaterial->textureSpecularity.length )
			mat->AddProperty( &pCurrentMaterial->textureSpecularity, AI_MATKEY_TEXTURE_SHININESS(0));
		
		// Store material property info in material array in scene
		pScene->mMaterials[ pScene->mNumMaterials ] = mat;
		pScene->mNumMaterials++;
	}
	
	// Test number of created materials.
	ai_assert( pScene->mNumMaterials == numMaterials );
}

// ------------------------------------------------------------------------------------------------
//	Appends this node to the parent node
void ObjFileImporter::appendChildToParentNode(aiNode *pParent, aiNode *pChild)
{
	// Checking preconditions
	ai_assert( NULL != pParent );
	ai_assert( NULL != pChild );

	// Assign parent to child
	pChild->mParent = pParent;
	size_t sNumChildren = 0;
	
	// If already children was assigned to the parent node, store them in a 
	std::vector<aiNode*> temp;
	if (pParent->mChildren != NULL)
	{
		sNumChildren = pParent->mNumChildren;
		ai_assert( 0 != sNumChildren );
		for (size_t index = 0; index < pParent->mNumChildren; index++)
		{
			temp.push_back(pParent->mChildren [ index ] );
		}
		delete [] pParent->mChildren;
	}
	
	// Copy node instances into parent node
	pParent->mNumChildren++;
	pParent->mChildren = new aiNode*[ pParent->mNumChildren ];
	for (size_t index = 0; index < pParent->mNumChildren-1; index++)
	{
		pParent->mChildren[ index ] = temp [ index ];
	}
	pParent->mChildren[ pParent->mNumChildren-1 ] = pChild;
}

// ------------------------------------------------------------------------------------------------

}	// Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER

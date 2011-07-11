/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2011, ASSIMP Development Team

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

/** @file  export.h
*  @brief Defines the C-API for the Assimp export interface
*/
#ifndef AI_EXPORT_H_INC
#define AI_EXPORT_H_INC

#ifndef ASSIMP_BUILD_NO_EXPORT

#include "aiTypes.h"

#ifdef __cplusplus
#include <boost/noncopyable.hpp>
extern "C" {
#endif

struct aiScene;  // aiScene.h

/** Describes an file format which Assimp can export to. Use aiGetExportFormatCount() to 
* learn how many export formats the current Assimp build supports and aiGetExportFormatDescription()
* to retrieve a description of an export format option.
*/
struct aiExportFormatDesc
{
	/// a short string ID to uniquely identify the export format. Use this ID string to 
	/// specify which file format you want to export to when calling aiExportScene().
	/// Example: "dae" or "obj"
	const char* id; 

	/// A short description of the file format to present to users. Useful if you want
	/// to allow the user to select an export format.
	const char* description;

	/// Recommended file extension for the exported file in lower case. 
	const char* fileExtension;
};

/** Returns the number of export file formats available in the current Assimp build.
* Use aiGetExportFormatDescription() to retrieve infos of a specific export format.
*/
ASSIMP_API size_t aiGetExportFormatCount(void);

/** Returns a description of the nth export file format. Use aiGetExportFormatCount()
* to learn how many export formats are supported. 
* @param pIndex Index of the export format to retrieve information for. Valid range is 0 to aiGetExportFormatCount()
* @return A description of that specific export format. NULL if pIndex is out of range.
*/
ASSIMP_API const C_STRUCT aiExportFormatDesc* aiGetExportFormatDescription( size_t pIndex);

/** Describes a blob of exported scene data. Use aiExportScene() to create a blob containing an
* exported scene. The memory referred by this structure is owned by Assimp. Use aiReleaseExportedFile()
* to free its resources. Don't try to free the memory on your side - it will crash for most build configurations
* due to conflicting heaps.
*
* Blobs can be nested - each blob may reference another blob, which may in turn reference another blob and so on.
* This is used when exporters write more than one output file for a given #aiScene. See the remarks for
* #aiExportDataBlob::name for more information.
*/
struct aiExportDataBlob 
#ifdef __cplusplus
	: public boost::noncopyable
#endif // __cplusplus
{
	/// Size of the data in bytes
	size_t size;

	/// The data. 
	void* data;

	/** Name of the blob. An empty string always
	    indicates the first (and primary) blob,
	    which contains the actual file data.
        Any other blobs are auxiliary files produced
	    by exporters (i.e. material files). Existence
	    of such files depends on the file format. Most
	    formats don't split assets across multiple files.

		If used, blob names usually contain the file
		extension that should be used when writing 
		the data to disc.
	 */
	aiString name;

	/** Pointer to the next blob in the chain or NULL if there is none. */
	aiExportDataBlob * next;

#ifdef __cplusplus
	/// Default constructor
	aiExportDataBlob() { size = 0; data = next = NULL; }
	/// Releases the data
	~aiExportDataBlob() { delete static_cast<char*>( data ); delete next; }
#endif // __cplusplus
};


// --------------------------------------------------------------------------------
/** Exports the given scene to a chosen file format and writes the result file(s) to disk.
* @param pScene The scene to export. Stays in possession of the caller, is not changed by the function.
* @param pFormatId ID string to specify to which format you want to export to. Use 
* aiGetExportFormatCount() / aiGetExportFormatDescription() to learn which export formats are available.
* @param pFileName Output file to write
* @param pIO custom IO implementation to be used. Use this if you use your own storage methods.
*   If none is supplied, a default implementation using standard file IO is used. Note that
*   #aiExportSceneToBlob is provided as convienience function to export to memory buffers.
* @return a status code indicating the result of the export
*/
ASSIMP_API aiReturn aiExportScene( const C_STRUCT aiScene* pScene, const char* pFormatId, const char* pFileName);


// --------------------------------------------------------------------------------
/** Exports the given scene to a chosen file format using custom IO logic supplied by you.
* @param pScene The scene to export. Stays in possession of the caller, is not changed by the function.
* @param pFormatId ID string to specify to which format you want to export to. Use 
* aiGetExportFormatCount() / aiGetExportFormatDescription() to learn which export formats are available.
* @param pFileName Output file to write
* @param pIO custom IO implementation to be used. Use this if you use your own storage methods.
*   If none is supplied, a default implementation using standard file IO is used. Note that
*   #aiExportSceneToBlob is provided as convienience function to export to memory buffers.
* @return a status code indicating the result of the export
*/
ASSIMP_API aiReturn aiExportSceneEx( const C_STRUCT aiScene* pScene, const char* pFormatId, const char* pFileName, C_STRUCT aiFileIO* pIO );

// --------------------------------------------------------------------------------
/** Exports the given scene to a chosen file format. Returns the exported data as a binary blob which
* you can write into a file or something. When you're done with the data, use aiReleaseExportedBlob()
* to free the resources associated with the export. 
* @param pScene The scene to export. Stays in possession of the caller, is not changed by the function.
* @param pFormatId ID string to specify to which format you want to export to. Use 
* aiGetExportFormatCount() / aiGetExportFormatDescription() to learn which export formats are available.
* @return the exported data or NULL in case of error
*/
ASSIMP_API const C_STRUCT aiExportDataBlob* aiExportSceneToBlob( const C_STRUCT aiScene* pScene, const char* pFormatId );



// --------------------------------------------------------------------------------
/** Releases the memory associated with the given exported data. Use this function to free a data blob
* returned by aiExportScene(). 
* @param pData the data blob returned by aiExportScenetoBlob
*/
ASSIMP_API C_STRUCT void aiReleaseExportData( const C_STRUCT aiExportDataBlob* pData );

#ifdef __cplusplus
}
#endif

#endif // ASSIMP_BUILD_NO_EXPORT
#endif // AI_EXPORT_H_INC


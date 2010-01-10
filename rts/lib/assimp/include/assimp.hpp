/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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

/** @file  assimp.hpp
 *  @brief Defines the C++-API to the Open Asset Import Library.
 */
#ifndef INCLUDED_AI_ASSIMP_HPP
#define INCLUDED_AI_ASSIMP_HPP

#ifndef __cplusplus
#	error This header requires C++ to be used. Use assimp.h for plain C. 
#endif

// Public ASSIMP data structures
#include "aiTypes.h"
#include "aiConfig.h"
#include "aiAssert.h"

namespace Assimp	{
	// =======================================================================
	// Public interface to Assimp 
	class Importer;
	class IOStream;
	class IOSystem;

	// =======================================================================
	// Plugin development
	//
	// Include the following headers for the declarations:
	// BaseImporter.h
	// BaseProcess.h
	class BaseImporter;
	class BaseProcess;
	class SharedPostProcessInfo;
	class BatchLoader;

	// =======================================================================
	// Holy stuff, only for members of the high council of the Jedi.
	class ImporterPimpl;
} //! namespace Assimp

#define AI_PROPERTY_WAS_NOT_EXISTING 0xffffffff

struct aiScene;
struct aiFileIO;
extern "C" ASSIMP_API const aiScene* aiImportFileEx( const char*, unsigned int, aiFileIO*);

namespace Assimp	{

// ----------------------------------------------------------------------------------
/** The Importer class forms an C++ interface to the functionality of the 
*   Open Asset Import Library.
*
* Create an object of this class and call ReadFile() to import a file. 
* If the import succeeds, the function returns a pointer to the imported data. 
* The data remains property of the object, it is intended to be accessed 
* read-only. The imported data will be destroyed along with the Importer 
* object. If the import fails, ReadFile() returns a NULL pointer. In this
* case you can retrieve a human-readable error description be calling 
* GetErrorString(). You can call ReadFile() multiple times with a single Importer
* instance. Actually, constructing Importer objects involves quite many
* allocations and may take some time, so it's better to reuse them as often as
* possible.
*
* If you need the Importer to do custom file handling to access the files,
* implement IOSystem and IOStream and supply an instance of your custom 
* IOSystem implementation by calling SetIOHandler() before calling ReadFile().
* If you do not assign a custion IO handler, a default handler using the 
* standard C++ IO logic will be used.
*
* @note One Importer instance is not thread-safe. If you use multiple
* threads for loading, each thread should maintain its own Importer instance.
*/
class ASSIMP_API Importer
{
	// used internally
	friend class BaseProcess;
	friend class BatchLoader;
	friend const aiScene* ::aiImportFileEx( const char*, unsigned int, aiFileIO*);

public:

	// -------------------------------------------------------------------
	/** Constructor. Creates an empty importer object. 
	 * 
	 * Call ReadFile() to start the import process. The configuration
	 * property table is initially empty.
	 */
	Importer();

	// -------------------------------------------------------------------
	/** Copy constructor.
	 * 
	 * This copies the configuration properties of another Importer.
	 * If this Importer owns a scene it won't be copied.
	 * Call ReadFile() to start the import process.
	 */
	Importer(const Importer& other);

	// -------------------------------------------------------------------
	/** Destructor. The object kept ownership of the imported data,
	 * which now will be destroyed along with the object. 
	 */
	~Importer();


	// -------------------------------------------------------------------
	/** Registers a new loader.
	 *
	 * @param pImp Importer to be added. The Importer instance takes 
	 *   ownership of the pointer, so it will be automatically deleted
	 *   with the Importer instance.
	 * @return AI_SUCCESS if the loader has been added. The registration
	 *   fails if there is already a loader for a specific file extension.
	 */
	aiReturn RegisterLoader(BaseImporter* pImp);

	// -------------------------------------------------------------------
	/** Unregisters a loader.
	 *
	 * @param pImp Importer to be unregistered.
	 * @return AI_SUCCESS if the loader has been removed. The function
	 *   fails if the loader is currently in use (this could happen
	 *   if the #Importer instance is used by more than one thread) or
	 *   if it has not yet been registered.
	 */
	aiReturn UnregisterLoader(BaseImporter* pImp);

#if 0
	// -------------------------------------------------------------------
	/** Registers a new post-process step.
	 *
	 * @param pImp Post-process step to be added. The Importer instance 
	 *   takes ownership of the pointer, so it will be automatically 
	 *   deleted with the Importer instance.
	 * @return AI_SUCCESS if the step has been added.
	 */
	aiReturn RegisterPPStep(BaseProcess* pImp);


	// -------------------------------------------------------------------
	/** Unregisters a post-process step.
	 *
	 * @param pImp Step to be unregistered. 
	 * @return AI_SUCCESS if the step has been removed. The function
	 *   fails if the step is currently in use (this could happen
	 *   if the #Importer instance is used by more than one thread) or
	 *   if it has not yet been registered.
	 */
	aiReturn UnregisterPPStep(BaseProcess* pImp);
#endif

	// -------------------------------------------------------------------
	/** Set an integer configuration property.
	 * @param szName Name of the property. All supported properties
	 *   are defined in the aiConfig.g header (all constants share the
	 *   prefix AI_CONFIG_XXX).
	 * @param iValue New value of the property
	 * @param bWasExisting Optional pointer to receive true if the
	 *   property was set before. The new value replaced the old value
	 *   in this case.
	 * @note Property of different types (float, int, string ..) are kept
	 *   on different stacks, so calling SetPropertyInteger() for a 
	 *   floating-point property has no effect - the loader will call
	 *   GetPropertyFloat() to read the property, but it won't be there.
	 */
	void SetPropertyInteger(const char* szName, int iValue, 
		bool* bWasExisting = NULL);

	// -------------------------------------------------------------------
	/** Set a floating-point configuration property.
	 * @see SetPropertyInteger()
	 */
	void SetPropertyFloat(const char* szName, float fValue, 
		bool* bWasExisting = NULL);

	// -------------------------------------------------------------------
	/** Set a string configuration property.
	 * @see SetPropertyInteger()
	 */
	void SetPropertyString(const char* szName, const std::string& sValue, 
		bool* bWasExisting = NULL);

	// -------------------------------------------------------------------
	/** Get a configuration property.
	 * @param szName Name of the property. All supported properties
	 *   are defined in the aiConfig.g header (all constants share the
	 *   prefix AI_CONFIG_XXX).
	 * @param iErrorReturn Value that is returned if the property 
	 *   is not found. 
	 * @return Current value of the property
	 * @note Property of different types (float, int, string ..) are kept
	 *   on different lists, so calling SetPropertyInteger() for a 
	 *   floating-point property has no effect - the loader will call
	 *   GetPropertyFloat() to read the property, but it won't be there.
	 */
	int GetPropertyInteger(const char* szName, 
		int iErrorReturn = 0xffffffff) const;

	// -------------------------------------------------------------------
	/** Get a floating-point configuration property
	 * @see GetPropertyInteger()
	 */
	float GetPropertyFloat(const char* szName, 
		float fErrorReturn = 10e10f) const;

	// -------------------------------------------------------------------
	/** Get a string configuration property
	 *
	 *  The return value remains valid until the property is modified.
	 * @see GetPropertyInteger()
	 */
	const std::string& GetPropertyString(const char* szName,
		const std::string& sErrorReturn = "") const;

	// -------------------------------------------------------------------
	/** Supplies a custom IO handler to the importer to use to open and
	 * access files. If you need the importer to use custion IO logic to 
	 * access the files, you need to provide a custom implementation of 
	 * IOSystem and IOFile to the importer. Then create an instance of 
	 * your custion IOSystem implementation and supply it by this function.
	 *
	 * The Importer takes ownership of the object and will destroy it 
	 * afterwards. The previously assigned handler will be deleted.
	 * Pass NULL to take again ownership of your IOSystem and reset Assimp
	 * to use its default implementation.
	 *
	 * @param pIOHandler The IO handler to be used in all file accesses 
	 *   of the Importer. 
	 */
	void SetIOHandler( IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** Retrieves the IO handler that is currently set.
	 * You can use IsDefaultIOHandler() to check whether the returned
	 * interface is the default IO handler provided by ASSIMP. The default
	 * handler is active as long the application doesn't supply its own
	 * custom IO handler via SetIOHandler().
	 * @return A valid IOSystem interface
	 */
	IOSystem* GetIOHandler();

	// -------------------------------------------------------------------
	/** Checks whether a default IO handler is active 
	 * A default handler is active as long the application doesn't 
	 * supply its own custom IO handler via SetIOHandler().
	 * @return true by default
	 */
	bool IsDefaultIOHandler();

	// -------------------------------------------------------------------
	/** @brief Check whether a given set of postprocessing flags
	 *  is supported.
	 *
	 *  Some flags are mutually exclusive, others are probably
	 *  not available because your excluded them from your
	 *  Assimp builds. Calling this function is recommended if 
	 *  you're unsure.
	 *
	 *  @param pFlags Bitwise combination of the aiPostProcess flags.
	 *  @return true if this flag combination is not supported.
	 */
	bool ValidateFlags(unsigned int pFlags);

	// -------------------------------------------------------------------
	/** Reads the given file and returns its contents if successful. 
	 * 
	 * If the call succeeds, the contents of the file are returned as a 
	 * pointer to an aiScene object. The returned data is intended to be 
	 * read-only, the importer object keeps ownership of the data and will
	 * destroy it upon destruction. If the import fails, NULL is returned.
	 * A human-readable error description can be retrieved by calling 
	 * GetErrorString(). The previous scene will be deleted during this call.
	 * @param pFile Path and filename to the file to be imported.
	 * @param pFlags Optional post processing steps to be executed after 
	 *   a successful import. Provide a bitwise combination of the 
	 *   #aiPostProcessSteps flags.
	 * @return A pointer to the imported data, NULL if the import failed.
	 *   The pointer to the scene remains in possession of the Importer
	 *   instance. Use GetOrphanedScene() to take ownership of it.
	 *
	 * @note Assimp is able to determine the file format of a file
	 * automatically. However, you should make sure that the input file
	 * does not even have a file extension at all to enable this feature.
	 */
	const aiScene* ReadFile( const char* pFile, unsigned int pFlags);

	// -------------------------------------------------------------------
	/** @brief Reads the given file and returns its contents if successful. 
	 *
	 * This function is provided for backward compatibility.
	 * See the const char* version for detailled docs.
	 * @see ReadFile(const char*, pFlags)
	 */
	const aiScene* ReadFile( const std::string& pFile, unsigned int pFlags);

	// -------------------------------------------------------------------
	/** Frees the current scene.
	 *
	 *  The function does nothing if no scene has previously been 
	 *  read via ReadFile(). FreeScene() is called automatically by the
	 *  destructor and ReadFile() itself.
	 */
	void FreeScene( );

	// -------------------------------------------------------------------
	/** Returns an error description of an error that occurred in ReadFile(). 
	 *
	 * Returns an empty string if no error occurred.
	 * @return A description of the last error, an empty string if no 
	 *   error occurred. The string is never NULL.
	 *
	 * @note The returned function remains valid until one of the 
	 * following methods is called: #ReadFile(), #FreeScene().
	 */
	const char* GetErrorString() const;

	// -------------------------------------------------------------------
	/** Returns whether a given file extension is supported by ASSIMP.
	 *
	 * @param szExtension Extension to be checked.
	 *   Must include a trailing dot '.'. Example: ".3ds", ".md3".
	 *   Cases-insensitive.
	 * @return true if the extension is supported, false otherwise
	 */
	bool IsExtensionSupported(const char* szExtension);

	// -------------------------------------------------------------------
	/** @brief Returns whether a given file extension is supported by ASSIMP.
	 *
	 * This function is provided for backward compatibility.
	 * See the const char* version for detailled docs.
	 * @see IsExtensionSupported(const char*)
	 */
	inline bool IsExtensionSupported(const std::string& szExtension);

	// -------------------------------------------------------------------
	/** Get a full list of all file extensions supported by ASSIMP.
	 *
	 * If a file extension is contained in the list this does of course not
	 * mean that ASSIMP is able to load all files with this extension.
	 * @param szOut String to receive the extension list. It just means there
     *   is a loader which handles such files.
	 *   Format of the list: "*.3ds;*.obj;*.dae". 
	 */
	void GetExtensionList(aiString& szOut);

	// -------------------------------------------------------------------
	/** @brief Get a full list of all file extensions supported by ASSIMP.
	 *
	 * This function is provided for backward compatibility.
	 * See the aiString version for detailled docs.
	 * @see GetExtensionList(aiString&)
	 */
	inline void GetExtensionList(std::string& szOut);

	// -------------------------------------------------------------------
	/** Find the loader corresponding to a specific file extension.
	*
	*  This is quite similar to IsExtensionSupported() except a
	*  BaseImporter instance is returned.
	*  @param szExtension Extension to be checked, cases insensitive,
	*    must include a trailing dot.
	*  @return NULL if there is no loader for the extension.
	*/
	BaseImporter* FindLoader (const char* szExtension);

	// -------------------------------------------------------------------
	/** Returns the scene loaded by the last successful call to ReadFile()
	 *
	 * @return Current scene or NULL if there is currently no scene loaded
	 */
	const aiScene* GetScene() const;

	// -------------------------------------------------------------------
	/** Returns the scene loaded by the last successful call to ReadFile()
	 *  and releases the scene from the ownership of the Importer 
	 *  instance. The application is now responsible for deleting the
	 *  scene. Any further calls to GetScene() or GetOrphanedScene()
	 *  will return NULL - until a new scene has been loaded via ReadFile().
	 *
	 * @return Current scene or NULL if there is currently no scene loaded
	 * @note Under windows, deleting the returned scene manually will 
	 * probably not work properly in applications using static runtime linkage.
	 */
	aiScene* GetOrphanedScene();

	// -------------------------------------------------------------------
	/** Returns the storage allocated by ASSIMP to hold the scene data
	 * in memory.
	 *
	 * This refers to the currently loaded file, see #ReadFile().
	 * @param in Data structure to be filled. 
	*/
	void GetMemoryRequirements(aiMemoryInfo& in) const;

	// -------------------------------------------------------------------
	/** Enables "extra verbose" mode. 
	 *
	 * In this mode the data structure is validated after every single
	 * post processing step to make sure everyone modifies the data
	 * structure in the defined manner. This is a debug feature and not
	 * intended for public use.
	 */
	void SetExtraVerbose(bool bDo);

protected:

	// Just because we don't want you to know how we're hacking around.
	ImporterPimpl* pimpl;
}; //! class Importer


// ----------------------------------------------------------------------------
// For compatibility, the interface of some functions taking a std::string was
// changed to const char* to avoid crashes between binary incompatible STL 
// versions. This code her is inlined,  so it shouldn't cause any problems.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
AI_FORCE_INLINE const aiScene* Importer::ReadFile( const std::string& pFile,
	unsigned int pFlags)
{
	return ReadFile(pFile.c_str(),pFlags);
}
// ----------------------------------------------------------------------------
AI_FORCE_INLINE void Importer::GetExtensionList(std::string& szOut)
{
	aiString s;
	GetExtensionList(s);
	szOut = s.data;
}
// ----------------------------------------------------------------------------
AI_FORCE_INLINE bool Importer::IsExtensionSupported(
	const std::string& szExtension)
{
	return IsExtensionSupported(szExtension.c_str());
}

} // !namespace Assimp
#endif // INCLUDED_AI_ASSIMP_HPP

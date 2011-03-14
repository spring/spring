/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file  BlenderLoader.h
 *  @brief Declaration of the Blender 3D (*.blend) importer class.
 */
#ifndef INCLUDED_AI_BLEND_LOADER_H
#define INCLUDED_AI_BLEND_LOADER_H

#include "BaseImporter.h"
namespace Assimp	{
	
	// TinyFormatter.h
	namespace Formatter {
		template <typename T,typename TR, typename A> class basic_formatter;
		typedef class basic_formatter< char, std::char_traits<char>, std::allocator<char> > format;
	}

	// BlenderDNA.h
	namespace Blender {
		class  FileDatabase;
		struct ElemBase;
	}

	// BlenderScene.h
	namespace Blender {
		struct Scene;
		struct Object;
		struct Mesh;
		struct Camera;
		struct Lamp;
		struct MTex;
		struct Image;
		struct Material;
	}

	// BlenderIntermediate.h
	namespace Blender {
		struct ConversionData;
		template <template <typename,typename> class TCLASS, typename T> struct TempArray;
	}

	// BlenderModifier.h
	namespace Blender {
		class BlenderModifierShowcase;
		class BlenderModifier;
	}

enum aiLoaderFlags 
{
	aiLoaderFlags_SupportAsciiFlavour = 0x1,
	aiLoaderFlags_SupportBinaryFlavour = 0x2,
	aiLoaderFlags_SupportCompressedFlavour = 0x4,

	aiLoaderFlags_LimitedSupport = 0x8,

	aiLoaderFlags_Experimental = 0x10,
	aiLoaderFlags_Testing = 0x20,
	aiLoaderFlags_Production = 0x40,
};

struct aiLoaderDesc 
{
	const char* mName;
	const char* mAuthor;
	const char* mMaintainer;
	const char* mComments;
	unsigned int mFlags;

	unsigned int mMinMajor;
	unsigned int mMinMinor;
	unsigned int mMaxMajor;
	unsigned int mMaxMinor;
};


// -------------------------------------------------------------------------------------------
/** Load blenders official binary format. The actual file structure (the `DNA` how they
 *  call it is outsourced to BlenderDNA.cpp/BlenderDNA.h. This class only performs the
 *  conversion from intermediate format to aiScene. */
// -------------------------------------------------------------------------------------------
class BlenderImporter : public BaseImporter
{
	friend class Importer;

protected:

	/** Constructor to be privately used by Importer */
	BlenderImporter();

	/** Destructor, private as well */
	~BlenderImporter();

public:

	// --------------------
	bool CanRead( const std::string& pFile, 
		IOSystem* pIOHandler,
		bool checkSig
	) const;

protected:

	// --------------------
	const aiLoaderDesc& GetInfo () const;

	// --------------------
	void GetExtensionList(std::set<std::string>& app);

	// --------------------
	void SetupProperties(const Importer* pImp);

	// --------------------
	void InternReadFile( const std::string& pFile, 
		aiScene* pScene, 
		IOSystem* pIOHandler
	);

	// --------------------
	void ParseBlendFile(Blender::FileDatabase& out, 
		boost::shared_ptr<IOStream> stream
	);

	// --------------------
	void ExtractScene(Blender::Scene& out, 
		const Blender::FileDatabase& file
	); 

	// --------------------
	void ConvertBlendFile(aiScene* out,
		const Blender::Scene& in,
		const Blender::FileDatabase& file
	);

private:

	// --------------------
	aiNode* ConvertNode(const Blender::Scene& in, 
		const Blender::Object* obj, 
		Blender::ConversionData& conv_info
	); 

	// --------------------
	void ConvertMesh(const Blender::Scene& in, 
		const Blender::Object* obj, 
		const Blender::Mesh* mesh, 
		Blender::ConversionData& conv_data,
		Blender::TempArray<std::vector,aiMesh>& temp
	); 

	// --------------------
	aiLight* ConvertLight(const Blender::Scene& in, 
		const Blender::Object* obj, 
		const Blender::Lamp* mesh, 
		Blender::ConversionData& conv_data
	); 

	// --------------------
	aiCamera* ConvertCamera(const Blender::Scene& in, 
		const Blender::Object* obj, 
		const Blender::Camera* mesh, 
		Blender::ConversionData& conv_data
	); 

	// --------------------
	void BuildMaterials(
		Blender::ConversionData& conv_data
	) ;

	// --------------------
	void ResolveTexture(
		MaterialHelper* out, 
		const Blender::Material* mat, 
		const Blender::MTex* tex,
		Blender::ConversionData& conv_data
	);

	// --------------------
	void ResolveImage(
		MaterialHelper* out, 
		const Blender::Material* mat, 
		const Blender::MTex* tex, 
		const Blender::Image* img,
		Blender::ConversionData& conv_data
	);

	void AddSentinelTexture(
		MaterialHelper* out, 
		const Blender::Material* mat,
		const Blender::MTex* tex, 
		Blender::ConversionData& conv_data
	);

private: // static stuff, mostly logging and error reporting.

	// --------------------
	static void CheckActualType(const Blender::ElemBase* dt, 
		const char* check
	);

	// --------------------
	static void NotSupportedObjectType(const Blender::Object* obj, 
		const char* type
	);

	// -------------------------------------------------------------------
	/** Prepend 'BLEND: ' and throw msg.*/
	static void ThrowException(const std::string& msg);

	// -------------------------------------------------------------------
	/** @defgroup blog Prepend 'BLEND: ' and write @c message to log.*/
	static void LogWarn  (const Formatter::format& message); //! @ingroup blog
	static void LogError (const Formatter::format& message); //! @ingroup blog
	static void LogInfo  (const Formatter::format& message); //! @ingroup blog
	static void LogDebug (const Formatter::format& message); //! @ingroup blog

private:

	Blender::BlenderModifierShowcase* modifier_cache;

}; // !class BlenderImporter

} // end of namespace Assimp
#endif // AI_UNREALIMPORTER_H_INC

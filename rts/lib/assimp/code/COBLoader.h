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

/** @file  COBLoader.h
 *  @brief Declaration of the TrueSpace (*.cob,*.scn) importer class.
 */
#ifndef INCLUDED_AI_COB_LOADER_H
#define INCLUDED_AI_COB_LOADER_H

#include "BaseImporter.h"
namespace Assimp	{
	class LineSplitter;
	
	// TinyFormatter.h
	namespace Formatter {
		template <typename T,typename TR, typename A> class basic_formatter;
		typedef class basic_formatter< char, std::char_traits<char>, std::allocator<char> > format;
	}

	// COBScene.h
	namespace COB {
		struct ChunkInfo;
		struct Node;
		struct Scene;
	}

// -------------------------------------------------------------------------------------------
/** Importer class to load TrueSpace files (cob,scn) up to v6. 
 *
 *  Currently relatively limited, loads only ASCII files and needs more test coverage. */
// -------------------------------------------------------------------------------------------
class COBImporter : public BaseImporter
{
	friend class Importer;

protected:

	/** Constructor to be privately used by Importer */
	COBImporter();

	/** Destructor, private as well */
	~COBImporter();

public:

	// --------------------
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler,
		bool checkSig) const;

protected:

	// --------------------
	void GetExtensionList(std::set<std::string>& app);

	// --------------------
	void SetupProperties(const Importer* pImp);

	// --------------------
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

private:

	// -------------------------------------------------------------------
	/** Prepend 'COB: ' and throw msg.*/
	static void ThrowException(const std::string& msg);

	// -------------------------------------------------------------------
	/** @brief Read from an ascii scene/object file
	 *  @param out Receives output data.
	 *  @param stream Stream to read from. */
	void ReadAsciiFile(COB::Scene& out, StreamReaderLE* stream);

	// -------------------------------------------------------------------
	/** @brief Read from a binary scene/object file
	 *  @param out Receives output data.
	 *  @param stream Stream to read from.  */
	void ReadBinaryFile(COB::Scene& out, StreamReaderLE* stream);


private:

	// Conversion to Assimp output format

	aiNode* BuildNodes(const COB::Node& root,const COB::Scene& scin,aiScene* fill);

private:

	// ASCII file support

	void UnsupportedChunk_Ascii(LineSplitter& splitter, const COB::ChunkInfo& nfo, const char* name);
	void ReadChunkInfo_Ascii(COB::ChunkInfo& out, const LineSplitter& splitter);
	void ReadBasicNodeInfo_Ascii(COB::Node& msh, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	template <typename T> void ReadFloat3Tuple_Ascii(T& fill, const char** in);

	void ReadPolH_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadBitM_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadMat1_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadGrou_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadBone_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadCame_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadLght_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadUnit_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);
	void ReadChan_Ascii(COB::Scene& out, LineSplitter& splitter, const COB::ChunkInfo& nfo);


	// ASCII file logging stuff to add proper line numbers to messages

	static void LogWarn_Ascii (const LineSplitter& splitter, const Formatter::format& message);
	static void LogError_Ascii(const LineSplitter& splitter, const Formatter::format& message);
	static void LogInfo_Ascii (const LineSplitter& splitter, const Formatter::format& message);
	static void LogDebug_Ascii(const LineSplitter& splitter, const Formatter::format& message);

	static void LogWarn_Ascii  (const Formatter::format& message);
	static void LogError_Ascii (const Formatter::format& message);
	static void LogInfo_Ascii  (const Formatter::format& message);
	static void LogDebug_Ascii (const Formatter::format& message);


	// Binary file support

	void UnsupportedChunk_Binary(StreamReaderLE& reader, const COB::ChunkInfo& nfo, const char* name);
	void ReadString_Binary(std::string& out, StreamReaderLE& reader);
	void ReadBasicNodeInfo_Binary(COB::Node& msh, StreamReaderLE& reader, const COB::ChunkInfo& nfo);

	void ReadPolH_Binary(COB::Scene& out, StreamReaderLE& reader, const COB::ChunkInfo& nfo);
	void ReadBitM_Binary(COB::Scene& out, StreamReaderLE& reader, const COB::ChunkInfo& nfo);
	void ReadMat1_Binary(COB::Scene& out, StreamReaderLE& reader, const COB::ChunkInfo& nfo);
	void ReadCame_Binary(COB::Scene& out, StreamReaderLE& reader, const COB::ChunkInfo& nfo);
	void ReadLght_Binary(COB::Scene& out, StreamReaderLE& reader, const COB::ChunkInfo& nfo);
	void ReadGrou_Binary(COB::Scene& out, StreamReaderLE& reader, const COB::ChunkInfo& nfo);
	void ReadUnit_Binary(COB::Scene& out, StreamReaderLE& reader, const COB::ChunkInfo& nfo);


}; // !class COBImporter

} // end of namespace Assimp
#endif // AI_UNREALIMPORTER_H_INC

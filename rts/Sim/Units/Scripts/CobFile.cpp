/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Sim/Misc/GlobalConstants.h"
#include "CobFile.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/Sound/ISound.h"
#include "System/Platform/byteorder.h"
#include "System/StringUtil.h"

#include <algorithm>
#include <locale>
#include <cctype>
#include <cstring>


//The following structure is taken from http://visualta.tauniverse.com/Downloads/ta-cob-fmt.txt
//Information on missing fields from Format_Cob.pas
typedef struct tagCOBHeader
{
	int VersionSignature;
	int NumberOfScripts;
	int NumberOfPieces;
	int TotalScriptLen;
	int NumberOfStaticVars;
	int Unknown_2; /* Always seems to be 0 */
	int OffsetToScriptCodeIndexArray;
	int OffsetToScriptNameOffsetArray;
	int OffsetToPieceNameOffsetArray;
	int OffsetToScriptCode;
	int Unknown_3; /* Always seems to point to first script name */

	int OffsetToSoundNameArray;		// These two are only found in TA:K scripts
	int NumberOfSounds;
} COBHeader;


#define READ_COBHEADER(ch,src)						\
do {									\
	unsigned int __tmp;						\
	unsigned short __isize = sizeof(unsigned int);			\
	unsigned int __c = 0;						\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).VersionSignature = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfScripts = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfPieces = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).TotalScriptLen = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfStaticVars = (int)swabDWord(__tmp);		\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).Unknown_2 = (int)swabDWord(__tmp);				\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToScriptCodeIndexArray = (int)swabDWord(__tmp);	\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToScriptNameOffsetArray = (int)swabDWord(__tmp);	\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToPieceNameOffsetArray = (int)swabDWord(__tmp);	\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToScriptCode = (int)swabDWord(__tmp);		\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).Unknown_3 = (int)swabDWord(__tmp);				\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToSoundNameArray = (int)swabDWord(__tmp);		\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfSounds = (int)swabDWord(__tmp);			\
} while (0)


static std::vector<uint8_t> cobFileData;


CCobFile::CCobFile(CFileHandler& in, const std::string& scriptName)
{
	name.assign(scriptName);
	scriptIndex.fill(-1);

	// handle errors (this is fairly fatal..)
	if (in.FileSize() < 0) {
		LOG_L(L_FATAL, "[%s] could not find script \"%s\"", __func__, name.c_str());
		return;
	}

	if (!in.IsBuffered()) {
		cobFileData.clear();
		cobFileData.resize(in.FileSize());
		// read the entire thing, we will need it
		in.Read(cobFileData.data(), cobFileData.size());
	} else {
		cobFileData = std::move(in.GetBuffer());
	}

	// time to parse
	COBHeader ch;
	READ_COBHEADER(ch, cobFileData.data());

	if (ch.NumberOfScripts == 0) {
		LOG_L(L_WARNING, "[%s] script \"%s\" is empty", __func__, name.c_str());
		return;
	}

	// prepare
	luaScripts.reserve(ch.NumberOfScripts);
	scriptNames.reserve(ch.NumberOfScripts);
	scriptOffsets.reserve(ch.NumberOfScripts);
	scriptLengths.reserve(ch.NumberOfScripts);
	pieceNames.reserve(ch.NumberOfPieces);

	for (int i = 0; i < ch.NumberOfScripts; ++i) {
		int ofs = *(int *) &cobFileData[ch.OffsetToScriptNameOffsetArray + i * 4];
		swabDWordInPlace(ofs);
		scriptNames.emplace_back(reinterpret_cast<const char*>(&cobFileData[ofs]));

		if (scriptNames[scriptNames.size() - 1].find("lua_") == 0) {
			luaScripts.emplace_back(scriptNames[scriptNames.size() - 1].c_str() + sizeof("lua_") - 1);
		} else {
			luaScripts.emplace_back("");
		}

		ofs = *(int *) &cobFileData[ch.OffsetToScriptCodeIndexArray + i * 4];
		swabDWordInPlace(ofs);
		scriptOffsets.push_back(ofs);
	}

	// check for zero-length scripts
	for (int i = 0; i < ch.NumberOfScripts - 1; ++i) {
		scriptLengths.push_back(scriptOffsets[i + 1] - scriptOffsets[i]);
	}

	scriptLengths.push_back(ch.TotalScriptLen - scriptOffsets[ch.NumberOfScripts - 1]);


	for (int i = 0; i < ch.NumberOfPieces; ++i) {
		int ofs = *(int *) &cobFileData[ch.OffsetToPieceNameOffsetArray + i * 4];
		swabDWordInPlace(ofs);
		pieceNames.emplace_back(StringToLower(reinterpret_cast<const char*>(&cobFileData[ofs])));
	}

	const int codeBytes = int(cobFileData.size()) - ch.OffsetToScriptCode;
	const int codeWords = codeBytes / 4 + 4;
	code.resize(codeWords);
	memcpy(code.data(), &cobFileData[ch.OffsetToScriptCode], codeBytes);
	for (int i = 0; i < codeWords; i++) {
		swabDWordInPlace(code[i]);
	}

	numStaticVars = ch.NumberOfStaticVars;

	// if this is a TA:K script, read the sound names
	if (ch.VersionSignature == 6) {
		sounds.reserve(ch.NumberOfSounds);

		for (int i = 0; i < ch.NumberOfSounds; ++i) {
			int ofs = *(int *) &cobFileData[ch.OffsetToSoundNameArray + i * 4];
			// FIXME: this probably isn't correct
			swabDWordInPlace(ofs);

			const std::string s = {reinterpret_cast<const char*>(&cobFileData[ofs])};

			if (sound->HasSoundItem(s)) {
				sounds.push_back(sound->GetSoundId(s));
			} else {
				// Load the wave file and store the ID for future use
				sounds.push_back(sound->GetSoundId("sounds/" + s + ".wav"));
			}
		}
	}


	// create a reverse mapping (name->int)
	for (size_t i = 0, n = scriptNames.size(); i < n; ++i) {
		scriptMap[scriptNames[i]] = i;
	}

	// map common function names to indices
	for (const auto& pair: CCobUnitScriptNames::GetScriptMap()) {
		const int fn = GetFunctionId(pair.first);

		if (fn < 0)
			continue;

		scriptIndex[pair.second] = fn;
	}
}


int CCobFile::GetFunctionId(const std::string& name)
{
	const auto i = scriptMap.find(name);

	if (i != scriptMap.end())
		return i->second;

	return -1;
}

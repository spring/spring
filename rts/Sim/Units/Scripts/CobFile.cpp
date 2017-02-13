/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Sim/Misc/GlobalConstants.h"
#include "CobFile.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/Sound/ISound.h"
#include "System/Platform/byteorder.h"
#include "System/Util.h"

#include <algorithm>
#include <locale>
#include <cctype>
#include <string.h>


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


CCobFile::CCobFile(CFileHandler &in, string name)
{
	char *cobdata = NULL;

	this->name = name;

	//Figure out size needed and allocate it
	int size = in.FileSize();

	// Handle errors (this is fairly fatal..)
	if (size < 0) {
		LOG_L(L_FATAL, "Could not find script for unit %s", name.c_str());
		exit(0);
	}

	cobdata = new char[size];

	//Read the entire thing, we will need it
	in.Read(cobdata, size);

	//Time to parse
	COBHeader ch;
	READ_COBHEADER(ch,cobdata);

	// prepare
	luaScripts.reserve(ch.NumberOfScripts);
	scriptNames.reserve(ch.NumberOfScripts);
	scriptOffsets.reserve(ch.NumberOfScripts);
	scriptLengths.reserve(ch.NumberOfScripts);
	pieceNames.reserve(ch.NumberOfPieces);

	for (int i = 0; i < ch.NumberOfScripts; ++i) {
		int ofs;

		ofs = *(int *)&cobdata[ch.OffsetToScriptNameOffsetArray + i * 4];
		swabDWordInPlace(ofs);
		const string s = &cobdata[ofs];
		scriptNames.push_back(s);
		if (s.find("lua_") == 0) {
			luaScripts.push_back(LuaHashString(s.substr(4)));
		} else {
			luaScripts.push_back(LuaHashString(""));
		}

		ofs = *(int *)&cobdata[ch.OffsetToScriptCodeIndexArray + i * 4];
		swabDWordInPlace(ofs);
		scriptOffsets.push_back(ofs);
	}

	//Check for zero-length scripts
	for (int i = 0; i < ch.NumberOfScripts - 1; ++i) {
		scriptLengths.push_back(scriptOffsets[i + 1] - scriptOffsets[i]);
	}
	scriptLengths.push_back(ch.TotalScriptLen - scriptOffsets[ch.NumberOfScripts - 1]);


	for (int i = 0; i < ch.NumberOfPieces; ++i) {
		int ofs;

		ofs = *(int *)&cobdata[ch.OffsetToPieceNameOffsetArray + i * 4];
		swabDWordInPlace(ofs);
		string s = StringToLower(&cobdata[ofs]);
		pieceNames.push_back(s);
	}

	int code_octets = size - ch.OffsetToScriptCode;
	int code_ints = (code_octets) / 4 + 4;
	code = new int[code_ints];
	memcpy(code, &cobdata[ch.OffsetToScriptCode], code_octets);
	for (int i = 0; i < code_ints; i++) {
		swabDWordInPlace(code[i]);
	}

	numStaticVars = ch.NumberOfStaticVars;

	// If this is a TA:K script, read the sound names
	if (ch.VersionSignature == 6) {
		sounds.reserve(ch.NumberOfSounds);
		for (int i = 0; i < ch.NumberOfSounds; ++i) {
			int ofs;
			ofs = *(int *)&cobdata[ch.OffsetToSoundNameArray + i * 4];
			/* TODO This probably isn't correct. */
			swabDWordInPlace(ofs);
			string s = &cobdata[ofs];

			if (sound->HasSoundItem(s))
				sounds.push_back(sound->GetSoundId(s));
			else
			{
				// Load the wave file and store the ID for future use
				s = "sounds/" + s + ".wav";
				sounds.push_back(sound->GetSoundId(s));
			}
		}
	}

	delete[] cobdata;

	//Create a reverse mapping (name->int)
	for (unsigned int i = 0; i < scriptNames.size(); ++i) {
		scriptMap[scriptNames[i]] = i;
	}

	//Map common function names to indices
	const spring::unordered_map<string, int>& nameMap = CCobUnitScriptNames::GetScriptMap();
	scriptIndex.resize(COBFN_Last + (MAX_WEAPONS_PER_UNIT * COBFN_Weapon_Funcs), -1);
	for (auto it = nameMap.begin(); it != nameMap.end(); ++it) {
		const int fn = GetFunctionId(it->first);
		if (fn >= 0) {
			scriptIndex[it->second] = fn;
		}
	}
}


CCobFile::~CCobFile()
{
	delete[] code;
}


int CCobFile::GetFunctionId(const std::string& name)
{
	const auto i = scriptMap.find(name);

	if (i != scriptMap.end())
		return i->second;

	return -1;
}

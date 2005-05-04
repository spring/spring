#include "StdAfx.h"
#include ".\cobfile.h"
#include "FileHandler.h"
#include "InfoConsole.h"
#include <algorithm>
#include <locale>
#include <cctype>
//#include "mmgr.h"

//The following structure is taken from http://visualta.tauniverse.com/Downloads/ta-cob-fmt.txt
//Information on missing fields from Format_Cob.pas
typedef struct tagCOBHeader
{
	long VersionSignature;
	long NumberOfScripts;
	long NumberOfPieces;
	long TotalScriptLen;
	long NumberOfStaticVars;
	long Unknown_2; /* Always seems to be 0 */
	long OffsetToScriptCodeIndexArray;
	long OffsetToScriptNameOffsetArray;
	long OffsetToPieceNameOffsetArray;
	long OffsetToScriptCode;
	long Unknown_3; /* Always seems to point to first script name */
} COBHeader;


CCobFile::CCobFile(CFileHandler &in, string name)
{
	char *cobdata = NULL;

	this->name = name;

	//Figure out size needed and allocate it
	long size = in.FileSize();
	cobdata = new char[size];

	//Read the entire thing, we will need it
	in.Read(cobdata, size);

	//Time to parse
	COBHeader ch;
	memcpy(&ch, cobdata, sizeof(ch));

	for (long i = 0; i < ch.NumberOfScripts; ++i) {
		long ofs;
		
		ofs = *(long *)&cobdata[ch.OffsetToScriptNameOffsetArray + i * 4];
		string s = &cobdata[ofs];
		scriptNames.push_back(s);

		ofs = *(long *)&cobdata[ch.OffsetToScriptCodeIndexArray + i * 4];
		scriptOffsets.push_back(ofs);
	}

	//Check for zero-length scripts
	for (long i = 0; i < ch.NumberOfScripts - 1; ++i) {
		scriptLengths.push_back(scriptOffsets[i + 1] - scriptOffsets[i]);
	}
	scriptLengths.push_back(ch.TotalScriptLen - scriptOffsets[ch.NumberOfScripts - 1]);

	for (long i = 0; i < ch.NumberOfPieces; ++i) {
		long ofs;

		ofs = *(long *)&cobdata[ch.OffsetToPieceNameOffsetArray + i * 4];
		string s = &cobdata[ofs];
		std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))std::tolower);
		pieceNames.push_back(s);
	}

	code = new long[(size - ch.OffsetToScriptCode) / 4 + 4];
	memcpy(code, &cobdata[ch.OffsetToScriptCode], size - ch.OffsetToScriptCode);

	numStaticVars = ch.NumberOfStaticVars;

	delete[] cobdata;

	//Create a reverse mapping (name->int)
	for (unsigned int i = 0; i < scriptNames.size(); ++i) {
		scriptMap[scriptNames[i]] = i;
	}

	//Map common function names to indicies
	scriptIndex.resize(COBFN_Last);
	scriptIndex[COBFN_Create] = getFunctionId("Create");
	scriptIndex[COBFN_StartMoving] = getFunctionId("StartMoving");
	scriptIndex[COBFN_StopMoving] = getFunctionId("StopMoving");
	scriptIndex[COBFN_QueryPrimary] = getFunctionId("QueryPrimary");
	scriptIndex[COBFN_QuerySecondary] = getFunctionId("QuerySecondary");
	scriptIndex[COBFN_QueryTertiary] = getFunctionId("QueryTertiary");
	scriptIndex[COBFN_AimPrimary] = getFunctionId("AimPrimary");
	scriptIndex[COBFN_AimSecondary] = getFunctionId("AimSecondary");
	scriptIndex[COBFN_AimTertiary] = getFunctionId("AimTertiary");
	scriptIndex[COBFN_AimFromPrimary] = getFunctionId("AimFromPrimary");
	scriptIndex[COBFN_AimFromSecondary] = getFunctionId("AimFromSecondary");
	scriptIndex[COBFN_AimFromTertiary] = getFunctionId("AimFromTertiary");
	scriptIndex[COBFN_Activate] = getFunctionId("Activate");
	scriptIndex[COBFN_FirePrimary] = getFunctionId("FirePrimary");
	scriptIndex[COBFN_FireSecondary] = getFunctionId("FireSecondary");
	scriptIndex[COBFN_FireTertiary] = getFunctionId("FireTertiary");
	scriptIndex[COBFN_Killed] = getFunctionId("Killed");
	scriptIndex[COBFN_Deactivate] = getFunctionId("Deactivate");
	scriptIndex[COBFN_SetDirection] = getFunctionId("SetDirection");
	scriptIndex[COBFN_SetSpeed] = getFunctionId("SetSpeed");
	scriptIndex[COBFN_RockUnit] = getFunctionId("RockUnit");
	scriptIndex[COBFN_HitByWeapon] = getFunctionId("HitByWeapon");
	scriptIndex[COBFN_MoveRate0] = getFunctionId("MoveRate0");
	scriptIndex[COBFN_MoveRate1] = getFunctionId("MoveRate1");
	scriptIndex[COBFN_MoveRate2] = getFunctionId("MoveRate2");
	scriptIndex[COBFN_MoveRate3] = getFunctionId("MoveRate3");
	scriptIndex[COBFN_SetSFXOccupy] = getFunctionId("setSFXoccupy");
}

CCobFile::~CCobFile(void)
{
	//test
	delete[] code;
}

int CCobFile::getFunctionId(const string &name)
{
	map<string, int>::iterator i;
	if ((i = scriptMap.find(name)) != scriptMap.end()) {
		return i->second;
	}

	return -1;
}
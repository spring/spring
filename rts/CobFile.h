#ifndef __COB_FILE_H__
#define __COB_FILE_H__

#include "archdef.h"

#include <iostream>
#include <vector>
#include <string>
#include <map>

//0 = none
//1 = script calls
//2 = show every instruction
#define COB_DEBUG	0

// Should return true for scripts that should have debug output. 
#define COB_DEBUG_FILTER (script.name == "scripts\\ARMJETH.cob")

using namespace std;

class CFileHandler;

//These are mapped by the CCobFile at startup to make common function calls faster
const COBFN_Create = 0;
const COBFN_StartMoving = 1;
const COBFN_StopMoving = 2;
const COBFN_QueryPrimary = 3;
const COBFN_QuerySecondary = 4;
const COBFN_QueryTertiary = 5;
const COBFN_AimPrimary = 6;
const COBFN_AimSecondary = 7;
const COBFN_AimTertiary = 8;
const COBFN_AimFromPrimary = 9;
const COBFN_AimFromSecondary = 10;
const COBFN_AimFromTertiary = 11;
const COBFN_Activate = 12;
const COBFN_FirePrimary = 13;
const COBFN_FireSecondary = 14;
const COBFN_FireTertiary = 15;
const COBFN_Killed = 16;
const COBFN_Deactivate = 17;
const COBFN_SetDirection = 18;
const COBFN_SetSpeed = 19;
const COBFN_RockUnit = 20;
const COBFN_HitByWeapon = 21;
const COBFN_MoveRate0 = 22;
const COBFN_MoveRate1 = 23;
const COBFN_MoveRate2 = 24;
const COBFN_MoveRate3 = 25;
const COBFN_SetSFXOccupy = 26;
const COBFN_Last = 27;					//Make sure to update this, so the array will be sized properly

class CCobFile
{
public:
	vector<string> scriptNames;
	vector<long> scriptOffsets;
	vector<long> scriptLengths;			//Assumes that the scripts are sorted by offset in the file
	vector<string> pieceNames;
	vector<int> scriptIndex;
	map<string, int> scriptMap;
	long* code;
	int numStaticVars;
	string name;
public:
	CCobFile(CFileHandler &in, string name);
	~CCobFile(void);
	int getFunctionId(const string &name);
};

#endif // __COB_FILE_H__

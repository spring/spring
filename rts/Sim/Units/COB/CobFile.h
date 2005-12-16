#ifndef __COB_FILE_H__
#define __COB_FILE_H__

#include <iostream>
#include <vector>
#include <string>
#include <map>

//0 = none
//1 = script calls
//2 = show every instruction
#define COB_DEBUG	0

// Should return true for scripts that should have debug output. 
#define COB_DEBUG_FILTER (script.name == "scripts/ARMJETH.cob")

using namespace std;

class CFileHandler;

//These are mapped by the CCobFile at startup to make common function calls faster
const int COBFN_Create = 0;
const int COBFN_StartMoving = 1;
const int COBFN_StopMoving = 2;
const int COBFN_Activate = 3;
const int COBFN_Killed = 4;
const int COBFN_Deactivate = 5;
const int COBFN_SetDirection = 6;
const int COBFN_SetSpeed = 7;
const int COBFN_RockUnit = 8;
const int COBFN_HitByWeapon = 9;
const int COBFN_MoveRate0 = 10;
const int COBFN_MoveRate1 = 11;
const int COBFN_MoveRate2 = 12;
const int COBFN_MoveRate3 = 13;
const int COBFN_SetSFXOccupy = 14;
const int COBFN_Last = 15;					//Make sure to update this, so the array will be sized properly

// These are special (they need space for MaxWeapons of each)
const int COB_MaxWeapons = 16;
const int COBFN_QueryPrimary = COBFN_Last;
const int COBFN_AimPrimary = COBFN_QueryPrimary + COB_MaxWeapons;
const int COBFN_AimFromPrimary = COBFN_AimPrimary + COB_MaxWeapons;
const int COBFN_FirePrimary = COBFN_AimFromPrimary + COB_MaxWeapons;
const int COBFN_EndBurst = COBFN_FirePrimary + COB_MaxWeapons;

class CCobFile
{
public:
	vector<string> scriptNames;
	vector<int> scriptOffsets;
	vector<int> scriptLengths;			//Assumes that the scripts are sorted by offset in the file
	vector<string> pieceNames;
	vector<int> scriptIndex;
	vector<int> sounds;
	map<string, int> scriptMap;
	int* code;
	int numStaticVars;
	string name;
public:
	CCobFile(CFileHandler &in, string name);
	~CCobFile(void);
	int getFunctionId(const string &name);
};

#endif // __COB_FILE_H__

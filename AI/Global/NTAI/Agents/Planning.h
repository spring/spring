#ifndef PLANNING_H
#define PLANNING_H
#include <list>
#include "AICallback.h"
#include "UnitDef.h"

#define ENERGY 1
#define MEX 2
#define FACTORY 3
#define MAKER 4
#define MINE 7
#define KAMIKAZE 8
#define CON 9
#define SUB 10
#define ARTILLERY 11
#define MISSILE 12
#define LASER 13
#define STRIKE 14
#define JAMMER 15
#define AIRSUPPORT 16
#define RADAR 17
#define SONAR 18
#define TRANSPORT 19
#define SCOUT 20
#define TELEPORT 21
#define ESTORE 22
#define MSTORE 23
#define STORE 24
#define WALL 25
#define TROOP 26
#define DROID 26
#define CIVILIAN 27
#define CANNON 28
#define CANON 28
#define SILO 29
#define ANTISILO 30
#define MLAYER 31
#define COMMANDER 32
#define AA 33
#define SAM 33
#define ANTIAIR 33
#define GUNSHIP 34
#define BOMBER 35
#define FIGHTER 36
#define TORPEDO 37


class Planning : public base {
public:
	Planning(){}
	virtual ~Planning(){}
	void InitAI(Global* GLI);
	bool feasable(string s, int builder);
	int DrawLine(float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
	int DrawLine(ARGB colours, float3 LocA, float3 LocB, int LifeTime, float Width = 5.0f, bool Arrow = false);
private:
	Global* G;
};
#endif

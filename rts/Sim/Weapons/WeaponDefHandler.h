#ifndef WEAPONDEFHANDLER_H
#define WEAPONDEFHANDLER_H

#include <string>
#include <map>
#include "Sim/Misc/DamageArray.h"
#include "Sim/Units/UnitDef.h"

#define WEAPONTYPE_ROCKET 1
#define WEAPONTYPE_CANNON 2
#define WEAPONTYPE_AACANNON 3
#define WEAPONTYPE_RIFLE 4
#define WEAPONTYPE_MELEE 5
#define WEAPONTYPE_AIRCRAFTBOMB 6
#define WEAPONTYPE_FLAME 7
#define WEAPONTYPE_MISSILELAUNCHER 8
#define WEAPONTYPE_LASERCANNON 9
#define WEAPONTYPE_EMGCANNON 10
#define WEAPONTYPE_STARBURSTLAUNCHER 11
#define WEAPONTYPE_UNKNOWN 12

#define WEAPON_RENDERTYPE_MODEL 1
#define WEAPON_RENDERTYPE_LASER 2
#define WEAPON_RENDERTYPE_PLASMA 3
#define WEAPON_RENDERTYPE_FIREBALL 4

class TdfParser;

struct WeaponDef
{
	std::string name;
	std::string type;

	/*std::string sfiresound;
	std::string ssoundhit;
	int firesoundId;
	int soundhitId;
	float firesoundVolume;
	float soundhitVolume;*/
	GuiSound firesound;
	GuiSound soundhit;

	float range;
	float heightmod;
	float accuracy;				//inaccuracy of whole burst
	float sprayangle;			//inaccuracy of individual shots inside burst
	float movingAccuracy; //inaccuracy while owner moving
	float targetMoveError;	//fraction of targets move speed that is used as error offset
	//float damage;
	//float airDamage;
	DamageArray damages;
	float areaOfEffect;
	bool noSelfDamage;
	float fireStarter;

	int salvosize;
	float salvodelay;
	float reload;
	float beamtime;

	float maxAngle;
	float restTime;

	float uptime;

	float metalcost;
	float energycost;
	float supplycost;

	int id;

	bool turret;
	bool onlyForward;
	bool waterweapon;
	bool tracks;
	bool dropped;
	bool paralyzer;			//weapon will only paralyze not do real damage

	bool noAutoTarget;			//cant target stuff (for antinuke,dgun)
	bool manualfire;			//use dgun button
	int interceptor;				//anti nuke
	int targetable;				//nuke (can be shot by interceptor)
	bool stockpile;					
	float coverageRange;		//range of anti nuke

	bool isPlasmaRepulser;
	float repulseEnergy;
	float repulseRange;
	float repulseForce;
	float repulseSpeed;

	float intensity;
	float thickness;

	int graphicsType;
	bool soundTrigger;

	bool selfExplode;
	bool gravityAffected;
	bool twophase;
	bool guided;
	bool vlaunch;
	bool selfprop;
	bool noExplode;
	float startvelocity;
	float weaponacceleration;
	float turnrate;
	float maxvelocity;

	float projectilespeed;

	unsigned int onlyTargetCategory;

	float wobble;						//how much the missile will wobble around its course
	float trajectoryHeight;			//how high trajectory missiles will try to fly in

	struct
	{
		float3 color;

		int renderType;
		//bool hasmodel;
		std::string modelName;

		bool smokeTrail;
		bool beamweapon;
	}visuals;
};

class CWeaponDefHandler
{
public:
	WeaponDef *weaponDefs;
	std::map<std::string, int> weaponID;
	int numWeapons;

	CWeaponDefHandler(void);
	~CWeaponDefHandler(void);

	WeaponDef *GetWeapon(const std::string weaponname);

	static void LoadSound(GuiSound &gsound);

protected:
	void ParseTAWeapon(TdfParser *sunparser, std::string weaponname, int id);
	float3 hs2rgb(float h, float s);
};

extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */

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
class CExplosionGenerator;
struct AtlasedTexture;

struct WeaponDef
{
	CR_DECLARE(WeaponDef);
	virtual ~WeaponDef();

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
	float edgeEffectivness;
	float size;
	float sizeGrowth;
	float collisionSize;

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

	float intensity;
	float thickness;
	float laserflaresize;
	float corethickness;
	float duration;

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
	float explosionSpeed;

	unsigned int onlyTargetCategory;

	float wobble;						//how much the missile will wobble around its course
	float trajectoryHeight;			//how high trajectory missiles will try to fly in

	struct
	{
		float3 color;
		float3 color2;

		int renderType;
		//bool hasmodel;
		std::string modelName;

		bool smokeTrail;
		bool beamweapon;

		AtlasedTexture *texture1;
		AtlasedTexture *texture2;
		AtlasedTexture *texture3;
		AtlasedTexture *texture4;
		float tilelength;
		float scrollspeed;
		float pulseSpeed;
	}visuals;

	bool largeBeamLaser;

	bool isShield;					//if the weapon is a shield rather than a weapon
	bool shieldRepulser;		//if the weapon should be repulsed or absorbed
	bool smartShield;				//only affect enemy projectiles
	bool exteriorShield;		//only affect stuff coming from outside shield radius
	bool visibleShield;			//if the shield should be graphically shown
	bool visibleShieldRepulse;	//if a small graphic should be shown at each repulse
	float shieldEnergyUse;	//energy use per shot or per second depending on projectile
	float shieldRadius;			//size of shielded area
	float shieldForce;			//shield acceleration on plasma stuff
	float shieldMaxSpeed;		//max speed shield can repulse plasma like weapons with
	float shieldPower;			//how much damage the shield can reflect (0=infinite)
	float shieldPowerRegen;	//how fast the power regenerates per second
	float shieldPowerRegenEnergy;	//how much energy is needed to regenerate power per second
	float3 shieldGoodColor;			//color when shield at full power
	float3 shieldBadColor;			//color when shield is empty
	float shieldAlpha;					//shield alpha value

	unsigned int shieldInterceptType;				//type of shield (bitfield)
	unsigned int interceptedByShieldType;		//weapon can be affected by shields where (shieldInterceptType & interceptedByShieldType) is not zero

	bool avoidFriendly;		//if true tried to avoid firendly units when fireing
	unsigned int collisionFlags;

	CExplosionGenerator *explosionGenerator; // can be zero for default explosions
};

class CExplosionGeneratorHandler;

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

	CExplosionGeneratorHandler* explGen;
};

extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */

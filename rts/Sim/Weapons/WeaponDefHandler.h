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

class LuaTable;
class CColorMap;
class CExplosionGenerator;
struct AtlasedTexture;

struct WeaponDef
{
	CR_DECLARE(WeaponDef);

	~WeaponDef();

	std::string name;
	std::string type;
	std::string description;
	std::string filename;

	GuiSoundSet firesound;
	GuiSoundSet soundhit;

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
	float edgeEffectiveness;
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
	int flighttime;

	float metalcost;
	float energycost;
	float supplycost;

	int projectilespershot;

	int id;
	int tdfId;	//the id= tag in the tdf

	bool turret;
	bool onlyForward;
	bool fixedLauncher;
	bool waterweapon;
	bool fireSubmersed;
	bool submissile; //Lets a torpedo travel above water like it does below water
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
	int   lodDistance;
	float falloffRate;

	int graphicsType;
	bool soundTrigger;

	bool selfExplode;
	bool gravityAffected;
	float myGravity;
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

	struct Visuals
	{
		float3 color;
		float3 color2;

		int renderType;
		//bool hasmodel;
		std::string modelName;
		CColorMap *colorMap;

		bool smokeTrail;
		bool beamweapon;

		AtlasedTexture *texture1;
		AtlasedTexture *texture2;
		AtlasedTexture *texture3;
		AtlasedTexture *texture4;
		float tilelength;
		float scrollspeed;
		float pulseSpeed;
		int beamttl;
		float beamdecay;

		int stages;
		float alphaDecay;
		float sizeDecay;
		float separation;
		bool noGap;
	};
	Visuals visuals;

	bool largeBeamLaser;

	bool isShield;					//if the weapon is a shield rather than a weapon
	bool shieldRepulser;		//if the weapon should be repulsed or absorbed
	bool smartShield;				//only affect enemy projectiles
	bool exteriorShield;		//only affect stuff coming from outside shield radius
	bool visibleShield;			//if the shield should be graphically shown
	bool visibleShieldRepulse;	//if a small graphic should be shown at each repulse
	int  visibleShieldHitFrames; //number of frames to draw the shield after it has been hit
	float shieldEnergyUse;	//energy use per shot or per second depending on projectile
	float shieldRadius;			//size of shielded area
	float shieldForce;			//shield acceleration on plasma stuff
	float shieldMaxSpeed;		//max speed shield can repulse plasma like weapons with
	float shieldPower;			//how much damage the shield can reflect (0=infinite)
	float shieldPowerRegen;	//how fast the power regenerates per second
	float shieldPowerRegenEnergy;	//how much energy is needed to regenerate power per second
	float shieldStartingPower;	//how much power the shield has when first created
	float3 shieldGoodColor;			//color when shield at full power
	float3 shieldBadColor;			//color when shield is empty
	float shieldAlpha;					//shield alpha value

	unsigned int shieldInterceptType;				//type of shield (bitfield)
	unsigned int interceptedByShieldType;		//weapon can be affected by shields where (shieldInterceptType & interceptedByShieldType) is not zero

	bool avoidFriendly;		//if true try to avoid friendly Units when aiming.
	bool avoidFeature;      //if true try to avoid Features while aiming.

	float targetBorder;		//if nonzero, targetting units will TryTarget at the edge of collision sphere (radius*tag value, [-1;1]) instead of its centre
	float cylinderTargetting;	//if greater than 0, range will be checked in a cylinder (height=unitradius*cylinderTargetting) instead of a sphere
	float minIntensity;		// for beamlasers - always hit with some minimum intensity (a damage coeffcient normally dependent on distance). do not confuse with intensity tag, it's completely unrelated.
	float heightBoostFactor;	//controls cannon range height boost. default: -1 -- automatically calculate a more or less sane value

	unsigned int collisionFlags;

	CExplosionGenerator* explosionGenerator; // can be zero for default explosions

	bool sweepFire;

	bool canAttackGround;

	float cameraShake;

	float dynDamageExp;
	float dynDamageMin;
	float dynDamageRange;
	bool dynDamageInverted;
};


class CExplosionGeneratorHandler;


class CWeaponDefHandler {
	public:
		CWeaponDefHandler();
		~CWeaponDefHandler();

		const WeaponDef* GetWeapon(const std::string weaponname);

		void LoadSound(const LuaTable&, GuiSoundSet&, const std::string& name);

		DamageArray DynamicDamages(DamageArray damages, float3 startPos,
					   float3 curPos, float range, float exp,
					   float damageMin, bool inverted);

	public:
		WeaponDef *weaponDefs;
		std::map<std::string, int> weaponID;
		int numWeaponDefs;

	private:
		void ParseTAWeapon(const LuaTable& wdTable, WeaponDef& wd);
		float3 hs2rgb(float h, float s);
};


extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */

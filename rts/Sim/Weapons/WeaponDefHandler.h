#ifndef WEAPONDEFHANDLER_H
#define WEAPONDEFHANDLER_H

#include <string>
#include <map>
#include "Sim/Misc/DamageArray.h"
#include "Sim/Units/UnitDef.h"

class LuaTable;
class CColorMap;
class CExplosionGenerator;
struct AtlasedTexture;

struct WeaponDef
{
	CR_DECLARE_STRUCT(WeaponDef);

        WeaponDef():explosionGenerator(0) {}
        WeaponDef(DamageArray damages) : damages(damages), explosionGenerator(0) {}

	~WeaponDef();

	S3DModel* LoadModel();

	std::string name;
	std::string type;
	std::string description;
	std::string filename;
	std::string cegTag;        // tag of CEG that projectiles fired by this weapon should use

	GuiSoundSet firesound;
	GuiSoundSet soundhit;

	float range;
	float heightmod;
	float accuracy;            // inaccuracy of whole burst
	float sprayAngle;          // inaccuracy of individual shots inside burst
	float movingAccuracy;      // inaccuracy while owner moving
	float targetMoveError;     // fraction of targets move speed that is used as error offset
	float leadLimit;           // maximum distance the weapon will lead the target
	float leadBonus;           // factor for increasing the leadLimit with experience
	float predictBoost;        // replaces hardcoded behaviour for burnblow cannons

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
	bool beamburst;

	bool waterBounce;
	bool groundBounce;
	float bounceRebound;
	float bounceSlip;
	int numBounce;

	float maxAngle;
	float restTime;

	float uptime;
	int flighttime;

	float metalcost;
	float energycost;
	float supplycost;

	int projectilespershot;

	int id;
	int tdfId;									// the id= tag in the tdf

	bool turret;
	bool onlyForward;
	bool fixedLauncher;
	bool waterweapon;
	bool fireSubmersed;
	bool submissile;            // Lets a torpedo travel above water like it does below water
	bool tracks;
	bool dropped;
	bool paralyzer;             // weapon will only paralyze not do real damage
	bool impactOnly;            // The weapon damages by impacting, not by exploding

	bool noAutoTarget;          // cant target stuff (for antinuke,dgun)
	bool manualfire;            // use dgun button
	int interceptor;            // anti nuke
	int targetable;             // nuke (can be shot by interceptor)
	bool stockpile;
	float coverageRange;        // range of anti nuke

	float stockpileTime;        // builtime of a missile

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
	int highTrajectory;         //Per-weapon high traj setting, 0=low, 1=high, 2=unit
	float myGravity;
	bool noExplode;
	float startvelocity;
	float weaponacceleration;
	float turnrate;
	float maxvelocity;

	float projectilespeed;
	float explosionSpeed;

	unsigned int onlyTargetCategory;

	float wobble;             // how much the missile will wobble around its course
	float dance;              // how much the missile will dance
	float trajectoryHeight;   // how high trajectory missiles will try to fly in

	struct Visuals
	{
		Visuals() : model(NULL) {};
		float3 color;
		float3 color2;

		//bool hasmodel;
		S3DModel* model;
		std::string modelName;
		CColorMap* colorMap;

		bool smokeTrail;
		bool beamweapon;
		bool hardStop;   // whether the shot should fade out or stop and contract at max range

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

		bool alwaysVisible;
	};
	Visuals visuals;

	bool largeBeamLaser;

	bool isShield;                   // if the weapon is a shield rather than a weapon
	bool shieldRepulser;             // if the weapon should be repulsed or absorbed
	bool smartShield;                // only affect enemy projectiles
	bool exteriorShield;             // only affect stuff coming from outside shield radius
	bool visibleShield;              // if the shield should be graphically shown
	bool visibleShieldRepulse;       // if a small graphic should be shown at each repulse
	int  visibleShieldHitFrames;     // number of frames to draw the shield after it has been hit
	float shieldEnergyUse;           // energy use per shot or per second depending on projectile
	float shieldRadius;              // size of shielded area
	float shieldForce;               // shield acceleration on plasma stuff
	float shieldMaxSpeed;            // max speed shield can repulse plasma like weapons with
	float shieldPower;               // how much damage the shield can reflect (0=infinite)
	float shieldPowerRegen;          // how fast the power regenerates per second
	float shieldPowerRegenEnergy;    // how much energy is needed to regenerate power per second
	float shieldStartingPower;       // how much power the shield has when first created
	int   shieldRechargeDelay;       // number of frames to delay recharging by after each hit
	float3 shieldGoodColor;          // color when shield at full power
	float3 shieldBadColor;           // color when shield is empty
	float shieldAlpha;               // shield alpha value

	unsigned int shieldInterceptType;      // type of shield (bitfield)
	unsigned int interceptedByShieldType;  // weapon can be affected by shields where (shieldInterceptType & interceptedByShieldType) is not zero

	bool avoidFriendly;     // if true, try to avoid friendly units while aiming
	bool avoidFeature;      // if true, try to avoid features while aiming
	bool avoidNeutral;      // if true, try to avoid neutral units while aiming
	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
	float targetBorder;
	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargetting) instead of a sphere.
	 */
	float cylinderTargetting;
	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coeffcient normally dependent on distance).
	 * Do not confuse this with the intensity tag, it i completely unrelated.
	 */
	float minIntensity;
	/**
	 * Controls cannon range height boost.
	 *
	 * default: -1: automatically calculate a more or less sane value
	 */
	float heightBoostFactor;
	float proximityPriority;     // multiplier for the distance to the target for priority calculations

	unsigned int collisionFlags;

	CExplosionGenerator* explosionGenerator;        // can be zero for default explosions
	CExplosionGenerator* bounceExplosionGenerator;  // called when a projectile bounces

	bool sweepFire;
	bool canAttackGround;

	float cameraShake;

	float dynDamageExp;
	float dynDamageMin;
	float dynDamageRange;
	bool dynDamageInverted;

	std::map<std::string, std::string> customParams;
};


class CExplosionGeneratorHandler;


class CWeaponDefHandler {
	public:
		CWeaponDefHandler();
		~CWeaponDefHandler();

		const WeaponDef* GetWeapon(const std::string weaponname);
		const WeaponDef* GetWeaponById(int weaponDefId);

		void LoadSound(const LuaTable&, GuiSoundSet&, const std::string& name);

		DamageArray DynamicDamages(DamageArray damages, float3 startPos,
					   float3 curPos, float range, float exp,
					   float damageMin, bool inverted);

	public:
		WeaponDef *weaponDefs;
		std::map<std::string, int> weaponID;
		int numWeaponDefs;

	private:
		void ParseWeapon(const LuaTable& wdTable, WeaponDef& wd);
		float3 hs2rgb(float h, float s);
};


//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
inline S3DModel* LoadModel(const WeaponDef* wdef)
{
	return const_cast<WeaponDef*>(wdef)->LoadModel();
}


extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */

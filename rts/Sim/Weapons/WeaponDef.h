/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WEAPON_DEF_H
#define _WEAPON_DEF_H

#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileTypes.h"
#include "System/float4.h"
#include "System/UnorderedMap.hpp"

struct AtlasedTexture;
class CColorMap;
struct S3DModel;
class LuaTable;

struct WeaponDef
{
public:
	WeaponDef();
	WeaponDef(const LuaTable& wdTable, const std::string& name, int id);

	S3DModel* LoadModel();
	S3DModel* LoadModel() const;

	bool IsAircraftWeapon() const {
		switch (projectileType) {
			case WEAPON_TORPEDO_PROJECTILE:   { return (                 true); } break;
			case WEAPON_EXPLOSIVE_PROJECTILE: { return (defInterceptType == 8); } break;
			default: {} break;
		}
		return false;
	}

	bool IsHitScanWeapon() const {
		switch (projectileType) {
			case WEAPON_BEAMLASER_PROJECTILE:      { return true; } break;
			case WEAPON_LARGEBEAMLASER_PROJECTILE: { return true; } break;
			case WEAPON_LIGHTNING_PROJECTILE:      { return true; } break;
			default: {} break;
		}

		return false;
	}

	bool IsVisibleShield() const { return (visibleShield || visibleShieldHitFrames > 0); }

public:
	std::string name;
	std::string type;
	std::string description;

	unsigned int ptrailExplosionGeneratorID; // must be custom, defined by ptrailExpGenTag
	unsigned int impactExplosionGeneratorID; // can be NULL for default explosions
	unsigned int bounceExplosionGeneratorID; // called when a projectile bounces

	GuiSoundSet fireSound;
	GuiSoundSet hitSound;

	float range;
	float heightmod;
	float accuracy;            ///< INaccuracy (!) of whole burst
	float sprayAngle;          ///< INaccuracy of individual shots inside burst
	float movingAccuracy;      ///< INaccuracy (!) while owner moving
	float ownerExpAccWeight;   ///< if 0, accuracy is not increased with owner experience (max. 1)
	float targetMoveError;     ///< fraction of targets move speed that is used as error offset
	float leadLimit;           ///< maximum distance the weapon will lead the target
	float leadBonus;           ///< factor for increasing the leadLimit with experience
	float predictBoost;        ///< replaces hardcoded behaviour for burnblow cannons

	DynDamageArray damages;

	float fireStarter;
	bool noSelfDamage;
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
	float maxFireAngle;

	float uptime;
	int flighttime;

	float metalcost;
	float energycost;

	int projectilespershot;

	int id;
	int tdfId;                  ///< the id= tag in the tdf

	bool isNulled;
	bool turret;
	bool onlyForward;
	bool allowNonBlockingAim;
	bool fixedLauncher;
	bool waterweapon;           ///< can target underwater objects/positions if true
	bool fireSubmersed;         ///< can fire even when underwater if true
	bool submissile;            ///< Lets a torpedo travel above water like it does below water
	bool tracks;
	bool paralyzer;             ///< weapon will only paralyze not do real damage
	bool impactOnly;            ///< The weapon damages by impacting, not by exploding

	bool noAutoTarget;          ///< cant target stuff (for antinuke,dgun)
	bool manualfire;            ///< if true, slave us to the ManualFire button

	bool sweepFire;
	bool canAttackGround;

	bool interceptSolo;
	int interceptor;            ///< if >= 1, weapon will fire at any interceptable projectiles
	int targetable;             ///< nuke (can be shot by interceptor)
	bool stockpile;
	float coverageRange;        ///< range of anti nuke

	float stockpileTime;        ///< builtime of a missile

	///< determines alpha-fading for BeamLasers (UNSYNCED);
	///< combines with falloffRate for Lasers to determine
	///< when projectile should be deleted (SYNCED) instead
	///< of TTL
	float intensity;
	float falloffRate;
	float duration;
	int beamLaserTTL;

	bool soundTrigger;

	bool selfExplode;
	bool gravityAffected;
	int highTrajectory;              ///< Per-weapon high traj setting, 0=low, 1=high, 2=unit
	float myGravity;
	bool noExplode;
	float startvelocity;
	float weaponacceleration;
	float turnrate;

	float projectilespeed;

	float wobble;                    ///< how much the missile will wobble around its course
	float dance;                     ///< how much the missile will dance
	float trajectoryHeight;          ///< how high trajectory missiles will try to fly in

	bool largeBeamLaser;             // whether a BeamLaser should spawn LargeBeamLaserProjectile's or regular ones
	bool laserHardStop;              // whether the shot should fade out or stop and contract at max-range (applies to LaserCannons only)

	bool isShield;                   // if the weapon is a shield rather than a weapon //FIXME REMOVE! (this information is/should be saved in the weapontype)
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
	float4 shieldGoodColor;          // color when shield at full power
	float4 shieldBadColor;           // color when shield is empty
	float shieldAlpha;               // shield alpha value
	int shieldArmorType;             // armor type for the damage table
	std::string shieldArmorTypeName; // name of the armor type

	unsigned int shieldInterceptType;      // type of shield (bitfield)
	unsigned int interceptedByShieldType;  // weapon can be affected by shields where (shieldInterceptType & interceptedByShieldType) is not zero
	unsigned int defInterceptType;

	bool avoidFriendly;     // if true, try to avoid friendly units while aiming
	bool avoidFeature;      // if true, try to avoid features while aiming
	bool avoidNeutral;      // if true, try to avoid neutral units while aiming
	bool avoidGround;       // if true, try to avoid ground while aiming
	bool avoidCloaked;      // if true, try to avoid cloaked units while aiming

	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
	float targetBorder;
	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargeting) instead of a sphere.
	 */
	float cylinderTargeting;
	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coefficient normally dependent on distance).
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

	unsigned int projectileType;
	unsigned int collisionFlags;

	float cameraShake;

	spring::unordered_map<std::string, std::string> customParams;

	struct Visuals {
		float3 color;
		float3 color2;

		S3DModel* model = nullptr;
		CColorMap* colorMap = nullptr;

		AtlasedTexture* texture1 = nullptr;
		AtlasedTexture* texture2 = nullptr;
		AtlasedTexture* texture3 = nullptr;
		AtlasedTexture* texture4 = nullptr;

		std::string modelName;
		std::string texNames[4];
		std::string colorMapStr;
		std::string ptrailExpGenTag; ///< tag of CEG that projectiles fired by this weapon should use during flight
		std::string impactExpGenTag; ///< tag of CEG that projectiles fired by this weapon should use on impact
		std::string bounceExpGenTag; ///< tag of CEG that projectiles fired by this weapon should use when bouncing

		int lodDistance = 0;
		int stages = 0;

		float tilelength = 0.0f;
		float scrollspeed = 0.0f;
		float pulseSpeed = 0.0f;
		float laserflaresize = 0.0f;
		float thickness = 0.0f;
		float corethickness = 0.0f;
		float beamdecay = 0.0f;
		float alphaDecay = 0.0f;
		float sizeDecay = 0.0f;
		float separation = 0.0f;

		/// TODO: make the scar-type configurable
		bool explosionScar = true;
		bool smokeTrail = false;

		bool noGap = true;
		bool alwaysVisible = true;
	};
	Visuals visuals;

private:
	void ParseWeaponSounds(const LuaTable& wdTable);
	void LoadSound(const LuaTable& wdTable, const std::string& soundKey, GuiSoundSet& soundSet);
};

#endif // _WEAPON_DEF_H

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubUnitDef.h"

#include "IncludesSources.h"

	springai::StubUnitDef::~StubUnitDef() {}
	void springai::StubUnitDef::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubUnitDef::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubUnitDef::SetUnitDefId(int unitDefId) {
		this->unitDefId = unitDefId;
	}
	int springai::StubUnitDef::GetUnitDefId() const {
		return unitDefId;
	}

	void springai::StubUnitDef::SetHeight(float height) {
		this->height = height;
	}
	float springai::StubUnitDef::GetHeight() {
		return height;
	}

	void springai::StubUnitDef::SetRadius(float radius) {
		this->radius = radius;
	}
	float springai::StubUnitDef::GetRadius() {
		return radius;
	}

	void springai::StubUnitDef::SetName(const char* name) {
		this->name = name;
	}
	const char* springai::StubUnitDef::GetName() {
		return name;
	}

	void springai::StubUnitDef::SetHumanName(const char* humanName) {
		this->humanName = humanName;
	}
	const char* springai::StubUnitDef::GetHumanName() {
		return humanName;
	}

	void springai::StubUnitDef::SetFileName(const char* fileName) {
		this->fileName = fileName;
	}
	const char* springai::StubUnitDef::GetFileName() {
		return fileName;
	}

	void springai::StubUnitDef::SetAiHint(int aiHint) {
		this->aiHint = aiHint;
	}
	int springai::StubUnitDef::GetAiHint() {
		return aiHint;
	}

	void springai::StubUnitDef::SetCobId(int cobId) {
		this->cobId = cobId;
	}
	int springai::StubUnitDef::GetCobId() {
		return cobId;
	}

	void springai::StubUnitDef::SetTechLevel(int techLevel) {
		this->techLevel = techLevel;
	}
	int springai::StubUnitDef::GetTechLevel() {
		return techLevel;
	}

	void springai::StubUnitDef::SetGaia(const char* gaia) {
		this->gaia = gaia;
	}
	const char* springai::StubUnitDef::GetGaia() {
		return gaia;
	}

	float springai::StubUnitDef::GetUpkeep(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetResourceMake(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetMakesResource(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetCost(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetExtractsResource(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetResourceExtractorRange(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetWindResourceGenerator(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetTidalResourceGenerator(Resource* resource) {
		return 0;
	}

	float springai::StubUnitDef::GetStorage(Resource* resource) {
		return 0;
	}

	bool springai::StubUnitDef::IsSquareResourceExtractor(Resource* resource) {
		return false;
	}

	void springai::StubUnitDef::SetBuildTime(float buildTime) {
		this->buildTime = buildTime;
	}
	float springai::StubUnitDef::GetBuildTime() {
		return buildTime;
	}

	void springai::StubUnitDef::SetAutoHeal(float autoHeal) {
		this->autoHeal = autoHeal;
	}
	float springai::StubUnitDef::GetAutoHeal() {
		return autoHeal;
	}

	void springai::StubUnitDef::SetIdleAutoHeal(float idleAutoHeal) {
		this->idleAutoHeal = idleAutoHeal;
	}
	float springai::StubUnitDef::GetIdleAutoHeal() {
		return idleAutoHeal;
	}

	void springai::StubUnitDef::SetIdleTime(int idleTime) {
		this->idleTime = idleTime;
	}
	int springai::StubUnitDef::GetIdleTime() {
		return idleTime;
	}

	void springai::StubUnitDef::SetPower(float power) {
		this->power = power;
	}
	float springai::StubUnitDef::GetPower() {
		return power;
	}

	void springai::StubUnitDef::SetHealth(float health) {
		this->health = health;
	}
	float springai::StubUnitDef::GetHealth() {
		return health;
	}

	void springai::StubUnitDef::SetCategory(int category) {
		this->category = category;
	}
	int springai::StubUnitDef::GetCategory() {
		return category;
	}

	void springai::StubUnitDef::SetSpeed(float speed) {
		this->speed = speed;
	}
	float springai::StubUnitDef::GetSpeed() {
		return speed;
	}

	void springai::StubUnitDef::SetTurnRate(float turnRate) {
		this->turnRate = turnRate;
	}
	float springai::StubUnitDef::GetTurnRate() {
		return turnRate;
	}

	void springai::StubUnitDef::SetTurnInPlace(bool isTurnInPlace) {
		this->isTurnInPlace = isTurnInPlace;
	}
	bool springai::StubUnitDef::IsTurnInPlace() {
		return isTurnInPlace;
	}

	void springai::StubUnitDef::SetTurnInPlaceDistance(float turnInPlaceDistance) {
		this->turnInPlaceDistance = turnInPlaceDistance;
	}
	float springai::StubUnitDef::GetTurnInPlaceDistance() {
		return turnInPlaceDistance;
	}

	void springai::StubUnitDef::SetTurnInPlaceSpeedLimit(float turnInPlaceSpeedLimit) {
		this->turnInPlaceSpeedLimit = turnInPlaceSpeedLimit;
	}
	float springai::StubUnitDef::GetTurnInPlaceSpeedLimit() {
		return turnInPlaceSpeedLimit;
	}

	void springai::StubUnitDef::SetUpright(bool isUpright) {
		this->isUpright = isUpright;
	}
	bool springai::StubUnitDef::IsUpright() {
		return isUpright;
	}

	void springai::StubUnitDef::SetCollide(bool isCollide) {
		this->isCollide = isCollide;
	}
	bool springai::StubUnitDef::IsCollide() {
		return isCollide;
	}

	void springai::StubUnitDef::SetLosRadius(float losRadius) {
		this->losRadius = losRadius;
	}
	float springai::StubUnitDef::GetLosRadius() {
		return losRadius;
	}

	void springai::StubUnitDef::SetAirLosRadius(float airLosRadius) {
		this->airLosRadius = airLosRadius;
	}
	float springai::StubUnitDef::GetAirLosRadius() {
		return airLosRadius;
	}

	void springai::StubUnitDef::SetLosHeight(float losHeight) {
		this->losHeight = losHeight;
	}
	float springai::StubUnitDef::GetLosHeight() {
		return losHeight;
	}

	void springai::StubUnitDef::SetRadarRadius(int radarRadius) {
		this->radarRadius = radarRadius;
	}
	int springai::StubUnitDef::GetRadarRadius() {
		return radarRadius;
	}

	void springai::StubUnitDef::SetSonarRadius(int sonarRadius) {
		this->sonarRadius = sonarRadius;
	}
	int springai::StubUnitDef::GetSonarRadius() {
		return sonarRadius;
	}

	void springai::StubUnitDef::SetJammerRadius(int jammerRadius) {
		this->jammerRadius = jammerRadius;
	}
	int springai::StubUnitDef::GetJammerRadius() {
		return jammerRadius;
	}

	void springai::StubUnitDef::SetSonarJamRadius(int sonarJamRadius) {
		this->sonarJamRadius = sonarJamRadius;
	}
	int springai::StubUnitDef::GetSonarJamRadius() {
		return sonarJamRadius;
	}

	void springai::StubUnitDef::SetSeismicRadius(int seismicRadius) {
		this->seismicRadius = seismicRadius;
	}
	int springai::StubUnitDef::GetSeismicRadius() {
		return seismicRadius;
	}

	void springai::StubUnitDef::SetSeismicSignature(float seismicSignature) {
		this->seismicSignature = seismicSignature;
	}
	float springai::StubUnitDef::GetSeismicSignature() {
		return seismicSignature;
	}

	void springai::StubUnitDef::SetStealth(bool isStealth) {
		this->isStealth = isStealth;
	}
	bool springai::StubUnitDef::IsStealth() {
		return isStealth;
	}

	void springai::StubUnitDef::SetSonarStealth(bool isSonarStealth) {
		this->isSonarStealth = isSonarStealth;
	}
	bool springai::StubUnitDef::IsSonarStealth() {
		return isSonarStealth;
	}

	void springai::StubUnitDef::SetBuildRange3D(bool isBuildRange3D) {
		this->isBuildRange3D = isBuildRange3D;
	}
	bool springai::StubUnitDef::IsBuildRange3D() {
		return isBuildRange3D;
	}

	void springai::StubUnitDef::SetBuildDistance(float buildDistance) {
		this->buildDistance = buildDistance;
	}
	float springai::StubUnitDef::GetBuildDistance() {
		return buildDistance;
	}

	void springai::StubUnitDef::SetBuildSpeed(float buildSpeed) {
		this->buildSpeed = buildSpeed;
	}
	float springai::StubUnitDef::GetBuildSpeed() {
		return buildSpeed;
	}

	void springai::StubUnitDef::SetReclaimSpeed(float reclaimSpeed) {
		this->reclaimSpeed = reclaimSpeed;
	}
	float springai::StubUnitDef::GetReclaimSpeed() {
		return reclaimSpeed;
	}

	void springai::StubUnitDef::SetRepairSpeed(float repairSpeed) {
		this->repairSpeed = repairSpeed;
	}
	float springai::StubUnitDef::GetRepairSpeed() {
		return repairSpeed;
	}

	void springai::StubUnitDef::SetMaxRepairSpeed(float maxRepairSpeed) {
		this->maxRepairSpeed = maxRepairSpeed;
	}
	float springai::StubUnitDef::GetMaxRepairSpeed() {
		return maxRepairSpeed;
	}

	void springai::StubUnitDef::SetResurrectSpeed(float resurrectSpeed) {
		this->resurrectSpeed = resurrectSpeed;
	}
	float springai::StubUnitDef::GetResurrectSpeed() {
		return resurrectSpeed;
	}

	void springai::StubUnitDef::SetCaptureSpeed(float captureSpeed) {
		this->captureSpeed = captureSpeed;
	}
	float springai::StubUnitDef::GetCaptureSpeed() {
		return captureSpeed;
	}

	void springai::StubUnitDef::SetTerraformSpeed(float terraformSpeed) {
		this->terraformSpeed = terraformSpeed;
	}
	float springai::StubUnitDef::GetTerraformSpeed() {
		return terraformSpeed;
	}

	void springai::StubUnitDef::SetMass(float mass) {
		this->mass = mass;
	}
	float springai::StubUnitDef::GetMass() {
		return mass;
	}

	void springai::StubUnitDef::SetPushResistant(bool isPushResistant) {
		this->isPushResistant = isPushResistant;
	}
	bool springai::StubUnitDef::IsPushResistant() {
		return isPushResistant;
	}

	void springai::StubUnitDef::SetStrafeToAttack(bool isStrafeToAttack) {
		this->isStrafeToAttack = isStrafeToAttack;
	}
	bool springai::StubUnitDef::IsStrafeToAttack() {
		return isStrafeToAttack;
	}

	void springai::StubUnitDef::SetMinCollisionSpeed(float minCollisionSpeed) {
		this->minCollisionSpeed = minCollisionSpeed;
	}
	float springai::StubUnitDef::GetMinCollisionSpeed() {
		return minCollisionSpeed;
	}

	void springai::StubUnitDef::SetSlideTolerance(float slideTolerance) {
		this->slideTolerance = slideTolerance;
	}
	float springai::StubUnitDef::GetSlideTolerance() {
		return slideTolerance;
	}

	void springai::StubUnitDef::SetMaxSlope(float maxSlope) {
		this->maxSlope = maxSlope;
	}
	float springai::StubUnitDef::GetMaxSlope() {
		return maxSlope;
	}

	void springai::StubUnitDef::SetMaxHeightDif(float maxHeightDif) {
		this->maxHeightDif = maxHeightDif;
	}
	float springai::StubUnitDef::GetMaxHeightDif() {
		return maxHeightDif;
	}

	void springai::StubUnitDef::SetMinWaterDepth(float minWaterDepth) {
		this->minWaterDepth = minWaterDepth;
	}
	float springai::StubUnitDef::GetMinWaterDepth() {
		return minWaterDepth;
	}

	void springai::StubUnitDef::SetWaterline(float waterline) {
		this->waterline = waterline;
	}
	float springai::StubUnitDef::GetWaterline() {
		return waterline;
	}

	void springai::StubUnitDef::SetMaxWaterDepth(float maxWaterDepth) {
		this->maxWaterDepth = maxWaterDepth;
	}
	float springai::StubUnitDef::GetMaxWaterDepth() {
		return maxWaterDepth;
	}

	void springai::StubUnitDef::SetArmoredMultiple(float armoredMultiple) {
		this->armoredMultiple = armoredMultiple;
	}
	float springai::StubUnitDef::GetArmoredMultiple() {
		return armoredMultiple;
	}

	void springai::StubUnitDef::SetArmorType(int armorType) {
		this->armorType = armorType;
	}
	int springai::StubUnitDef::GetArmorType() {
		return armorType;
	}

	void springai::StubUnitDef::SetMaxWeaponRange(float maxWeaponRange) {
		this->maxWeaponRange = maxWeaponRange;
	}
	float springai::StubUnitDef::GetMaxWeaponRange() {
		return maxWeaponRange;
	}

	void springai::StubUnitDef::SetType(const char* type) {
		this->type = type;
	}
	const char* springai::StubUnitDef::GetType() {
		return type;
	}

	void springai::StubUnitDef::SetTooltip(const char* tooltip) {
		this->tooltip = tooltip;
	}
	const char* springai::StubUnitDef::GetTooltip() {
		return tooltip;
	}

	void springai::StubUnitDef::SetWreckName(const char* wreckName) {
		this->wreckName = wreckName;
	}
	const char* springai::StubUnitDef::GetWreckName() {
		return wreckName;
	}

	void springai::StubUnitDef::SetDeathExplosion(springai::WeaponDef* deathExplosion) {
		this->deathExplosion = deathExplosion;
	}
	springai::WeaponDef* springai::StubUnitDef::GetDeathExplosion() {
		return deathExplosion;
	}

	void springai::StubUnitDef::SetSelfDExplosion(springai::WeaponDef* selfDExplosion) {
		this->selfDExplosion = selfDExplosion;
	}
	springai::WeaponDef* springai::StubUnitDef::GetSelfDExplosion() {
		return selfDExplosion;
	}

	void springai::StubUnitDef::SetCategoryString(const char* categoryString) {
		this->categoryString = categoryString;
	}
	const char* springai::StubUnitDef::GetCategoryString() {
		return categoryString;
	}

	void springai::StubUnitDef::SetAbleToSelfD(bool isAbleToSelfD) {
		this->isAbleToSelfD = isAbleToSelfD;
	}
	bool springai::StubUnitDef::IsAbleToSelfD() {
		return isAbleToSelfD;
	}

	void springai::StubUnitDef::SetSelfDCountdown(int selfDCountdown) {
		this->selfDCountdown = selfDCountdown;
	}
	int springai::StubUnitDef::GetSelfDCountdown() {
		return selfDCountdown;
	}

	void springai::StubUnitDef::SetAbleToSubmerge(bool isAbleToSubmerge) {
		this->isAbleToSubmerge = isAbleToSubmerge;
	}
	bool springai::StubUnitDef::IsAbleToSubmerge() {
		return isAbleToSubmerge;
	}

	void springai::StubUnitDef::SetAbleToFly(bool isAbleToFly) {
		this->isAbleToFly = isAbleToFly;
	}
	bool springai::StubUnitDef::IsAbleToFly() {
		return isAbleToFly;
	}

	void springai::StubUnitDef::SetAbleToMove(bool isAbleToMove) {
		this->isAbleToMove = isAbleToMove;
	}
	bool springai::StubUnitDef::IsAbleToMove() {
		return isAbleToMove;
	}

	void springai::StubUnitDef::SetAbleToHover(bool isAbleToHover) {
		this->isAbleToHover = isAbleToHover;
	}
	bool springai::StubUnitDef::IsAbleToHover() {
		return isAbleToHover;
	}

	void springai::StubUnitDef::SetFloater(bool isFloater) {
		this->isFloater = isFloater;
	}
	bool springai::StubUnitDef::IsFloater() {
		return isFloater;
	}

	void springai::StubUnitDef::SetBuilder(bool isBuilder) {
		this->isBuilder = isBuilder;
	}
	bool springai::StubUnitDef::IsBuilder() {
		return isBuilder;
	}

	void springai::StubUnitDef::SetActivateWhenBuilt(bool isActivateWhenBuilt) {
		this->isActivateWhenBuilt = isActivateWhenBuilt;
	}
	bool springai::StubUnitDef::IsActivateWhenBuilt() {
		return isActivateWhenBuilt;
	}

	void springai::StubUnitDef::SetOnOffable(bool isOnOffable) {
		this->isOnOffable = isOnOffable;
	}
	bool springai::StubUnitDef::IsOnOffable() {
		return isOnOffable;
	}

	void springai::StubUnitDef::SetFullHealthFactory(bool isFullHealthFactory) {
		this->isFullHealthFactory = isFullHealthFactory;
	}
	bool springai::StubUnitDef::IsFullHealthFactory() {
		return isFullHealthFactory;
	}

	void springai::StubUnitDef::SetFactoryHeadingTakeoff(bool isFactoryHeadingTakeoff) {
		this->isFactoryHeadingTakeoff = isFactoryHeadingTakeoff;
	}
	bool springai::StubUnitDef::IsFactoryHeadingTakeoff() {
		return isFactoryHeadingTakeoff;
	}

	void springai::StubUnitDef::SetReclaimable(bool isReclaimable) {
		this->isReclaimable = isReclaimable;
	}
	bool springai::StubUnitDef::IsReclaimable() {
		return isReclaimable;
	}

	void springai::StubUnitDef::SetCapturable(bool isCapturable) {
		this->isCapturable = isCapturable;
	}
	bool springai::StubUnitDef::IsCapturable() {
		return isCapturable;
	}

	void springai::StubUnitDef::SetAbleToRestore(bool isAbleToRestore) {
		this->isAbleToRestore = isAbleToRestore;
	}
	bool springai::StubUnitDef::IsAbleToRestore() {
		return isAbleToRestore;
	}

	void springai::StubUnitDef::SetAbleToRepair(bool isAbleToRepair) {
		this->isAbleToRepair = isAbleToRepair;
	}
	bool springai::StubUnitDef::IsAbleToRepair() {
		return isAbleToRepair;
	}

	void springai::StubUnitDef::SetAbleToSelfRepair(bool isAbleToSelfRepair) {
		this->isAbleToSelfRepair = isAbleToSelfRepair;
	}
	bool springai::StubUnitDef::IsAbleToSelfRepair() {
		return isAbleToSelfRepair;
	}

	void springai::StubUnitDef::SetAbleToReclaim(bool isAbleToReclaim) {
		this->isAbleToReclaim = isAbleToReclaim;
	}
	bool springai::StubUnitDef::IsAbleToReclaim() {
		return isAbleToReclaim;
	}

	void springai::StubUnitDef::SetAbleToAttack(bool isAbleToAttack) {
		this->isAbleToAttack = isAbleToAttack;
	}
	bool springai::StubUnitDef::IsAbleToAttack() {
		return isAbleToAttack;
	}

	void springai::StubUnitDef::SetAbleToPatrol(bool isAbleToPatrol) {
		this->isAbleToPatrol = isAbleToPatrol;
	}
	bool springai::StubUnitDef::IsAbleToPatrol() {
		return isAbleToPatrol;
	}

	void springai::StubUnitDef::SetAbleToFight(bool isAbleToFight) {
		this->isAbleToFight = isAbleToFight;
	}
	bool springai::StubUnitDef::IsAbleToFight() {
		return isAbleToFight;
	}

	void springai::StubUnitDef::SetAbleToGuard(bool isAbleToGuard) {
		this->isAbleToGuard = isAbleToGuard;
	}
	bool springai::StubUnitDef::IsAbleToGuard() {
		return isAbleToGuard;
	}

	void springai::StubUnitDef::SetAbleToAssist(bool isAbleToAssist) {
		this->isAbleToAssist = isAbleToAssist;
	}
	bool springai::StubUnitDef::IsAbleToAssist() {
		return isAbleToAssist;
	}

	void springai::StubUnitDef::SetAssistable(bool isAssistable) {
		this->isAssistable = isAssistable;
	}
	bool springai::StubUnitDef::IsAssistable() {
		return isAssistable;
	}

	void springai::StubUnitDef::SetAbleToRepeat(bool isAbleToRepeat) {
		this->isAbleToRepeat = isAbleToRepeat;
	}
	bool springai::StubUnitDef::IsAbleToRepeat() {
		return isAbleToRepeat;
	}

	void springai::StubUnitDef::SetAbleToFireControl(bool isAbleToFireControl) {
		this->isAbleToFireControl = isAbleToFireControl;
	}
	bool springai::StubUnitDef::IsAbleToFireControl() {
		return isAbleToFireControl;
	}

	void springai::StubUnitDef::SetFireState(int fireState) {
		this->fireState = fireState;
	}
	int springai::StubUnitDef::GetFireState() {
		return fireState;
	}

	void springai::StubUnitDef::SetMoveState(int moveState) {
		this->moveState = moveState;
	}
	int springai::StubUnitDef::GetMoveState() {
		return moveState;
	}

	void springai::StubUnitDef::SetWingDrag(float wingDrag) {
		this->wingDrag = wingDrag;
	}
	float springai::StubUnitDef::GetWingDrag() {
		return wingDrag;
	}

	void springai::StubUnitDef::SetWingAngle(float wingAngle) {
		this->wingAngle = wingAngle;
	}
	float springai::StubUnitDef::GetWingAngle() {
		return wingAngle;
	}

	void springai::StubUnitDef::SetDrag(float drag) {
		this->drag = drag;
	}
	float springai::StubUnitDef::GetDrag() {
		return drag;
	}

	void springai::StubUnitDef::SetFrontToSpeed(float frontToSpeed) {
		this->frontToSpeed = frontToSpeed;
	}
	float springai::StubUnitDef::GetFrontToSpeed() {
		return frontToSpeed;
	}

	void springai::StubUnitDef::SetSpeedToFront(float speedToFront) {
		this->speedToFront = speedToFront;
	}
	float springai::StubUnitDef::GetSpeedToFront() {
		return speedToFront;
	}

	void springai::StubUnitDef::SetMyGravity(float myGravity) {
		this->myGravity = myGravity;
	}
	float springai::StubUnitDef::GetMyGravity() {
		return myGravity;
	}

	void springai::StubUnitDef::SetMaxBank(float maxBank) {
		this->maxBank = maxBank;
	}
	float springai::StubUnitDef::GetMaxBank() {
		return maxBank;
	}

	void springai::StubUnitDef::SetMaxPitch(float maxPitch) {
		this->maxPitch = maxPitch;
	}
	float springai::StubUnitDef::GetMaxPitch() {
		return maxPitch;
	}

	void springai::StubUnitDef::SetTurnRadius(float turnRadius) {
		this->turnRadius = turnRadius;
	}
	float springai::StubUnitDef::GetTurnRadius() {
		return turnRadius;
	}

	void springai::StubUnitDef::SetWantedHeight(float wantedHeight) {
		this->wantedHeight = wantedHeight;
	}
	float springai::StubUnitDef::GetWantedHeight() {
		return wantedHeight;
	}

	void springai::StubUnitDef::SetVerticalSpeed(float verticalSpeed) {
		this->verticalSpeed = verticalSpeed;
	}
	float springai::StubUnitDef::GetVerticalSpeed() {
		return verticalSpeed;
	}

	void springai::StubUnitDef::SetAbleToCrash(bool isAbleToCrash) {
		this->isAbleToCrash = isAbleToCrash;
	}
	bool springai::StubUnitDef::IsAbleToCrash() {
		return isAbleToCrash;
	}

	void springai::StubUnitDef::SetHoverAttack(bool isHoverAttack) {
		this->isHoverAttack = isHoverAttack;
	}
	bool springai::StubUnitDef::IsHoverAttack() {
		return isHoverAttack;
	}

	void springai::StubUnitDef::SetAirStrafe(bool isAirStrafe) {
		this->isAirStrafe = isAirStrafe;
	}
	bool springai::StubUnitDef::IsAirStrafe() {
		return isAirStrafe;
	}

	void springai::StubUnitDef::SetDlHoverFactor(float dlHoverFactor) {
		this->dlHoverFactor = dlHoverFactor;
	}
	float springai::StubUnitDef::GetDlHoverFactor() {
		return dlHoverFactor;
	}

	void springai::StubUnitDef::SetMaxAcceleration(float maxAcceleration) {
		this->maxAcceleration = maxAcceleration;
	}
	float springai::StubUnitDef::GetMaxAcceleration() {
		return maxAcceleration;
	}

	void springai::StubUnitDef::SetMaxDeceleration(float maxDeceleration) {
		this->maxDeceleration = maxDeceleration;
	}
	float springai::StubUnitDef::GetMaxDeceleration() {
		return maxDeceleration;
	}

	void springai::StubUnitDef::SetMaxAileron(float maxAileron) {
		this->maxAileron = maxAileron;
	}
	float springai::StubUnitDef::GetMaxAileron() {
		return maxAileron;
	}

	void springai::StubUnitDef::SetMaxElevator(float maxElevator) {
		this->maxElevator = maxElevator;
	}
	float springai::StubUnitDef::GetMaxElevator() {
		return maxElevator;
	}

	void springai::StubUnitDef::SetMaxRudder(float maxRudder) {
		this->maxRudder = maxRudder;
	}
	float springai::StubUnitDef::GetMaxRudder() {
		return maxRudder;
	}

	std::vector<short> springai::StubUnitDef::GetYardMap(int facing) {
		return std::vector<short>();
	}

	void springai::StubUnitDef::SetXSize(int xSize) {
		this->xSize = xSize;
	}
	int springai::StubUnitDef::GetXSize() {
		return xSize;
	}

	void springai::StubUnitDef::SetZSize(int zSize) {
		this->zSize = zSize;
	}
	int springai::StubUnitDef::GetZSize() {
		return zSize;
	}

	void springai::StubUnitDef::SetBuildAngle(int buildAngle) {
		this->buildAngle = buildAngle;
	}
	int springai::StubUnitDef::GetBuildAngle() {
		return buildAngle;
	}

	void springai::StubUnitDef::SetLoadingRadius(float loadingRadius) {
		this->loadingRadius = loadingRadius;
	}
	float springai::StubUnitDef::GetLoadingRadius() {
		return loadingRadius;
	}

	void springai::StubUnitDef::SetUnloadSpread(float unloadSpread) {
		this->unloadSpread = unloadSpread;
	}
	float springai::StubUnitDef::GetUnloadSpread() {
		return unloadSpread;
	}

	void springai::StubUnitDef::SetTransportCapacity(int transportCapacity) {
		this->transportCapacity = transportCapacity;
	}
	int springai::StubUnitDef::GetTransportCapacity() {
		return transportCapacity;
	}

	void springai::StubUnitDef::SetTransportSize(int transportSize) {
		this->transportSize = transportSize;
	}
	int springai::StubUnitDef::GetTransportSize() {
		return transportSize;
	}

	void springai::StubUnitDef::SetMinTransportSize(int minTransportSize) {
		this->minTransportSize = minTransportSize;
	}
	int springai::StubUnitDef::GetMinTransportSize() {
		return minTransportSize;
	}

	void springai::StubUnitDef::SetAirBase(bool isAirBase) {
		this->isAirBase = isAirBase;
	}
	bool springai::StubUnitDef::IsAirBase() {
		return isAirBase;
	}

	void springai::StubUnitDef::SetFirePlatform(bool isFirePlatform) {
		this->isFirePlatform = isFirePlatform;
	}
	bool springai::StubUnitDef::IsFirePlatform() {
		return isFirePlatform;
	}

	void springai::StubUnitDef::SetTransportMass(float transportMass) {
		this->transportMass = transportMass;
	}
	float springai::StubUnitDef::GetTransportMass() {
		return transportMass;
	}

	void springai::StubUnitDef::SetMinTransportMass(float minTransportMass) {
		this->minTransportMass = minTransportMass;
	}
	float springai::StubUnitDef::GetMinTransportMass() {
		return minTransportMass;
	}

	void springai::StubUnitDef::SetHoldSteady(bool isHoldSteady) {
		this->isHoldSteady = isHoldSteady;
	}
	bool springai::StubUnitDef::IsHoldSteady() {
		return isHoldSteady;
	}

	void springai::StubUnitDef::SetReleaseHeld(bool isReleaseHeld) {
		this->isReleaseHeld = isReleaseHeld;
	}
	bool springai::StubUnitDef::IsReleaseHeld() {
		return isReleaseHeld;
	}

	void springai::StubUnitDef::SetNotTransportable(bool isNotTransportable) {
		this->isNotTransportable = isNotTransportable;
	}
	bool springai::StubUnitDef::IsNotTransportable() {
		return isNotTransportable;
	}

	void springai::StubUnitDef::SetTransportByEnemy(bool isTransportByEnemy) {
		this->isTransportByEnemy = isTransportByEnemy;
	}
	bool springai::StubUnitDef::IsTransportByEnemy() {
		return isTransportByEnemy;
	}

	void springai::StubUnitDef::SetTransportUnloadMethod(int transportUnloadMethod) {
		this->transportUnloadMethod = transportUnloadMethod;
	}
	int springai::StubUnitDef::GetTransportUnloadMethod() {
		return transportUnloadMethod;
	}

	void springai::StubUnitDef::SetFallSpeed(float fallSpeed) {
		this->fallSpeed = fallSpeed;
	}
	float springai::StubUnitDef::GetFallSpeed() {
		return fallSpeed;
	}

	void springai::StubUnitDef::SetUnitFallSpeed(float unitFallSpeed) {
		this->unitFallSpeed = unitFallSpeed;
	}
	float springai::StubUnitDef::GetUnitFallSpeed() {
		return unitFallSpeed;
	}

	void springai::StubUnitDef::SetAbleToCloak(bool isAbleToCloak) {
		this->isAbleToCloak = isAbleToCloak;
	}
	bool springai::StubUnitDef::IsAbleToCloak() {
		return isAbleToCloak;
	}

	void springai::StubUnitDef::SetStartCloaked(bool isStartCloaked) {
		this->isStartCloaked = isStartCloaked;
	}
	bool springai::StubUnitDef::IsStartCloaked() {
		return isStartCloaked;
	}

	void springai::StubUnitDef::SetCloakCost(float cloakCost) {
		this->cloakCost = cloakCost;
	}
	float springai::StubUnitDef::GetCloakCost() {
		return cloakCost;
	}

	void springai::StubUnitDef::SetCloakCostMoving(float cloakCostMoving) {
		this->cloakCostMoving = cloakCostMoving;
	}
	float springai::StubUnitDef::GetCloakCostMoving() {
		return cloakCostMoving;
	}

	void springai::StubUnitDef::SetDecloakDistance(float decloakDistance) {
		this->decloakDistance = decloakDistance;
	}
	float springai::StubUnitDef::GetDecloakDistance() {
		return decloakDistance;
	}

	void springai::StubUnitDef::SetDecloakSpherical(bool isDecloakSpherical) {
		this->isDecloakSpherical = isDecloakSpherical;
	}
	bool springai::StubUnitDef::IsDecloakSpherical() {
		return isDecloakSpherical;
	}

	void springai::StubUnitDef::SetDecloakOnFire(bool isDecloakOnFire) {
		this->isDecloakOnFire = isDecloakOnFire;
	}
	bool springai::StubUnitDef::IsDecloakOnFire() {
		return isDecloakOnFire;
	}

	void springai::StubUnitDef::SetAbleToKamikaze(bool isAbleToKamikaze) {
		this->isAbleToKamikaze = isAbleToKamikaze;
	}
	bool springai::StubUnitDef::IsAbleToKamikaze() {
		return isAbleToKamikaze;
	}

	void springai::StubUnitDef::SetKamikazeDist(float kamikazeDist) {
		this->kamikazeDist = kamikazeDist;
	}
	float springai::StubUnitDef::GetKamikazeDist() {
		return kamikazeDist;
	}

	void springai::StubUnitDef::SetTargetingFacility(bool isTargetingFacility) {
		this->isTargetingFacility = isTargetingFacility;
	}
	bool springai::StubUnitDef::IsTargetingFacility() {
		return isTargetingFacility;
	}

	bool springai::StubUnitDef::CanManualFire() {
		return false;
	}

	void springai::StubUnitDef::SetNeedGeo(bool isNeedGeo) {
		this->isNeedGeo = isNeedGeo;
	}
	bool springai::StubUnitDef::IsNeedGeo() {
		return isNeedGeo;
	}

	void springai::StubUnitDef::SetFeature(bool isFeature) {
		this->isFeature = isFeature;
	}
	bool springai::StubUnitDef::IsFeature() {
		return isFeature;
	}

	void springai::StubUnitDef::SetHideDamage(bool isHideDamage) {
		this->isHideDamage = isHideDamage;
	}
	bool springai::StubUnitDef::IsHideDamage() {
		return isHideDamage;
	}

	void springai::StubUnitDef::SetCommander(bool isCommander) {
		this->isCommander = isCommander;
	}
	bool springai::StubUnitDef::IsCommander() {
		return isCommander;
	}

	void springai::StubUnitDef::SetShowPlayerName(bool isShowPlayerName) {
		this->isShowPlayerName = isShowPlayerName;
	}
	bool springai::StubUnitDef::IsShowPlayerName() {
		return isShowPlayerName;
	}

	void springai::StubUnitDef::SetAbleToResurrect(bool isAbleToResurrect) {
		this->isAbleToResurrect = isAbleToResurrect;
	}
	bool springai::StubUnitDef::IsAbleToResurrect() {
		return isAbleToResurrect;
	}

	void springai::StubUnitDef::SetAbleToCapture(bool isAbleToCapture) {
		this->isAbleToCapture = isAbleToCapture;
	}
	bool springai::StubUnitDef::IsAbleToCapture() {
		return isAbleToCapture;
	}

	void springai::StubUnitDef::SetHighTrajectoryType(int highTrajectoryType) {
		this->highTrajectoryType = highTrajectoryType;
	}
	int springai::StubUnitDef::GetHighTrajectoryType() {
		return highTrajectoryType;
	}

	void springai::StubUnitDef::SetNoChaseCategory(int noChaseCategory) {
		this->noChaseCategory = noChaseCategory;
	}
	int springai::StubUnitDef::GetNoChaseCategory() {
		return noChaseCategory;
	}

	void springai::StubUnitDef::SetLeaveTracks(bool isLeaveTracks) {
		this->isLeaveTracks = isLeaveTracks;
	}
	bool springai::StubUnitDef::IsLeaveTracks() {
		return isLeaveTracks;
	}

	void springai::StubUnitDef::SetTrackWidth(float trackWidth) {
		this->trackWidth = trackWidth;
	}
	float springai::StubUnitDef::GetTrackWidth() {
		return trackWidth;
	}

	void springai::StubUnitDef::SetTrackOffset(float trackOffset) {
		this->trackOffset = trackOffset;
	}
	float springai::StubUnitDef::GetTrackOffset() {
		return trackOffset;
	}

	void springai::StubUnitDef::SetTrackStrength(float trackStrength) {
		this->trackStrength = trackStrength;
	}
	float springai::StubUnitDef::GetTrackStrength() {
		return trackStrength;
	}

	void springai::StubUnitDef::SetTrackStretch(float trackStretch) {
		this->trackStretch = trackStretch;
	}
	float springai::StubUnitDef::GetTrackStretch() {
		return trackStretch;
	}

	void springai::StubUnitDef::SetTrackType(int trackType) {
		this->trackType = trackType;
	}
	int springai::StubUnitDef::GetTrackType() {
		return trackType;
	}

	void springai::StubUnitDef::SetAbleToDropFlare(bool isAbleToDropFlare) {
		this->isAbleToDropFlare = isAbleToDropFlare;
	}
	bool springai::StubUnitDef::IsAbleToDropFlare() {
		return isAbleToDropFlare;
	}

	void springai::StubUnitDef::SetFlareReloadTime(float flareReloadTime) {
		this->flareReloadTime = flareReloadTime;
	}
	float springai::StubUnitDef::GetFlareReloadTime() {
		return flareReloadTime;
	}

	void springai::StubUnitDef::SetFlareEfficiency(float flareEfficiency) {
		this->flareEfficiency = flareEfficiency;
	}
	float springai::StubUnitDef::GetFlareEfficiency() {
		return flareEfficiency;
	}

	void springai::StubUnitDef::SetFlareDelay(float flareDelay) {
		this->flareDelay = flareDelay;
	}
	float springai::StubUnitDef::GetFlareDelay() {
		return flareDelay;
	}

	void springai::StubUnitDef::SetFlareDropVector(springai::AIFloat3 flareDropVector) {
		this->flareDropVector = flareDropVector;
	}
	springai::AIFloat3 springai::StubUnitDef::GetFlareDropVector() {
		return flareDropVector;
	}

	void springai::StubUnitDef::SetFlareTime(int flareTime) {
		this->flareTime = flareTime;
	}
	int springai::StubUnitDef::GetFlareTime() {
		return flareTime;
	}

	void springai::StubUnitDef::SetFlareSalvoSize(int flareSalvoSize) {
		this->flareSalvoSize = flareSalvoSize;
	}
	int springai::StubUnitDef::GetFlareSalvoSize() {
		return flareSalvoSize;
	}

	void springai::StubUnitDef::SetFlareSalvoDelay(int flareSalvoDelay) {
		this->flareSalvoDelay = flareSalvoDelay;
	}
	int springai::StubUnitDef::GetFlareSalvoDelay() {
		return flareSalvoDelay;
	}

	void springai::StubUnitDef::SetAbleToLoopbackAttack(bool isAbleToLoopbackAttack) {
		this->isAbleToLoopbackAttack = isAbleToLoopbackAttack;
	}
	bool springai::StubUnitDef::IsAbleToLoopbackAttack() {
		return isAbleToLoopbackAttack;
	}

	void springai::StubUnitDef::SetLevelGround(bool isLevelGround) {
		this->isLevelGround = isLevelGround;
	}
	bool springai::StubUnitDef::IsLevelGround() {
		return isLevelGround;
	}

	void springai::StubUnitDef::SetUseBuildingGroundDecal(bool isUseBuildingGroundDecal) {
		this->isUseBuildingGroundDecal = isUseBuildingGroundDecal;
	}
	bool springai::StubUnitDef::IsUseBuildingGroundDecal() {
		return isUseBuildingGroundDecal;
	}

	void springai::StubUnitDef::SetBuildingDecalType(int buildingDecalType) {
		this->buildingDecalType = buildingDecalType;
	}
	int springai::StubUnitDef::GetBuildingDecalType() {
		return buildingDecalType;
	}

	void springai::StubUnitDef::SetBuildingDecalSizeX(int buildingDecalSizeX) {
		this->buildingDecalSizeX = buildingDecalSizeX;
	}
	int springai::StubUnitDef::GetBuildingDecalSizeX() {
		return buildingDecalSizeX;
	}

	void springai::StubUnitDef::SetBuildingDecalSizeY(int buildingDecalSizeY) {
		this->buildingDecalSizeY = buildingDecalSizeY;
	}
	int springai::StubUnitDef::GetBuildingDecalSizeY() {
		return buildingDecalSizeY;
	}

	void springai::StubUnitDef::SetBuildingDecalDecaySpeed(float buildingDecalDecaySpeed) {
		this->buildingDecalDecaySpeed = buildingDecalDecaySpeed;
	}
	float springai::StubUnitDef::GetBuildingDecalDecaySpeed() {
		return buildingDecalDecaySpeed;
	}

	void springai::StubUnitDef::SetMaxFuel(float maxFuel) {
		this->maxFuel = maxFuel;
	}
	float springai::StubUnitDef::GetMaxFuel() {
		return maxFuel;
	}

	void springai::StubUnitDef::SetRefuelTime(float refuelTime) {
		this->refuelTime = refuelTime;
	}
	float springai::StubUnitDef::GetRefuelTime() {
		return refuelTime;
	}

	void springai::StubUnitDef::SetMinAirBasePower(float minAirBasePower) {
		this->minAirBasePower = minAirBasePower;
	}
	float springai::StubUnitDef::GetMinAirBasePower() {
		return minAirBasePower;
	}

	void springai::StubUnitDef::SetMaxThisUnit(int maxThisUnit) {
		this->maxThisUnit = maxThisUnit;
	}
	int springai::StubUnitDef::GetMaxThisUnit() {
		return maxThisUnit;
	}

	void springai::StubUnitDef::SetDecoyDef(springai::UnitDef* decoyDef) {
		this->decoyDef = decoyDef;
	}
	springai::UnitDef* springai::StubUnitDef::GetDecoyDef() {
		return decoyDef;
	}

	void springai::StubUnitDef::SetDontLand(bool isDontLand) {
		this->isDontLand = isDontLand;
	}
	bool springai::StubUnitDef::IsDontLand() {
		return isDontLand;
	}

	void springai::StubUnitDef::SetShieldDef(springai::WeaponDef* shieldDef) {
		this->shieldDef = shieldDef;
	}
	springai::WeaponDef* springai::StubUnitDef::GetShieldDef() {
		return shieldDef;
	}

	void springai::StubUnitDef::SetStockpileDef(springai::WeaponDef* stockpileDef) {
		this->stockpileDef = stockpileDef;
	}
	springai::WeaponDef* springai::StubUnitDef::GetStockpileDef() {
		return stockpileDef;
	}

	void springai::StubUnitDef::SetBuildOptions(std::vector<springai::UnitDef*> buildOptions) {
		this->buildOptions = buildOptions;
	}
	std::vector<springai::UnitDef*> springai::StubUnitDef::GetBuildOptions() {
		return buildOptions;
	}

	void springai::StubUnitDef::SetCustomParams(std::map<std::string,std::string> customParams) {
		this->customParams = customParams;
	}
	std::map<std::string,std::string> springai::StubUnitDef::GetCustomParams() {
		return customParams;
	}

	void springai::StubUnitDef::SetWeaponMounts(std::vector<springai::WeaponMount*> weaponMounts) {
		this->weaponMounts = weaponMounts;
	}
	std::vector<springai::WeaponMount*> springai::StubUnitDef::GetWeaponMounts() {
		return weaponMounts;
	}

	void springai::StubUnitDef::SetMoveData(springai::MoveData* moveData) {
		this->moveData = moveData;
	}
	springai::MoveData* springai::StubUnitDef::GetMoveData() {
		return moveData;
	}

	void springai::StubUnitDef::SetFlankingBonus(springai::FlankingBonus* flankingBonus) {
		this->flankingBonus = flankingBonus;
	}
	springai::FlankingBonus* springai::StubUnitDef::GetFlankingBonus() {
		return flankingBonus;
	}


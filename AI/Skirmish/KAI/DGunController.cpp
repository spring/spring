// class controlling basic commander dgun-behavior
// written by Kloot for KAI, licensed under GPL v2

#include <cstdlib>
#include <ctime>

#include "DGunController.hpp"

#include "System/Exceptions.h"

CR_BIND(DGunController ,(NULL))

CR_REG_METADATA(DGunController,(
				CR_MEMBER(ai),
				CR_MEMBER(inited),
				CR_MEMBER(commanderID),
				CR_MEMBER(targetID),
				CR_MEMBER(hasDGunOrder),
				CR_MEMBER(hasReclaimOrder),
				CR_MEMBER(hasRetreatOrder),
				CR_MEMBER(dgunOrderFrame),
				CR_MEMBER(reclaimOrderFrame),
				CR_MEMBER(retreatOrderFrame),
				CR_MEMBER(startingPos),
				CR_MEMBER(bt),
				CR_RESERVED(256),
				CR_POSTLOAD(PostLoad)
				));

DGunController::DGunController(AIClasses* ai) {
	srand((unsigned) time(0));

	this -> inited = false;
	this -> ai = ai;
	(this -> units) = (int*) calloc(MAX_UNITS, sizeof(int));

	this -> commanderID = 0;
	this -> commanderUD = 0;
	this -> startingPos = float3(0,0,0);

	this -> targetID			= -1;
	this -> hasDGunOrder		= false;
	this -> hasReclaimOrder		= false;
	this -> hasRetreatOrder		= false;
	this -> dgunOrderFrame		= 0;
	this -> reclaimOrderFrame	= 0;
	this -> retreatOrderFrame	= 0;
	this -> bt                  = 0;
	this -> commanderDGun       = 0;
}
DGunController::~DGunController(void) {
	free(this -> units);
}

void DGunController::PostLoad()
{
	if (inited) {
		CALLBACK = ai->cb;
		this -> commanderUD = CALLBACK -> GetUnitDef(commanderID);
		for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i = commanderUD->weapons.begin();i!=commanderUD->weapons.end();i++)
			if (i->def->type=="DGun") {
				commanderDGun=i->def;
				break;
			}
		if (!commanderDGun) throw content_error("DGunController: Cannot find dgun weapon");
	}
}

void DGunController::init(IAICallback* callback, int commanderID) {
	CALLBACK = callback;

	this -> inited = true;
	this -> commanderID = commanderID;
	this -> commanderUD = callback -> GetUnitDef(commanderID);
//	this -> startingPos = callback -> GetUnitPos(commanderID);
	this -> startingPos = *callback -> GetStartPos();
	for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i = commanderUD->weapons.begin();i!=commanderUD->weapons.end();i++)
		if (i->def->type=="DGun") {
			commanderDGun=i->def;
			break;
		}
	if (!commanderDGun) throw content_error("DGunController: Cannot find dgun weapon");
	
	this -> targetID			= -1;
	this -> hasDGunOrder		= false;
	this -> hasReclaimOrder		= false;
	this -> hasRetreatOrder		= false;
	this -> dgunOrderFrame		= 0;
	this -> reclaimOrderFrame	= 0;
	this -> retreatOrderFrame	= 0;

	this -> bt                  = ai -> uh -> GetBuilderTracker(commanderID);

	// set commander to hold fire
	this -> setFireState(0);
}




bool DGunController::inRange(float3 commanderPos, float3 attackerPos, float s) {
	float maxRange = CALLBACK -> GetUnitMaxRange(this -> commanderID);

	return ((commanderPos.distance(attackerPos)) < (maxRange * s));
}


void DGunController::handleDestroyEvent(int attackerID, int targetID) {
	// if we were dgunning or reclaiming this unit and it died
	// (reclaiming unit causes attackerID of 0 to be passed)
	if (/*attackerID == 0 || */this -> targetID == targetID) {
		this -> targetID = -1;
		this -> hasDGunOrder = false;
		this -> hasReclaimOrder = false;
	}
}


// function that handles UnitDamaged() event for commander
void DGunController::handleAttackEvent(int attackerID, float damage, float3 attackerDir, float3 attackerPos) {
	// in eg. EE init() is never called (no dgun)
	if (!this -> inited)
		return;
	if (hasDGunOrder && damage<DGUN_MAX_DAMAGE_LEVEL) return;
	if (hasReclaimOrder && (CALLBACK -> GetEnergy()) < DGUN_MIN_ENERGY_LEVEL && damage<DGUN_MAX_DAMAGE_LEVEL) return;
	if (hasRetreatOrder) return;

	int currentFrame = CALLBACK -> GetCurrentFrame();
	float3 commanderPos = CALLBACK -> GetUnitPos(this -> commanderID);
	float healthCur = CALLBACK -> GetUnitHealth(this -> commanderID);
	float healthMax = CALLBACK -> GetUnitMaxHealth(this -> commanderID);

	// check if target within immediate range (if not then better call backup)
	if (this -> inRange(commanderPos, attackerPos, 1.0f)) {
		// do we have valid target?
		if ((attackerID > 0) && (CALLBACK -> GetUnitHealth(attackerID) > 0)) {
			const UnitDef* attackerDef = CALLBACK -> GetUnitDef(attackerID);

			// don't bother trying to kill planes and also don't directly pop enemy commanders
			if (!attackerDef -> canfly && (!attackerDef -> isCommander && !attackerDef -> canDGun)) {

				// prevent friendly-fire "incidents"
				if ((CALLBACK -> GetMyTeam()) != (CALLBACK -> GetUnitTeam(attackerID))) {
					// can we blast it right now?
					if ((CALLBACK -> GetEnergy()) >= DGUN_MIN_ENERGY_LEVEL) {
						// check if we already issued a dgun order
						if (!this -> hasDGunOrder) {
							this -> issueOrder(attackerID, CMD_DGUN, currentFrame, 0);
						}
					}

					// no, suck it instead
					else {
						// if we are already reclaiming one unit ignore attacks
						// from others (while we do not have energy to dgun and
						// order not too old)
						if (!this -> hasReclaimOrder) {
							this -> issueOrder(attackerID, CMD_RECLAIM, currentFrame, 0);
						}
					}
				}

			}
		}
	}


	// if we are close to going boom then it's time to run our boy
	if ((healthMax > 0) && ((healthCur / healthMax) < DGUN_MIN_HEALTH_RATIO)) {
		// retreat back to commander starting pos if we aren't already there
		if (!this -> inRange(commanderPos, this -> startingPos, 0.5f)) {
			this -> issueOrder(this -> startingPos, CMD_MOVE, currentFrame, 0);
		}
	}
	// if we are under attack from something nasty and can't shoot back
	if (damage >= DGUN_MAX_DAMAGE_LEVEL) {
		// retreat back to commander starting pos if we aren't already there
		if (!this -> inRange(commanderPos, this -> startingPos, 0.5f)) {
			this -> issueOrder(this -> startingPos, CMD_MOVE, currentFrame, 0);
		}
	}
}


// update routine to ensure dgun behavior is not solely reactive
void DGunController::update(unsigned int currentFrame) {
	// in eg. EE init() is never called (no dgun)
	if (!this -> inited)
		return;

	// make sure over-age dgun and reclaim orders are erased
	this -> clearOrders(currentFrame);

	float3 commanderPos = CALLBACK -> GetUnitPos(this -> commanderID);

	if (this -> hasRetreatOrder) {
		float healthCur = CALLBACK -> GetUnitHealth(this -> commanderID);
		float healthMax = CALLBACK -> GetUnitMaxHealth(this -> commanderID);

		if ((healthMax > 0) && ((healthCur / healthMax) >= (DGUN_MIN_HEALTH_RATIO * 2)) &&
			ai->uh->Distance2DToNearestFactory(commanderPos.x,commanderPos.z)>DEFCBS_RADIUS/4) {
			// cancel retreat order if we are no longer in danger
			bt->customOrderId = 0;
			this -> hasRetreatOrder = false;
		}
		else {
			// otherwise prevent our boy from making any rash moves
			// return;
		}
	}

	if (ai->uh->Distance2DToNearestFactory(commanderPos.x,commanderPos.z)>DEFCBS_RADIUS/2) {
		this -> issueOrder(this -> startingPos, CMD_MOVE, currentFrame, 0);
	}

	// if we do not have any live outstanding orders
	if (!this -> hasDGunOrder && (!this -> hasReclaimOrder || (CALLBACK -> GetEnergy()) >= DGUN_MIN_ENERGY_LEVEL) && (!hasRetreatOrder)) {

		// if our commander is dead then position will be (0, 0, 0)
		if (commanderPos.x <= 0 && commanderPos.z <= 0) {
			return;
		}

		// get all units within near-immediate dgun range
		float maxRange = CALLBACK -> GetUnitMaxRange(this -> commanderID);
		float s = (this -> hasRetreatOrder)? 1.0f: 2.0f;
		int numUnits = CALLBACK -> GetEnemyUnits(this -> units, commanderPos, maxRange * s);
/*
		for (int i = 0; i < numUnits; i++) {
			// if enemy unit found in array
			if (units[i] > 0) {
				// check if unit still alive (needed since when UnitDestroyed()
				// triggered GetEnemyUnits() is not immediately updated as well)
				if (CALLBACK -> GetUnitHealth(units[i]) > 0) {
					const UnitDef* attackerDef = CALLBACK -> GetUnitDef(units[i]);

					// don't bother trying to kill planes and also don't directly pop enemy commanders
					if (!attackerDef -> canfly && (!attackerDef -> isCommander && !attackerDef -> canDGun)) {

						// blast it
						if ((CALLBACK -> GetEnergy()) >= DGUN_MIN_ENERGY_LEVEL) {
							this -> issueOrder(units[i], CMD_DGUN, currentFrame, 0);
							units[i] = -1;
						}
						// suck it
						else {
							this -> issueOrder(units[i], CMD_RECLAIM, currentFrame, 0);
							units[i] = -1;
						}

						return;
					}
				}
			}
		}
*/
		int nearestEnemy = 0;
		const UnitDef* nearestEnemyDef = 0;
		int nearestDist = 1000;
		for (int i = 0; i < numUnits; i++) {
			// if enemy unit found in array
			const UnitDef* enemyDef = CALLBACK -> GetUnitDef(units[i]);
			if (!enemyDef) continue;
			float dist = (commanderPos-CALLBACK->GetUnitPos(units[i])).Length();
			if (enemyDef->canfly && (dist>50)) continue;
//			float maxenemydist=0;
//			for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i=enemyDef->weapons.begin();i!=enemyDef->weapons.end();i++) {
//				if (maxenemydist<i->def->range) maxenemydist = i->def->range;
//			}
//			if (dist>maxenemydist) continue;
			if (dist<nearestDist) {
				nearestDist = dist;
				nearestEnemy = units[i];
				nearestEnemyDef = enemyDef;
			}
		}
		if (nearestEnemy) {
			if (nearestEnemyDef->isCommander || nearestEnemyDef->canDGun) {
				this -> issueOrder(this -> startingPos, CMD_MOVE, currentFrame, 0);
				return;
			}
			if ((CALLBACK -> GetEnergy()) >= DGUN_MIN_ENERGY_LEVEL) {
				this -> issueOrder(nearestEnemy, CMD_DGUN, currentFrame, 0);
			}
			// suck it
			else {
				this -> issueOrder(nearestEnemy, CMD_RECLAIM, currentFrame, 0);
			}
		}
	}
}




// let commander walk in direction orthogonal to attack vector
void DGunController::evadeIncomingFire(float3 attackerDir, float3 attackerPos, int keyMod) {
	attackerPos = attackerPos;

	float3 upDir(0, 1, 0);
	float3 crossDir = (attackerDir.cross(upDir)).Normalize();
	float3 commanderPos = CALLBACK -> GetUnitPos(commanderID);
	int evadeDir = (rand() > (RAND_MAX >> 1))? 1: -1;

	float x = commanderPos.x + (crossDir.x * ((rand() / (float) RAND_MAX) * 1024)) * evadeDir;
	float y = commanderPos.y + (crossDir.y * ((rand() / (float) RAND_MAX) * 1024)) * evadeDir;
	float z = commanderPos.z + (crossDir.z * ((rand() / (float) RAND_MAX) * 1024)) * evadeDir;
	float3 target(x, y, z);

	this -> issueOrder(target, CMD_MOVE, 0, keyMod);
}


void DGunController::issueOrder(float3 target, int orderType, unsigned int currentFrame, int keyMod) {
	clearBuild();
	Command c;
	c.id = orderType;
	c.options |= keyMod;
	c.params.push_back(target.x);
	c.params.push_back(target.y);
	c.params.push_back(target.z);

	this -> targetID = -1;

	if (orderType == CMD_MOVE) {
		// regard CMD_MOVE as order to retreat
		this -> hasRetreatOrder = true;
		this -> retreatOrderFrame = currentFrame;
	}
	bt->customOrderId = orderType;
	CALLBACK -> GiveOrder(this -> commanderID, &c);
}

void DGunController::issueOrder(int target, int orderType, unsigned int currentFrame, int keyMod) {
	clearBuild();
	Command c;
	c.id = orderType;
	c.options |= keyMod;
	c.params.push_back(target);

	this -> targetID = target;

	if (orderType == CMD_DGUN) {
		this -> hasDGunOrder = true;
		this -> dgunOrderFrame = currentFrame;
	}
	if (orderType == CMD_RECLAIM) {
		this -> hasReclaimOrder = true;
		this -> reclaimOrderFrame = currentFrame;
	}

	bt->customOrderId = orderType;
	CALLBACK -> GiveOrder(this -> commanderID, &c);
}

void DGunController::clearOrders(unsigned int currentFrame) {
	if ((currentFrame - (this -> dgunOrderFrame)) > (ORDERS_TIMEOUT)) {
		// dgun order expired
		bt->customOrderId = 0;
		this -> hasDGunOrder = false;
	}
	if ((currentFrame - (this -> reclaimOrderFrame)) > (ORDERS_TIMEOUT)) {
		// reclaim order expired
		bt->customOrderId = 0;
		this -> hasReclaimOrder = false;
	}
}


// we need this since 1 and 2 interfere with dgun and reclaim orders
// (state can be 0: hold fire, 1: return fire, 2: fire at will)
void DGunController::setFireState(int state) {
	Command c;
	c.id = CMD_FIRE_STATE;
	c.params.push_back(state);

	CALLBACK -> GiveOrder(this -> commanderID, &c);
}

void DGunController::clearBuild()
{
	ai->uh->IdleUnitRemove(commanderID);
	if (bt->buildTaskId) ai->uh->BuildTaskRemoved(bt);
	if (bt->taskPlanId) ai->uh->TaskPlanRemoved(bt);
	if (bt->factoryId) ai->uh->FactoryBuilderRemoved(bt);
}

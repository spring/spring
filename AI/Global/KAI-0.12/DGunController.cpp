// class controlling basic commander dgun-behavior
// written by Kloot for KAI, licensed under GPL v2

#include <cstdlib>
#include <ctime>

#include "DGunController.hpp"

DGunController::DGunController(void) {
	srand((unsigned) time(0));

	(this -> units) = (int*) calloc(MAX_UNITS, sizeof(int));
}
DGunController::~DGunController(void) {
	free(this -> units);
}

void DGunController::init(IAICallback* callback, int commanderID) {
	CALLBACK = callback;

	this -> commanderID = commanderID;
	this -> targetID = -1;
	this -> hasDGunOrder = false;
	this -> hasReclaimOrder = false;
	this -> dgunOrderFrame = 0;
	this -> reclaimOrderFrame = 0;

	// set commander to hold fire
	this -> setFireState(0);
}




bool DGunController::inRange(float3 commanderPos, float3 attackerPos) {
	float maxRange = CALLBACK -> GetUnitMaxRange(this -> commanderID);

	return ((commanderPos.distance(attackerPos)) < (maxRange * 2));
}


void DGunController::handleDestroyEvent(int attackerID, int targetID) {
	// if we were dgunning or reclaiming this unit and it died
	// (NOTE: reclaiming unit causes attackerID of 0 to be passed)
	if (attackerID == 0 || this -> targetID == targetID) {
		this -> targetID = -1;
		this -> hasDGunOrder = false;
		this -> hasReclaimOrder = false;
	}
}


// function that handles UnitDamaged() event for commander
void DGunController::handleAttackEvent(int attackerID, float damage, float3 attackerDir, float3 attackerPos) {
	int currentFrame = CALLBACK -> GetCurrentFrame();
	float3 commanderPos = CALLBACK -> GetUnitPos(this -> commanderID);


	// check if target within reasonable range (if not then better call backup)
	if (this -> inRange(commanderPos, attackerPos)) {
		// do we have valid target?
		if ((attackerID > 0) && (CALLBACK -> GetUnitHealth(attackerID) > 0)) {
			// prevent friendly-fire "incidents"
			if ((CALLBACK -> GetMyTeam()) != (CALLBACK -> GetUnitTeam(attackerID))) {

				// can we blast it right now?
				if ((CALLBACK -> GetEnergy()) >= DGUN_MIN_ENERGY_LEVEL) {
					// check if we already issued a dgun order
					if (this -> hasDGunOrder) {
						// check age of order so we do not get stuck in idle mode forever
						if ((currentFrame - (this -> dgunOrderFrame)) <= (FRAMERATE >> 2)) {
							return;
						}
						else {
							// order expired
							this -> targetID = -1;
							this -> hasDGunOrder = false;
						}
					}

					this -> issueOrder(attackerID, CMD_DGUN, CALLBACK -> GetCurrentFrame(), 0);
				}

				// no, suck it instead
				else {
					// if we are already reclaiming one unit ignore attacks
					// from others (while we do not have energy to dgun and
					// order not too old)
					if (this -> hasReclaimOrder) {
						// check age of order so we do not get stuck in idle mode forever
						if ((currentFrame - (this -> reclaimOrderFrame)) <= (FRAMERATE << 2)) {
							return;
						}
						else {
							// order expired
							this -> targetID = -1;
							this -> hasReclaimOrder = false;
						}
					}

					this -> issueOrder(attackerID, CMD_RECLAIM, CALLBACK -> GetCurrentFrame(), 0);
				}
			}
		}
	}
}


// update routine to ensure dgun behavior is not solely reactive
void DGunController::update(unsigned int currentFrame) {
	// if our commanderID has been set before update()
	if ((this -> commanderID) > 0) {
		// if we do not have any live outstanding orders
		if (!this -> hasDGunOrder && !this -> hasReclaimOrder) {
			float3 commanderPos = CALLBACK -> GetUnitPos(this -> commanderID);

			// if our commander is dead then position will be (0, 0, 0)
			if (commanderPos.x <= 0 && commanderPos.z <= 0) {
				return;
			}

			// get all units within immediate dgun range
			float maxRange = CALLBACK -> GetUnitMaxRange(this -> commanderID);
			int numUnits = CALLBACK -> GetEnemyUnits(this -> units, commanderPos, maxRange);

			for (int i = 0; i < numUnits; i++) {
				// if enemy unit found in array
				if (units[i] > 0) {
					// check if unit still alive (needed since when UnitDestroyed()
					// triggered GetEnemyUnits() is not immediately updated as well)
					if (CALLBACK -> GetUnitHealth(units[i]) > 0) {
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
	}
}




// let commander walk in direction orthogonal to attack vector
void DGunController::evadeIncomingFire(float3 attackerDir, float3 attackerPos, int keyMod) {
	attackerPos = attackerPos;

	float3 upDir(0, 1, 0);
	float3 crossDir = (attackerDir.cross(upDir)).Normalize();
	float3 commanderPos = CALLBACK -> GetUnitPos(commanderID);
	int dir = (rand() > (RAND_MAX >> 1))? 1: -1;

	Command c;
	c.id = CMD_MOVE;
	c.options |= keyMod;
	c.params.push_back(commanderPos.x + (crossDir.x * ((rand() / (float) RAND_MAX) * 1024)) * dir);
	c.params.push_back(commanderPos.y + (crossDir.y * ((rand() / (float) RAND_MAX) * 1024)) * dir);
	c.params.push_back(commanderPos.z + (crossDir.z * ((rand() / (float) RAND_MAX) * 1024)) * dir);

	CALLBACK -> GiveOrder(this -> commanderID, &c);
}


void DGunController::issueOrder(int targetID, int orderType, unsigned int currentFrame, int keyMod) {
	this -> targetID = targetID;

	if (orderType == CMD_DGUN) {
		this -> hasDGunOrder = true;
		this -> dgunOrderFrame = currentFrame;
	}
	if (orderType == CMD_RECLAIM) {
		this -> hasReclaimOrder = true;
		this -> reclaimOrderFrame = currentFrame;
	}

	Command c;
	c.id = orderType;
	c.options |= keyMod;
	c.params.push_back(targetID);

	CALLBACK -> GiveOrder(this -> commanderID, &c);
}


// we need this since 1 and 2 interfere with dgun and reclaim orders
// (state can be 0: hold fire, 1: return fire, 2: fire at will)
void DGunController::setFireState(int state) {
	Command c;
	c.id = CMD_FIRE_STATE;
	c.params.push_back(state);

	CALLBACK -> GiveOrder(this -> commanderID, &c);
}

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CommandAI.h"
#include "BuilderCAI.h"
#include "FactoryCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/WaitCommandsAI.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/creg/STL_Set.h"
#include "System/creg/STL_Deque.h"
#include <assert.h>

// number of SlowUpdate calls that a target (unit) must
// be out of radar (and hence LOS) contact before it is
// considered 'lost' and invalid (for attack orders etc)
//
// historically this value was 120, which meant that it
// took (120 * UNIT_SLOWUPDATE_RATE) / GAME_SPEED == 64
// seconds (!) before eg. aircraft would stop tracking a
// target that cloaked after flying over it --> obviously
// unreasonable
static const int TARGET_LOST_TIMER = 4;
static const float COMMAND_CANCEL_DIST = 17.0f;

CR_BIND(CCommandQueue, );
CR_REG_METADATA(CCommandQueue, (
	CR_MEMBER(queue),
	CR_ENUM_MEMBER(queueType),
	CR_MEMBER(tagCounter)
));

CR_BIND_DERIVED(CCommandAI, CObject, );
CR_REG_METADATA(CCommandAI, (
	CR_MEMBER(stockpileWeapon),

	CR_MEMBER(possibleCommands),
	CR_MEMBER(commandQue),
	CR_MEMBER(nonQueingCommands),
	CR_MEMBER(lastUserCommand),
	CR_MEMBER(selfDCountdown),
	CR_MEMBER(lastFinishCommand),

	CR_MEMBER(owner),

	CR_MEMBER(orderTarget),
	CR_MEMBER(targetDied),
	CR_MEMBER(inCommand),
	CR_MEMBER(repeatOrders),
	CR_MEMBER(lastSelectedCommandPage),
	CR_MEMBER(unimportantMove),
	CR_MEMBER(commandDeathDependences),
	CR_MEMBER(targetLostTimer),

	CR_POSTLOAD(PostLoad)
));

CCommandAI::CCommandAI():
	stockpileWeapon(0),
	lastUserCommand(-1000),
	selfDCountdown(0),
	lastFinishCommand(0),
	owner(NULL),
	orderTarget(0),
	targetDied(false),
	inCommand(false),
	repeatOrders(false),
	lastSelectedCommandPage(0),
	unimportantMove(false),
	targetLostTimer(TARGET_LOST_TIMER)
{}

CCommandAI::CCommandAI(CUnit* owner):
	stockpileWeapon(0),
	lastUserCommand(-1000),
	selfDCountdown(0),
	lastFinishCommand(0),
	owner(owner),
	orderTarget(0),
	targetDied(false),
	inCommand(false),
	repeatOrders(false),
	lastSelectedCommandPage(0),
	unimportantMove(false),
	targetLostTimer(TARGET_LOST_TIMER)
{
	owner->commandAI = this;
	CommandDescription c;
	c.id = CMD_STOP;
	c.action = "stop";
	c.type = CMDTYPE_ICON;
	c.name = "Stop";
	c.mouseicon = c.name;
	c.tooltip = "Stop: Cancel the units current actions";
	possibleCommands.push_back(c);

	if (IsAttackCapable()) {
		c.id = CMD_ATTACK;
		c.action = "attack";
		c.type = CMDTYPE_ICON_UNIT_OR_MAP;
		c.name = "Attack";
		c.mouseicon = c.name;
		c.tooltip = "Attack: Attacks a unit or a position on the ground";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canManualFire) {
		c.id = CMD_MANUALFIRE;
		c.action = "manualfire";
		c.type = CMDTYPE_ICON_MAP;
		c.name = "ManualFire";
		c.mouseicon = c.name;
		c.tooltip = "ManualFire: Attacks with manually-fired weapon";
		possibleCommands.push_back(c);
	}

	c.id = CMD_WAIT;
	c.action = "wait";
	c.type = CMDTYPE_ICON;
	c.name = "Wait";
	c.mouseicon = c.name;
	c.tooltip = "Wait: Tells the unit to wait until another units handles it";
	possibleCommands.push_back(c);
//	nonQueingCommands.insert(CMD_WAIT);

	c.id = CMD_TIMEWAIT;
	c.action = "timewait";
	c.type = CMDTYPE_NUMBER;
	c.name = "TimeWait";
	c.mouseicon=c.name;
	c.tooltip = "TimeWait: Wait for a period of time before continuing";
	c.params.push_back("1");  // min
	c.params.push_back("60"); // max
	c.hidden = true;
	possibleCommands.push_back(c);
	c.hidden = false;
	c.params.clear();

	// only for games with 2 ally teams  --  checked later
	c.id = CMD_DEATHWAIT;
	c.action = "deathwait";
	c.type = CMDTYPE_ICON_UNIT_OR_RECTANGLE;
	c.name = "DeathWait";
	c.mouseicon=c.name;
	c.tooltip = "DeathWait: Wait until units die before continuing";
	c.hidden = true;
	possibleCommands.push_back(c);
	c.hidden = false;

	c.id = CMD_SQUADWAIT;
	c.action = "squadwait";
	c.type = CMDTYPE_NUMBER;
	c.name = "SquadWait";
	c.mouseicon=c.name;
	c.tooltip = "SquadWait: Wait for a number of units to arrive before continuing";
	c.params.push_back("2");   // min
	c.params.push_back("100"); // max
	c.hidden = true;
	possibleCommands.push_back(c);
	c.hidden = false;
	c.params.clear();

	c.id = CMD_GATHERWAIT;
	c.action = "gatherwait";
	c.type = CMDTYPE_ICON;
	c.name = "GatherWait";
	c.mouseicon=c.name;
	c.tooltip = "GatherWait: Wait until all units arrive before continuing";
	c.hidden = true;
	possibleCommands.push_back(c);
	c.hidden = false;

	if (owner->unitDef->canSelfD) {
		c.id = CMD_SELFD;
		c.action = "selfd";
		c.type = CMDTYPE_ICON;
		c.name = "SelfD";
		c.mouseicon = c.name;
		c.tooltip = "SelfD: Tells the unit to self destruct";
		c.hidden = true;
		possibleCommands.push_back(c);
		c.hidden = false;
	}
//	nonQueingCommands.insert(CMD_SELFD);

	if(CanChangeFireState()) {
		c.id = CMD_FIRE_STATE;
		c.action = "firestate";
		c.type = CMDTYPE_ICON_MODE;
		c.name = "Fire state";
		c.mouseicon = c.name;
		c.params.push_back(IntToString(FIRESTATE_FIREATWILL));
		c.params.push_back("Hold fire");
		c.params.push_back("Return fire");
		c.params.push_back("Fire at will");
		c.tooltip = "Fire State: Sets under what conditions an\n unit will start to fire at enemy units\n without an explicit attack order";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_FIRE_STATE);
	}

	if (owner->unitDef->canmove || owner->unitDef->builder) {
		c.params.clear();
		c.id = CMD_MOVE_STATE;
		c.action = "movestate";
		c.type = CMDTYPE_ICON_MODE;
		c.name = "Move state";
		c.mouseicon = c.name;
		c.params.push_back(IntToString(MOVESTATE_MANEUVER));
		c.params.push_back("Hold pos");
		c.params.push_back("Maneuver");
		c.params.push_back("Roam");
		c.tooltip = "Move State: Sets how far out of its way\n an unit will move to attack enemies";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_MOVE_STATE);
	} else {
		owner->moveState = MOVESTATE_HOLDPOS;
	}

	if (owner->unitDef->canRepeat) {
		c.params.clear();
		c.id = CMD_REPEAT;
		c.action = "repeat";
		c.type = CMDTYPE_ICON_MODE;
		c.name = "Repeat";
		c.mouseicon = c.name;
		c.params.push_back("0");
		c.params.push_back("Repeat off");
		c.params.push_back("Repeat on");
		c.tooltip = "Repeat: If on the unit will continuously\n push finished orders to the end of its\n order queue";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_REPEAT);
	}

	if (owner->unitDef->highTrajectoryType>1) {
		c.params.clear();
		c.id = CMD_TRAJECTORY;
		c.action = "trajectory";
		c.type = CMDTYPE_ICON_MODE;
		c.name = "Trajectory";
		c.mouseicon = c.name;
		c.params.push_back("0");
		c.params.push_back("Low traj");
		c.params.push_back("High traj");
		c.tooltip = "Trajectory: If set to high, weapons that\n support it will try to fire in a higher\n trajectory than usual (experimental)";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_TRAJECTORY);
	}

	if (owner->unitDef->onoffable) {
		c.params.clear();
		c.id = CMD_ONOFF;
		c.action = "onoff";
		c.type = CMDTYPE_ICON_MODE;
		c.name = "Active state";
		c.mouseicon = c.name;

		if (owner->unitDef->activateWhenBuilt) {
			c.params.push_back("1");
		} else {
			c.params.push_back("0");
		}

		c.params.push_back(" Off ");
		c.params.push_back(" On ");

		c.tooltip = "Active State: Sets the active state of the unit to on or off";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_ONOFF);
	}

	if (owner->unitDef->canCloak) {
		c.params.clear();
		c.id = CMD_CLOAK;
		c.action = "cloak";
		c.type = CMDTYPE_ICON_MODE;
		c.name = "Cloak state";
		c.mouseicon = c.name;

		if (owner->unitDef->startCloaked) {
			c.params.push_back("1");
		} else {
			c.params.push_back("0");
		}

		c.params.push_back("UnCloaked");
		c.params.push_back("Cloaked");

		c.tooltip = "Cloak State: Sets whether the unit is cloaked or not";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_CLOAK);
	}
}

CCommandAI::~CCommandAI()
{
	SetOrderTarget(NULL);
	ClearCommandDependencies();
}

void CCommandAI::ClearCommandDependencies() {
	while (!commandDeathDependences.empty()) {
		DeleteDeathDependence(*commandDeathDependences.begin(), DEPENDENCE_COMMANDQUE);
	}
}

void CCommandAI::AddCommandDependency(const Command& c) {
	int cpos;
	if (c.IsObjectCommand(cpos)) {
		int refId = c.params[cpos];
		CObject* ref = (refId < unitHandler->MaxUnits()) ?
				static_cast<CObject*>(unitHandler->GetUnit(refId)) :
				static_cast<CObject*>(featureHandler->GetFeature(refId - unitHandler->MaxUnits()));
		if (ref) {
			AddDeathDependence(ref, DEPENDENCE_COMMANDQUE);
		}
	}
}

vector<CommandDescription>& CCommandAI::GetPossibleCommands()
{
	return possibleCommands;
}


bool CCommandAI::IsAttackCapable() const
{
	const UnitDef* ud = owner->unitDef;
	const bool b = (!ud->weapons.empty() || ud->canKamikaze || (ud->IsFactoryUnit()));

	return (ud->canAttack && b);
}



static inline const CUnit* GetCommandUnit(const Command& c, int idx) {
	if (idx >= c.params.size()) {
		return NULL;
	}

	if (c.IsAreaCommand()) {
		return NULL;
	}

	const CUnit* unit = unitHandler->GetUnit(c.params[idx]);
	return unit;
}

static inline bool IsCommandInMap(const Command& c)
{
	// TODO:
	//   extend the check to commands for which
	//   position is not stored in params[0..2]
	if (c.params.size() < 3) {
		return true;
	}
	if ((c.GetPos(0)).IsInBounds()) {
		return true;
	}
	const float3 pos = c.GetPos(0);
	LOG_L(L_DEBUG, "Dropped command %d: outside of map (x:%f y:%f z:%f)", c.GetID(), pos.x, pos.y, pos.z);

	return false;

}

static inline bool AdjustGroundAttackCommand(const Command& c, bool fromSynced, bool aiOrder)
{
	if (c.params.size() < 3)
		return false;
	if (aiOrder)
		return false;

	const float3 cPos = c.GetPos(0);

	#if 0
	// check if attack-ground is really attack-ground
	//
	// NOTE:
	//     problematic if command contains value from UHM
	//     but is evaluated in synced context against SHM
	//     after roundtrip (when UHM and SHM differ a lot)
	//
	//     instead just clamp the elevation, which creates
	//     fewer issues overall (eg. artillery force-firing
	//     at positions outside LOS where UHM and SHM do not
	//     match will not be broken)
	//
	if (math::fabs(cPos.y - gHeight) > SQUARE_SIZE) {
		return false;
	}
	#else
	// FIXME: is fromSynced really sync-safe???
	// NOTE:
	//   uses gHeight = min(cPos.y, GetHeightAboveWater) instead
	//   of gHeight = GetHeightReal because GuiTraceRay can stop
	//   at water surface, so the attack-position would be moved
	//   UNDERWATER and cause ground-attack orders to fail (this
	//   SHOULD no longer happen, Weapon::AdjustTargetPosToWater
	//   always forcibly adjusts positions to respect waterWeapon
	//   now)
	//
	Command& cc = const_cast<Command&>(c);

	cc.params[1] = std::min(cPos.y, CGround::GetHeightAboveWater(cPos.x, cPos.z, true || fromSynced));
	// cc.params[1] = CGround::GetHeightReal(cPos.x, cPos.z, true || fromSynced);

	return true;
	#endif
}



bool CCommandAI::AllowedCommand(const Command& c, bool fromSynced)
{
	const int cmdID = c.GetID();

	// TODO check if the command is in the map first, for more commands
	switch (cmdID) {
		case CMD_MOVE:
		case CMD_ATTACK:
		case CMD_AREA_ATTACK:
		case CMD_RECLAIM:
		case CMD_REPAIR:
		case CMD_RESURRECT:
		case CMD_PATROL:
		case CMD_RESTORE:
		case CMD_FIGHT:
		case CMD_MANUALFIRE:
		case CMD_UNLOAD_UNIT:
		case CMD_UNLOAD_UNITS: {
			if (!IsCommandInMap(c)) {
				return false;
			}
		} break;

		default: {
			// build commands
			if (cmdID < 0) {
				if (!IsCommandInMap(c))
					return false;

				const CBuilderCAI* bcai = dynamic_cast<const CBuilderCAI*>(this);
				const CFactoryCAI* fcai = dynamic_cast<const CFactoryCAI*>(this);

				// non-builders cannot ever execute these
				// we can get here if a factory is selected along with the
				// unit it is currently building and a build-order is given
				// to the former
				if (fcai == NULL && bcai == NULL)
					return false;

				// {Builder,Factory}CAI::GiveCommandReal (should) handle the
				// case where buildOptions.find(cmdID) == buildOptions.end()
			}
		} break;
	}


	const UnitDef* ud = owner->unitDef;
	// AI's may do as they like
	const CSkirmishAIHandler::ids_t& saids = skirmishAIHandler.GetSkirmishAIsInTeam(owner->team);
	const bool aiOrder = (!saids.empty());

	switch (cmdID) {
		case CMD_MANUALFIRE:
			if (!ud->canManualFire)
				return false;
			// fall through

		case CMD_ATTACK: {
			if (!IsAttackCapable())
				return false;

			if (c.params.size() == 1) {
				const CUnit* attackee = GetCommandUnit(c, 0);

				if (attackee == NULL)
					return false;
				if (!attackee->pos.IsInBounds())
					return false;
			} else {
				AdjustGroundAttackCommand(c, fromSynced, aiOrder);
			}
			break;
		}

		case CMD_MOVE:      if (!ud->canmove)       return false; break;
		case CMD_FIGHT:     if (!ud->canFight)      return false; break;
		case CMD_GUARD: {
			const CUnit* guardee = GetCommandUnit(c, 0);

			if (!ud->canGuard) { return false; }
			if (owner && !owner->pos.IsInBounds()) { return false; }
			if (guardee && !guardee->pos.IsInBounds()) { return false; }
		} break;

		case CMD_PATROL: {
			if (!ud->canPatrol) { return false; }
		} break;

		case CMD_CAPTURE: {
			const CUnit* capturee = GetCommandUnit(c, 0);

			if (!ud->canCapture) { return false; }
			if (capturee && !capturee->pos.IsInBounds()) { return false; }
		} break;

		case CMD_RECLAIM: {
			const CUnit* reclaimeeUnit = GetCommandUnit(c, 0);
			const CFeature* reclaimeeFeature = NULL;

			if (c.IsAreaCommand()) { return true; }
			if (!ud->canReclaim) { return false; }
			if (reclaimeeUnit && !reclaimeeUnit->unitDef->reclaimable) { return false; }
			if (reclaimeeUnit && !reclaimeeUnit->AllowedReclaim(owner)) { return false; }
			if (reclaimeeUnit && !reclaimeeUnit->pos.IsInBounds()) { return false; }

			if (reclaimeeUnit == NULL && !c.params.empty()) {
				const unsigned int reclaimeeFeatureID(c.params[0]);

				if (reclaimeeFeatureID >= unitHandler->MaxUnits()) {
					reclaimeeFeature = featureHandler->GetFeature(reclaimeeFeatureID - unitHandler->MaxUnits());

					if (reclaimeeFeature && !reclaimeeFeature->def->reclaimable) {
						return false;
					}
				}
			}
		} break;

		case CMD_RESTORE: {
			if (!ud->canRestore || mapDamage->disabled) {
				return false;
			}
		} break;

		case CMD_RESURRECT: {
			if (!ud->canResurrect) { return false; }
		} break;

		case CMD_REPAIR: {
			const CUnit* repairee = GetCommandUnit(c, 0);

			if (!ud->canRepair && !ud->canAssist) { return false; }
			if (repairee && !repairee->pos.IsInBounds()) { return false; }
			if (repairee && ((repairee->beingBuilt && !ud->canAssist) || (!repairee->beingBuilt && !ud->canRepair))) { return false; }
		} break;
	}


	if (cmdID == CMD_FIRE_STATE
			&& (c.params.empty() || !CanChangeFireState()))
	{
		return false;
	}
	if (cmdID == CMD_MOVE_STATE
			&& (c.params.empty() || (!ud->canmove && !ud->builder)))
	{
		return false;
	}
	if (cmdID == CMD_REPEAT && (c.params.empty() || !ud->canRepeat || ((int)c.params[0] % 2) != (int)c.params[0]/* only 0 or 1 allowed */))
	{
		return false;
	}
	if (cmdID == CMD_TRAJECTORY
			&& (c.params.empty() || ud->highTrajectoryType < 2))
	{
		return false;
	}
	if (cmdID == CMD_ONOFF
			&& (c.params.empty() || !ud->onoffable || owner->beingBuilt || ((int)c.params[0] % 2) != (int)c.params[0]/* only 0 or 1 allowed */))
	{
		return false;
	}
	if (cmdID == CMD_CLOAK && (c.params.empty() || !ud->canCloak || ((int)c.params[0] % 2) != (int)c.params[0]/* only 0 or 1 allowed */))
	{
		return false;
	}
	if (cmdID == CMD_STOCKPILE && !stockpileWeapon)
	{
		return false;
	}
	return true;
}


void CCommandAI::GiveCommand(const Command& c, bool fromSynced)
{
	if (!eventHandler.AllowCommand(owner, c, fromSynced)) {
		return;
	}
	eventHandler.UnitCommand(owner, c);
	this->GiveCommandReal(c, fromSynced); // send to the sub-classes
}


void CCommandAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	if (!AllowedCommand(c, fromSynced)) {
		return;
	}

	GiveAllowedCommand(c, fromSynced);
}


inline void CCommandAI::SetCommandDescParam0(const Command& c)
{
	for (unsigned int n = 0; n < possibleCommands.size(); n++) {
		if (possibleCommands[n].id != c.GetID())
			continue;

		possibleCommands[n].params[0] = IntToString(int(c.params[0]), "%d");
		break;
	}
}


bool CCommandAI::ExecuteStateCommand(const Command& c)
{
	switch (c.GetID()) {
		case CMD_FIRE_STATE: {
			owner->fireState = (int)c.params[0];
			SetCommandDescParam0(c);
			selectedUnitsHandler.PossibleCommandChange(owner);
			return true;
		}
		case CMD_MOVE_STATE: {
			owner->moveState = (int)c.params[0];
			SetCommandDescParam0(c);
			selectedUnitsHandler.PossibleCommandChange(owner);
			return true;
		}
		case CMD_REPEAT: {
			if (c.params[0] == 1) {
				repeatOrders = true;
			} else if(c.params[0] == 0) {
				repeatOrders = false;
			} else {
				// cause some code parts need it to be either 0 or 1,
				// we can not accept any other values as valid
				return false;
			}
			SetCommandDescParam0(c);
			selectedUnitsHandler.PossibleCommandChange(owner);
			return true;
		}
		case CMD_TRAJECTORY: {
			owner->useHighTrajectory = !!c.params[0];
			SetCommandDescParam0(c);
			selectedUnitsHandler.PossibleCommandChange(owner);
			return true;
		}
		case CMD_ONOFF: {
			if (c.params[0] == 1) {
				owner->Activate();
			} else if (c.params[0] == 0) {
				owner->Deactivate();
			} else {
				// cause some code parts need it to be either 0 or 1,
				// we can not accept any other values as valid
				return false;
			}
			SetCommandDescParam0(c);
			selectedUnitsHandler.PossibleCommandChange(owner);
			return true;
		}
		case CMD_CLOAK: {
			if (c.params[0] == 1) {
				owner->wantCloak = true;
			} else if(c.params[0] == 0) {
				owner->wantCloak = false;
				owner->curCloakTimeout = gs->frameNum + owner->cloakTimeout;
			} else {
				// cause some code parts need it to be either 0 or 1,
				// we can not accept any other values as valid
				return false;
			}
			SetCommandDescParam0(c);
			selectedUnitsHandler.PossibleCommandChange(owner);
			return true;
		}
		case CMD_STOCKPILE: {
			int change = 1;
			if (c.options & RIGHT_MOUSE_KEY) { change *= -1; }
			if (c.options & SHIFT_KEY)       { change *=  5; }
			if (c.options & CONTROL_KEY)     { change *= 20; }
			stockpileWeapon->numStockpileQued += change;
			if (stockpileWeapon->numStockpileQued < 0) {
				stockpileWeapon->numStockpileQued = 0;
			}
			UpdateStockpileIcon();
			return true;
		}
	}
	return false;
}


void CCommandAI::ClearTargetLock(const Command &c) {
	if (((c.GetID() == CMD_ATTACK) || (c.GetID() == CMD_MANUALFIRE)) && (c.options & META_KEY) == 0) {
		// no meta-bit attack lock, clear the order
		owner->AttackUnit(NULL, false, false);
	}
}


void CCommandAI::GiveAllowedCommand(const Command& c, bool fromSynced)
{
	if (ExecuteStateCommand(c)) {
		return;
	}

	switch (c.GetID()) {
		case CMD_SELFD: {
			if (owner->unitDef->canSelfD) {
				if (!(c.options & SHIFT_KEY) || commandQue.empty()) {
					if (owner->selfDCountdown != 0) {
						owner->selfDCountdown = 0;
					} else {
						owner->selfDCountdown = owner->unitDef->selfDCountdown*2+1;
					}
				}
				else if (commandQue.back().GetID() == CMD_SELFD) {
					commandQue.pop_back();
				} else {
					commandQue.push_back(c);
				}
			}
			return;
		}
		case CMD_SET_WANTED_MAX_SPEED: {
			if (CanSetMaxSpeed() &&
			    (commandQue.empty() ||
			     (commandQue.back().GetID() != CMD_SET_WANTED_MAX_SPEED))) {
				// bail early, do not check for overlaps or queue cancelling
				commandQue.push_back(c);
				if (commandQue.size()==1 && !owner->beingBuilt) {
					SlowUpdate();
				}
			}
			return;
		}
		case CMD_WAIT: {
			GiveWaitCommand(c);
			return;
		}
		case CMD_INSERT: {
			ExecuteInsert(c, fromSynced);
			return;
		}
		case CMD_REMOVE: {
			ExecuteRemove(c);
			return;
		}
	}

	// flush the queue for immediate commands
	// NOTE: CMD_STOP can be a queued order (!)
	if (!(c.options & SHIFT_KEY)) {
		waitCommandsAI.ClearUnitQueue(owner, commandQue);
		ClearTargetLock((commandQue.empty())? Command(CMD_STOP): commandQue.front());
		ClearCommandDependencies();
		SetOrderTarget(NULL);

		// if c is an attack command, the actual order-target
		// gets set via ExecuteAttack (called from SlowUpdate
		// at the end of this function)
		commandQue.clear();
		assert(commandQue.empty());

		inCommand = false;
	}

	AddCommandDependency(c);

	if (c.GetID() == CMD_PATROL) {
		CCommandQueue::iterator ci = commandQue.begin();
		for (; ci != commandQue.end() && ci->GetID() != CMD_PATROL; ++ci) {
			// just increment
		}
		if (ci == commandQue.end()) {
			if (commandQue.empty()) {
				Command c2(CMD_PATROL, c.options, owner->pos);
				commandQue.push_back(c2);
			} else {
				do {
					--ci;
					if (ci->params.size() >= 3) {
						Command c2(CMD_PATROL, c.options);
						c2.params = ci->params;
						commandQue.push_back(c2);
						break;
					} else if (ci == commandQue.begin()) {
						Command c2(CMD_PATROL, c.options, owner->pos);
						commandQue.push_back(c2);
						break;
					}
				}
				while (ci != commandQue.begin());
			}
		}
	}

	// cancel duplicated commands
	bool first;
	if (CancelCommands(c, commandQue, first) > 0) {
		if (first) {
			Command stopCommand(CMD_STOP);
			commandQue.push_front(stopCommand);
			SlowUpdate();
		}
		return;
	}

	// do not allow overlapping commands
	if (!GetOverlapQueued(c).empty()) {
		return;
	}

	if (c.GetID() == CMD_ATTACK) {
		// avoid weaponless units moving to 0 distance when given attack order
		if (owner->weapons.empty() && (!owner->unitDef->canKamikaze)) {
			Command c2(CMD_STOP);
			commandQue.push_back(c2);
			return;
		}
	}

	commandQue.push_back(c);

	if (commandQue.size() == 1 && !owner->beingBuilt && !owner->IsStunned()) {
		SlowUpdate();
	}
}


void CCommandAI::GiveWaitCommand(const Command& c)
{
	if (commandQue.empty()) {
		commandQue.push_back(c);
		return;
	}
	else if (c.options & SHIFT_KEY) {
		if (commandQue.back().GetID() == CMD_WAIT) {
			waitCommandsAI.RemoveWaitCommand(owner, commandQue.back());
			commandQue.pop_back();
		} else {
			commandQue.push_back(c);
			return;
		}
	}
	else if (commandQue.front().GetID() == CMD_WAIT) {
		waitCommandsAI.RemoveWaitCommand(owner, commandQue.front());
		commandQue.pop_front();
		return;
	}
	else {
		// shutdown the current order
		owner->AttackUnit(NULL, false, true);
		StopMove();
		inCommand = false;
		targetDied = false;
		unimportantMove = false;
		commandQue.push_front(c);
		return;
	}

	if (commandQue.empty()) {
		if (!owner->group) {
			eoh->UnitIdle(*owner);
		}
		eventHandler.UnitIdle(owner);
	} else {
		SlowUpdate();
	}
}


void CCommandAI::ExecuteInsert(const Command& c, bool fromSynced)
{
	if (c.params.size() < 3) {
		return;
	}

	// make the command
	Command newCmd((int)c.params[1], (unsigned char)c.params[2]);
	for (int p = 3; p < (int)c.params.size(); p++) {
		newCmd.PushParam(c.params[p]);
	}

	// validate the command
	if (!AllowedCommand(newCmd, fromSynced)) {
		return;
	}

	CCommandQueue* queue = &commandQue;

	bool facBuildQueue = false;
	CFactoryCAI* facCAI = dynamic_cast<CFactoryCAI*>(this);
	if (facCAI) {
		if (c.options & CONTROL_KEY) {
			// check the build order
			const map<int, CFactoryCAI::BuildOption>& bOpts = facCAI->buildOptions;
			if ((newCmd.GetID() != CMD_STOP) && (newCmd.GetID() != CMD_WAIT) &&
			    ((newCmd.GetID() >= 0) || (bOpts.find(newCmd.GetID()) == bOpts.end()))) {
				return;
			}
			facBuildQueue = true;
		} else {
			// use the new commands
			queue = &facCAI->newUnitCommands;
		}
	}

	CCommandQueue::iterator insertIt = queue->begin();

	if (c.options & ALT_KEY) {
		// treat param0 as a position
		int pos = (int)c.params[0];
		const unsigned int qsize = queue->size();
		if (pos < 0) {
			pos = qsize + pos + 1; // convert the negative index
			if (pos < 0) {
				pos = 0;
			}
		}
		if (pos > qsize) {
			pos = qsize;
		}
		std::advance(insertIt, pos);
	}
	else {
		// treat param0 as a command tag
		const unsigned int tag = (unsigned int)c.params[0];
		CCommandQueue::iterator ci;
		bool found = false;
		for (ci = queue->begin(); ci != queue->end(); ++ci) {
			const Command& qc = *ci;
			if (qc.tag == tag) {
				insertIt = ci;
				found = true;
				break;
			}
		}
		if (!found) {
			return;
		}
		if ((c.options & RIGHT_MOUSE_KEY) && (insertIt != queue->end())) {
			++insertIt; // insert after the tagged command
		}
	}

	if (facBuildQueue) {
		facCAI->InsertBuildCommand(insertIt, newCmd);
		if (!owner->IsStunned()) {
			SlowUpdate();
		}
		return;
	}

	// shutdown the current order if the insertion is at the beginning
	if (!queue->empty() && (insertIt == queue->begin())) {
		inCommand = false;
		targetDied = false;
		unimportantMove = false;
		SetOrderTarget(NULL);
		const Command& cmd = queue->front();
		eoh->CommandFinished(*owner, cmd);
		eventHandler.UnitCmdDone(owner, cmd);
		ClearTargetLock(cmd);
	}

	queue->insert(insertIt, newCmd);

	if (!owner->IsStunned()) {
		SlowUpdate();
	}
}


void CCommandAI::ExecuteRemove(const Command& c)
{
	CCommandQueue* queue = &commandQue;
	CFactoryCAI* facCAI = dynamic_cast<CFactoryCAI*>(this);

	// if false, remove commands by tag
	const bool removeByID = (c.options & ALT_KEY);
	// disable repeating during the removals
	const bool prevRepeat = repeatOrders;

	// erase commands by a list of command types
	bool active = false;
	bool facBuildQueue = false;

	if (facCAI) {
		if (c.options & CONTROL_KEY) {
			// keep using the build-order queue
			facBuildQueue = true;
		} else {
			// use the command-queue for new units
			queue = &facCAI->newUnitCommands;
		}
	}

	if ((c.params.size() <= 0) || (queue->size() <= 0)) {
		return;
	}

	repeatOrders = false;

	for (unsigned int p = 0; p < c.params.size(); p++) {
		const int removeValue = c.params[p]; // tag or id

		if (facBuildQueue && !removeByID && (removeValue == 0)) {
			// don't remove commands with tag 0 from build queues, they
			// are used the same way that CMD_STOP is (to void orders)
			continue;
		}

		CCommandQueue::iterator ci;

		do {
			for (ci = queue->begin(); ci != queue->end(); ++ci) {
				const Command& qc = *ci;

				if (removeByID) {
					if (qc.GetID() != removeValue)  { continue; }
				} else {
					if (qc.tag != removeValue) { continue; }
				}

				if (qc.GetID() == CMD_WAIT) {
					waitCommandsAI.RemoveWaitCommand(owner, qc);
				}

				if (facBuildQueue) {
					// if ci == queue->begin() and !queue->empty(), this pop_front()'s
					// via CFAI::ExecuteStop; otherwise only modifies *ci (not <queue>)
					if (facCAI->RemoveBuildCommand(ci)) {
						ci = queue->begin(); break;
					}
				}

				if (!facCAI && (ci == queue->begin())) {
					if (!active) {
						active = true;
						FinishCommand();
						ci = queue->begin();
						break;
					}
					active = true;
				}

				queue->erase(ci);
				ci = queue->begin();

				// the removal may have corrupted the iterator
				break;
			}
		}
		while (ci != queue->end());
	}

	repeatOrders = prevRepeat;
}


bool CCommandAI::WillCancelQueued(const Command& c)
{
	return (GetCancelQueued(c, commandQue) != commandQue.end());
}


CCommandQueue::iterator CCommandAI::GetCancelQueued(const Command& c, CCommandQueue& q)
{
	CCommandQueue::iterator ci = q.end();

	while (ci != q.begin()) {
		--ci; //iterate from the end and dont check the current order
		const Command& c2 = *ci;
		const int cmdID = c.GetID();
		const int cmd2ID = c2.GetID();

		const bool attackAndFight = (cmdID == CMD_ATTACK && cmd2ID == CMD_FIGHT && c2.params.size() == 1);

		if (c2.params.size() != c.params.size())
			continue;

		if ((cmdID == cmd2ID) || (cmdID < 0 && cmd2ID < 0) || attackAndFight) {
			if (c.params.size() == 1) {
				// assume the param is a unit-ID or feature-ID
				if ((c2.params[0] == c.params[0]) &&
				    (cmd2ID != CMD_SET_WANTED_MAX_SPEED)) {
					return ci;
				}
			}
			else if (c.params.size() >= 3) {
				if (cmdID < 0) {
					BuildInfo bc1(c);
					BuildInfo bc2(c2);

					if (bc1.def == NULL) continue;
					if (bc2.def == NULL) continue;

					if (math::fabs(bc1.pos.x - bc2.pos.x) * 2 <= std::max(bc1.GetXSize(), bc2.GetXSize()) * SQUARE_SIZE &&
					    math::fabs(bc1.pos.z - bc2.pos.z) * 2 <= std::max(bc1.GetZSize(), bc2.GetZSize()) * SQUARE_SIZE) {
						return ci;
					}
				} else {
					// assume c and c2 are positional commands
					const float3& c1p = c.GetPos(0);
					const float3& c2p = c2.GetPos(0);

					if ((c1p - c2p).SqLength2D() >= (COMMAND_CANCEL_DIST * COMMAND_CANCEL_DIST))
						continue;
					if ((c.options & SHIFT_KEY) != 0 && (c.options & INTERNAL_ORDER) != 0)
						continue;

					return ci;
				}
			}
		}
	}
	return q.end();
}


int CCommandAI::CancelCommands(const Command &c, CCommandQueue& q, bool& first)
{
	first = false;
	int cancelCount = 0;

	while (true) {
		CCommandQueue::iterator ci = GetCancelQueued(c, q);
		if (ci == q.end()) {
			return cancelCount;
		}
		first = first || (ci == q.begin());
		cancelCount++;

		CCommandQueue::iterator firstErase = ci;
		CCommandQueue::iterator lastErase = ci;

		++ci;
		if ((ci != q.end()) && (ci->GetID() == CMD_SET_WANTED_MAX_SPEED)) {
			lastErase = ci;
			cancelCount++;
			++ci;
		}

		if ((ci != q.end()) && (ci->GetID() == CMD_WAIT)) {
			waitCommandsAI.RemoveWaitCommand(owner, *ci);
			lastErase = ci;
			cancelCount++;
			++ci;
		}

		++lastErase; // STL: erase the range [first, last)
		q.erase(firstErase, lastErase);

		if (c.GetID() >= 0) {
			return cancelCount; // only delete one non-build order
		}
	}

	return cancelCount;
}


std::vector<Command> CCommandAI::GetOverlapQueued(const Command& c)
{
	return GetOverlapQueued(c, commandQue);
}


std::vector<Command> CCommandAI::GetOverlapQueued(const Command& c, CCommandQueue& q)
{
	CCommandQueue::iterator ci = q.end();
	std::vector<Command> v;
	BuildInfo cbi(c);

	if (ci != q.begin()) {
		do {
			--ci; //iterate from the end and dont check the current order
			const Command& t = *ci;

			if (t.params.size() != c.params.size())
				continue;

			if (t.GetID() == c.GetID() || (c.GetID() < 0 && t.GetID() < 0)) {
				if (c.params.size() == 1) {
					// assume the param is a unit or feature id
					if (t.params[0] == c.params[0]) {
						v.push_back(t);
					}
				}
				else if (c.params.size() >= 3) {
					// assume c and t are positional commands
					// NOTE: uses a BuildInfo structure, but <t> can be ANY command
					BuildInfo tbi;
					if (tbi.Parse(t)) {
						const float dist2X = 2.0f * math::fabs(cbi.pos.x - tbi.pos.x);
						const float dist2Z = 2.0f * math::fabs(cbi.pos.z - tbi.pos.z);
						const float addSizeX = SQUARE_SIZE * (cbi.GetXSize() + tbi.GetXSize());
						const float addSizeZ = SQUARE_SIZE * (cbi.GetZSize() + tbi.GetZSize());
						const float maxSizeX = SQUARE_SIZE * std::max(cbi.GetXSize(), tbi.GetXSize());
						const float maxSizeZ = SQUARE_SIZE * std::max(cbi.GetZSize(), tbi.GetZSize());

						if (cbi.def == NULL) continue;
						if (tbi.def == NULL) continue;

						if (((dist2X > maxSizeX) || (dist2Z > maxSizeZ)) &&
						    ((dist2X < addSizeX) && (dist2Z < addSizeZ))) {
							v.push_back(t);
						}
					} else {
						if ((cbi.pos - tbi.pos).SqLength2D() >= (COMMAND_CANCEL_DIST * COMMAND_CANCEL_DIST))
							continue;
						if ((c.options & SHIFT_KEY) != 0 && (c.options & INTERNAL_ORDER) != 0)
							continue;

						v.push_back(t);
					}
				}
			}
		} while (ci != q.begin());
	}
	return v;
}


int CCommandAI::UpdateTargetLostTimer(int targetUnitID)
{
	const CUnit* targetUnit = unitHandler->GetUnit(targetUnitID);
	const UnitDef* targetUnitDef = (targetUnit != NULL)? targetUnit->unitDef: NULL;

	if (targetUnit == NULL)
		return (targetLostTimer = 0);

	if (targetUnitDef->IsImmobileUnit())
		return (targetLostTimer = TARGET_LOST_TIMER);

	// keep tracking so long as target is on radar (or indefinitely if immobile)
	if ((targetUnit->losStatus[owner->allyteam] & LOS_INRADAR))
		return (targetLostTimer = TARGET_LOST_TIMER);

	return (std::max(--targetLostTimer, 0));
}


void CCommandAI::ExecuteAttack(Command& c)
{
	assert(owner->unitDef->canAttack);

	if (inCommand) {
		if (targetDied || (c.params.size() == 1 && UpdateTargetLostTimer(int(c.params[0])) == 0)) {
			FinishCommand();
			return;
		}
		if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
			FinishCommand();
			return;
		}
	} else {
		if (c.params.size() == 1) {
			CUnit* targetUnit = unitHandler->GetUnit(c.params[0]);

			if (targetUnit == NULL) { FinishCommand(); return; }
			if (targetUnit == owner) { FinishCommand(); return; }
			if (targetUnit->GetTransporter() != NULL && !modInfo.targetableTransportedUnits) {
				FinishCommand(); return;
			}

			SetOrderTarget(targetUnit);
			owner->AttackUnit(targetUnit, (c.options & INTERNAL_ORDER) == 0, c.GetID() == CMD_MANUALFIRE);
			inCommand = true;
		} else {
			owner->AttackGround(c.GetPos(0), (c.options & INTERNAL_ORDER) == 0, c.GetID() == CMD_MANUALFIRE);
			inCommand = true;
		}
	}
}


void CCommandAI::ExecuteStop(Command &c)
{
	owner->AttackUnit(NULL, false, true);
	std::vector<CWeapon*>::iterator wi;

	for (wi = owner->weapons.begin(); wi != owner->weapons.end(); ++wi) {
		(*wi)->HoldFire();
	}

	FinishCommand();
}


void CCommandAI::SlowUpdate()
{
	if (gs->paused) // Commands issued may invoke SlowUpdate when paused
		return;
	if (commandQue.empty()) {
		return;
	}

	Command& c = commandQue.front();

	switch (c.GetID()) {
		case CMD_WAIT: {
			return;
		}
		case CMD_SELFD: {
			if ((owner->selfDCountdown != 0) || !owner->unitDef->canSelfD) {
				owner->selfDCountdown = 0;
			} else {
				owner->selfDCountdown = (owner->unitDef->selfDCountdown * 2) + 1;
			}
			FinishCommand();
			return;
		}
		case CMD_STOP: {
			ExecuteStop(c);
			return;
		}
		case CMD_ATTACK: {
			ExecuteAttack(c);
			return;
		}
		case CMD_MANUALFIRE: {
			ExecuteAttack(c);
			return;
		}
	}

	if (ExecuteStateCommand(c)) {
		FinishCommand();
		return;
	}

	if (!eventHandler.CommandFallback(owner, c)) {
		return; // luaRules wants the command to stay at the front
	}

	FinishCommand();
}


int CCommandAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	if (pointed) {
		if (!teamHandler->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (IsAttackCapable()) {
				return CMD_ATTACK;
			}
		}
	}
	return CMD_STOP;
}


void CCommandAI::AddDeathDependence(CObject* o, DependenceType dep) {
	if (dep == DEPENDENCE_COMMANDQUE) {
		if (commandDeathDependences.insert(o).second) // prevent multiple dependencies for the same object
			CObject::AddDeathDependence(o, dep);
		return;
	}
	CObject::AddDeathDependence(o, dep);
}


void CCommandAI::DeleteDeathDependence(CObject* o, DependenceType dep) {
	if (dep == DEPENDENCE_COMMANDQUE) {
		if (commandDeathDependences.erase(o))
			CObject::DeleteDeathDependence(o, dep);
		return;
	}
	CObject::DeleteDeathDependence(o,dep);
}


void CCommandAI::DependentDied(CObject* o)
{
	if (o == orderTarget) {
		targetDied = true;
		orderTarget = NULL;
	}

	if (commandDeathDependences.erase(o) && o != owner) {
		CFactoryCAI* facCAI = dynamic_cast<CFactoryCAI*>(this);
		CCommandQueue& dq = facCAI ? facCAI->newUnitCommands : commandQue;
		int lastTag;
		int curTag = -1;
		do {
			lastTag = curTag;
			for (CCommandQueue::iterator qit = dq.begin(); qit != dq.end(); ++qit) {
				Command &c = *qit;
				int cpos;
				if (c.IsObjectCommand(cpos) && (c.params[cpos] == CSolidObject::GetDeletingRefID())) {
					curTag = c.tag;
					Command removeCmd(CMD_REMOVE, 0, curTag);
					ExecuteRemove(removeCmd);
					break;
				}
			}
		} while(curTag != lastTag);
	}
}



void CCommandAI::FinishCommand()
{
	assert(!commandQue.empty());

	const Command cmd = commandQue.front(); //cppcheck false positive, copy is needed here
	const bool dontRepeat = (cmd.options & INTERNAL_ORDER);

	if (repeatOrders
	    && !dontRepeat
	    && (cmd.GetID() != CMD_STOP)
	    && (cmd.GetID() != CMD_PATROL)
	    && (cmd.GetID() != CMD_SET_WANTED_MAX_SPEED)){
		commandQue.push_back(cmd);
	}

	commandQue.pop_front();
	inCommand = false;
	targetDied = false;
	unimportantMove = false;
	SetOrderTarget(NULL);
	eoh->CommandFinished(*owner, cmd);
	eventHandler.UnitCmdDone(owner, cmd);
	ClearTargetLock(cmd);

	if (commandQue.empty()) {
		if (!owner->group) {
			eoh->UnitIdle(*owner);
		}
		eventHandler.UnitIdle(owner);
	}

	// avoid infinite loops
	if (lastFinishCommand != gs->frameNum) {
		lastFinishCommand = gs->frameNum;
		if (!owner->IsStunned()) {
			SlowUpdate();
		}
	}
}

void CCommandAI::AddStockpileWeapon(CWeapon* weapon)
{
	stockpileWeapon=weapon;
	CommandDescription c;
	c.id=CMD_STOCKPILE;
	c.action="stockpile";
	c.type=CMDTYPE_ICON;
	c.name="0/0";
	c.tooltip="Stockpile: Queue up ammunition for later use";
	c.iconname="bitmaps/armsilo1.bmp";
	possibleCommands.push_back(c);
}

void CCommandAI::StockpileChanged(CWeapon* weapon)
{
	UpdateStockpileIcon();
}

void CCommandAI::UpdateStockpileIcon()
{
	for (unsigned int n = 0; n < possibleCommands.size(); n++) {
		if (possibleCommands[n].id != CMD_STOCKPILE)
			continue;

		possibleCommands[n].name =
			IntToString(stockpileWeapon->numStockpiled                                    ) + "/" +
			IntToString(stockpileWeapon->numStockpiled + stockpileWeapon->numStockpileQued);

		selectedUnitsHandler.PossibleCommandChange(owner);
		break;
	}
}

void CCommandAI::WeaponFired(CWeapon* weapon, bool mainWeapon, bool lastSalvo)
{
	// copy: SelectNewAreaAttackTargetOrPos can call
	// FinishCommand which would invalidate a pointer
	const Command c = commandQue.empty()? Command(CMD_STOP): commandQue.front();

	const bool haveGroundAttackCmd = (c.GetID() == CMD_ATTACK && c.GetParamsCount() >= 3);
	const bool haveAreaAttackCmd = (c.GetID() == CMD_AREA_ATTACK);

	bool nextOrder = false;

	if (mainWeapon && lastSalvo && (haveAreaAttackCmd || (haveGroundAttackCmd && HasMoreMoveCommands()))) {
		// if we have an area-attack command (or a regular attack command
		// followed by anything that requires movement) and this was the
		// last salvo of our main weapon, assume we completed an attack
		// (run) on one position and move to the next
		//
		// if we have >= 2 consecutive CMD_ATTACK's, then
		//   SelectNAATP --> FinishCommand (inCommand=false) -->
		//   SlowUpdate --> ExecuteAttack (inCommand=true) -->
		//   queue has advanced
		//
		nextOrder = !SelectNewAreaAttackTargetOrPos(c);
	}

	if (!inCommand)
		return;
	if (!weapon->weaponDef->manualfire || (c.options & META_KEY))
		return;

	if (c.GetID() == CMD_ATTACK || c.GetID() == CMD_MANUALFIRE) {
		if (c.GetParamsCount() < 3) {
			// clear previous target
			owner->AttackUnit(NULL, (c.options & INTERNAL_ORDER) == 0, true);
		} else {
			// not needed in this case
			// owner->AttackGround(ZeroVector, (c.options & INTERNAL_ORDER) == 0, true);
		}

		eoh->WeaponFired(*owner, *(weapon->weaponDef));

		if (!nextOrder) {
			FinishCommand();
		}
	}
}

void CCommandAI::PushOrUpdateReturnFight(const float3& cmdPos1, const float3& cmdPos2)
{
	const float3 pos = ClosestPointOnLine(cmdPos1, cmdPos2, owner->pos);
	Command& c(commandQue.front());
	assert(c.GetID() == CMD_FIGHT && c.params.size() >= 3);

	if (c.params.size() >= 6) {
		c.SetPos(0, pos);
	} else {
		// make the new fight command inherit <c>'s options
		Command c2(CMD_FIGHT, c.options, pos);
		c2.PushPos(c.GetPos(0));
		commandQue.push_front(c2);
	}
}


bool CCommandAI::HasCommand(int cmdID) const {
	if (commandQue.empty())
		return false;
	if (cmdID < 0)
		return ((commandQue.front()).IsBuildCommand());

	return ((commandQue.front()).GetID() == cmdID);
}

bool CCommandAI::HasMoreMoveCommands() const
{
	if (commandQue.size() <= 1)
		return false;

	// skip the first command
	for (CCommandQueue::const_iterator i = ++commandQue.begin(); i != commandQue.end(); ++i) {
		switch (i->GetID()) {
			case CMD_AREA_ATTACK:
			case CMD_ATTACK:
			case CMD_CAPTURE:
			case CMD_FIGHT:
			case CMD_GUARD:
			case CMD_LOAD_UNITS:
			case CMD_MANUALFIRE:
			case CMD_MOVE:
			case CMD_PATROL:
			case CMD_RECLAIM:
			case CMD_REPAIR:
			case CMD_RESTORE:
			case CMD_RESURRECT:
			case CMD_UNLOAD_UNIT:
			case CMD_UNLOAD_UNITS:
				return true;

			case CMD_DEATHWAIT:
			case CMD_GATHERWAIT:
			case CMD_SELFD:
			case CMD_SQUADWAIT:
			case CMD_STOP:
			case CMD_TIMEWAIT:
			case CMD_WAIT:
				return false;

			default:
				// build commands are no different from reclaim or repair commands
				// in that they can require a unit to move, so return true when we
				// have one
				if (i->IsBuildCommand())
					return true;
		}
	}

	return false;
}


bool CCommandAI::SkipParalyzeTarget(const CUnit* target)
{
	// check to see if we are about to paralyze a unit that is already paralyzed
	if ((target == NULL) || (owner->weapons.empty())) {
		return false;
	}
	const CWeapon* w = owner->weapons.front();
	if (!w->weaponDef->paralyzer) {
		return false;
	}
	// visible and stunned?
	if ((target->losStatus[owner->allyteam] & LOS_INLOS) && target->IsStunned() && HasMoreMoveCommands()) {
		return true;
	}
	return false;
}

bool CCommandAI::CanChangeFireState() {
	const UnitDef* ud = owner->unitDef;
	const bool b = (!ud->weapons.empty() || ud->canKamikaze || ud->IsFactoryUnit());

	return (ud->canFireControl && b);
}


void CCommandAI::StopAttackingAllyTeam(int ally)
{
	std::vector<int> todel;

	// erasing in the middle invalidates all iterators
	for (CCommandQueue::iterator it = commandQue.begin(); it != commandQue.end(); ++it) {
		const Command& c = *it;

		if ((c.GetID() == CMD_FIGHT || c.GetID() == CMD_ATTACK) && c.params.size() == 1) {
			const CUnit* target = unitHandler->GetUnit(c.params[0]);

			if (target && target->allyteam == ally) {
				todel.push_back(it - commandQue.begin());
			}
		}
	}
	for (std::vector<int>::reverse_iterator it = todel.rbegin(); it != todel.rend(); ++it) {
		commandQue.erase(commandQue.begin() + *it);
	}
}



void CCommandAI::SetScriptMaxSpeed(float speed, bool persistent) {
	if (!persistent) {
		// find the first CMD_SET_WANTED_MAX_SPEED and modify it
		// NOTE:
		//     this has no effect if the unit does not already have
		//     such an order, and only lasts until a new move-order
		//     is given (hence non-persistent)
		CCommandQueue::iterator it;

		for (it = commandQue.begin(); it != commandQue.end(); ++it) {
			Command& c = *it;

			if (c.GetID() != CMD_SET_WANTED_MAX_SPEED)
				continue;

			owner->moveType->SetWantedMaxSpeed(c.params[0] = speed);
			break;
		}
	} else {
		// permanently change the unit's speed
		owner->moveType->SetMaxSpeed(speed);
	}
}

void CCommandAI::SlowUpdateMaxSpeed() {
	if (commandQue.size() < 2)
		return;

	// grab the second command
	const CCommandQueue::const_iterator it = ++(commandQue.begin());
	const Command& c = *it;

	// treat any following CMD_SET_WANTED_MAX_SPEED commands as options
	// to the current command (and ignore them when it's their turn)
	if (c.GetID() != CMD_SET_WANTED_MAX_SPEED)
		return;

	assert(!c.params.empty());
	owner->moveType->SetWantedMaxSpeed(c.params[0]);
}


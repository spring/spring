
#include <string>
#include <vector>
#include <map>

#include "Platform/errorhandler.h"
#include "command.h"

using namespace std;

//cmds lower than 0 is reserved for build options (cmd -x = unitdefs[x])

static map<string, int> actionToId;
static map<int, string> idToAction;
static map<int, CommandDescription> idToCommandDescription;

struct CommandBinding {
  int id;
  const char* action;
  const char* hotkey;
}
commandBindings[] = {
  { CMD_STOP,                  "stop",               "s"          },
	{ CMD_WAIT,                  "wait",               "w"          },
	{ CMD_MOVE,                  "move",               "m"          },
	{ CMD_PATROL,                "patrol",             "p"          },
	{ CMD_FIGHT,                 "fight",              "f"          },
	{ CMD_ATTACK,                "attack",             "a"          },
	{ CMD_AREA_ATTACK,           "areaattack",         "Ctrl+a"     },
	{ CMD_GUARD,                 "guard",              "g"          },
	{ CMD_AISELECT,              "aiselect",           ""           },
	{ CMD_GROUPSELECT,           "groupselect",        ""           },
	{ CMD_GROUPADD,              "groupadd",           ""           },
	{ CMD_GROUPCLEAR,            "groupclear",         ""           },
	{ CMD_REPAIR,                "repair",             ""           },
	{ CMD_FIRE_STATE,            "firestate",          ""           },
	{ CMD_MOVE_STATE,            "movestate",          ""           },
	{ CMD_SETBASE,               "setbase",            ""           },
	{ CMD_INTERNAL,              "internal",           ""           },
	{ CMD_SELFD,                 "selfd",              "Ctrl+d"     },
	{ CMD_SET_WANTED_MAX_SPEED,  "setwantedmaxspeed",  ""           },
	{ CMD_LOAD_UNITS,            "loadunits",          "l"          },
	{ CMD_UNLOAD_UNITS,          "unloadunits",        "u"          },
	{ CMD_UNLOAD_UNIT,           "unloadunit",         "Ctrl+u"     },
	{ CMD_ONOFF,                 "onoff",              "x"          },
	{ CMD_RECLAIM,               "reclaim",            ""           },
	{ CMD_CLOAK,                 "cloak",              "k"          },
	{ CMD_STOCKPILE,             "stockpile",          ""           },
	{ CMD_DGUN,                  "dgun",               "d"          },
	{ CMD_RESTORE,               "restore",            ""           },
	{ CMD_REPEAT,                "repeat",             ""           },
	{ CMD_TRAJECTORY,            "trajectory",         "j"          },
	{ CMD_RESURRECT,             "resurrect",          ""           },
	{ CMD_CAPTURE,               "capture",            "c"          },
	{ CMD_AUTOREPAIRLEVEL,       "autorepairlevel",    ""           },
	{ CMD_LOOPBACKATTACK,        "loopbackattack",     ""           }
};
const int commandBindingsCount = sizeof(commandBindings) / sizeof(commandBindings[0]);

/******************************************************************************/       

void CommandDescription::Init()
{
  actionToId.clear();
  idToAction.clear();
  idToCommandDescription.clear();

	for (int i = 0; i < commandBindingsCount; i++) {
	  CommandBinding& cb = commandBindings[i];
		actionToId[cb.action] = cb.id;
		idToAction[cb.id] = cb.action;
	}

	
	CommandDescription c;

	// common settings
	c.onlyKey = false;
	c.showUnique = false;
	c.iconname = "";
	c.mouseicon = "";

	// STOP
	c.id = CMD_STOP;
	c.type = CMDTYPE_ICON;
	c.action = "stop";
	c.hotkey = "s";
	c.name = "Stop";
	c.tooltip = "Stop: Cancel the units current actions";
	idToCommandDescription[c.id] = c;

	// WAIT	
 	c.id=CMD_WAIT;
 	c.type=CMDTYPE_ICON;
	c.action="wait";
 	c.name="Wait";
 	c.hotkey="w";
 	c.tooltip="Wait: Tells the unit to wait until another units handles him";
	c.onlyKey=true;
	idToCommandDescription[c.id] = c;
	c.onlyKey=false;

	// MOVE
	c.id = CMD_MOVE;
	c.type = CMDTYPE_ICON_MAP;
	c.action = "move";
	c.hotkey = "m";
	c.name = "Move";
	c.tooltip = "Move: Order ready built units to move to a position";
	
	// PATROL
	c.id=CMD_PATROL;
	c.type=CMDTYPE_ICON_MAP;
	c.action="patrol";
	c.name="Patrol";
	c.hotkey="p";
	c.tooltip="Patrol: Sets the aircraft to patrol a path to one or more waypoints";
	idToCommandDescription[c.id] = c;

	// FIGHT	
	c.id = CMD_FIGHT;
	c.type = CMDTYPE_ICON_MAP;
	c.action="fight";
	c.name = "Fight";
	c.hotkey = "f";
	c.tooltip = "Fight: Order the aircraft to take action while moving to a position";
	idToCommandDescription[c.id] = c;

	// ATTACK
	c.id = CMD_ATTACK;
	c.type = CMDTYPE_ICON_UNIT_OR_MAP;
	c.action = "attack";
	c.hotkey = "a";
	c.name = "Attack";
	c.tooltip = "Attack: Attacks an unit or a position on the ground";
	idToCommandDescription[c.id] = c;

	// AREA_ATTACK	
	c.id=CMD_AREA_ATTACK;
	c.type=CMDTYPE_ICON_AREA;
	c.action="areaattack";
	c.name="Area attack";
	c.hotkey="Alt+a";
	c.tooltip="Sets the aircraft to attack enemy units within a circle";
	idToCommandDescription[c.id] = c;

	// LOOPBACKATTACK
	c.id=CMD_LOOPBACKATTACK;
	c.type=CMDTYPE_ICON_MODE;
	c.action="loopbackattack";
	c.hotkey="";
	c.name="Loopback";
	c.tooltip="Loopback attack: Sets if the aircraft should loopback after an attack instead of overflying target";
	c.params.push_back("0");
	c.params.push_back("Normal");
	c.params.push_back("Loopback");
	idToCommandDescription[c.id] = c;
	c.params.clear();
	
	// GUARD
	c.id=CMD_GUARD;
	c.type=CMDTYPE_ICON_UNIT;
	c.action="guard";
	c.name="Guard";
	c.hotkey="g";
	c.tooltip="Guard: Order a unit to guard another unit and attack units attacking it";
	idToCommandDescription[c.id] = c;

	// DGUN
	c.id = CMD_DGUN;
	c.type = CMDTYPE_ICON_UNIT_OR_MAP;
	c.action = "dgun";
	c.hotkey = "d";
	c.name = "DGun";
	c.tooltip = "DGun: Attacks using the units special weapon";
	idToCommandDescription[c.id] = c;

	// AUTOREPAIRLEVEL
	c.id=CMD_AUTOREPAIRLEVEL;
	c.type=CMDTYPE_ICON_MODE;
	c.action="autorepairlevel";
	c.hotkey="";
	c.name="Repair level";
	c.tooltip="Repair level: Sets at which health level an aircraft will try to find a repair pad";
	c.params.push_back("1");
	c.params.push_back("LandAt 0");
	c.params.push_back("LandAt 30");
	c.params.push_back("LandAt 50");
	c.params.clear();
	idToCommandDescription[c.id] = c;

	// SELFD	
	c.id=CMD_SELFD;
	c.type=CMDTYPE_ICON;
	c.action="selfd";
	c.name="SelfD";
	c.hotkey="Ctrl+d";
	c.tooltip="SelfD: Tells the unit to self destruct";
	c.onlyKey=true;
	idToCommandDescription[c.id] = c;
	c.onlyKey=false;

	// FIRE_STATE
	c.id=CMD_FIRE_STATE;
	c.type=CMDTYPE_ICON_MODE;
	c.action="firestate";
	c.name="Fire state";
	c.hotkey="";
	c.tooltip="Fire State: Sets under what conditions an\n"
	          " unit will start to fire at enemy units\n"
	          " without an explicit attack order";
	c.params.push_back("2");
	c.params.push_back("Hold fire");
	c.params.push_back("Return fire");
	c.params.push_back("Fire at will");
	idToCommandDescription[c.id] = c;
	c.params.clear();

	// MOVE_STATE
	c.id=CMD_MOVE_STATE;
	c.type=CMDTYPE_ICON_MODE;
	c.action="movestate";
	c.hotkey="";
	c.name="Move state";
	c.tooltip="Move State: Sets how far out of its way\n"
	          " an unit will move to attack enemies";
	c.params.push_back("1");
	c.params.push_back("Hold pos");
	c.params.push_back("Maneuver");
	c.params.push_back("Roam");
	idToCommandDescription[c.id] = c;
	c.params.clear();

	// REPEAT
	c.id=CMD_REPEAT;
	c.type=CMDTYPE_ICON_MODE;
	c.action="repeat";
	c.name="Repeat";
	c.tooltip="Repeat: If on the unit will continously\n"
	          " push finished orders to the end of its\n order que";
	c.params.push_back("0");
	c.params.push_back("Repeat off");
	c.params.push_back("Repeat on");
	idToCommandDescription[c.id] = c;
	c.params.clear();

	// CLOAK    (set param[0] to (owner->unitDef->startCloaked ? "1" : "0"))
	c.id=CMD_CLOAK;
	c.type=CMDTYPE_ICON_MODE;
	c.action="cloak";
	c.hotkey="k";
	c.name="Cloak state";
	c.tooltip="Cloak State: Sets wheter the unit is cloaked or not";
	c.params.push_back("0"); // See Note ^^^
	c.params.push_back("UnCloaked");
	c.params.push_back("Cloaked");
	idToCommandDescription[c.id] = c;
	c.params.clear();

	// ONOFF    (set param[0] to (owner->unitDef->activateWhenBuilt ? "1" : "0"))
	c.id=CMD_ONOFF;
	c.type=CMDTYPE_ICON_MODE;
	c.action="onoff";
	c.hotkey="x";
	c.name="Active state";
	c.tooltip="Active State: Sets the active state of the unit to on or off";
	c.params.push_back("0");
	c.params.push_back("Off");
	c.params.push_back("On");
	idToCommandDescription[c.id] = c;
	c.params.clear();
	
	// TRAJECTORY
	c.id=CMD_TRAJECTORY;
	c.type=CMDTYPE_ICON_MODE;
	c.action="trajectory";
	c.hotkey="";
	c.name="Move state";
	c.tooltip="Trajectory: If set to high, weapons that\n"
	          " support it will try to fire in a higher\n"
	          " trajectory than usual (experimental)";
	c.params.push_back("0");
	c.params.push_back("Low traj");
	c.params.push_back("High traj");
	idToCommandDescription[c.id] = c;
	c.params.clear();

	// REPAIR
	c.id=CMD_REPAIR;
	c.type=CMDTYPE_ICON_UNIT_OR_AREA;
	c.action="repair";
	c.hotkey="r";
	c.name="Repair";
	c.tooltip="Repair: Repairs another unit";
	idToCommandDescription[c.id] = c;

	// RECLAIM
	c.id=CMD_RECLAIM;
	c.type=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
	c.action="reclaim";
	c.hotkey="e";
	c.name="Reclaim";
	c.tooltip="Reclaim: Sucks in the metal/energy content of a unit/feature and add it to your storage";
	idToCommandDescription[c.id] = c;

	// RESTORE
	c.id=CMD_RESTORE;
	c.type=CMDTYPE_ICON_AREA;
	c.action="restore";
	c.hotkey="";
	c.name="Restore";
	c.tooltip="Restore: Restores an area of the map to its original height";
	c.params.push_back("200");
	idToCommandDescription[c.id] = c;
	c.params.clear();

	// RESURRECT	
	c.id=CMD_RESURRECT;
	c.type=CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
	c.action="resurrect";
	c.hotkey="";
	c.name="Resurrect";
	c.tooltip="Resurrect: Resurrects a unit from a feature";
	idToCommandDescription[c.id] = c;

	// CAPTURE
	c.id=CMD_CAPTURE;
	c.type=CMDTYPE_ICON_UNIT_OR_AREA;
	c.action="capture";
	c.hotkey="";
	c.name="Capture";
	c.tooltip="Capture: Captures a unit from the enemy";
	idToCommandDescription[c.id] = c;

	// LOAD_UNITS
	c.id=CMD_LOAD_UNITS;
	c.type=CMDTYPE_ICON_UNIT_OR_AREA;
	c.action="loadunits";
	c.hotkey="l";
	c.name="Load units";
	c.tooltip="Sets the transport to load a unit or units within an area";
	idToCommandDescription[c.id] = c;

	// UNLOAD_UNITS	
	c.id=CMD_UNLOAD_UNITS;
	c.type=CMDTYPE_ICON_AREA;
	c.action="unloadunits";
	c.name="Unload units";
	c.hotkey="u";
	c.tooltip="Sets the transport to unload units in an area";
	idToCommandDescription[c.id] = c;

	// AISELECT
	c.id=CMD_AISELECT;
	c.type=CMDTYPE_COMBO_BOX;
	c.action="aiselect";
	c.hotkey="Ctrl+q";
	c.name="Select AI";
	c.tooltip="Create a new group using the selected units and with the ai selected";
	c.params.push_back("0");
	c.params.push_back("None");
	c.showUnique = true;
	idToCommandDescription[c.id] = c;
	c.showUnique = false;
	c.params.clear();

	// GROUPCLEAR	
	c.id=CMD_GROUPCLEAR;
	c.type=CMDTYPE_ICON;
	c.action="groupclear";
	c.hotkey="Shift+q";
	c.name="Clear group";
	c.tooltip="Removes the units from any group they belong to";
	idToCommandDescription[c.id] = c;

	// GROUPADD	
	c.id=CMD_GROUPADD;
	c.type=CMDTYPE_ICON;
	c.action="groupadd";
	c.hotkey="q";
	c.name="Add to group";
	c.tooltip="Adds the selected to an existing group (of which one or more units is already selected)";
	idToCommandDescription[c.id] = c;

	// GROUPSELECT	
	c.id=CMD_GROUPSELECT;
	c.type=CMDTYPE_ICON;
	c.action="groupselect";
	c.hotkey="q";
	c.name="Select group";
	c.tooltip="Select the group that these units belong to";
	idToCommandDescription[c.id] = c;

	// CMD_STOCKPILE
	c.id=CMD_STOCKPILE;
	c.type=CMDTYPE_ICON;
	c.hotkey="stockpile";
	c.name="0/0";
	c.hotkey="";
	c.tooltip="Stockpile: Queue up ammunition for later use";
	c.iconname="bitmaps/flare.bmp";
	idToCommandDescription[c.id] = c;
	c.iconname="";


}


bool CommandDescription::GetId(const string& a, int& id)
{
	map<string, int>::const_iterator it = actionToId.find(a);
	if (it == actionToId.end()) {
	  return false;
	}
	id = it->second;
	return true;
}


bool CommandDescription::GetAction(int id, string& a)
{
	map<int, string>::const_iterator it = idToAction.find(id);
	if (it == idToAction.end()) {
	  return false;
	}
	a = it->second;
	return true;
}


bool CommandDescription::SetupCommandDefaults(int cmd)
{
	// reset the definition
	id = 0;
	type = 0;
	action = "";
	hotkey = "";
	name = "";
	tooltip = "";
	iconname = "";
	mouseicon = "";
	onlyKey = false;
	showUnique = false;
	params.clear();
	
	map<int, CommandDescription>::const_iterator it =
	  idToCommandDescription.find(cmd);
	if (it == idToCommandDescription.end()) {
		char buf[16];
		sprintf(buf, "%i", cmd);
		handleerror(0, "CommandDescription::Init()  missing entry: ", buf, 0);
		return false;
	}
	
	*this = it->second;
	
	return true;
}


/******************************************************************************/       



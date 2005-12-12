#ifndef COMMAND_H
#define COMMAND_H

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>

using namespace std;

//cmds lower than 0 is reserved for build options (cmd -x = unitdefs[x])
#define CMD_STOP 0
#define CMD_WAIT 5
#define CMD_MOVE 10
#define CMD_PATROL 15
#define CMD_ATTACK 20
#define CMD_AREA_ATTACK 21
#define CMD_GUARD 25
#define CMD_AISELECT 30
#define CMD_GROUPSELECT 35
#define CMD_GROUPADD 36
#define CMD_GROUPCLEAR 37
#define CMD_REPAIR 40
#define CMD_FIRE_STATE 45
#define CMD_MOVE_STATE 50
#define CMD_SETBASE 55
#define CMD_INTERNAL 60
#define CMD_SELFD 65
#define CMD_SET_WANTED_MAX_SPEED 70
#define CMD_LOAD_UNITS 75
#define CMD_UNLOAD_UNITS 80
#define CMD_UNLOAD_UNIT 81
#define CMD_ONOFF 85
#define CMD_RECLAIM 90
#define CMD_CLOAK 95
#define CMD_STOCKPILE 100
#define CMD_DGUN 105
#define CMD_RESTORE 110
#define CMD_REPEAT 115
#define CMD_TRAJECTORY 120
#define CMD_RESURRECT 125
#define CMD_CAPTURE 130
#define CMD_AUTOREPAIRLEVEL 135

#define CMDTYPE_ICON 0																		//expects 0 parameters in return
#define CMDTYPE_ICON_MODE 5																//expect 1 parameter in return (number selected mode)
#define CMDTYPE_ICON_MAP 10																//expect 3 parameters in return (mappos)
#define CMDTYPE_ICON_AREA 11															//expect 4 parameters in return (mappos+radius)
#define CMDTYPE_ICON_UNIT 12															//expect 1 parameters in return (unitid)
#define CMDTYPE_ICON_UNIT_OR_MAP 13												//expect 1 parameters in return (unitid) or 3 parameters in return (mappos)
#define CMDTYPE_ICON_FRONT 14															//expect 3 or 6 parameters in return (middle of front and right side of front if a front was defined)
#define CMDTYPE_COMBO_BOX 15															//expect 1 parameter in return (number selected option)
#define CMDTYPE_ICON_UNIT_OR_AREA 16											//expect 1 parameter in return (unitid) or 4 parameters in return (mappos+radius)
#define CMDTYPE_NEXT 17
#define CMDTYPE_PREV 18
#define CMDTYPE_ICON_UNIT_FEATURE_OR_AREA 19							//expect 1 parameter in return (unitid or featureid+MAX_UNITS (id>MAX_UNITS=feature)) or 4 parameters in return (mappos+radius)
#define CMDTYPE_ICON_BUILDING 20													//expect 3 parameters in return (mappos)

//bits for the option field of Command
#define INTERNAL_ORDER 8
#define RIGHT_MOUSE_KEY 16
#define SHIFT_KEY 32
#define CONTROL_KEY 64
#define ALT_KEY 128

struct CommandDescription {
	int id;
	int type;
	string name;
	string iconname;
	string mouseicon;
	string tooltip;

	vector<string> params;
	bool showUnique;
	bool onlyKey;											//if true dont show a button for the command
	unsigned char key;
	unsigned char switchKeys;					//uses SHIFT_KEY etc

	CommandDescription()
		: showUnique(false),
		key(0),
		switchKeys(0),
		onlyKey(false)
		{};
};

struct Command {
	Command():
		timeOut(INT_MAX),
		options(0){};
	int id;
	vector<float> params;
	unsigned char options;
	int timeOut;									//remove this command after this frame, can only be set localy not sent over net etc (used for temporary orders)
};

#endif /* COMMAND_H */

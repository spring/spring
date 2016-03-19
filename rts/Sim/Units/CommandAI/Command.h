/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <climits> // for INT_MAX

#ifdef BUILDING_AI
#include <vector>
#else
#include "System/SafeVector.h"
#endif

#include "System/creg/creg_cond.h"
#include "System/float3.h"



// ID's lower than 0 are reserved for build options (cmd -x = unitdefs[x])
#define CMD_STOP                   0
#define CMD_INSERT                 1
#define CMD_REMOVE                 2
#define CMD_WAIT                   5
#define CMD_TIMEWAIT               6
#define CMD_DEATHWAIT              7
#define CMD_SQUADWAIT              8
#define CMD_GATHERWAIT             9
#define CMD_MOVE                  10
#define CMD_PATROL                15
#define CMD_FIGHT                 16
#define CMD_ATTACK                20
#define CMD_AREA_ATTACK           21
#define CMD_GUARD                 25
#define CMD_AISELECT              30 //FIXME REMOVE
#define CMD_GROUPSELECT           35
#define CMD_GROUPADD              36
#define CMD_GROUPCLEAR            37
#define CMD_REPAIR                40
#define CMD_FIRE_STATE            45
#define CMD_MOVE_STATE            50
#define CMD_SETBASE               55
#define CMD_INTERNAL              60
#define CMD_SELFD                 65
#define CMD_SET_WANTED_MAX_SPEED  70
#define CMD_LOAD_UNITS            75
#define CMD_LOAD_ONTO             76
#define CMD_UNLOAD_UNITS          80
#define CMD_UNLOAD_UNIT           81
#define CMD_ONOFF                 85
#define CMD_RECLAIM               90
#define CMD_CLOAK                 95
#define CMD_STOCKPILE            100
#define CMD_MANUALFIRE           105
#define CMD_RESTORE              110
#define CMD_REPEAT               115
#define CMD_TRAJECTORY           120
#define CMD_RESURRECT            125
#define CMD_CAPTURE              130
#define CMD_AUTOREPAIRLEVEL      135
#define CMD_IDLEMODE             145
#define CMD_FAILED               150

#define CMDTYPE_ICON                        0  // expect 0 parameters in return
#define CMDTYPE_ICON_MODE                   5  // expect 1 parameter in return (number selected mode)
#define CMDTYPE_ICON_MAP                   10  // expect 3 parameters in return (mappos)
#define CMDTYPE_ICON_AREA                  11  // expect 4 parameters in return (mappos+radius)
#define CMDTYPE_ICON_UNIT                  12  // expect 1 parameters in return (unitid)
#define CMDTYPE_ICON_UNIT_OR_MAP           13  // expect 1 parameters in return (unitid) or 3 parameters in return (mappos)
#define CMDTYPE_ICON_FRONT                 14  // expect 3 or 6 parameters in return (middle of front and right side of front if a front was defined)
#define CMDTYPE_COMBO_BOX                  15  // expect 1 parameter in return (number selected option)
#define CMDTYPE_ICON_UNIT_OR_AREA          16  // expect 1 parameter in return (unitid) or 4 parameters in return (mappos+radius)
#define CMDTYPE_NEXT                       17  // used with CMD_INTERNAL
#define CMDTYPE_PREV                       18  // used with CMD_INTERNAL
#define CMDTYPE_ICON_UNIT_FEATURE_OR_AREA  19  // expect 1 parameter in return (unitid or featureid+unitHandler->MaxUnits() (id>unitHandler->MaxUnits()=feature)) or 4 parameters in return (mappos+radius)
#define CMDTYPE_ICON_BUILDING              20  // expect 3 parameters in return (mappos)
#define CMDTYPE_CUSTOM                     21  // used with CMD_INTERNAL
#define CMDTYPE_ICON_UNIT_OR_RECTANGLE     22  // expect 1 parameter in return (unitid)
                                               //     or 3 parameters in return (mappos)
                                               //     or 6 parameters in return (startpos+endpos)
#define CMDTYPE_NUMBER                     23  // expect 1 parameter in return (number)


// wait codes
#define CMD_WAITCODE_TIMEWAIT    1.0f
#define CMD_WAITCODE_DEATHWAIT   2.0f
#define CMD_WAITCODE_SQUADWAIT   3.0f
#define CMD_WAITCODE_GATHERWAIT  4.0f


// bits for the option field of Command
// NOTE:
//   these names are misleading, eg. the SHIFT_KEY bit
//   really means that an order gets queued instead of
//   executed immediately (a better name for it would
//   be QUEUED_ORDER), ALT_KEY in most contexts means
//   OVERRIDE_QUEUED_ORDER, etc.
//
#define META_KEY        (1 << 2) //   4
#define INTERNAL_ORDER  (1 << 3) //   8
#define RIGHT_MOUSE_KEY (1 << 4) //  16
#define SHIFT_KEY       (1 << 5) //  32
#define CONTROL_KEY     (1 << 6) //  64
#define ALT_KEY         (1 << 7) // 128


#ifdef __GNUC__
	#define _deprecated __attribute__ ((deprecated))
#else
	#define _deprecated
#endif

#if defined(BUILDING_AI)
#define CMD_DGUN CMD_MANUALFIRE
#endif

enum {
	MOVESTATE_NONE     = -1,
	MOVESTATE_HOLDPOS  =  0,
	MOVESTATE_MANEUVER =  1,
	MOVESTATE_ROAM     =  2,
};
enum {
	FIRESTATE_NONE       = -1,
	FIRESTATE_HOLDFIRE   =  0,
	FIRESTATE_RETURNFIRE =  1,
	FIRESTATE_FIREATWILL =  2,
	FIRESTATE_FIREATNEUTRAL =  3,
};




#if (defined(__cplusplus))
extern "C" {
#endif

	// this must have C linkage
	struct RawCommand {
		int id;
		int aiCommandId;
		int timeOut;

		unsigned int numParams;

		/// unique id within a CCommandQueue
		unsigned int tag;

		/// option bits (RIGHT_MOUSE_KEY, ...)
		unsigned char options;

		/// command parameters
		float* params;
	};

#if (defined(__cplusplus))
// extern "C"
};
#endif



struct Command
{
private:
	CR_DECLARE_STRUCT(Command)

public:
	Command()
		: id(0)
		, aiCommandId(-1)
	{
		SetFlags(INT_MAX, 0, 0);
	}

	Command(const Command& c) {
		*this = c;
	}

	Command& operator = (const Command& c) {
		id = c.id;
		aiCommandId = c.aiCommandId;

		SetFlags(c.timeOut, c.tag, c.options);

		params = c.params;
		return *this;
	}

	Command(const float3& pos)
		: id(0)
		, aiCommandId(-1)
	{
		PushPos(pos);
		SetFlags(INT_MAX, 0, 0);
	}

	Command(const int cmdID)
		: id(cmdID)
		, aiCommandId(-1)
	{
		SetFlags(INT_MAX, 0, 0);
	}

	Command(const int cmdID, const float3& pos)
		: id(cmdID)
		, aiCommandId(-1)
	{
		PushPos(pos);
		SetFlags(INT_MAX, 0, 0);
	}

	Command(const int cmdID, const unsigned char cmdOptions)
		: id(cmdID)
		, aiCommandId(-1)
	{
		SetFlags(INT_MAX, 0, cmdOptions);
	}

	Command(const int cmdID, const unsigned char cmdOptions, const float param)
		: id(cmdID)
		, aiCommandId(-1)
	{
		PushParam(param);
		SetFlags(INT_MAX, 0, cmdOptions);
	}

	Command(const int cmdID, const unsigned char cmdOptions, const float3& pos)
		: id(cmdID)
		, aiCommandId(-1)
	{
		PushPos(pos);
		SetFlags(INT_MAX, 0, cmdOptions);
	}

	Command(const int cmdID, const unsigned char cmdOptions, const float param, const float3& pos)
		: id(cmdID)
		, aiCommandId(-1)
	{
		PushParam(param);
		PushPos(pos);
		SetFlags(INT_MAX, 0, cmdOptions);
	}



	RawCommand ToRawCommand() {
		RawCommand rc;
		rc.id          = id;
		rc.aiCommandId = aiCommandId;
		rc.timeOut     = timeOut;

		rc.numParams   = params.size();
		rc.tag         = tag;
		rc.options     = options;
		rc.params      = &params[0];
		return rc;
	}

	void FromRawCommand(const RawCommand& rc) {
		id          = rc.id;
		aiCommandId = rc.aiCommandId;

		SetFlags(rc.timeOut, rc.tag, rc.options);
		params.reserve(rc.numParams);

		for (unsigned int n = 0; n < rc.numParams; n++) {
			PushParam(rc.params[n]);
		}
	}


	// returns true if the command references another object and
	// in this case also returns the param index of the object in cpos
	bool IsObjectCommand(int& cpos) const {
		const unsigned int psize = params.size();

		switch (id) {
			case CMD_ATTACK:
			case CMD_FIGHT:
			case CMD_MANUALFIRE:
				cpos = 0;
				return (1 <= psize && psize < 3);
			case CMD_GUARD:
			case CMD_LOAD_ONTO:
				cpos = 0;
				return (psize >= 1);
			case CMD_CAPTURE:
			case CMD_LOAD_UNITS:
			case CMD_RECLAIM:
			case CMD_REPAIR:
			case CMD_RESURRECT:
				cpos = 0;
				return (1 <= psize && psize < 4);
			case CMD_UNLOAD_UNIT:
				cpos = 3;
				return (psize >= 4);
			case CMD_INSERT: {
				if (psize < 3)
					return false;

				Command icmd((int)params[1], (unsigned char)params[2]);

				for (int p = 3; p < psize; p++)
					icmd.params.push_back(params[p]);

				if (!icmd.IsObjectCommand(cpos))
					return false;

				cpos += 3;
				return true;
			}
		}
		return false;
	}

	bool IsMoveCommand() const {
		switch (id) {
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
				return IsBuildCommand();
		}
	}

	bool IsAreaCommand() const {
		switch (id) {
			case CMD_CAPTURE:
			case CMD_LOAD_UNITS:
			case CMD_RECLAIM:
			case CMD_REPAIR:
			case CMD_RESURRECT:
				// params[0..2] always holds the position, params[3] the radius
				return (params.size() == 4);
			case CMD_UNLOAD_UNITS:
				return (params.size() == 5);
			case CMD_AREA_ATTACK:
				return true;
		}
		return false;
	}
	bool IsBuildCommand() const { return (id < 0); }

	void PushParam(float par) { params.push_back(par); }
	float GetParam(size_t idx) const { return params[idx]; }

	/// const safe_vector<float>& GetParams() const { return params; }
	const size_t GetParamsCount() const { return params.size(); }

	void SetID(int id) _deprecated { this->id = id; params.clear(); }
	int GetID() const { return id; }

	void PushPos(const float3& pos)
	{
		params.push_back(pos.x);
		params.push_back(pos.y);
		params.push_back(pos.z);
	}

	void PushPos(const float* pos)
	{
		params.push_back(pos[0]);
		params.push_back(pos[1]);
		params.push_back(pos[2]);
	}

	float3 GetPos(const int idx) const {
		float3 p;
		p.x = params[idx    ];
		p.y = params[idx + 1];
		p.z = params[idx + 2];
		return p;
	}

	void SetPos(const int idx, const float3& p) {
		if (params.size() < 3)
			params.resize(3);
		params[idx    ] = p.x;
		params[idx + 1] = p.y;
		params[idx + 2] = p.z;
	}

	void SetFlags(int cmdTimeOut, int cmdTag, int cmdOpts) {
		timeOut = cmdTimeOut;
		tag     = cmdTag;
		options = cmdOpts;
	}

public:
	/// CMD_xxx code  (custom codes can also be used)
	int id;

	/**
	 * AI Command callback id (passed in on handleCommand, returned
	 * in CommandFinished event)
	 */
	int aiCommandId;

	/**
	 * Remove this command after this frame (absolute).
	 * This can only be set locally and is not sent over the network.
	 * (used for temporary orders)
	 * Examples:
	 * - 0
	 * - MAX_INT
	 * - currenFrame + 60
	 */
	int timeOut;

	/// unique id within a CCommandQueue
	unsigned int tag;

	/// option bits (RIGHT_MOUSE_KEY, ...)
	unsigned char options;

	/// command parameters
	#ifdef BUILDING_AI
	std::vector<float> params;
	#else
	safe_vector<float> params;
	#endif
};

#endif // COMMAND_H

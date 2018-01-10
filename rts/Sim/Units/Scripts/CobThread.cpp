/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobThread.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "CobEngine.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

#include <sstream>

CR_BIND(CCobThread, )

CR_REG_METADATA(CCobThread, (
	CR_MEMBER(owner),
	CR_MEMBER(cob),

	CR_MEMBER(id),
	CR_MEMBER(pc),

	CR_MEMBER(wakeTime),
	CR_MEMBER(paramCount),
	CR_MEMBER(retCode),
	CR_MEMBER(cbParam),
	CR_MEMBER(signalMask),

	CR_MEMBER(waitAxis),
	CR_MEMBER(waitPiece),

	CR_IGNORED(errorCounter),

	CR_MEMBER(cbType),
	CR_MEMBER(state),

	CR_MEMBER(luaArgs),
	CR_MEMBER(dataStack),
	CR_MEMBER(callStack)
))

CR_BIND(CCobThread::CallInfo,)

CR_REG_METADATA_SUB(CCobThread, CallInfo,(
	CR_MEMBER(functionId),
	CR_MEMBER(returnAddr),
	CR_MEMBER(stackTop)
))


CCobThread::CCobThread(CCobInstance* _owner)
	: owner(_owner)
	, cob(_owner->cobFile)
{
	memset(&luaArgs[0], 0, MAX_LUA_COB_ARGS * sizeof(luaArgs[0]));
}

CCobThread& CCobThread::operator = (CCobThread&& t) {
	id = t.id;
	pc = t.pc;

	wakeTime = t.wakeTime;
	paramCount = t.paramCount;
	retCode = t.retCode;
	cbParam = t.cbParam;
	signalMask = t.signalMask;

	waitAxis = t.waitAxis;
	waitPiece = t.waitPiece;

	memcpy(luaArgs, t.luaArgs, sizeof(luaArgs));

	callStack = std::move(t.callStack);
	dataStack = std::move(t.dataStack);
	// execTrace = std::move(t.execTrace);

	state = t.state;
	cbType = t.cbType;

	owner = t.owner; t.owner = nullptr;
	cob = t.cob; t.cob = nullptr;

	t.owner = nullptr;
	t.cob = nullptr;
	return *this;
}


void CCobThread::Start(int functionId, int sigMask, const std::vector<int>& args, bool schedule)
{
	state = Run;
	pc = cob->scriptOffsets[functionId];

	paramCount = args.size();
	signalMask = sigMask;

	CallInfo ci;
	ci.functionId = functionId;
	ci.returnAddr = -1;
	ci.stackTop   = 0;

	callStack.push_back(ci);

	// copy arguments
	dataStack = args;

	// add to scheduler
	if (schedule)
		cobEngine->ScheduleThread(this);
}

void CCobThread::Stop()
{
	if (owner == nullptr)
		return;

	if (cbType != CCobInstance::CBNone)
		owner->ThreadCallback(cbType, retCode, cbParam);

	owner->RemoveThreadID(id);
	SetState(Dead);

	owner = nullptr;
	cob = nullptr;
}

const std::string& CCobThread::GetName()
{
	return cob->scriptNames[callStack[0].functionId];
}

int CCobThread::CheckStack(unsigned int size, bool warn)
{
	if (size <= dataStack.size())
		return size;

	if (warn) {
		char msg[512];
		const char* fmt =
			"stack-size mismatch: need %u but have %lu arguments "
			"(too many passed to function or too few returned?)";

		SNPRINTF(msg, sizeof(msg), fmt, size, dataStack.size());
		ShowError(msg);
	}

	return (dataStack.size());
}


// Command documentation from http://visualta.tauniverse.com/Downloads/cob-commands.txt
// And some information from basm0.8 source (basm ops.txt)

// Model interaction
constexpr int MOVE       = 0x10001000;
constexpr int TURN       = 0x10002000;
constexpr int SPIN       = 0x10003000;
constexpr int STOP_SPIN  = 0x10004000;
constexpr int SHOW       = 0x10005000;
constexpr int HIDE       = 0x10006000;
constexpr int CACHE      = 0x10007000;
constexpr int DONT_CACHE = 0x10008000;
constexpr int MOVE_NOW   = 0x1000B000;
constexpr int TURN_NOW   = 0x1000C000;
constexpr int SHADE      = 0x1000D000;
constexpr int DONT_SHADE = 0x1000E000;
constexpr int EMIT_SFX   = 0x1000F000;

// Blocking operations
constexpr int WAIT_TURN  = 0x10011000;
constexpr int WAIT_MOVE  = 0x10012000;
constexpr int SLEEP      = 0x10013000;

// Stack manipulation
constexpr int PUSH_CONSTANT    = 0x10021001;
constexpr int PUSH_LOCAL_VAR   = 0x10021002;
constexpr int PUSH_STATIC      = 0x10021004;
constexpr int CREATE_LOCAL_VAR = 0x10022000;
constexpr int POP_LOCAL_VAR    = 0x10023002;
constexpr int POP_STATIC       = 0x10023004;
constexpr int POP_STACK        = 0x10024000; ///< Not sure what this is supposed to do

// Arithmetic operations
constexpr int ADD         = 0x10031000;
constexpr int SUB         = 0x10032000;
constexpr int MUL         = 0x10033000;
constexpr int DIV         = 0x10034000;
constexpr int MOD		  = 0x10034001; ///< spring specific
constexpr int BITWISE_AND = 0x10035000;
constexpr int BITWISE_OR  = 0x10036000;
constexpr int BITWISE_XOR = 0x10037000;
constexpr int BITWISE_NOT = 0x10038000;

// Native function calls
constexpr int RAND           = 0x10041000;
constexpr int GET_UNIT_VALUE = 0x10042000;
constexpr int GET            = 0x10043000;

// Comparison
constexpr int SET_LESS             = 0x10051000;
constexpr int SET_LESS_OR_EQUAL    = 0x10052000;
constexpr int SET_GREATER          = 0x10053000;
constexpr int SET_GREATER_OR_EQUAL = 0x10054000;
constexpr int SET_EQUAL            = 0x10055000;
constexpr int SET_NOT_EQUAL        = 0x10056000;
constexpr int LOGICAL_AND          = 0x10057000;
constexpr int LOGICAL_OR           = 0x10058000;
constexpr int LOGICAL_XOR          = 0x10059000;
constexpr int LOGICAL_NOT          = 0x1005A000;

// Flow control
constexpr int START           = 0x10061000;
constexpr int CALL            = 0x10062000; ///< converted when executed
constexpr int REAL_CALL       = 0x10062001; ///< spring custom
constexpr int LUA_CALL        = 0x10062002; ///< spring custom
constexpr int JUMP            = 0x10064000;
constexpr int RETURN          = 0x10065000;
constexpr int JUMP_NOT_EQUAL  = 0x10066000;
constexpr int SIGNAL          = 0x10067000;
constexpr int SET_SIGNAL_MASK = 0x10068000;

// Piece destruction
constexpr int EXPLODE    = 0x10071000;
constexpr int PLAY_SOUND = 0x10072000;

// Special functions
constexpr int SET    = 0x10082000;
constexpr int ATTACH = 0x10083000;
constexpr int DROP   = 0x10084000;

// Indices for SET, GET, and GET_UNIT_VALUE for LUA return values
#define LUA0 110 // (LUA0 returns the lua call status, 0 or 1)
#define LUA1 111
#define LUA2 112
#define LUA3 113
#define LUA4 114
#define LUA5 115
#define LUA6 116
#define LUA7 117
#define LUA8 118
#define LUA9 119

#define GET_LONG_PC() (cob->code[pc++])


static const char* GetOpcodeName(int opcode)
{
	switch (opcode) {
		case MOVE: return "move";
		case TURN: return "turn";
		case SPIN: return "spin";
		case STOP_SPIN: return "stop-spin";
		case SHOW: return "show";
		case HIDE: return "hide";
		case CACHE: return "cache";
		case DONT_CACHE: return "dont-cache";
		case TURN_NOW: return "turn-now";
		case MOVE_NOW: return "move-now";
		case SHADE: return "shade";
		case DONT_SHADE: return "dont-shade";
		case EMIT_SFX: return "sfx";

		case WAIT_TURN: return "wait-for-turn";
		case WAIT_MOVE: return "wait-for-move";
		case SLEEP: return "sleep";

		case PUSH_CONSTANT: return "pushc";
		case PUSH_LOCAL_VAR: return "pushl";
		case PUSH_STATIC: return "pushs";
		case CREATE_LOCAL_VAR: return "clv";
		case POP_LOCAL_VAR: return "popl";
		case POP_STATIC: return "pops";
		case POP_STACK: return "pop-stack";

		case ADD: return "add";
		case SUB: return "sub";
		case MUL: return "mul";
		case DIV: return "div";
		case MOD: return "mod";
		case BITWISE_AND: return "and";
		case BITWISE_OR: return "or";
		case BITWISE_XOR: return "xor";
		case BITWISE_NOT: return "not";

		case RAND: return "rand";
		case GET_UNIT_VALUE: return "getuv";
		case GET: return "get";

		case SET_LESS: return "setl";
		case SET_LESS_OR_EQUAL: return "setle";
		case SET_GREATER: return "setg";
		case SET_GREATER_OR_EQUAL: return "setge";
		case SET_EQUAL: return "sete";
		case SET_NOT_EQUAL: return "setne";
		case LOGICAL_AND: return "land";
		case LOGICAL_OR: return "lor";
		case LOGICAL_XOR: return "lxor";
		case LOGICAL_NOT: return "neg";

		case START: return "start";
		case CALL: return "call";
		case REAL_CALL: return "call";
		case LUA_CALL: return "lua_call";
		case JUMP: return "jmp";
		case RETURN: return "return";
		case JUMP_NOT_EQUAL: return "jne";
		case SIGNAL: return "signal";
		case SET_SIGNAL_MASK: return "mask";

		case EXPLODE: return "explode";
		case PLAY_SOUND: return "play-sound";

		case SET: return "set";
		case ATTACH: return "attach";
		case DROP: return "drop";
	}

	return "unknown";
}



int CCobThread::POP()
{
	if (!dataStack.empty()) {
		const int r = dataStack.back();
		dataStack.pop_back();
		return r;
	}

	return 0;
}

bool CCobThread::Tick()
{
	assert(state != Sleep);
	assert(owner != nullptr);

	if (IsDead())
		return false;

	state = Run;

	int r1, r2, r3, r4, r5, r6;

	while (state == Run) {
		const int opcode = GET_LONG_PC();

		switch (opcode) {
			case PUSH_CONSTANT: {
				r1 = GET_LONG_PC();
				dataStack.push_back(r1);
			} break;
			case SLEEP: {
				r1 = POP();
				wakeTime = cobEngine->GetCurrentTime() + r1;
				state = Sleep;

				cobEngine->ScheduleThread(this);
				return true;
			} break;
			case SPIN: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();         // speed
				r4 = POP();         // accel
				owner->Spin(r1, r2, r3, r4);
			} break;
			case STOP_SPIN: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();         // decel

				owner->StopSpin(r1, r2, r3);
			} break;
			case RETURN: {
				retCode = POP();

				if (callStack.back().returnAddr == -1) {
					state = Dead;

					// leave values intact on stack in case caller wants to check them
					// callStack.pop_back();
					return false;
				}

				// return to caller
				pc = callStack.back().returnAddr;

				while (dataStack.size() > callStack.back().stackTop) {
					dataStack.pop_back();
				}

				callStack.pop_back();
			} break;


			case SHADE: {
				r1 = GET_LONG_PC();
			} break;
			case DONT_SHADE: {
				r1 = GET_LONG_PC();
			} break;
			case CACHE: {
				r1 = GET_LONG_PC();
			} break;
			case DONT_CACHE: {
				r1 = GET_LONG_PC();
			} break;


			case CALL: {
				r1 = GET_LONG_PC();
				pc--;

				if (cob->scriptNames[r1].find("lua_") == 0) {
					cob->code[pc - 1] = LUA_CALL;
					LuaCall();
					break;
				}

				cob->code[pc - 1] = REAL_CALL;

				// fall-through
			}
			case REAL_CALL: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				// do not call zero-length functions
				if (cob->scriptLengths[r1] == 0)
					break;

				CallInfo ci;
				ci.functionId = r1;
				ci.returnAddr = pc;
				ci.stackTop = dataStack.size() - r2;
				callStack.push_back(ci);
				paramCount = r2;

				// call cob->scriptNames[r1]
				pc = cob->scriptOffsets[r1];
			} break;
			case LUA_CALL: {
				LuaCall();
			} break;


			case POP_STATIC: {
				r1 = GET_LONG_PC();
				r2 = POP();

				if (static_cast<size_t>(r1) < owner->staticVars.size())
					owner->staticVars[r1] = r2;
			} break;
			case POP_STACK: {
				POP();
			} break;


			case START: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (cob->scriptLengths[r1] == 0)
					break;

				callArgs.clear();
				callArgs.reserve(r2);

				for (r3 = 0; r3 < r2; ++r3) {
					callArgs.push_back(r4 = POP());
				}


				CCobThread t(owner);

				t.SetID(cobEngine->GenThreadID());
				t.Start(r1, signalMask, callArgs, true);

				// calling AddThread directly might move <this>, defer it
				cobEngine->QueueAddThread(std::move(t));
			} break;

			case CREATE_LOCAL_VAR: {
				if (paramCount == 0) {
					dataStack.push_back(0);
				} else {
					paramCount--;
				}
			} break;
			case GET_UNIT_VALUE: {
				r1 = POP();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					dataStack.push_back(luaArgs[r1 - LUA0]);
					break;
				}
				r1 = owner->GetUnitVal(r1, 0, 0, 0, 0);
				dataStack.push_back(r1);
			} break;


			case JUMP_NOT_EQUAL: {
				r1 = GET_LONG_PC();
				r2 = POP();

				if (r2 == 0)
					pc = r1;

			} break;
			case JUMP: {
				r1 = GET_LONG_PC();
				// this seem to be an error in the docs..
				//r2 = cob->scriptOffsets[callStack.back().functionId] + r1;
				pc = r1;
			} break;


			case POP_LOCAL_VAR: {
				r1 = GET_LONG_PC();
				r2 = POP();
				dataStack[callStack.back().stackTop + r1] = r2;
			} break;
			case PUSH_LOCAL_VAR: {
				r1 = GET_LONG_PC();
				r2 = dataStack[callStack.back().stackTop + r1];
				dataStack.push_back(r2);
			} break;


			case BITWISE_AND: {
				r1 = POP();
				r2 = POP();
				dataStack.push_back(r1 & r2);
			} break;
			case BITWISE_OR: {
				r1 = POP();
				r2 = POP();
				dataStack.push_back(r1 | r2);
			} break;
			case BITWISE_XOR: {
				r1 = POP();
				r2 = POP();
				dataStack.push_back(r1 ^ r2);
			} break;
			case BITWISE_NOT: {
				r1 = POP();
				dataStack.push_back(~r1);
			} break;

			case EXPLODE: {
				r1 = GET_LONG_PC();
				r2 = POP();
				owner->Explode(r1, r2);
			} break;

			case PLAY_SOUND: {
				r1 = GET_LONG_PC();
				r2 = POP();
				owner->PlayUnitSound(r1, r2);
			} break;

			case PUSH_STATIC: {
				r1 = GET_LONG_PC();

				if (static_cast<size_t>(r1) < owner->staticVars.size())
					dataStack.push_back(owner->staticVars[r1]);
			} break;

			case SET_NOT_EQUAL: {
				r1 = POP();
				r2 = POP();

				dataStack.push_back(int(r1 != r2));
			} break;
			case SET_EQUAL: {
				r1 = POP();
				r2 = POP();

				dataStack.push_back(int(r1 == r2));
			} break;

			case SET_LESS: {
				r2 = POP();
				r1 = POP();

				dataStack.push_back(int(r1 < r2));
			} break;
			case SET_LESS_OR_EQUAL: {
				r2 = POP();
				r1 = POP();

				dataStack.push_back(int(r1 <= r2));
			} break;

			case SET_GREATER: {
				r2 = POP();
				r1 = POP();

				dataStack.push_back(int(r1 > r2));
			} break;
			case SET_GREATER_OR_EQUAL: {
				r2 = POP();
				r1 = POP();

				dataStack.push_back(int(r1 >= r2));
			} break;

			case RAND: {
				r2 = POP();
				r1 = POP();
				r3 = gsRNG.NextInt(r2 - r1 + 1) + r1;
				dataStack.push_back(r3);
			} break;
			case EMIT_SFX: {
				r1 = POP();
				r2 = GET_LONG_PC();
				owner->EmitSfx(r1, r2);
			} break;
			case MUL: {
				r1 = POP();
				r2 = POP();
				dataStack.push_back(r1 * r2);
			} break;


			case SIGNAL: {
				r1 = POP();
				owner->Signal(r1);
			} break;
			case SET_SIGNAL_MASK: {
				r1 = POP();
				signalMask = r1;
			} break;


			case TURN: {
				r2 = POP();
				r1 = POP();
				r3 = GET_LONG_PC(); // piece
				r4 = GET_LONG_PC(); // axis

				owner->Turn(r3, r4, r1, r2);
			} break;
			case GET: {
				r5 = POP();
				r4 = POP();
				r3 = POP();
				r2 = POP();
				r1 = POP();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					dataStack.push_back(luaArgs[r1 - LUA0]);
					break;
				}
				r6 = owner->GetUnitVal(r1, r2, r3, r4, r5);
				dataStack.push_back(r6);
			} break;
			case ADD: {
				r2 = POP();
				r1 = POP();
				dataStack.push_back(r1 + r2);
			} break;
			case SUB: {
				r2 = POP();
				r1 = POP();
				r3 = r1 - r2;
				dataStack.push_back(r3);
			} break;

			case DIV: {
				r2 = POP();
				r1 = POP();

				if (r2 != 0) {
					r3 = r1 / r2;
				} else {
					r3 = 1000; // infinity!
					ShowError("division by zero");
				}
				dataStack.push_back(r3);
			} break;
			case MOD: {
				r2 = POP();
				r1 = POP();

				if (r2 != 0) {
					dataStack.push_back(r1 % r2);
				} else {
					dataStack.push_back(0);
					ShowError("modulo division by zero");
				}
			} break;


			case MOVE: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r4 = POP();
				r3 = POP();
				owner->Move(r1, r2, r3, r4);
			} break;
			case MOVE_NOW: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();
				owner->MoveNow(r1, r2, r3);
			} break;
			case TURN_NOW: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();
				owner->TurnNow(r1, r2, r3);
			} break;


			case WAIT_TURN: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (owner->NeedsWait(CCobInstance::ATurn, r1, r2)) {
					state = WaitTurn;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			} break;
			case WAIT_MOVE: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (owner->NeedsWait(CCobInstance::AMove, r1, r2)) {
					state = WaitMove;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			} break;


			case SET: {
				r2 = POP();
				r1 = POP();

				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					luaArgs[r1 - LUA0] = r2;
					break;
				}

				owner->SetUnitVal(r1, r2);
			} break;


			case ATTACH: {
				r3 = POP();
				r2 = POP();
				r1 = POP();
				owner->AttachUnit(r2, r1);
			} break;
			case DROP: {
				r1 = POP();
				owner->DropUnit(r1);
			} break;

			// like bitwise ops, but only on values 1 and 0
			case LOGICAL_NOT: {
				r1 = POP();
				dataStack.push_back(int(r1 == 0));
			} break;
			case LOGICAL_AND: {
				r1 = POP();
				r2 = POP();
				dataStack.push_back(int(r1 && r2));
			} break;
			case LOGICAL_OR: {
				r1 = POP();
				r2 = POP();
				dataStack.push_back(int(r1 || r2));
			} break;
			case LOGICAL_XOR: {
				r1 = POP();
				r2 = POP();
				dataStack.push_back(int((!!r1) ^ (!!r2)));
			} break;


			case HIDE: {
				r1 = GET_LONG_PC();
				owner->SetVisibility(r1, false);
			} break;

			case SHOW: {
				r1 = GET_LONG_PC();

				int i;
				for (i = 0; i < MAX_WEAPONS_PER_UNIT; ++i)
					if (callStack.back().functionId == cob->scriptIndex[COBFN_FirePrimary + COBFN_Weapon_Funcs * i])
						break;

				// if true, we are in a Fire-script and should show a special flare effect
				if (i < MAX_WEAPONS_PER_UNIT) {
					owner->ShowFlare(r1);
				} else {
					owner->SetVisibility(r1, true);
				}
			} break;

			default: {
				const char* name = cob->name.c_str();
				const char* func = cob->scriptNames[callStack.back().functionId].c_str();

				LOG_L(L_ERROR, "[COBThread::%s] unknown opcode %x (in %s:%s at %x)", __func__, opcode, name, func, pc - 1);

				#if 0
				auto ei = execTrace.begin();
				while (ei != execTrace.end()) {
					LOG_L(L_ERROR, "\tprogctr: %3x  opcode: %s", __func__, *ei, GetOpcodeName(cob->code[*ei]));
					++ei;
				}
				#endif

				state = Dead;
				return false;
			} break;
		}
	}

	// can arrive here as dead, through CCobInstance::Signal()
	return (state != Dead);
}

void CCobThread::ShowError(const char* msg)
{
	if ((errorCounter = std::max(errorCounter - 1, 0)) == 0)
		return;

	if (callStack.empty()) {
		LOG_L(L_ERROR, "[COBThread::%s] %s outside script execution (?)", __func__, msg);
		return;
	}

	const char* name = cob->name.c_str();
	const char* func = cob->scriptNames[callStack.back().functionId].c_str();

	LOG_L(L_ERROR, "[COBThread::%s] %s (in %s:%s at %x)", __func__, msg, name, func, pc - 1);
}


void CCobThread::LuaCall()
{
	const int r1 = GET_LONG_PC(); // script id
	const int r2 = GET_LONG_PC(); // arg count

	// setup the parameter array
	const int size = (int) dataStack.size();
	const int argCount = std::min(r2, MAX_LUA_COB_ARGS);
	const int start = std::max(0, size - r2);
	const int end = std::min(size, start + argCount);

	for (int a = 0, i = start; i < end; i++) {
		luaArgs[a++] = dataStack[i];
	}

	if (r2 >= size) {
		dataStack.clear();
	} else {
		dataStack.resize(size - r2);
	}

	if (!luaRules) {
		luaArgs[0] = 0; // failure
		return;
	}

	// check script index validity
	if (static_cast<size_t>(r1) >= cob->luaScripts.size()) {
		luaArgs[0] = 0; // failure
		return;
	}

	int argsCount = argCount;
	luaRules->Cob2Lua(cob->luaScripts[r1], owner->GetUnit(), argsCount, luaArgs);
	retCode = luaArgs[0];
}


void CCobThread::AnimFinished(CUnitScript::AnimType type, int piece, int axis)
{
	if (piece != waitPiece || axis != waitAxis)
		return;

	if (!Reschedule(type))
		return;

	state = Run;
	waitPiece = -1;
	waitAxis = -1;

	cobEngine->ScheduleThread(this);
}


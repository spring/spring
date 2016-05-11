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
	CR_MEMBER(wakeTime),
	CR_MEMBER(PC),
	CR_MEMBER(paramCount),
	CR_MEMBER(retCode),
	CR_MEMBER(cbType),
	CR_MEMBER(cbParam),
	CR_MEMBER(waitAxis),
	CR_MEMBER(waitPiece),
	CR_MEMBER(state),
	CR_MEMBER(signalMask),
	CR_MEMBER(luaArgs),
	CR_MEMBER(stack),
	CR_MEMBER(callStack)
))

CR_BIND(CCobThread::CallInfo,)

CR_REG_METADATA_SUB(CCobThread, CallInfo,(
		CR_MEMBER(functionId),
		CR_MEMBER(returnAddr),
		CR_MEMBER(stackTop)
))

//creg only
CCobThread::CCobThread()
{ }


CCobThread::CCobThread(CCobInstance* owner)
	: owner(owner)
	, wakeTime(0)
	, PC(0)
	, paramCount(0)
	, retCode(-1)
	, cbType(CCobInstance::CBNone)
	, cbParam(0)
	, waitAxis(-1)
	, waitPiece(-1)
	, state(Init)
	, signalMask(0)
{
	memset(&luaArgs[0], 0, MAX_LUA_COB_ARGS * sizeof(luaArgs[0]));
	owner->threads.push_back(this);
}

CCobThread::~CCobThread()
{
	if (owner != nullptr) {
		if (cbType != CCobInstance::CBNone) {
			//LOG_L(L_DEBUG, "%s callback with %d", script.scriptNames[callStack.back().functionId].c_str(), retCode);
			owner->ThreadCallback(cbType, retCode, cbParam);
		}
		auto it = std::find(owner->threads.begin(), owner->threads.end(), this);
		assert(it != owner->threads.end());
		owner->threads.erase(it);
	}
}

void CCobThread::SetCallback(CCobInstance::ThreadCallbackType cb, int cbp)
{
	cbType = cb;
	cbParam = cbp;
}

void CCobThread::Start(int functionId, const vector<int>& args, bool schedule)
{
	state = Run;
	PC = owner->script->scriptOffsets[functionId];

	CallInfo ci;
	ci.functionId = functionId;
	ci.returnAddr = -1;
	ci.stackTop   = 0;

	callStack.push_back(ci);
	paramCount = args.size();

	// copy arguments
	stack = args;

	// Add to scheduler
	if (schedule)
		cobEngine->AddThread(this);
}

const string& CCobThread::GetName()
{
	return owner->script->scriptNames[callStack[0].functionId];
}

int CCobThread::CheckStack(unsigned int size, bool warn)
{
	if (size <= stack.size())
		return size;

	if (warn) {
		static char msg[512];
		static const char* fmt =
			"stack-size mismatch: need %u but have %lu arguments "
			"(too many passed to function or too few returned?)";
		SNPRINTF(msg, sizeof(msg), fmt, size, stack.size());
		ShowError(msg);
	}
	return stack.size();
}

int CCobThread::GetStackVal(int pos)
{
	return stack[pos];
}

int CCobThread::GetWakeTime() const
{
	return wakeTime;
}

// Command documentation from http://visualta.tauniverse.com/Downloads/cob-commands.txt
// And some information from basm0.8 source (basm ops.txt)

// Model interaction
const int MOVE       = 0x10001000;
const int TURN       = 0x10002000;
const int SPIN       = 0x10003000;
const int STOP_SPIN  = 0x10004000;
const int SHOW       = 0x10005000;
const int HIDE       = 0x10006000;
const int CACHE      = 0x10007000;
const int DONT_CACHE = 0x10008000;
const int MOVE_NOW   = 0x1000B000;
const int TURN_NOW   = 0x1000C000;
const int SHADE      = 0x1000D000;
const int DONT_SHADE = 0x1000E000;
const int EMIT_SFX   = 0x1000F000;

// Blocking operations
const int WAIT_TURN  = 0x10011000;
const int WAIT_MOVE  = 0x10012000;
const int SLEEP      = 0x10013000;

// Stack manipulation
const int PUSH_CONSTANT    = 0x10021001;
const int PUSH_LOCAL_VAR   = 0x10021002;
const int PUSH_STATIC      = 0x10021004;
const int CREATE_LOCAL_VAR = 0x10022000;
const int POP_LOCAL_VAR    = 0x10023002;
const int POP_STATIC       = 0x10023004;
const int POP_STACK        = 0x10024000; ///< Not sure what this is supposed to do

// Arithmetic operations
const int ADD         = 0x10031000;
const int SUB         = 0x10032000;
const int MUL         = 0x10033000;
const int DIV         = 0x10034000;
const int MOD		  = 0x10034001; ///< spring specific
const int BITWISE_AND = 0x10035000;
const int BITWISE_OR  = 0x10036000;
const int BITWISE_XOR = 0x10037000;
const int BITWISE_NOT = 0x10038000;

// Native function calls
const int RAND           = 0x10041000;
const int GET_UNIT_VALUE = 0x10042000;
const int GET            = 0x10043000;

// Comparison
const int SET_LESS             = 0x10051000;
const int SET_LESS_OR_EQUAL    = 0x10052000;
const int SET_GREATER          = 0x10053000;
const int SET_GREATER_OR_EQUAL = 0x10054000;
const int SET_EQUAL            = 0x10055000;
const int SET_NOT_EQUAL        = 0x10056000;
const int LOGICAL_AND          = 0x10057000;
const int LOGICAL_OR           = 0x10058000;
const int LOGICAL_XOR          = 0x10059000;
const int LOGICAL_NOT          = 0x1005A000;

// Flow control
const int START           = 0x10061000;
const int CALL            = 0x10062000; ///< converted when executed
const int REAL_CALL       = 0x10062001; ///< spring custom
const int LUA_CALL        = 0x10062002; ///< spring custom
const int JUMP            = 0x10064000;
const int RETURN          = 0x10065000;
const int JUMP_NOT_EQUAL  = 0x10066000;
const int SIGNAL          = 0x10067000;
const int SET_SIGNAL_MASK = 0x10068000;

// Piece destruction
const int EXPLODE    = 0x10071000;
const int PLAY_SOUND = 0x10072000;

// Special functions
const int SET    = 0x10082000;
const int ATTACH = 0x10083000;
const int DROP   = 0x10084000;

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


// Handy macros
#define GET_LONG_PC() (owner->script->code[PC++])
// #define POP() (stack.size() > 0) ? stack.back(), stack.pop_back(); : 0

int CCobThread::POP()
{
	if (!stack.empty()) {
		int r = stack.back();
		stack.pop_back();
		return r;
	}

	return 0;
}

bool CCobThread::Tick()
{
	if (state == Sleep) {
		LOG_L(L_ERROR, "sleeping thread ticked!");
	}
	if (state == Dead || owner == nullptr) {
		return false;
	}

	state = Run;

	int r1, r2, r3, r4, r5, r6;

	vector<int> args;
	vector<int>::iterator ei;

	//execTrace.clear();

	//LOG_L(L_DEBUG, "Executing in %s (from %s)", script.scriptNames[callStack.back().functionId].c_str(), GetName().c_str());

	while (state == Run) {
		//int opcode = *(int*)&script.code[PC];

		// Disabling exec trace gives about a 50% speedup on vm-intensive code
		//execTrace.push_back(PC);

		int opcode = GET_LONG_PC();

		//LOG_L(L_DEBUG, "PC: %x opcode: %x (%s)", PC - 1, opcode, GetOpcodeName(opcode).c_str());

		switch(opcode) {
			case PUSH_CONSTANT:
				r1 = GET_LONG_PC();
				stack.push_back(r1);
				break;
			case SLEEP:
				r1 = POP();
				wakeTime = cobEngine->GetCurrentTime() + r1;
				state = Sleep;
				cobEngine->AddThread(this);
				//LOG_L(L_DEBUG, "%s sleeping for %d ms", script.scriptNames[callStack.back().functionId].c_str(), r1);
				return true;
			case SPIN:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();         // speed
				r4 = POP();         // accel
				owner->Spin(r1, r2, r3, r4);
				break;
			case STOP_SPIN:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();         // decel
				//LOG_L(L_DEBUG, "Stop spin of %s around %d", script.pieceNames[r1].c_str(), r2);
				owner->StopSpin(r1, r2, r3);
				break;
			case RETURN:
				retCode = POP();
				if (callStack.back().returnAddr == -1) {
					//LOG_L(L_DEBUG, "%s returned %d", script.scriptNames[callStack.back().functionId].c_str(), retCode);
					state = Dead;
					//callStack.pop_back();
					// Leave values intact on stack in case caller wants to check them
					return false;
				}

				PC = callStack.back().returnAddr;
				while (stack.size() > callStack.back().stackTop) {
					stack.pop_back();
				}
				callStack.pop_back();
				//LOG_L(L_DEBUG, "Returning to %s", owner->script->scriptNames[callStack.back().functionId].c_str());
				break;
			case SHADE:
				r1 = GET_LONG_PC();
				break;
			case DONT_SHADE:
				r1 = GET_LONG_PC();
				break;
			case CACHE:
				r1 = GET_LONG_PC();
				break;
			case DONT_CACHE:
				r1 = GET_LONG_PC();
				break;
			case CALL: {
				r1 = GET_LONG_PC();
				PC--;
				const string& name = owner->script->scriptNames[r1];
				if (name.find("lua_") == 0) {
					owner->script->code[PC - 1] = LUA_CALL;
					LuaCall();
					break;
				}
				owner->script->code[PC - 1] = REAL_CALL;

				// fall through //
			}
			case REAL_CALL:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (owner->script->scriptLengths[r1] == 0) {
					//LOG_L(L_DEBUG, "Preventing call to zero-len script %s", owner->script->scriptNames[r1].c_str());
					break;
				}

				CallInfo ci;
				ci.functionId = r1;
				ci.returnAddr = PC;
				ci.stackTop = stack.size() - r2;
				callStack.push_back(ci);
				paramCount = r2;

				PC = owner->script->scriptOffsets[r1];
				//LOG_L(L_DEBUG, "Calling %s", owner->script->scriptNames[r1].c_str());
				break;
			case LUA_CALL:
				LuaCall();
				break;
			case POP_STATIC:
				r1 = GET_LONG_PC();
				r2 = POP();
				owner->staticVars[r1] = r2;
				//LOG_L(L_DEBUG, "Pop static var %d val %d", r1, r2);
				break;
			case POP_STACK:
				POP();
				break;
			case START: {
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (owner->script->scriptLengths[r1] == 0) {
					//LOG_L(L_DEBUG, "Preventing start of zero-len script %s", owner->script->scriptNames[r1].c_str());
					break;
				}

				args.clear();
				args.reserve(r2);
				for (r3 = 0; r3 < r2; ++r3) {
					r4 = POP();
					args.push_back(r4);
				}

				CCobThread* thread = new CCobThread(owner);
				thread->Start(r1, args, true);

				// Seems that threads should inherit signal mask from creator
				thread->signalMask = signalMask;
				//LOG_L(L_DEBUG, "Starting %s %d", owner->script->scriptNames[r1].c_str(), signalMask);
			} break;
			case CREATE_LOCAL_VAR:
				if (paramCount == 0) {
					stack.push_back(0);
				} else {
					paramCount--;
				}
				break;
			case GET_UNIT_VALUE:
				r1 = POP();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					stack.push_back(luaArgs[r1 - LUA0]);
					break;
				}
				r1 = owner->GetUnitVal(r1, 0, 0, 0, 0);
				stack.push_back(r1);
				break;
			case JUMP_NOT_EQUAL:
				r1 = GET_LONG_PC();
				r2 = POP();
				if (r2 == 0) {
					PC = r1;
				}
				break;
			case JUMP:
				r1 = GET_LONG_PC();
				// this seem to be an error in the docs..
				//r2 = owner->script->scriptOffsets[callStack.back().functionId] + r1;
				PC = r1;
				break;
			case POP_LOCAL_VAR:
				r1 = GET_LONG_PC();
				r2 = POP();
				stack[callStack.back().stackTop + r1] = r2;
				break;
			case PUSH_LOCAL_VAR:
				r1 = GET_LONG_PC();
				r2 = stack[callStack.back().stackTop + r1];
				stack.push_back(r2);
				break;
			case SET_LESS_OR_EQUAL:
				r2 = POP();
				r1 = POP();
				if (r1 <= r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case BITWISE_AND:
				r1 = POP();
				r2 = POP();
				stack.push_back(r1 & r2);
				break;
			case BITWISE_OR: // seems to want stack contents or'd, result places on stack
				r1 = POP();
				r2 = POP();
				stack.push_back(r1 | r2);
				break;
			case BITWISE_XOR:
				r1 = POP();
				r2 = POP();
				stack.push_back(r1 ^ r2);
				break;
			case BITWISE_NOT:
				r1 = POP();
				stack.push_back(~r1);
				break;
			case EXPLODE:
				r1 = GET_LONG_PC();
				r2 = POP();
				owner->Explode(r1, r2);
				break;
			case PLAY_SOUND:
				r1 = GET_LONG_PC();
				r2 = POP();
				owner->PlayUnitSound(r1, r2);
				break;
			case PUSH_STATIC:
				r1 = GET_LONG_PC();
				stack.push_back(owner->staticVars[r1]);
				//LOG_L(L_DEBUG, "Push static %d val %d", r1, owner->staticVars[r1]);
				break;
			case SET_NOT_EQUAL:
				r1 = POP();
				r2 = POP();
				if (r1 != r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case SET_EQUAL:
				r1 = POP();
				r2 = POP();
				if (r1 == r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case SET_LESS:
				r2 = POP();
				r1 = POP();
				if (r1 < r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case SET_GREATER:
				r2 = POP();
				r1 = POP();
				if (r1 > r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case SET_GREATER_OR_EQUAL:
				r2 = POP();
				r1 = POP();
				if (r1 >= r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case RAND:
				r2 = POP();
				r1 = POP();
				r3 = gs->randInt() % (r2 - r1 + 1) + r1;
				stack.push_back(r3);
				break;
			case EMIT_SFX:
				r1 = POP();
				r2 = GET_LONG_PC();
				owner->EmitSfx(r1, r2);
				break;
			case MUL:
				r1 = POP();
				r2 = POP();
				stack.push_back(r1 * r2);
				break;
			case SIGNAL:
				r1 = POP();
				owner->Signal(r1);
				break;
			case SET_SIGNAL_MASK:
				r1 = POP();
				signalMask = r1;
				break;
			case TURN:
				r2 = POP();
				r1 = POP();
				r3 = GET_LONG_PC();
				r4 = GET_LONG_PC();
				//LOG_L(L_DEBUG, "Turning piece %s axis %d to %d speed %d", owner->script->pieceNames[r3].c_str(), r4, r2, r1);
				owner->Turn(r3, r4, r1, r2);
				break;
			case GET:
				r5 = POP();
				r4 = POP();
				r3 = POP();
				r2 = POP();
				r1 = POP();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					stack.push_back(luaArgs[r1 - LUA0]);
					break;
				}
				r6 = owner->GetUnitVal(r1, r2, r3, r4, r5);
				stack.push_back(r6);
				break;
			case ADD:
				r2 = POP();
				r1 = POP();
				stack.push_back(r1 + r2);
				break;
			case SUB:
				r2 = POP();
				r1 = POP();
				r3 = r1 - r2;
				stack.push_back(r3);
				break;
			case DIV:
				r2 = POP();
				r1 = POP();
				if (r2 != 0)
					r3 = r1 / r2;
				else {
					r3 = 1000; // infinity!
					ShowError("division by zero");
				}
				stack.push_back(r3);
				break;
			case MOD:
				r2 = POP();
				r1 = POP();
				if (r2 != 0)
					stack.push_back(r1 % r2);
				else {
					stack.push_back(0);
					ShowError("modulo division by zero");
				}
				break;
			case MOVE:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r4 = POP();
				r3 = POP();
				owner->Move(r1, r2, r3, r4);
				break;
			case MOVE_NOW:{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();
				owner->MoveNow(r1, r2, r3);
				break;}
			case TURN_NOW:{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();
				owner->TurnNow(r1, r2, r3);
				break;}
			case WAIT_TURN:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				//LOG_L(L_DEBUG, "Waiting for turn on piece %s around axis %d", owner->script->pieceNames[r1].c_str(), r2);
				if (owner->NeedsWait(CCobInstance::ATurn, r1, r2)) {
					state = WaitTurn;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
				else
					break;
			case WAIT_MOVE:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				//LOG_L(L_DEBUG, "Waiting for move on piece %s on axis %d", owner->script->pieceNames[r1].c_str(), r2);
				if (owner->NeedsWait(CCobInstance::AMove, r1, r2)) {
					state = WaitMove;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
				break;
			case SET:
				r2 = POP();
				r1 = POP();
				//LOG_L(L_DEBUG, "Setting unit value %d to %d", r1, r2);
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					luaArgs[r1 - LUA0] = r2;
					break;
				}
				owner->SetUnitVal(r1, r2);
				break;
			case ATTACH:
				r3 = POP();
				r2 = POP();
				r1 = POP();
				owner->AttachUnit(r2, r1);
				break;
			case DROP:
				r1 = POP();
				owner->DropUnit(r1);
				break;
			case LOGICAL_NOT: // Like bitwise, but only on values 1 and 0.
				r1 = POP();
				if (r1 == 0)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case LOGICAL_AND:
				r1 = POP();
				r2 = POP();
				if (r1 && r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case LOGICAL_OR:
				r1 = POP();
				r2 = POP();
				if (r1 || r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case LOGICAL_XOR:
				r1 = POP();
				r2 = POP();
				if ( (!!r1) ^ (!!r2))
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case HIDE:
				r1 = GET_LONG_PC();
				owner->SetVisibility(r1, false);
				//LOG_L(L_DEBUG, "Hiding %d", r1);
				break;
			case SHOW:{
				r1 = GET_LONG_PC();
				int i;
				for (i = 0; i < MAX_WEAPONS_PER_UNIT; ++i)
					if (callStack.back().functionId == owner->script->scriptIndex[COBFN_FirePrimary + COBFN_Weapon_Funcs * i])
						break;

				// If true, we are in a Fire-script and should show a special flare effect
				if (i < MAX_WEAPONS_PER_UNIT) {
					owner->ShowFlare(r1);
				}
				else {
					owner->SetVisibility(r1, true);
				}
				//LOG_L(L_DEBUG, "Showing %d", r1);
				break;}
			default:
				LOG_L(L_ERROR, "Unknown opcode %x (in %s:%s at %x)",
						opcode, owner->script->name.c_str(),
						owner->script->scriptNames[callStack.back().functionId].c_str(),
						PC - 1);
				LOG_L(L_ERROR, "Exec trace:");
				// ei = execTrace.begin();
				// while (ei != execTrace.end()) {
					// LOG_L(L_ERROR, "PC: %3x  opcode: %s", *ei, GetOpcodeName(owner->script->code[*ei]).c_str());
					// ++ei;
				// }
				state = Dead;
				return false;
		}
	}

	return (state != Dead); // can arrive here as dead, through CCobInstance::Signal()
}

void CCobThread::ShowError(const string& msg)
{
	static int spamPrevention = 100;
	if (spamPrevention < 0) return;
	--spamPrevention;
	if (callStack.empty()) {
		LOG_L(L_ERROR, "%s outside script execution (?)", msg.c_str());
	} else {
		LOG_L(L_ERROR, "%s (in %s:%s at %x)", msg.c_str(),
				owner->script->name.c_str(),
				owner->script->scriptNames[callStack.back().functionId].c_str(),
				PC - 1);
	}
}

string CCobThread::GetOpcodeName(int opcode)
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

/******************************************************************************/

void CCobThread::LuaCall()
{
	const int r1 = GET_LONG_PC(); // script id
	const int r2 = GET_LONG_PC(); // arg count

	// setup the parameter array
	const int size = (int) stack.size();
	const int argCount = std::min(r2, MAX_LUA_COB_ARGS);
	const int start = std::max(0, size - r2);
	const int end = std::min(size, start + argCount);
	int a = 0;
	for (int i = start; i < end; i++) {
		luaArgs[a] = stack[i];
		a++;
	}
	if (r2 >= size) {
		stack.clear();
	} else {
		stack.resize(size - r2);
	}

	if (!luaRules) {
		luaArgs[0] = 0; // failure
		return;
	}

	// check script index validity
	if ((r1 < 0) || (static_cast<size_t>(r1) >= owner->script->luaScripts.size())) {
		luaArgs[0] = 0; // failure
		return;
	}
	const LuaHashString& hs = owner->script->luaScripts[r1];

	//LOG_L(L_DEBUG, "Cob2Lua %s", hs.GetString().c_str());

	int argsCount = argCount;
	luaRules->Cob2Lua(hs, owner->GetUnit(), argsCount, luaArgs);
	retCode = luaArgs[0];
}


/******************************************************************************/
/******************************************************************************/


void CCobThread::AnimFinished(CUnitScript::AnimType type, int piece, int axis)
{
	if (piece != waitPiece || axis != waitAxis)
		return;

	// Not sure how to do this more cleanly.. Will probably rewrite it
	if ((state == WaitMove && type == CCobInstance::AMove) ||
	    (state == WaitTurn && type == CCobInstance::ATurn)) {
		state = Run;
		waitPiece = -1;
		waitAxis = -1;
		cobEngine->AddThread(this);
	}
}

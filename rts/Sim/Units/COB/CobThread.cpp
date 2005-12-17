#include "System/StdAfx.h"
#include "CobThread.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "CobEngine.h"

#include "Game/UI/InfoConsole.h"
//#include "System/mmgr.h"

#ifdef _CONSOLE
#include "System/GlobalStuff.h"
#else
#include "System/GlobalStuff.h"
#endif

CCobThread::CCobThread(CCobFile &script, CCobInstance *owner)
: owner(owner), script(script) 
{
	state = Init;
	owner->threads.push_back(this);
	AddDeathDependence(owner);
	signalMask = 42;
}

//Inform the vultures that we finally croaked
CCobThread::~CCobThread(void)
{
	if (callback != NULL) {
		//info->AddLine("%s callback with %d", script.scriptNames[callStack.back().functionId].c_str(), retCode);
		(*callback)(retCode, cbParam1, cbParam2);
	}
	if(owner)
		owner->threads.remove(this);
}

//Sets a callback that will be called when the thread dies. There can be only one.
void CCobThread::SetCallback(CBCobThreadFinish cb, void *p1, void *p2)
{
	callback = cb;
	cbParam1 = p1;
	cbParam2 = p2;
}

//This function sets the thread in motion. Should only be called once.
//If schedule is false the thread is not added to the scheduler, and thus
//it is expected that the starter is responsible for ticking it.
void CCobThread::Start(int functionId, const vector<long> &args, bool schedule) 
{
	state = Run;
	PC = script.scriptOffsets[functionId];

	struct callInfo ci;
	ci.functionId = functionId;
	ci.returnAddr = -1;
	ci.stackTop = 0;
	callStack.push_back(ci);
	paramCount = 0;
	signalMask = 0;
	callback = NULL;
	retCode = -1;

	for(vector<long>::const_iterator i = args.begin(); i != args.end(); ++i) {
		stack.push_back(*i);
		paramCount++;
	}

	//Add to scheduler
	if (schedule)
		GCobEngine.AddThread(this);
}

const string &CCobThread::GetName()
{
	return script.scriptNames[callStack[0].functionId];
}

//Returns the value at pos in the stack. Neater than exposing the actual stack
int CCobThread::GetStackVal(int pos)
{
	return stack[pos];
}

int CCobThread::GetWakeTime() const
{
	return wakeTime;
}

//Command documentation from http://visualta.tauniverse.com/Downloads/cob-commands.txt
//And some information from basm0.8 source (basm ops.txt)

//Model interaction
const long int MOVE = 0x10001000;
const long int TURN = 0x10002000;
const long int SPIN = 0x10003000;
const long int STOP_SPIN = 0x10004000;
const long int SHOW = 0x10005000;
const long int HIDE = 0x10006000;
const long int CACHE = 0x10007000;
const long int DONT_CACHE = 0x10008000;
const long int MOVE_NOW = 0x1000B000;
const long int TURN_NOW = 0x1000C000;
const long int SHADE = 0x1000D000;
const long int DONT_SHADE = 0x1000E000;
const long int EMIT_SFX = 0x1000F000;

//Blocking operations
const long int WAIT_TURN = 0x10011000;
const long int WAIT_MOVE = 0x10012000;
const long int SLEEP = 0x10013000;

//Stack manipulation
const long int PUSH_CONSTANT = 0x10021001;
const long int PUSH_LOCAL_VAR = 0x10021002;
const long int PUSH_STATIC = 0x10021004;
const long int CREATE_LOCAL_VAR = 0x10022000;
const long int POP_LOCAL_VAR = 0x10023002;
const long int POP_STATIC = 0x10023004;
const long int POP_STACK = 0x10024000;		// Not sure what this is supposed to do

//Arithmetic operations
const long int ADD = 0x10031000;
const long int SUB = 0x10032000;
const long int MUL = 0x10033000;
const long int DIV = 0x10034000;
const long int BITWISE_AND = 0x10035000;
const long int BITWISE_OR = 0x10036000;
const long int BITWISE_XOR = 0x10037000;
const long int BITWISE_NOT = 0x10038000;

//Native function calls
const long int RAND = 0x10041000;
const long int GET_UNIT_VALUE = 0x10042000;
const long int GET = 0x10043000;

//Comparison
const long int SET_LESS = 0x10051000;
const long int SET_LESS_OR_EQUAL = 0x10052000;
const long int SET_GREATER = 0x10053000;
const long int SET_GREATER_OR_EQUAL = 0x10054000;
const long int SET_EQUAL = 0x10055000;
const long int SET_NOT_EQUAL = 0x10056000;
const long int LOGICAL_AND = 0x10057000;
const long int LOGICAL_OR = 0x10058000;
const long int LOGICAL_XOR = 0x10059000;
const long int LOGICAL_NOT = 0x1005A000;

//Flow control
const long int START = 0x10061000;
const long int CALL = 0x10062000;
const long int JUMP = 0x10064000;
const long int RETURN = 0x10065000;
const long int JUMP_NOT_EQUAL = 0x10066000;
const long int SIGNAL = 0x10067000;
const long int SET_SIGNAL_MASK = 0x10068000;

//Piece destruction
const long int EXPLODE = 0x10071000;
const long int PLAY_SOUND = 0x10072000;

//Special functions
const long int SET = 0x10082000;
const long int ATTACH = 0x10083000;
const long int DROP = 0x10084000;

//Handy macros
#define GET_LONG_PC() (script.code[PC++])
//#define POP() (stack.size() > 0) ? stack.back(), stack.pop_back(); : 0

int CCobThread::POP(void)
{
	if (stack.size() > 0) {
		int r = stack.back();
		stack.pop_back();
		return r;
	}
	else
		return 0;
}

//Returns -1 if this thread is dead and needs to be killed
int CCobThread::Tick(int deltaTime)
{
	if (state == Sleep) {
		info->AddLine("CobError: sleeping thread ticked!");
	}
	if (state == Dead || !owner) {
		return -1;
	}

	state = Run;
	GCobEngine.SetCurThread(this);

	long r1, r2, r3, r4, r5, r6;
	vector<long> args;
	CCobThread *thread;

	execTrace.clear();
	delayedAnims.clear();
	//list<long>::iterator ei;
	vector<long>::iterator ei;

#if COB_DEBUG > 0
	if (COB_DEBUG_FILTER)
		info->AddLine("Executing in %s (from %s)", script.scriptNames[callStack.back().functionId].c_str(), GetName().c_str());
#endif

	while (state == Run) {
		//long opcode = *(long *)&script.code[PC];
		
		//Disabling exec trace gives about a 50% speedup on vm-intensive code
		//execTrace.push_back(PC);

		long opcode = GET_LONG_PC();

#if COB_DEBUG > 1
		if (COB_DEBUG_FILTER)
			info->AddLine("PC: %x opcode: %x (%s)", PC - 1, opcode, GetOpcodeName(opcode).c_str());
#endif

		switch(opcode) {
			case PUSH_CONSTANT:
				r1 = GET_LONG_PC();
				stack.push_back(r1);
				break;
			case SLEEP:
				r1 = POP();
				wakeTime = GCurrentTime + r1;
				state = Sleep;
				GCobEngine.AddThread(this);
				GCobEngine.SetCurThread(NULL);

#if COB_DEBUG > 0
				if (COB_DEBUG_FILTER)
					info->AddLine("%s sleeping for %d ms", script.scriptNames[callStack.back().functionId].c_str(), r1);
#endif
				return 0;
			case SPIN:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();				//speed
				r4 = POP();				//accel
				owner->Spin(r1, r2, r3, r4);
				break;
			case STOP_SPIN:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();				//decel
				//info->AddLine("Stop spin of %s around %d", script.pieceNames[r1].c_str(), r2);
				owner->StopSpin(r1, r2, r3);
				break;
			case RETURN:
				retCode = POP();
				if (callStack.back().returnAddr == -1) {

#if COB_DEBUG > 0
					if (COB_DEBUG_FILTER)
						info->AddLine("%s returned %d", script.scriptNames[callStack.back().functionId].c_str(), retCode);
#endif

					state = Dead;
					GCobEngine.SetCurThread(NULL);
					//callStack.pop_back();
					//Leave values intact on stack in case caller wants to check them
					return -1;
				}
				
				PC = callStack.back().returnAddr;
				while (stack.size() > callStack.back().stackTop) {
					stack.pop_back();
				}
				callStack.pop_back();

#if COB_DEBUG > 0
				if (COB_DEBUG_FILTER)
					info->AddLine("Returning to %s", script.scriptNames[callStack.back().functionId].c_str());
#endif

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
			case CALL:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (script.scriptLengths[r1] == 0) {
					//info->AddLine("Preventing call to zero-len script %s", script.scriptNames[r1].c_str());
					break;
				}

				struct callInfo ci;
				ci.functionId = r1;
				ci.returnAddr = PC;
				ci.stackTop = stack.size() - r2;
				callStack.push_back(ci);
				paramCount = r2;

				PC = script.scriptOffsets[r1];
#if COB_DEBUG > 0
				if (COB_DEBUG_FILTER)
					info->AddLine("Calling %s", script.scriptNames[r1].c_str());
#endif

				break;
			case POP_STATIC:
				r1 = GET_LONG_PC();
				r2 = POP();
				owner->staticVars[r1] = r2;
				//info->AddLine("Pop static var %d val %d", r1, r2);
				break;
			case POP_STACK:
				break;
			case START:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();

				if (script.scriptLengths[r1] == 0) {
					//info->AddLine("Preventing start of zero-len script %s", script.scriptNames[r1].c_str());
					break;
				}

				args.clear();
				for (r3 = 0; r3 < r2; ++r3) {
					r4 = POP();
					args.push_back(r4);
				}		

				thread = new CCobThread(script, owner);
				thread->Start(r1, args, true);

				//Seems that threads should inherit signal mask from creator
				thread->signalMask = signalMask;

#if COB_DEBUG > 0
				if (COB_DEBUG_FILTER)
					info->AddLine("Starting %s %d", script.scriptNames[r1].c_str(), signalMask);
#endif

				break;
			case CREATE_LOCAL_VAR:
				if (paramCount == 0) {
					stack.push_back(0);
				}
				else {
					paramCount--;
				}
				break;
			case GET_UNIT_VALUE:
				r1 = POP();
				ForceCommitAllAnims();			// getunitval could possibly read piece locations
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
				//this seem to be an error in the docs..
				//r2 = script.scriptOffsets[callStack.back().functionId] + r1;
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
			case BITWISE_OR:	//seems to want stack contents or'd, result places on stack
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
				stack.push_back(!r1);
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
				//info->AddLine("Push static %d val %d", r1, owner->staticVars[r1]);
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
				//info->AddLine("Turning piece %s axis %d to %d speed %d", script.pieceNames[r3].c_str(), r4, r2, r1);
				ForceCommitAnim(1, r3, r4);
				owner->Turn(r3, r4, r1, r2);
				break;
			case GET:
				r5 = POP();
				r4 = POP();
				r3 = POP();
				r2 = POP();
				r1 = POP();
				ForceCommitAllAnims();
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
					r3 = 1000;	//infinity!
					info->AddLine("CobError: division by zero");
				}
				stack.push_back(r3);
				break;
			case MOVE:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r4 = POP();
				r3 = POP();
				ForceCommitAnim(2, r1, r2);
				owner->Move(r1, r2, r3, r4);
				break;
			case MOVE_NOW:{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();

				if (owner->smoothAnim) {
					DelayedAnim a;
					a.type = 2;
					a.piece = r1;
					a.axis = r2;
					a.dest = r3;
					delayedAnims.push_back(a);

					//info->AddLine("Delayed move %s %d %d", owner->pieces[r1].name.c_str(), r2, r3);
				}
				else {
					owner->MoveNow(r1, r2, r3);
				}

				break;}
			case TURN_NOW:{
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				r3 = POP();
				
				if (owner->smoothAnim) {
					DelayedAnim a;
					a.type = 1;
					a.piece = r1;
					a.axis = r2;
					a.dest = r3;
					delayedAnims.push_back(a);
				}
				else {
					owner->TurnNow(r1, r2, r3);
				}

				break;}
			case WAIT_TURN:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				//info->AddLine("Waiting for turn on piece %s around axis %d", script.pieceNames[r1].c_str(), r2);
				if (owner->AddTurnListener(r1, r2, this)) {
					state = WaitTurn;
					GCobEngine.SetCurThread(NULL);
					return 0;
				}
				else
					break;
			case WAIT_MOVE:
				r1 = GET_LONG_PC();
				r2 = GET_LONG_PC();
				//info->AddLine("Waiting for move on piece %s on axis %d", script.pieceNames[r1].c_str(), r2);
				if (owner->AddMoveListener(r1, r2, this)) {
					state = WaitMove;
					GCobEngine.SetCurThread(NULL);
					return 0;
				}
				break;
			case SET:
				r2 = POP();
				r1 = POP();
				//info->AddLine("Setting unit value %d to %d", r1, r2);
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
			case LOGICAL_NOT:		//Like bitwise, but only on values 1 and 0.
				r1 = POP();
				if (r1 == 0)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case LOGICAL_AND:
				r1 = POP();
				r2 = POP();
				if (r1 & r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case LOGICAL_OR:
				r1 = POP();
				r2 = POP();
				if (r1 | r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case LOGICAL_XOR:
				r1 = POP();
				r2 = POP();
				if (r1 ^ r2)
					stack.push_back(1);
				else
					stack.push_back(0);
				break;
			case HIDE:
				r1 = GET_LONG_PC();
				owner->SetVisibility(r1, false);
				//info->AddLine("Hiding %d", r1);
				break;
			case SHOW:{
				r1 = GET_LONG_PC();
				int i;
				for (i = 0; i < COB_MaxWeapons; ++i)
					if (callStack.back().functionId == script.scriptIndex[COBFN_FirePrimary + i])
						break;

				// If true, we are in a Fire-script and should show a special flare effect
				if (i < COB_MaxWeapons) {
					owner->ShowFlare(r1);
				}
				else {
					owner->SetVisibility(r1, true);
				}
				//info->AddLine("Showing %d", r1);
				break;}
			default:
				info->AddLine("CobError: Unknown opcode %x (in %s:%s at %x)", opcode, script.name.c_str(), script.scriptNames[callStack.back().functionId].c_str(), PC - 1);
				info->AddLine("Exec trace:");
				ei = execTrace.begin();
				while (ei != execTrace.end()) {
					info->AddLine("PC: %3x  opcode: %s", *ei, GetOpcodeName(script.code[*ei]).c_str());
					ei++;
				}
				state = Dead;
				GCobEngine.SetCurThread(NULL);
				return -1;
				break;
		}
		
	}

	GCobEngine.SetCurThread(NULL);
	return 0;
}

// Shows an errormessage which includes the current state of the script interpreter
void CCobThread::ShowError(const string& msg)
{
	if (callStack.size() == 0)
		info->AddLine("CobError: %s outside script execution (?)", msg.c_str());
	else 
		info->AddLine("CobError: %s (in %s:%s at %x)", msg.c_str(), script.name.c_str(), script.scriptNames[callStack.back().functionId].c_str(), PC - 1);
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

void CCobThread::DependentDied(CObject* o)
{
	if(o==owner)
		owner=0;
}

void CCobThread::CommitAnims(int deltaTime)
{
	for (vector<DelayedAnim>::iterator anim = delayedAnims.begin(); anim != delayedAnims.end(); ++anim) {

		//Only consider smoothing when the thread is sleeping for a short while, but not too short
		int delta = wakeTime - GCurrentTime;
		bool smooth = (state == Sleep) && (delta < 300) && (delta > deltaTime);

//		info->AddLine("Commiting %s type %d %d", owner->pieces[anim->piece].name.c_str(), smooth, anim->dest);

		switch (anim->type) {
			case 1:
				if (smooth)
					owner->TurnSmooth(anim->piece, anim->axis, anim->dest, delta, deltaTime);
				else
					owner->TurnNow(anim->piece, anim->axis, anim->dest);
				break;
			case 2:
				if (smooth)
					owner->MoveSmooth(anim->piece, anim->axis, anim->dest, delta, deltaTime);
				else
					owner->MoveNow(anim->piece, anim->axis, anim->dest);
				break;
		}
	}
	delayedAnims.clear();
}

void CCobThread::ForceCommitAnim(int type, int piece, int axis) 
{
	for (vector<DelayedAnim>::iterator anim = delayedAnims.begin(); anim != delayedAnims.end(); ++anim) {
		if ((anim->type == type) && (anim->piece == piece) && (anim->axis == axis)) {
			switch (type) {
				case 1:
					owner->TurnNow(piece, axis, anim->dest);
					break;
				case 2:
					owner->MoveNow(piece, axis, anim->dest);
					break;
			}

			//Remove it so it does not interfere later
			delayedAnims.erase(anim);
			return;
		}
	}
}

void CCobThread::ForceCommitAllAnims() 
{
	for (vector<DelayedAnim>::iterator anim = delayedAnims.begin(); anim != delayedAnims.end(); ++anim) {
		switch (anim->type) {
			case 1:
				owner->TurnNow(anim->piece, anim->axis, anim->dest);
				break;
			case 2:
				owner->MoveNow(anim->piece, anim->axis, anim->dest);
				break;
		}
	}
	delayedAnims.clear();
}


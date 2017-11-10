/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "SerializeLuaState.h"
#include "System/UnorderedMap.hpp"
#include <deque>

struct creg_lua_State;
struct creg_Proto;
struct creg_UpVal;


#define ASSERT_SIZE(structName) static_assert(sizeof(creg_ ## structName) == sizeof(structName), #structName " Size mismatch");
#define cregCommonHeader creg_GCObjectPtr next; lu_byte tt; lu_byte marked
#define CR_COMMON_HEADER() CR_MEMBER(next), CR_MEMBER(tt), CR_MEMBER(marked)

class LuaAllocator{
public:
	LuaAllocator() : lcd(nullptr) { }
	void* alloc(size_t n) {
		assert(lcd != nullptr);
		return spring_lua_alloc(lcd, NULL, 0, n);
	}
	void SetContext(luaContextData* newLcd) { lcd = newLcd; }
private:
	luaContextData* lcd;
};


static LuaAllocator luaAllocator;


// TString structs have variable size which c++ and/or creg
// don't support too well, so they're handled separately.
static spring::unsynced_map<TString*, int> stringToIdx;
static std::deque<TString*> stringVec;
static spring::unsynced_map<TString**, int> stringPtrToIdx;






/*
 * Ptr types
 */


struct creg_TStringPtr {
	CR_DECLARE_STRUCT(creg_TStringPtr)
	TString* ts;
	void Serialize(creg::ISerializer* s);
};


struct creg_GCObjectPtr {
	CR_DECLARE_STRUCT(creg_GCObjectPtr)
	union {
		GCheader* gch;
		creg_TStringPtr ts;
		//struct Udata u;
		//struct Closure cl;
		//struct Table h;
		creg_Proto* p;
		creg_UpVal* uv;
		creg_lua_State* th;
	} p;
	void Serialize(creg::ISerializer* s);
};


/*
 * Converted from lobject.h
 */
union creg_Value{
  creg_GCObjectPtr gc;
  void *p;
  lua_Number n;
  int b;
};

struct creg_TValue {
	CR_DECLARE_STRUCT(creg_TValue)
	creg_Value value;
	int tt;
	void Serialize(creg::ISerializer* s);
};

ASSERT_SIZE(TValue)


struct creg_LocVar {
	CR_DECLARE_STRUCT(creg_LocVar)
	creg_TStringPtr varname;
	int startpc;  /* first point where variable is active */
	int endpc;    /* first point where variable is dead */
};

ASSERT_SIZE(LocVar)


struct creg_Proto {
	CR_DECLARE_STRUCT(creg_Proto)
	cregCommonHeader;
	creg_TValue *k;  /* constants used by the function */
	Instruction *code;
	creg_Proto **p;  /* functions defined inside the function */
	int *lineinfo;  /* map from opcodes to source lines */
	creg_LocVar *locvars;  /* information about local variables */
	creg_TStringPtr *upvalues;  /* upvalue names */
	creg_TStringPtr  source;
	int sizeupvalues;
	int sizek;  /* size of `k' */
	int sizecode;
	int sizelineinfo;
	int sizep;  /* size of `p' */
	int sizelocvars;
	int linedefined;
	int lastlinedefined;
	creg_GCObjectPtr gclist;
	lu_byte nups;  /* number of upvalues */
	lu_byte numparams;
	lu_byte is_vararg;
	lu_byte maxstacksize;
	void Serialize(creg::ISerializer* s);
};


ASSERT_SIZE(Proto)


struct creg_UpVal {
	CR_DECLARE_STRUCT(creg_UpVal)
	cregCommonHeader;
	creg_TValue *v;  /* points to stack or to its own value */
	union {
		creg_TValue value;  /* the value (when closed) */
		struct {  /* double linked list (when open) */
			struct creg_UpVal *prev;
			struct creg_UpVal *next;
		} l;
	} u;
	void Serialize(creg::ISerializer* s);
};

ASSERT_SIZE(UpVal)

/*
 * Converted from lstate.h
 */

struct creg_global_State {
	CR_DECLARE_STRUCT(creg_global_State)
	stringtable strt;  /* hash table for strings */
	lua_Alloc frealloc;  /* function to reallocate memory */
	void *ud;         /* auxiliary data to `frealloc' */
	lu_byte currentwhite;
	lu_byte gcstate;  /* state of garbage collector */
	int sweepstrgc;  /* position of sweep in `strt' */
	creg_GCObjectPtr rootgc;  /* list of all collectable objects */
	GCObject **sweepgc;  /* position of sweep in `rootgc' */
	creg_GCObjectPtr gray;  /* list of gray objects */
	creg_GCObjectPtr grayagain;  /* list of objects to be traversed atomically */
	creg_GCObjectPtr weak;  /* list of weak tables (to be cleared) */
	creg_GCObjectPtr tmudata;  /* last element of list of userdata to be GC */
	Mbuffer buff;  /* temporary buffer for string concatentation */
	lu_mem GCthreshold;
	lu_mem totalbytes;  /* number of bytes currently allocated */
	lu_mem estimate;  /* an estimate of number of bytes actually in use */
	lu_mem gcdept;  /* how much GC is `behind schedule' */
	int gcpause;  /* size of pause between successive GCs */
	int gcstepmul;  /* GC `granularity' */
	lua_CFunction panic;  /* to be called in unprotected errors */
	creg_TValue l_registry;
	creg_lua_State *mainthread;
	creg_UpVal uvhead;  /* head of double-linked list of all open upvalues */
	struct Table *mt[NUM_TAGS];  /* metatables for basic types */
	creg_TStringPtr tmname[TM_N];  /* array with tag-method names */

	//SPRING additions
	lua_Func_fopen  fopen_func;
	lua_Func_popen  popen_func;
	lua_Func_pclose pclose_func;
	lua_Func_system system_func;
	lua_Func_remove remove_func;
	lua_Func_rename rename_func;
};

ASSERT_SIZE(global_State)


struct creg_lua_State {
	CR_DECLARE_STRUCT(creg_lua_State)
	cregCommonHeader;
	lu_byte status;
	StkId top;  /* first free slot in the stack */
	StkId base;  /* base of current function */
	creg_global_State *l_G;
	CallInfo *ci;  /* call info for current function */
	const Instruction *savedpc;  /* `savedpc' of current function */
	StkId stack_last;  /* last free slot in the stack */
	StkId stack;  /* stack base */
	CallInfo *end_ci;  /* points after end of ci array*/
	CallInfo *base_ci;  /* array of CallInfo's */
	int stacksize;
	int size_ci;  /* size of array `base_ci' */
	unsigned short nCcalls;  /* number of nested C calls */
	unsigned short baseCcalls;  /* nested C calls when resuming coroutine */
	lu_byte hookmask;
	lu_byte allowhook;
	int basehookcount;
	int hookcount;
	lua_Hook hook;
	creg_TValue l_gt;  /* table of globals */
	creg_TValue env;  /* temporary place for environments */
	creg_GCObjectPtr openupval;  /* list of open upvalues in this stack */
	creg_GCObjectPtr gclist;
	struct lua_longjmp *errorJmp;  /* current error recover point */
	ptrdiff_t errfunc;  /* current error handling function (stack index) */
};

ASSERT_SIZE(lua_State)


struct creg_LG {
	CR_DECLARE_STRUCT(creg_LG)
	creg_lua_State l;
	creg_global_State g;
};


CR_BIND(creg_TStringPtr, )
CR_REG_METADATA(creg_TStringPtr, (
	CR_IGNORED(ts), // late serialized ptr
	CR_SERIALIZER(Serialize)
))


CR_BIND(creg_TValue, )
CR_REG_METADATA(creg_TValue, (
	CR_IGNORED(value), //union
	CR_MEMBER(tt),
	CR_SERIALIZER(Serialize)
))


CR_BIND(creg_LocVar, )
CR_REG_METADATA(creg_LocVar, (
	CR_MEMBER(varname),
	CR_MEMBER(startpc),
	CR_MEMBER(endpc)
))


CR_BIND(creg_Proto, )
CR_REG_METADATA(creg_Proto, (
	CR_COMMON_HEADER(),
	CR_MEMBER(k), // vector
	CR_IGNORED(code), // vector
	CR_IGNORED(p), // vector
	CR_IGNORED(lineinfo), // vector
	CR_IGNORED(locvars), // vector
	CR_IGNORED(upvalues), // vector
	CR_MEMBER(source),
	CR_MEMBER(sizeupvalues),
	CR_MEMBER(sizek),
	CR_MEMBER(sizecode),
	CR_MEMBER(sizelineinfo),
	CR_MEMBER(sizep),
	CR_MEMBER(sizelocvars),
	CR_MEMBER(linedefined),
	CR_MEMBER(lastlinedefined),
	CR_MEMBER(gclist),
	CR_MEMBER(nups),
	CR_MEMBER(numparams),
	CR_MEMBER(is_vararg),
	CR_MEMBER(maxstacksize),
	CR_SERIALIZER(Serialize)
))


CR_BIND(creg_UpVal, )
CR_REG_METADATA(creg_UpVal, (
	CR_COMMON_HEADER(),
	CR_MEMBER(v),
	CR_IGNORED(u), //union
	CR_SERIALIZER(Serialize)
))


CR_BIND(creg_GCObjectPtr, )
CR_REG_METADATA(creg_GCObjectPtr, (
	CR_IGNORED(p), //union
	CR_SERIALIZER(Serialize)
))



CR_BIND(creg_global_State, )
CR_REG_METADATA(creg_global_State, (
	CR_IGNORED(strt),
	CR_IGNORED(frealloc),
	CR_IGNORED(ud),
	CR_MEMBER(currentwhite),
	CR_MEMBER(gcstate),
	CR_MEMBER(sweepstrgc),
	CR_MEMBER(rootgc),
	CR_IGNORED(sweepgc),
	CR_MEMBER(gray),
	CR_MEMBER(grayagain),
	CR_MEMBER(weak),
	CR_IGNORED(tmudata),
	CR_IGNORED(buff),
	CR_MEMBER(GCthreshold),
	CR_MEMBER(totalbytes),
	CR_MEMBER(estimate),
	CR_MEMBER(gcdept),
	CR_MEMBER(gcpause),
	CR_MEMBER(gcstepmul),
	CR_IGNORED(panic),
	CR_MEMBER(l_registry),
	CR_MEMBER(mainthread),
	CR_MEMBER(uvhead),
	CR_IGNORED(mt),
	CR_MEMBER(tmname),
	CR_IGNORED(fopen_func),
	CR_IGNORED(popen_func),
	CR_IGNORED(pclose_func),
	CR_IGNORED(system_func),
	CR_IGNORED(remove_func),
	CR_IGNORED(rename_func)
))

CR_BIND(creg_lua_State, )
CR_REG_METADATA(creg_lua_State, (
	CR_COMMON_HEADER(),
	CR_MEMBER(status),
	CR_IGNORED(top),
	CR_IGNORED(base),
	CR_MEMBER(l_G),
	CR_IGNORED(ci),
	CR_IGNORED(savedpc),
	CR_IGNORED(stack_last),
	CR_IGNORED(stack),
	CR_IGNORED(end_ci),
	CR_IGNORED(base_ci),
	CR_MEMBER(stacksize),
	CR_MEMBER(size_ci),
	CR_MEMBER(nCcalls),
	CR_MEMBER(baseCcalls),
	CR_MEMBER(hookmask),
	CR_MEMBER(allowhook),
	CR_MEMBER(basehookcount),
	CR_MEMBER(hookcount),
	CR_IGNORED(hook),
	CR_MEMBER(l_gt),
	CR_MEMBER(env),
	CR_MEMBER(openupval),
	CR_MEMBER(gclist),
	CR_IGNORED(errorJmp),
	CR_MEMBER(errfunc)
))


CR_BIND(creg_LG, )
CR_REG_METADATA(creg_LG, (
	CR_MEMBER(l),
	CR_MEMBER(g)
))


template<typename T, typename C>
inline void SerializeCVector(creg::ISerializer* s, T** vecPtr, C* count)
{
	s->SerializeInt(count, sizeof(*count));
	std::unique_ptr<creg::IType> elemType = creg::DeduceType<T>::Get();
	T* vec;
	if (!(s->IsWriting())) {
		vec = (T*) luaAllocator.alloc(*count * sizeof(T));
		*vecPtr = vec;
	} else {
		vec = *vecPtr;
	}

	for (unsigned i = 0; i < unsigned(*count); ++i) {
		elemType->Serialize(s, &vec[i]);
	}
}

template<typename T>
void SerializePtr(creg::ISerializer* s, T** t) {
	creg::ObjectPointerType<T> opt;
	opt.Serialize(s, t);
}

template<typename T>
void SerializeInstance(creg::ISerializer* s, T* t) {
	s->SerializeObjectInstance(t, t->GetClass());
}


void creg_TStringPtr::Serialize(creg::ISerializer* s)
{
	int idx;
	if (s->IsWriting()) {
		auto it = stringToIdx.find(ts);
		if (it == stringToIdx.end()) {
			idx = stringVec.size();
			stringVec.push_back(ts);
			stringToIdx[ts] = idx;
		} else {
			idx = stringToIdx[ts];
		}
	}

	s->SerializeInt(&idx, sizeof(idx));

	if (!(s->IsWriting())) {
		stringPtrToIdx[&ts] = idx;
	}
}


void creg_TValue::Serialize(creg::ISerializer* s)
{
	switch(tt) {
		case LUA_TNIL: { return; }
		case LUA_TBOOLEAN: { s->SerializeInt(&value.b, sizeof(value.b)); return; }
		case LUA_TLIGHTUSERDATA: { return; }
		case LUA_TNUMBER: { s->SerializeInt(&value.n, sizeof(value.n)); return; }
		case LUA_TSTRING: {SerializeInstance(s, &value.gc.p.ts); return; }
		case LUA_TTABLE: { return; }
		case LUA_TFUNCTION: { return; }
		case LUA_TUSERDATA: { return; }
		case LUA_TTHREAD: { SerializePtr(s, &value.gc.p.th); return; }
		default: { assert(false); return; }
	}
}


void creg_UpVal::Serialize(creg::ISerializer* s)
{
	bool closed;
	if (s->IsWriting())
		closed = (v == &u.value);

	s->SerializeInt(&closed, sizeof(closed));

	if (closed) {
		s->SerializeObjectInstance(&u.value, u.value.GetClass());
	} else {
		creg::ObjectPointerType<creg_UpVal> opt;
		opt.Serialize(s, &u.l.prev);
		opt.Serialize(s, &u.l.next);
	}
}

void creg_Proto::Serialize(creg::ISerializer* s)
{
	SerializeCVector(s, &k,        &sizek);
	SerializeCVector(s, &code,     &sizecode);
	SerializeCVector(s, &p,        &sizep);
	SerializeCVector(s, &lineinfo, &sizelineinfo);
	SerializeCVector(s, &locvars,  &sizelocvars);
	SerializeCVector(s, &upvalues, &sizeupvalues);
}

void creg_GCObjectPtr::Serialize(creg::ISerializer* s)
{
	switch(p.gch->tt) {
		case LUA_TSTRING: { SerializeInstance(s, &p.ts); return; }
		case LUA_TUSERDATA: { return; }
		case LUA_TFUNCTION: { return; }
		case LUA_TTABLE: { return; }
		case LUA_TPROTO: { SerializePtr(s, &p.p); return; }
		case LUA_TUPVAL: { SerializePtr(s, &p.uv); return; }
		case LUA_TTHREAD: { SerializePtr(s, &p.th); return; }
		default: { assert(false); return; }
	}
}


namespace creg {

void SerializeLuaState(creg::ISerializer* s, lua_State** L, luaContextData& lcd) {
	assert(stringToIdx.empty());
	assert(stringVec.empty());
	assert(stringPtrToIdx.empty());

	luaAllocator.SetContext(&lcd);

	creg_LG* clg;
	if (s->IsWriting()) {
		assert(*L != nullptr);
		clg = (creg_LG*) *L;
	} else {
		assert(*L == nullptr);
		clg = (creg_LG*) luaAllocator.alloc(sizeof(creg_LG) + LUAI_EXTRASPACE);
		*L = (lua_State*) &(clg->l);
	}

	SerializeInstance(s, clg);

	// TString - see comment above
	if (s->IsWriting()) {
		int count = stringVec.size();
		s->SerializeInt(&count, sizeof(count));
		for (TString* ts: stringVec) {
			// Serialize length first, so we know how much to alloc
			s->SerializeInt(&ts->tsv.len, sizeof(ts->tsv.len));
			s->Serialize(ts + 1, ts->tsv.len);

			SerializeInstance(s, (creg_GCObjectPtr*) &ts->tsv.next);
			// no need to serialize tt
			s->SerializeInt(&ts->tsv.marked, sizeof(ts->tsv.marked));
			s->SerializeInt(&ts->tsv.reserved, sizeof(ts->tsv.reserved));
		}
	} else {
		int count;
		s->SerializeInt(&count, sizeof(count));
		stringVec.resize(count);
		for (int i = 0; i < count; ++i) {
			size_t length;
			s->SerializeInt(&length, sizeof(length));
			TString* ts = (TString*) luaAllocator.alloc((length + 1) * sizeof(char) + sizeof(TString));
			s->Serialize(ts + 1, ts->tsv.len);

			SerializeInstance(s, (creg_GCObjectPtr*) &ts->tsv.next);
			s->SerializeInt(&ts->tsv.marked, sizeof(ts->tsv.marked));
			s->SerializeInt(&ts->tsv.reserved, sizeof(ts->tsv.reserved));

			ts->tsv.tt = LUA_TSTRING;
			ts->tsv.hash = lua_calchash(getstr(ts), length);
			ts->tsv.len = length;
			stringVec[i] = ts;
		}
		for (auto& it: stringPtrToIdx) {
			*(it.first) = stringVec[it.second];
		}
	}


	luaAllocator.SetContext(nullptr);
	stringToIdx.clear();
	stringVec.clear();
	stringPtrToIdx.clear();
}

}


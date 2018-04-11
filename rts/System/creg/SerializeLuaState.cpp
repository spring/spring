/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "SerializeLuaState.h"

#include "VarTypes.h"

#include "System/UnorderedMap.hpp"
#include <deque>

struct creg_lua_State;
struct creg_Proto;
struct creg_UpVal;
struct creg_Node;
struct creg_Table;
struct creg_GCObject;

#define ASSERT_SIZE(structName) static_assert(sizeof(creg_ ## structName) == sizeof(structName), #structName " Size mismatch");

static_assert(LUAI_EXTRASPACE == 0, "LUAI_EXTRASPACE isn't 0");

class LuaContext{
public:
	LuaContext() : context(nullptr), frealloc(nullptr), panic(nullptr) { }
	void* alloc(size_t n) const {
		assert(context != nullptr && frealloc != nullptr);
		return frealloc(context, NULL, 0, n);
	}
	template<typename T>
	T* alloc() const {
		assert(context != nullptr && frealloc != nullptr);
		return (T*) frealloc(context, NULL, 0, sizeof(T));
	}
	void SetContext(void* newLcd, lua_Alloc newfrealloc, lua_CFunction newPanic) { context = newLcd; frealloc = newfrealloc; panic = newPanic; }
	lua_CFunction GetPanic() const { return panic; }
	void* GetContext() const { return context; }
	lua_Alloc Getfrealloc() const { return frealloc; }
private:
	void* context;
	lua_Alloc frealloc;
	lua_CFunction panic;
};

void freeProtector(void *m) {
	assert(false);
}

void* allocProtector() {
	assert(false);
	return nullptr;
}


static LuaContext luaContext;


// TString structs have variable size which c++ and/or creg
// don't support too well, so they're handled separately.
static spring::unsynced_map<TString*, int> stringToIdx;
static std::deque<TString*> stringVec;
static spring::unsynced_map<TString**, int> stringPtrToIdx;

// Same for Closure structs
static spring::unsynced_map<Closure*, int> closureToIdx;
static std::deque<Closure*> closureVec;
static spring::unsynced_map<Closure**, int> closurePtrToIdx;

// And userdata
static spring::unsynced_map<Udata*, int> udataToIdx;
static std::deque<Udata*> udataVec;
static spring::unsynced_map<Udata**, int> udataPtrToIdx;


// C functions in lua have to be specially registered in order to
// be serialized correctly
static spring::unsynced_map<std::string, lua_CFunction> nameToFunc;
static spring::unsynced_map<lua_CFunction, std::string> funcToName;





/*
 * Ptr types
 */


struct creg_TStringPtr {
	CR_DECLARE_STRUCT(creg_TStringPtr)
	TString* ts;
	void Serialize(creg::ISerializer* s);
};


struct creg_ClosurePtr {
	CR_DECLARE_STRUCT(creg_ClosurePtr)
	Closure* c;
	void Serialize(creg::ISerializer* s);
};


struct creg_UdataPtr {
	CR_DECLARE_STRUCT(creg_UdataPtr)
	Udata* u;
	void Serialize(creg::ISerializer* s);
};


/*
 * Copied from lfunc.h
 */


#define sizeCclosure(n)	(cast(int, sizeof(CClosure)) + \
                         cast(int, sizeof(TValue)*((n)-1)))

#define sizeLclosure(n)	(cast(int, sizeof(LClosure)) + \
                         cast(int, sizeof(TValue *)*((n)-1)))


/*
 * Converted from lobject.h
 */

#define creg_CommonHeader creg_GCObject* next; lu_byte tt; lu_byte marked
#define CR_COMMON_HEADER() CR_MEMBER(next), CR_MEMBER(tt), CR_MEMBER(marked)

union creg_Value{
	creg_GCObject *gc;
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

union creg_TKey {
	struct {
		creg_Value value;
		int tt;
		creg_Node *next;  /* for chaining */
	} nk;
	creg_TValue tvk;
};


struct creg_Node {
	CR_DECLARE_STRUCT(creg_Node)
	creg_TValue i_val;
	creg_TKey i_key;
	void Serialize(creg::ISerializer* s);
};

ASSERT_SIZE(Node)


struct creg_Table {
	CR_DECLARE_STRUCT(creg_Table)
	creg_CommonHeader;
	lu_byte flags;  /* 1<<p means tagmethod(p) is not present */
	lu_byte lsizenode;  /* log2 of size of `node' array */
	creg_Table *metatable;
	creg_TValue *array;  /* array part */
	creg_Node *node;
	creg_Node *lastfree;  /* any free position is before this position */
	creg_GCObject *gclist;
	int sizearray;  /* size of `array' array */
	void Serialize(creg::ISerializer* s);
};

ASSERT_SIZE(Table)


struct creg_LocVar {
	CR_DECLARE_STRUCT(creg_LocVar)
	creg_TStringPtr varname;
	int startpc;  /* first point where variable is active */
	int endpc;    /* first point where variable is dead */
};

ASSERT_SIZE(LocVar)


struct creg_Proto {
	CR_DECLARE_STRUCT(creg_Proto)
	creg_CommonHeader;
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
	creg_GCObject *gclist;
	lu_byte nups;  /* number of upvalues */
	lu_byte numparams;
	lu_byte is_vararg;
	lu_byte maxstacksize;
	void Serialize(creg::ISerializer* s);
};


ASSERT_SIZE(Proto)


struct creg_UpVal {
	CR_DECLARE_STRUCT(creg_UpVal)
	creg_CommonHeader;
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

struct creg_stringtable {
	CR_DECLARE_STRUCT(creg_stringtable)
	creg_GCObject **hash;
	lu_int32 nuse;  /* number of elements */
	int size;
	void Serialize(creg::ISerializer* s);
};

ASSERT_SIZE(stringtable)


struct creg_global_State {
	CR_DECLARE_STRUCT(creg_global_State)
	creg_stringtable strt;  /* hash table for strings */
	lua_Alloc frealloc;  /* function to reallocate memory */
	void *ud;         /* auxiliary data to `frealloc' */
	lu_byte currentwhite;
	lu_byte gcstate;  /* state of garbage collector */
	int sweepstrgc;  /* position of sweep in `strt' */
	creg_GCObject *rootgc;  /* list of all collectable objects */
	creg_GCObject **sweepgc;  /* position of sweep in `rootgc' */
	creg_GCObject *gray;  /* list of gray objects */
	creg_GCObject *grayagain;  /* list of objects to be traversed atomically */
	creg_GCObject *weak;  /* list of weak tables (to be cleared) */
	creg_GCObject *tmudata;  /* last element of list of userdata to be GC */
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
	creg_Table *mt[NUM_TAGS];  /* metatables for basic types */
	creg_TStringPtr tmname[TM_N];  /* array with tag-method names */

	//SPRING additions
	lua_Func_fopen  fopen_func;
	lua_Func_popen  popen_func;
	lua_Func_pclose pclose_func;
	lua_Func_system system_func;
	lua_Func_remove remove_func;
	lua_Func_rename rename_func;
	void Serialize(creg::ISerializer* s);
};

ASSERT_SIZE(global_State)


struct creg_lua_State {
	CR_DECLARE_STRUCT(creg_lua_State)
	creg_CommonHeader;
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
	creg_GCObject *openupval;  /* list of open upvalues in this stack */
	creg_GCObject *gclist;
	struct lua_longjmp *errorJmp;  /* current error recover point */
	ptrdiff_t errfunc;  /* current error handling function (stack index) */
	void Serialize(creg::ISerializer* s);
};

ASSERT_SIZE(lua_State)


struct creg_GCObject {
	CR_DECLARE_STRUCT(creg_GCObject)
	union {
		GCheader gch;
		//creg_TString* ts;
		//creg_Udata* u;
		//creg_Closure* cl;
		creg_Table h;
		creg_Proto p;
		creg_UpVal uv;
		creg_lua_State th;
	} u;
	void Serialize(creg::ISerializer* s);
	size_t GetSize();
};


struct creg_LG {
	CR_DECLARE_STRUCT(creg_LG)
	creg_lua_State l;
	creg_global_State g;
};


CR_BIND_POOL(creg_TStringPtr, , allocProtector, freeProtector)
CR_REG_METADATA(creg_TStringPtr, (
	CR_IGNORED(ts), // late serialized ptr
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_ClosurePtr, , allocProtector, freeProtector)
CR_REG_METADATA(creg_ClosurePtr, (
	CR_IGNORED(c), // late serialized ptr
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_UdataPtr, , allocProtector, freeProtector)
CR_REG_METADATA(creg_UdataPtr, (
	CR_IGNORED(u), // late serialized ptr
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_GCObject, , luaContext.alloc, freeProtector)
CR_REG_METADATA(creg_GCObject, (
	CR_IGNORED(u), //union
	//CR_GETSIZE(GetSize),
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_TValue, , allocProtector, freeProtector)
CR_REG_METADATA(creg_TValue, (
	CR_IGNORED(value), //union
	CR_MEMBER(tt),
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_Node, , allocProtector, freeProtector)
CR_REG_METADATA(creg_Node, (
	CR_MEMBER(i_val),
	CR_IGNORED(i_key),
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_Table, , luaContext.alloc<creg_Table>, freeProtector)
CR_REG_METADATA(creg_Table, (
	CR_COMMON_HEADER(),
	CR_MEMBER(flags),
	CR_MEMBER(lsizenode),
	CR_MEMBER(metatable),
	CR_IGNORED(array), //vector
	CR_IGNORED(node), //vector
	CR_IGNORED(lastfree), //serialized separately
	CR_IGNORED(gclist), //probably unneeded
	CR_MEMBER(sizearray),
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_LocVar, , allocProtector, freeProtector)
CR_REG_METADATA(creg_LocVar, (
	CR_MEMBER(varname),
	CR_MEMBER(startpc),
	CR_MEMBER(endpc)
))


CR_BIND_POOL(creg_Proto, , luaContext.alloc<creg_Proto>, freeProtector)
CR_REG_METADATA(creg_Proto, (
	CR_COMMON_HEADER(),
	CR_IGNORED(k), // vector
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
	CR_IGNORED(gclist), //probably unneeded
	CR_MEMBER(nups),
	CR_MEMBER(numparams),
	CR_MEMBER(is_vararg),
	CR_MEMBER(maxstacksize),
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_UpVal, , luaContext.alloc<creg_UpVal>, freeProtector)
CR_REG_METADATA(creg_UpVal, (
	CR_COMMON_HEADER(),
	CR_MEMBER(v),
	CR_IGNORED(u), //union
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_stringtable, , allocProtector, freeProtector)
CR_REG_METADATA(creg_stringtable, (
	CR_IGNORED(hash), //vector
	CR_MEMBER(nuse),
	CR_MEMBER(size),
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_global_State, , allocProtector, freeProtector)
CR_REG_METADATA(creg_global_State, (
	CR_MEMBER(strt),
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
	CR_MEMBER(tmudata),
	CR_IGNORED(buff), // this is a temporary buffer, no need to store
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
	CR_MEMBER(mt),
	CR_MEMBER(tmname),
	CR_IGNORED(fopen_func),
	CR_IGNORED(popen_func),
	CR_IGNORED(pclose_func),
	CR_IGNORED(system_func),
	CR_IGNORED(remove_func),
	CR_IGNORED(rename_func),
	CR_SERIALIZER(Serialize)
))

CR_BIND_POOL(creg_lua_State, , luaContext.alloc<creg_lua_State>, freeProtector)
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
	CR_IGNORED(env), // temporary
	CR_MEMBER(openupval),
	CR_IGNORED(gclist), //probably unneeded
	CR_IGNORED(errorJmp),
	CR_MEMBER(errfunc),
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_LG, , allocProtector, freeProtector)
CR_REG_METADATA(creg_LG, (
	CR_MEMBER(l),
	CR_MEMBER(g)
))


template<typename T, typename C>
inline void SerializeCVector(creg::ISerializer* s, T** vecPtr, C count)
{
	std::unique_ptr<creg::IType> elemType = creg::DeduceType<T>::Get();
	T* vec;
	if (!(s->IsWriting())) {
		vec = (T*) luaContext.alloc(count * sizeof(T));
		*vecPtr = vec;
	} else {
		vec = *vecPtr;
	}

	for (unsigned i = 0; i < unsigned(count); ++i) {
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
		if (ts == nullptr) {
			idx = 0;
		} else {
			auto it = stringToIdx.find(ts);
			if (it == stringToIdx.end()) {
				idx = stringVec.size();
				stringVec.push_back(ts);
				stringToIdx[ts] = idx + 1;
			} else {
				idx = stringToIdx[ts];
			}
		}
	}

	s->SerializeInt(&idx, sizeof(idx));

	if (!(s->IsWriting())) {
		if (idx == 0) {
			ts = nullptr;
		} else {
			stringPtrToIdx[&ts] = idx - 1;
		}
	}
}


void creg_ClosurePtr::Serialize(creg::ISerializer* s)
{
	int idx;
	if (s->IsWriting()) {
		if (c == nullptr) {
			idx = 0;
		} else {
			auto it = closureToIdx.find(c);
			if (it == closureToIdx.end()) {
				idx = closureVec.size() + 1;
				closureVec.push_back(c);
				closureToIdx[c] = idx;
			} else {
				idx = closureToIdx[c];
			}
		}
	}

	s->SerializeInt(&idx, sizeof(idx));

	if (!(s->IsWriting())) {
		if (idx == 0) {
			c = nullptr;
		} else {
			closurePtrToIdx[&c] = idx - 1;
		}
	}
}


void creg_UdataPtr::Serialize(creg::ISerializer* s)
{
	int idx;
	if (s->IsWriting()) {
		if (u == nullptr) {
			idx = 0;
		} else {
			auto it = udataToIdx.find(u);
			if (it == udataToIdx.end()) {
				idx = udataVec.size() + 1;
				udataVec.push_back(u);
				udataToIdx[u] = idx;
			} else {
				idx = udataToIdx[u];
			}
		}
	}

	s->SerializeInt(&idx, sizeof(idx));

	if (!(s->IsWriting())) {
		if (idx == 0) {
			u = nullptr;
		} else {
			udataPtrToIdx[&u] = idx - 1;
		}
	}
}


void creg_GCObject::Serialize(creg::ISerializer* s)
{

	int tt;
	if (s->IsWriting())
		tt = u.gch.tt;

	s->SerializeInt(&tt, sizeof(tt));

	switch(tt) {
		// case LUA_TSTRING: { SerializeInstance(s, &u.ts); return; }
		// case LUA_TUSERDATA: { SerializeInstance(s, &u.u); }
		// case LUA_TFUNCTION: { SerializeInstance(s, &u.cl); return; }
		case LUA_TTABLE: { SerializeInstance(s, &u.h); return; }
		case LUA_TPROTO: { SerializeInstance(s, &u.p); return; }
		case LUA_TUPVAL: { SerializeInstance(s, &u.uv); return; }
		case LUA_TTHREAD: { SerializeInstance(s, &u.th); return; }
		default: { assert(false); return; }
	}
}


size_t creg_GCObject::GetSize()
{
	int tt = u.gch.tt;
	switch(tt) {
		// case LUA_TSTRING: { SerializeInstance(s, &u.ts); return; }
		// case LUA_TUSERDATA: { SerializeInstance(s, &u.u); }
		// case LUA_TFUNCTION: { SerializeInstance(s, &u.cl); return; }
		case LUA_TTABLE: { return sizeof(Table); }
		case LUA_TPROTO: { return sizeof(Proto); }
		case LUA_TUPVAL: { return sizeof(UpVal); }
		case LUA_TTHREAD: { return sizeof(lua_State); }
		default: { assert(false); return 0; }
	}
	return 0;
}


void creg_TValue::Serialize(creg::ISerializer* s)
{
	switch(tt) {
		case LUA_TNIL: { return; }
		case LUA_TBOOLEAN: { s->SerializeInt(&value.b, sizeof(value.b)); return; }
		case LUA_TLIGHTUSERDATA: { assert(false); return; } // No support for light user data atm
		case LUA_TNUMBER: { s->SerializeInt(&value.n, sizeof(value.n)); return; }
		// case LUA_TSTRING: {SerializeInstance(s, &value.gc.p.ts); return; }
		case LUA_TTABLE: { SerializePtr(s, &value.gc); return; }
		// case LUA_TFUNCTION: { SerializeInstance(s, &value.gc.p.cl); return; }
		// case LUA_TUSERDATA: { SerializeInstance(s, &value.gc.p.u); }
		case LUA_TTHREAD: { SerializePtr(s, &value.gc); return; }
		default: { assert(false); return; }
	}
}


void creg_Node::Serialize(creg::ISerializer* s)
{
	SerializeInstance(s, &i_key.tvk);
	SerializePtr(s, &i_key.nk.next);
}


void creg_Table::Serialize(creg::ISerializer* s)
{
	int sizenode = twoto(lsizenode);

	SerializeCVector(s, &array, sizearray);
	SerializeCVector(s, &node, sizenode);
	ptrdiff_t lastfreeOffset;
	if (s->IsWriting())
		lastfreeOffset = lastfree - node;

	s->SerializeInt(&lastfreeOffset, sizeof(lastfreeOffset));

	if (!s->IsWriting())
		lastfree = node + lastfreeOffset;
}


void creg_Proto::Serialize(creg::ISerializer* s)
{
	SerializeCVector(s, &k,        sizek);
	SerializeCVector(s, &code,     sizecode);
	SerializeCVector(s, &p,        sizep);
	SerializeCVector(s, &lineinfo, sizelineinfo);
	SerializeCVector(s, &locvars,  sizelocvars);
	SerializeCVector(s, &upvalues, sizeupvalues);
}


void creg_UpVal::Serialize(creg::ISerializer* s)
{
	bool closed;
	if (s->IsWriting())
		closed = (v == &u.value);

	s->SerializeInt(&closed, sizeof(closed));

	if (closed) {
		SerializeInstance(s, &u.value);
	} else {
		SerializePtr(s, &u.l.prev);
		SerializePtr(s, &u.l.next);
	}
}


void creg_stringtable::Serialize(creg::ISerializer* s)
{
	SerializeCVector(s, &hash, size);
}


void creg_lua_State::Serialize(creg::ISerializer* s)
{
	// stack should be empty when saving!
	if (s->IsWriting()) {
		assert(base == top);
		assert(ci->base == top);
		assert(stack_last == stack + stacksize - EXTRA_STACK - 1);
		assert(base_ci == ci);
		assert(end_ci == base_ci + size_ci - 1);

		assert(hook == NULL);
		assert(errorJmp == NULL);
	} else {
		// adapted from stack_init lstate.cpp
		base_ci = (CallInfo *) luaContext.alloc(size_ci * sizeof(*base_ci));
		ci = base_ci;
		end_ci = base_ci + size_ci - 1;

		stack = (StkId) luaContext.alloc(stacksize * sizeof(*stack));
		top = stack;
		stack_last = stack + stacksize - EXTRA_STACK - 1;

		ci->func = top;
		setnilvalue(top++);
		base = ci->base = top;
		ci->top = top + LUA_MINSTACK;

		errorJmp = NULL;
		hook = NULL;
	}
}


void creg_global_State::Serialize(creg::ISerializer* s)
{
	if (s->IsWriting()) {
		assert(fopen_func  == NULL);
		assert(popen_func  == NULL);
		assert(pclose_func == NULL);
		assert(system_func == NULL);
		assert(remove_func == NULL);
		assert(rename_func == NULL);
		char pointsToRoot = sweepgc == &rootgc ? 1 : 0;
		s->SerializeInt(&pointsToRoot, sizeof(char));
		// if it doesn't point into rootgc, it must point to some valid GCObject's
		// 'next' field which is the exact address of the parent object, as it's
		// the first field
		if (sweepgc != &rootgc)
			SerializePtr(s, (creg_GCObject**) &sweepgc);
	} else {
		buff.buffer = NULL;
		buff.buffsize = 0;
		buff.n = 0;

		fopen_func  = NULL;
		popen_func  = NULL;
		pclose_func = NULL;
		system_func = NULL;
		remove_func = NULL;
		rename_func = NULL;
		char pointsToRoot;
		s->SerializeInt(&pointsToRoot, sizeof(char));
		if (pointsToRoot) {
			sweepgc = &rootgc;
		} else {
			SerializePtr(s, (creg_GCObject**) &sweepgc);
		}
	}
}


namespace creg {

void SerializeLuaState(creg::ISerializer* s, lua_State** L)
{
	assert(stringToIdx.empty());
	assert(stringVec.empty());
	assert(stringPtrToIdx.empty());


	creg_LG* clg;
	if (s->IsWriting()) {
		assert(*L != nullptr);
		clg = (creg_LG*) *L;
		// a garbage pointer that needs fixing
		clg->g.uvhead.next = nullptr;
		clg->g.uvhead.v = nullptr;
	} else {
		//assert(*L == nullptr);
		clg = (creg_LG*) luaContext.alloc(sizeof(creg_LG));
		*L = (lua_State*) &(clg->l);
		clg->g.ud = luaContext.GetContext();
		clg->g.panic = luaContext.GetPanic();
		clg->g.frealloc = luaContext.Getfrealloc();
	}

	SerializeInstance(s, clg);

	// // TString - - see comment in top of page
	// if (s->IsWriting()) {
		// int count = stringVec.size();
		// s->SerializeInt(&count, sizeof(count));
		// for (TString* ts: stringVec) {
			// // Serialize length first, so we know how much to alloc
			// s->SerializeInt(&ts->tsv.len, sizeof(ts->tsv.len));
			// s->Serialize(ts + 1, ts->tsv.len);

			// SerializeInstance(s, (creg_GCObjectPtr*) &ts->tsv.next);
			// // no need to serialize tt
			// // no need to serialize hash
			// s->SerializeInt(&ts->tsv.marked, sizeof(ts->tsv.marked));
			// s->SerializeInt(&ts->tsv.reserved, sizeof(ts->tsv.reserved));
		// }
	// } else {
		// int count;
		// s->SerializeInt(&count, sizeof(count));
		// stringVec.resize(count);
		// for (int i = 0; i < count; ++i) {
			// size_t length;
			// s->SerializeInt(&length, sizeof(length));
			// TString* ts = (TString*) luaContext.alloc((length + 1) * sizeof(char) + sizeof(TString));
			// s->Serialize(ts + 1, length);

			// SerializeInstance(s, (creg_GCObjectPtr*) &ts->tsv.next);
			// s->SerializeInt(&ts->tsv.marked, sizeof(ts->tsv.marked));
			// s->SerializeInt(&ts->tsv.reserved, sizeof(ts->tsv.reserved));

			// ts->tsv.tt = LUA_TSTRING;
			// ts->tsv.hash = lua_calchash(getstr(ts), length);
			// ts->tsv.len = length;

			// stringVec[i] = ts;
		// }
		// for (auto& it: stringPtrToIdx) {
			// *(it.first) = stringVec[it.second];
		// }
	// }


	// // Closure - - see comment in top of page
	// StringType sType;
	// if (s->IsWriting()) {
		// int count = closureVec.size();
		// s->SerializeInt(&count, sizeof(count));
		// for (Closure* c: closureVec) {
			// // Serialize type and number of upvalues first,
			// // so we know how much to alloc
			// s->SerializeInt(&c->c.isC, sizeof(c->c.isC));
			// s->SerializeInt(&c->c.nupvalues, sizeof(c->c.nupvalues));

			// SerializeInstance(s, (creg_GCObjectPtr*) &c->c.next);
			// // no need to serialize tt
			// s->SerializeInt(&c->c.marked, sizeof(c->c.marked));
			// // no need to serialize gclist
			// if (c->c.isC) {
				// CClosure* cc = &c->c;
				// assert(funcToName.find(cc->f) != funcToName.end());
				// std::string name = funcToName[cc->f];
				// sType.Serialize(s, &name);
				// for (unsigned i = 0; i < cc->nupvalues; ++i) {
					// SerializeInstance(s, (creg_TValue *) &cc->upvalue[i]);
				// }
			// } else {
				// LClosure* lc = &c->l;
				// SerializePtr(s, (creg_Proto **) &lc->p);
				// for (unsigned i = 0; i < lc->nupvalues; ++i) {
					// SerializePtr(s, (creg_UpVal **) &lc->upvals[i]);
				// }
			// }
		// }
	// } else {
		// int count;
		// s->SerializeInt(&count, sizeof(count));
		// closureVec.resize(count);
		// for (int i = 0; i < count; ++i) {
			// lu_byte isC;
			// lu_byte nupvalues;
			// s->SerializeInt(&isC, sizeof(isC));
			// s->SerializeInt(&nupvalues, sizeof(nupvalues));
			// Closure* c;
			// if (isC) {
				// c = (Closure*) luaContext.alloc(sizeCclosure(nupvalues));
			// } else {
				// c = (Closure*) luaContext.alloc(sizeLclosure(nupvalues));
			// }
			// c->c.isC = isC;
			// c->c.nupvalues = nupvalues;

			// SerializeInstance(s, (creg_GCObjectPtr*) &c->c.next);
			// c->c.tt = LUA_TFUNCTION;
			// s->SerializeInt(&c->c.marked, sizeof(c->c.marked));
			// // no need to deserialize gclist

			// if (isC) {
				// CClosure* cc = &c->c;
				// std::string name;
				// sType.Serialize(s, &name);
				// assert(nameToFunc.find(name) != nameToFunc.end());
				// cc->f = nameToFunc[name];
				// for (unsigned j = 0; j < cc->nupvalues; ++j) {
					// SerializeInstance(s, (creg_TValue *) &cc->upvalue[j]);
				// }
			// } else {
				// LClosure* lc = &c->l;
				// SerializePtr(s, (creg_Proto **) &lc->p);
				// for (unsigned j = 0; j < lc->nupvalues; ++j) {
					// SerializePtr(s, (creg_UpVal **) &lc->upvals[j]);
				// }
			// }

			// closureVec[i] = c;
		// }
		// for (auto& it: closurePtrToIdx) {
			// *(it.first) = closureVec[it.second];
		// }
	// }


	// // UData - see comment in top of page
	// if (s->IsWriting()) {
		// int count = udataVec.size();
		// s->SerializeInt(&count, sizeof(count));
		// for (Udata* ud: udataVec) {
			// // Serialize length first, so we know how much to alloc
			// assert(ud->uv.len == sizeof(int));
			// s->SerializeInt(&ud->uv.len, sizeof(ud->uv.len));

			// SerializeInstance(s, (creg_GCObjectPtr*) &ud->uv.next);
			// // no need to serialize tt
			// s->SerializeInt(&ud->uv.marked, sizeof(ud->uv.marked));
			// SerializePtr(s, (creg_Table **) &ud->uv.metatable);
			// SerializePtr(s, (creg_Table **) &ud->uv.env);
			// s->SerializeInt((int *) (ud + 1), ud->uv.len);
		// }
	// } else {
		// int count;
		// s->SerializeInt(&count, sizeof(count));
		// udataVec.resize(count);
		// for (int i = 0; i < count; ++i) {
			// size_t length;
			// s->SerializeInt(&length, sizeof(length));
			// assert(length == sizeof(int));
			// Udata* ud = (Udata*) luaContext.alloc(length + sizeof(Udata));

			// SerializeInstance(s, (creg_GCObjectPtr*) &ud->uv.next);
			// ud->uv.tt = LUA_TUSERDATA;
			// s->SerializeInt(&ud->uv.marked, sizeof(ud->uv.marked));
			// SerializePtr(s, (creg_Table **) &ud->uv.metatable);
			// SerializePtr(s, (creg_Table **) &ud->uv.env);
			// ud->uv.len = length;
			// s->SerializeInt((int *) (ud + 1), ud->uv.len);

			// udataVec[i] = ud;
		// }
		// for (auto& it: udataPtrToIdx) {
			// *(it.first) = udataVec[it.second];
		// }
	// }


	stringToIdx.clear();
	stringVec.clear();
	stringPtrToIdx.clear();
}

void RegisterCFunction(const char* name, lua_CFunction f)
{
	assert(nameToFunc.find(std::string(name)) == nameToFunc.end());
	nameToFunc[name] = f;
	funcToName[f] = name;
}
void SetLuaContext(void* context, lua_Alloc frealloc, lua_CFunction panic)
{
	luaContext.SetContext(context, frealloc, panic);
}
}


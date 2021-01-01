/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "SerializeLuaState.h"

#include "VarTypes.h"

#ifndef UNIT_TEST
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/TimeProfiler.h"
#endif

#include "lib/lua/src/ltable.h"
#include "System/UnorderedMap.hpp"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include <deque>

struct creg_lua_State;
struct creg_Proto;
struct creg_UpVal;
struct creg_Node;
struct creg_Table;
union creg_GCObject;
struct creg_TString;

#define ASSERT_SIZE(structName) static_assert(sizeof(creg_ ## structName) == sizeof(structName), #structName " Size mismatch");

static_assert(LUAI_EXTRASPACE == 0, "LUAI_EXTRASPACE isn't 0");

class LuaContext{
public:
	LuaContext() = default;
	void* alloc(size_t n) const {
		assert(context != nullptr && frealloc != nullptr);
		return frealloc(context, nullptr, 0, n);
	}
	void SetContext(void* newLcd, lua_Alloc newfrealloc, lua_CFunction newPanic) { context = newLcd; frealloc = newfrealloc; panic = newPanic; }
	void SetMainthread(lua_State* L) { mainthread = L; }
	lua_CFunction GetPanic() const { return panic; }
	void* GetContext() const { return context; }
	lua_Alloc Getfrealloc() const { return frealloc; }
	lua_State* GetMainthread() const { return mainthread; }
private:
	void* context = nullptr;
	lua_Alloc frealloc = nullptr;
	lua_CFunction panic = nullptr;
	lua_State* mainthread;
};

void freeProtector(void *m) {
	assert(false);
}

void* allocProtector(size_t size) {
	assert(false);
	return nullptr;
}


static LuaContext luaContext;

// Used for hackish heuristic for deciphering lightuserdata
static bool inClosure = false;

// C functions in lua have to be specially registered in order to
// be serialized correctly
static spring::unsynced_map<std::string, lua_CFunction> nameToFunc;
static spring::unsynced_map<lua_CFunction, std::string> funcToName;


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
	void PostLoad();
};

ASSERT_SIZE(Table)


struct creg_LocVar {
	CR_DECLARE_STRUCT(creg_LocVar)
	creg_TString *varname;
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
	creg_TString **upvalues;  /* upvalue names */
	creg_TString *source;
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


struct creg_TString {
	CR_DECLARE_STRUCT(creg_TString)
	union {
		L_Umaxalign dummy;  /* ensures maximum alignment for strings */
		struct {
			creg_CommonHeader;
			lu_byte reserved;
			unsigned int hash;
			size_t len;
		} tsv;
	} u;
	void Serialize(creg::ISerializer* s);
	size_t GetSize();
};

ASSERT_SIZE(TString)

#define creg_ClosureHeader \
	creg_CommonHeader; lu_byte isC; lu_byte nupvalues; creg_GCObject *gclist; \
	creg_Table *env

#define CR_CLOSURE_HEADER() CR_COMMON_HEADER(), CR_MEMBER(isC), CR_MEMBER(nupvalues), CR_IGNORED(gclist), \
	CR_MEMBER(env)

struct creg_CClosure {
	CR_DECLARE_STRUCT(creg_CClosure)
	creg_ClosureHeader;
	lua_CFunction f;
	creg_TValue upvalue[1];
	void Serialize(creg::ISerializer* s);
	size_t GetSize();
};

ASSERT_SIZE(CClosure)

struct creg_LClosure {
	CR_DECLARE_STRUCT(creg_LClosure)
	creg_ClosureHeader;
	creg_Proto *p;
	creg_UpVal *upvals[1];
	void Serialize(creg::ISerializer* s);
	size_t GetSize();
};

ASSERT_SIZE(LClosure)

union creg_Closure {
	creg_CClosure c;
	creg_LClosure l;
};

ASSERT_SIZE(Closure)


struct creg_Udata {
	CR_DECLARE_STRUCT(creg_Udata)
	union {
		L_Umaxalign dummy;  /* ensures maximum alignment for strings */
		struct {
			creg_CommonHeader;
			creg_Table *metatable;
			creg_Table *env;
			size_t len;
		} uv;
	} u;
	void Serialize(creg::ISerializer* s);
	size_t GetSize();
};

ASSERT_SIZE(Udata)


creg_Node* GetDummyNode()
{
	static creg_Node* dummyNode = nullptr;
	if (dummyNode != nullptr)
		return dummyNode;

	lua_State* L = lua_open();
	lua_newtable(L);
	creg_Table* t = (creg_Table*) lua_topointer(L, -1);
	dummyNode = t->node;
	lua_close(L);

	return dummyNode;
}


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
	creg_TString *tmname[TM_N];  /* array with tag-method names */

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
	void PostLoad();
};

ASSERT_SIZE(lua_State)


union creg_GCObject {
	GCheader gch;
	creg_TString ts;
	creg_Udata u;
	creg_Closure cl;
	creg_Table h;
	creg_Proto p;
	creg_UpVal uv;
	creg_lua_State th;
};


// Specialization because we have to figure the real class and not
// serialize GCObject* pointers.
namespace creg {
template<>
class ObjectPointerType<creg_GCObject> : public IType
{
public:
	ObjectPointerType() : IType(sizeof(creg_GCObject*)) { }
	void Serialize(ISerializer *s, void *instance) override{
		void **ptr = (void**)instance;
		int tt;
		creg_GCObject *gco = (creg_GCObject *) *ptr;
		if (s->IsWriting())
			tt = gco == nullptr ? LUA_TNONE : gco->gch.tt;

		s->SerializeInt(&tt, sizeof(tt));

		if (tt == LUA_TNONE) {
			if(!s->IsWriting())
				*ptr = nullptr;

			return;
		}

		Class *c = nullptr;

		switch(tt) {
			case LUA_TSTRING: { c = creg_TString::StaticClass(); break; }
			case LUA_TUSERDATA: { c = creg_Udata::StaticClass(); break; }
			case LUA_TFUNCTION: {
				bool isC;
				if (s->IsWriting())
					isC = gco->cl.c.isC;

				s->SerializeInt(&isC, sizeof(isC));
				if (isC) {
					c = creg_CClosure::StaticClass();
				} else {
					c = creg_LClosure::StaticClass();
				}
				break;
				}
			case LUA_TTABLE: { c = creg_Table::StaticClass(); break; }
			case LUA_TPROTO: { c = creg_Proto::StaticClass(); break; }
			case LUA_TUPVAL: { c = creg_UpVal::StaticClass(); break; }
			case LUA_TTHREAD: { c = creg_lua_State::StaticClass(); break; }
			default: { assert(false); break; }
		}

		s->SerializeObjectPtr(ptr, c);
	}
	std::string GetName() const override {
		return "creg_GCObject*";
	}
};
}


struct creg_LG {
	CR_DECLARE_STRUCT(creg_LG)
	creg_lua_State l;
	creg_global_State g;
};


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


CR_BIND_POOL(creg_Table, , luaContext.alloc, freeProtector)
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
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
))


CR_BIND_POOL(creg_LocVar, , allocProtector, freeProtector)
CR_REG_METADATA(creg_LocVar, (
	CR_MEMBER(varname),
	CR_MEMBER(startpc),
	CR_MEMBER(endpc)
))


CR_BIND_POOL(creg_Proto, , luaContext.alloc, freeProtector)
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


CR_BIND_POOL(creg_UpVal, , luaContext.alloc, freeProtector)
CR_REG_METADATA(creg_UpVal, (
	CR_COMMON_HEADER(),
	CR_MEMBER(v),
	CR_IGNORED(u), //union
	CR_SERIALIZER(Serialize)
))


CR_BIND_POOL(creg_TString, , luaContext.alloc, freeProtector)
CR_REG_METADATA(creg_TString, (
	CR_IGNORED(u), //union
	CR_SERIALIZER(Serialize),
	CR_GETSIZE(GetSize)
))


CR_BIND_POOL(creg_CClosure, , luaContext.alloc, freeProtector)
CR_REG_METADATA(creg_CClosure, (
	CR_CLOSURE_HEADER(),
	CR_IGNORED(f),
	CR_IGNORED(upvalue),
	CR_SERIALIZER(Serialize),
	CR_GETSIZE(GetSize)
))


CR_BIND_POOL(creg_LClosure, , luaContext.alloc, freeProtector)
CR_REG_METADATA(creg_LClosure, (
	CR_CLOSURE_HEADER(),
	CR_MEMBER(p),
	CR_IGNORED(upvals),
	CR_SERIALIZER(Serialize),
	CR_GETSIZE(GetSize)
))


CR_BIND_POOL(creg_Udata, , luaContext.alloc, freeProtector)
CR_REG_METADATA(creg_Udata, (
	CR_IGNORED(u),
	CR_SERIALIZER(Serialize),
	CR_GETSIZE(GetSize)
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


CR_BIND_POOL(creg_lua_State, , luaContext.alloc, freeProtector)
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
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
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

void SerializeLightUserData(creg::ISerializer* s, void **p)
{
	if (!inClosure) {
		s->SerializeInt(p, sizeof(*p));
		return;
	}
#ifndef UNIT_TEST
	enum DefType {
		DT_Feature,
		DT_Unit,
		DT_Weapon
	} defType;

	int idx;
	if (s->IsWriting()) {
		auto& fdVec = featureDefHandler->GetFeatureDefsVec();
		auto& udVec = unitDefHandler->GetUnitDefsVec();
		auto& wdVec = weaponDefHandler->GetWeaponDefsVec();
		if (*p >= fdVec.data() && *p < fdVec.data() + fdVec.size()) {
			idx = (int) ((const FeatureDef*) *p - fdVec.data());
			defType = DT_Feature;
		} else if (*p >= udVec.data() && *p < udVec.data() + udVec.size()){
			idx = (int) ((const UnitDef*) *p - udVec.data());
			defType = DT_Unit;
		} else if (*p >= wdVec.data() && *p < wdVec.data() + wdVec.size()) {
			idx = (int) ((const WeaponDef*) *p - wdVec.data());
			defType = DT_Weapon;
		} else {
			assert(false);
		}
	}
	s->SerializeInt(&idx, sizeof(idx));
	s->SerializeInt(&defType, sizeof(defType));
	if (!s->IsWriting()) {
		switch(defType) {
			case DT_Feature: { *p = (void *) featureDefHandler->GetFeatureDefByID(idx); break;}
			case DT_Unit:    { *p = (void *) unitDefHandler->GetUnitDefByID(idx); break;}
			case DT_Weapon:  { *p = (void *) weaponDefHandler->GetWeaponDefByID(idx); break;}
		}
	}
#endif
}


void creg_TValue::Serialize(creg::ISerializer* s)
{
	switch(tt) {
		case LUA_TNIL: { return; }
		case LUA_TBOOLEAN: { s->SerializeInt(&value.b, sizeof(value.b)); return; }
		case LUA_TLIGHTUSERDATA: { SerializeLightUserData(s, &value.p); return; } // No support for light user data atm
		case LUA_TNUMBER: { s->SerializeInt(&value.n, sizeof(value.n)); return; }
		case LUA_TSTRING: { SerializePtr(s, &value.gc); return; }
		case LUA_TTABLE: { SerializePtr(s, &value.gc); return; }
		case LUA_TFUNCTION: { SerializePtr(s, &value.gc); return; }
		case LUA_TUSERDATA: { SerializePtr(s, &value.gc); }
		case LUA_TTHREAD: { SerializePtr(s, &value.gc); return; }
		case LUA_TDEADKEY: { return; }
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
	bool empty;
	creg_Node* dummy = GetDummyNode();
	if (s->IsWriting())
		empty = (node == dummy);

	s->SerializeInt(&empty, sizeof(empty));

	if (empty) {
		if (!s->IsWriting()) {
			node = dummy;
		} else {
			assert(node == dummy);
		}
	} else {
		SerializeCVector(s, &node, sizenode);
	}

	ptrdiff_t lastfreeOffset;
	if (s->IsWriting())
		lastfreeOffset = lastfree - node;

	s->SerializeInt(&lastfreeOffset, sizeof(lastfreeOffset));

	if (!s->IsWriting())
		lastfree = node + lastfreeOffset;
}

void creg_Table::PostLoad()
{
	// table may contain pointers as keys so will require reordering
	int sizenode = twoto(lsizenode);

	bool reorder = false;
	for (int i = 0; i < sizenode; ++i) {
		int tt = node[i].i_key.nk.tt;
		if (tt != LUA_TNIL && tt != LUA_TBOOLEAN &&
		    tt != LUA_TNUMBER && tt != LUA_TSTRING &&
		    tt != LUA_TDEADKEY) {
			reorder = true;
			break;
		}
	}
	if (!reorder)
		return;

	lua_State* L = luaContext.GetMainthread();
	// see ltable.cpp
	lsizenode = 0;
	creg_Node* onode = node;
	node = GetDummyNode();
	lastfree = node;
	for (int i = 0; i < sizenode; ++i) {
		if (onode[i].i_val.tt != LUA_TNIL)
			setobjt2t(L, luaH_set(L, (Table*) this, (TValue*) &(onode[i].i_key.tvk)), (TValue*) &(onode[i].i_val));
	}
	luaContext.Getfrealloc()(luaContext.GetContext(), onode, sizenode * sizeof(Node), 0);
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


void creg_TString::Serialize(creg::ISerializer* s)
{
	SerializePtr(s, &u.tsv.next);
	s->SerializeInt(&u.tsv.tt, sizeof(u.tsv.tt));
	s->SerializeInt(&u.tsv.marked, sizeof(u.tsv.marked));
	s->SerializeInt(&u.tsv.reserved, sizeof(u.tsv.reserved));
	s->SerializeInt(&u.tsv.len, sizeof(u.tsv.len));
	s->Serialize(this + 1, u.tsv.len);
	if (!s->IsWriting()) {
		((char *)(this+1))[u.tsv.len] = '\0';
		u.tsv.hash = lua_calchash(getstr(this), u.tsv.len);
	}
}


size_t creg_TString::GetSize()
{
	return sizeof(creg_TString) + u.tsv.len + 1;
}


void creg_CClosure::Serialize(creg::ISerializer* s)
{
	inClosure = true;
	for (unsigned i = 0; i < nupvalues; ++i) {
		SerializeInstance(s, &upvalue[i]);
	}
	inClosure = false;

	creg::StringType sType;
	if (s->IsWriting()) {
		//assert(funcToName.find(f) != funcToName.end());
		if (funcToName.find(f) == funcToName.end()) {
			LOG_L(L_ERROR, "Function 0x%p not found", f);
			return;
		}
		std::string name = funcToName[f];
		sType.Serialize(s, &name);
	} else {
		std::string name;
		sType.Serialize(s, &name);
		assert(nameToFunc.find(name) != nameToFunc.end());
		f = nameToFunc[name];
	}
}


size_t creg_CClosure::GetSize()
{
	return sizeCclosure(nupvalues);
}


void creg_LClosure::Serialize(creg::ISerializer* s)
{
	for (unsigned i = 0; i < nupvalues; ++i) {
		SerializePtr(s, &upvals[i]);
	}
}


size_t creg_LClosure::GetSize()
{
	return sizeLclosure(nupvalues);
}


void creg_Udata::Serialize(creg::ISerializer* s)
{
	SerializePtr(s, &u.uv.next);
	s->SerializeInt(&u.uv.tt, sizeof(u.uv.tt));
	s->SerializeInt(&u.uv.marked, sizeof(u.uv.marked));
	SerializePtr(s, &u.uv.metatable);
	SerializePtr(s, &u.uv.env);
	s->SerializeInt(&u.uv.len, sizeof(u.uv.len));

	// currently we only support integer user data
	assert(u.uv.len == sizeof(int));
	s->SerializeInt((int *) (this + 1), sizeof(int));
}


size_t creg_Udata::GetSize()
{
	return sizeof(creg_Udata) + u.uv.len;
}


void creg_stringtable::Serialize(creg::ISerializer* s)
{
	SerializeCVector(s, &hash, size);
}

inline creg_Proto* GetProtoFromCallInfo(CallInfo* ci)
{
	return ((creg_TValue*) ci->func)->value.gc->cl.l.p;
}

inline bool InstructionInCode(const Instruction* inst, CallInfo* ci)
{
	const creg_Proto* p = GetProtoFromCallInfo(ci);
	return (inst >= p->code) && (inst < (p->code + p->sizecode));
}

void creg_lua_State::Serialize(creg::ISerializer* s)
{
	ptrdiff_t ci_offset;
	ptrdiff_t base_offset;
	ptrdiff_t top_offset;
	ptrdiff_t savedpc_offset;
	if (s->IsWriting()) {
		ci_offset = ci - base_ci;
		base_offset = base - stack;
		top_offset = top - stack;
		assert(end_ci == base_ci + size_ci - 1);
		assert(stack_last == stack + stacksize - EXTRA_STACK - 1);
		assert(ci_offset == 0 || InstructionInCode(savedpc, ci - 1));

		//assert(hook == nullptr);
		//assert(errorJmp == nullptr);

		if (ci_offset > 0) {
			savedpc_offset = (savedpc - GetProtoFromCallInfo(ci - 1)->code);
		}
	}

	s->SerializeInt(&ci_offset, sizeof(ci_offset));
	s->SerializeInt(&base_offset, sizeof(base_offset));
	s->SerializeInt(&top_offset, sizeof(top_offset));
	if (ci_offset > 0) {
		s->SerializeInt(&savedpc_offset, sizeof(savedpc_offset));
	}

	if (!s->IsWriting()) {
		base_ci = (CallInfo *) luaContext.alloc(size_ci * sizeof(*base_ci));
		ci = base_ci + ci_offset;
		end_ci = base_ci + size_ci - 1;

		stack = (StkId) luaContext.alloc(stacksize * sizeof(*stack));
		base = stack + base_offset;
		top = stack + top_offset;
		stack_last = stack + stacksize - EXTRA_STACK - 1;

		if (ci_offset > 0) {
			savedpc = (const Instruction *) savedpc_offset; // will be fixed in PostLoad
		}
	}

	for (StkId st = stack; st < top; ++st) {
		SerializeInstance(s, (creg_TValue*) st);
	}

	for (CallInfo *c = base_ci; c <= ci; ++c) {
		ptrdiff_t c_base_offset;
		ptrdiff_t c_top_offset;
		ptrdiff_t c_func_offset;
		ptrdiff_t c_savedpc_offset;
		if (s->IsWriting()) {
			c_base_offset = c->base - stack;
			c_top_offset = c->top - stack;
			c_func_offset = c->func - stack;
			if (c > base_ci && c < ci) {
				assert(InstructionInCode(c->savedpc, c));
				c_savedpc_offset = c->savedpc - GetProtoFromCallInfo(c)->code;
			}
		}
		s->SerializeInt(&c_base_offset, sizeof(c_base_offset));
		s->SerializeInt(&c_top_offset, sizeof(c_top_offset));
		s->SerializeInt(&c_func_offset, sizeof(c_func_offset));
		if (c > base_ci && c < ci) {
			s->SerializeInt(&c_savedpc_offset, sizeof(c_savedpc_offset));
		} else {
			c_savedpc_offset = 0;
		}
		s->SerializeInt(&c->nresults, sizeof(c->nresults));
		s->SerializeInt(&c->tailcalls, sizeof(c->tailcalls));
		if (!s->IsWriting()) {
			c->base = stack + c_base_offset;
			c->top = stack + c_top_offset;
			c->func = stack + c_func_offset;
			c->savedpc = (const Instruction *) c_savedpc_offset; // will be fixed in PostLoad
		}
	}
}


void creg_lua_State::PostLoad()
{
	if (base_ci == ci)
		return;

	for (CallInfo *c = base_ci + 1; c < ci; ++c) {
		size_t c_savedpc_offset = * (size_t *) &c->savedpc;
		c->savedpc = GetProtoFromCallInfo(c)->code + c_savedpc_offset;
	}

	size_t savedpc_offset = * (size_t *) &savedpc;
	savedpc = GetProtoFromCallInfo(ci)->code + savedpc_offset;
}


void creg_global_State::Serialize(creg::ISerializer* s)
{
	if (s->IsWriting()) {
		assert(fopen_func  == nullptr);
		assert(popen_func  == nullptr);
		assert(pclose_func == nullptr);
		assert(system_func == nullptr);
		assert(remove_func == nullptr);
		assert(rename_func == nullptr);
		char pointsToRoot = sweepgc == &rootgc ? 1 : 0;
		s->SerializeInt(&pointsToRoot, sizeof(char));
		// if it doesn't point into rootgc, it must point to some valid GCObject's
		// 'next' field which is the exact address of the parent object, as it's
		// the first field
		if (sweepgc != &rootgc)
			SerializePtr(s, (creg_GCObject**) &sweepgc);
	} else {
		frealloc = luaContext.Getfrealloc();
		ud = luaContext.GetContext();
		panic = luaContext.GetPanic();

		buff.buffer = nullptr;
		buff.buffsize = 0;
		buff.n = 0;

		fopen_func  = nullptr;
		popen_func  = nullptr;
		pclose_func = nullptr;
		system_func = nullptr;
		remove_func = nullptr;
		rename_func = nullptr;
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
	creg_LG* clg;
	if (s->IsWriting()) {
		assert(*L != nullptr);
		clg = (creg_LG*) *L;
		// a garbage pointer that needs fixing
		clg->g.uvhead.next = nullptr;
		clg->g.uvhead.v = nullptr;
	} else {
		assert(*L == nullptr);
		clg = (creg_LG*) luaContext.alloc(sizeof(creg_LG));
		*L = (lua_State*) &(clg->l);
		luaContext.SetMainthread(*L);
	}

	SerializeInstance(s, clg);
}

void SerializeLuaThread(creg::ISerializer* s, lua_State** L)
{
	s->SerializeObjectPtr((void**)(L), creg_lua_State::StaticClass());
}

void RegisterCFunction(const char* name, lua_CFunction f)
{
	assert((nameToFunc.find(std::string(name)) == nameToFunc.end()) || (nameToFunc[name] == f));
	nameToFunc[name] = f;

	// if the function is already registered under a different name, that's no problem, pick the
	// shorter one
	if (funcToName.find(f) == funcToName.end() || funcToName[f].size() > strlen(name))
		funcToName[f] = name;
}

constexpr int MAX_REC_DEPTH = 7;

void RecursiveAutoRegisterFunction(const std::string& handle, lua_State* L, int depth);
void RecursiveAutoRegisterTable(const std::string& handle, lua_State* L, int depth);


void RecursiveAutoRegisterFunction(const std::string& handle, lua_State* L, int depth)
{
	if (depth > MAX_REC_DEPTH)
		return;

	RegisterCFunction(handle.c_str(), lua_tocfunction(L,-1));
	for (int upval = 1; lua_getupvalue(L, -1, upval) != nullptr; ++upval, lua_pop(L,1)) {
		const std::string newHandle = handle + "::" + IntToString(upval);
		if (lua_iscfunction(L, -1)) {
			RecursiveAutoRegisterFunction(newHandle, L, depth + 1);
		} else if (lua_istable(L, -1)) {
			RecursiveAutoRegisterTable(newHandle, L, depth + 1);
		}
	}

}


void RecursiveAutoRegisterTable(const std::string& handle, lua_State* L, int depth)
{
	if (depth > MAX_REC_DEPTH)
		return;

	for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
		const std::string key = lua_israwstring(L, -2) ? lua_tostring(L,-2) : IntToString(lua_toint(L,-2));
		if (lua_iscfunction(L, -1)) {
			RecursiveAutoRegisterFunction(handle + key, L, depth + 1);
		} else if (lua_istable(L, -1)) {
			if (key == std::string("_G") && depth > 1)
				continue;

			RecursiveAutoRegisterTable(handle + key + ".", L, depth + 1);
		}

		{ // handle metatables
			int ret = lua_getmetatable(L, -1);
			if (ret == 0)
				continue;
			RecursiveAutoRegisterTable(handle + key + ".__mt.", L, depth + 1);
			lua_pop(L, 1);
		}
	}
}

void AutoRegisterCFunctions(const std::string& handle, lua_State* L)
{
#ifndef UNIT_TEST
	ScopedOnceTimer timer("creg::AutoRegisterCFunctions(" + handle + ")");
#endif
	lua_pushvalue(L,LUA_GLOBALSINDEX);
	RecursiveAutoRegisterTable(handle, L, 0);
	lua_pop(L, 1);
	lua_getregistry(L);
	RecursiveAutoRegisterTable(handle, L, 0);
	lua_pop(L, 1);
}

void CopyLuaContext(lua_State* L)
{
	luaContext.SetContext(G(L)->ud, G(L)->frealloc, G(L)->panic);
}
}

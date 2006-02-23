/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 
*/

#ifndef jc_CR_HEADER
#define jc_CR_HEADER

#include <vector>
#include <string>

#include "ISerializer.h"

namespace creg {

	class IType;
	class Class;
	class ClassBinder;

// -------------------------------------------------------------------
// Type/Class system
// -------------------------------------------------------------------

	// Fundamental/basic types
	enum BasicTypeID
	{
		crInt,		crUInt,
		crShort,	crUShort,
		crChar,		crUChar,
		crFloat,
		crDouble,
		crBool,
	};

	class IType
	{
	public:
		// Type interface can go here...
		virtual ~IType() {}

		virtual void Serialize (ISerializer* s, void *instance) = 0;

		static IType* CreateBasicType (BasicTypeID t);
		static IType* CreateStringType ();
		static IType* CreateObjInstanceType (Class *objectType);
		static IType* CreateEnumeratedType (size_t size);
	};

	class IMemberRegistrator
	{
	public:
		virtual void RegisterMembers (Class *cls) = 0;
	};

	enum ClassFlags {
		CF_AllowCopy = 1,   // copying is allowed
		CF_AllowLocal = 2,  // can be used as type in local script variables
		CF_Abstract = 4
	};

	class Class
	{
	public:
		struct Member
		{
			const char* name;
			IType* type;
			unsigned int offset;
		};

		Class ();
		~Class ();

		bool IsSubclassOf (Class* other);
		void AddMember (const char *name, IType* type, unsigned int offset);
		void SerializeInstance (ISerializer* s, void *instance);

		std::vector <Member*> members;
		ClassBinder* binder;
		std::string name;
		Class *base;
	};

// -------------------------------------------------------------------
// Container Type templates
// -------------------------------------------------------------------

	struct Iterator; // Undefined Iterator struct to denote a container iterator

	// vector,deque container (from script, both look like a vector)
	template<typename T>
	class DynamicArrayType : public IType
	{
	public:
		typedef typename T::iterator iterator;
		typedef typename T::value_type ElemT;

		IType *elemType;
		
		DynamicArrayType (IType *elemType) : elemType(elemType) {}
		~DynamicArrayType () { if (elemType) delete elemType; }

		void Serialize (ISerializer *s, void *inst) {
			T& ct = *(T*)inst;
			if (s->IsWriting ()) {
				int size = (int)ct.size();
				s->Serialize (&size,sizeof(int));
				for (int a=0;a<size;a++)
					elemType->Serialize (s, &ct[a]);
			} else {
				int size;
				s->Serialize (&size, sizeof(int));
				ct.resize (size);
				for (int a=0;a<size;a++)
					elemType->Serialize (s, &ct[a]);
			}
		}
	};

	template<typename ValueType, int Size>
	class StaticArrayType : public IType
	{
	public:
		typedef ValueType T[Size];

		IType *elemType;

		StaticArrayType(IType *et) : elemType(et) {}
		~StaticArrayType() { if (elemType) delete elemType; }

		void Serialize (ISerializer *s, void *instance)
		{
			T* array = (T*)instance;
			for (int a=0;a<Size;a++)
				elemType->Serialize (s, &array[a]);
		}
	};

#include "TypeDeduction.h"

// -------------------------------------------------------------------
// ClassBinder: Registers the class to the class list
// -------------------------------------------------------------------

	class ClassBinder
	{
	public:
		ClassBinder (const char *className, ClassFlags cf, ClassBinder* base, IMemberRegistrator **mreg, int instanceSize, void (*constructorProc)(void *instance), void (*destructorProc)(void *instance));

		Class *class_;
		ClassBinder *base;
		ClassFlags flags;
		IMemberRegistrator **memberRegistrator;
		const char *name;
		int size; // size of an instance in bytes
		void (*constructor)(void *instance); 
		void (*destructor)(void *instance); // needed for classes without virtual destructor (classes/structs declared with CR_DECLARE_STRUCT)

		static const std::vector<Class*>& GetClasses() { return classes; }
		static void InitializeClasses ();
		static void FreeClasses ();
		static Class* GetClass(const std::string& name);

	protected:
		ClassBinder* nextBinder;

		static ClassBinder *binderList;
		static std::vector<Class*> classes;
	};

// Use this within the class declaration
#define CR_DECLARE(TCls)	public:					\
	static creg::ClassBinder binder;				\
	static creg::IMemberRegistrator *memberRegistrator;	 \
	virtual creg::Class* GetClass();				\
	inline static creg::Class *StaticClass() { return binder.class_; }

// Use this to declare a structure (POD, no vtable)
#define CR_DECLARE_STRUCT(TStr)		public:			\
	static creg::ClassBinder binder;				\
	static creg::IMemberRegistrator *memberRegistrator;	\
	creg::Class* GetClass() { return binder.class_; }	\
	inline static creg::Class *StaticClass() { return binder.class_; }

// Bind a structure (POD, no vtable)
#define CR_BIND_STRUCT(TStr)					\
	creg::IMemberRegistrator* TStr::memberRegistrator=0;	\
	creg::ClassBinder TStr::binder(#TStr, (creg::ClassFlags)(creg::CF_AllowCopy | creg::CF_AllowLocal), 0, &TStr::memberRegistrator, sizeof(TStr), 0, 0);

// To be used in the .cpp implementation:
// Bind a class derived from another class
#define CR_BIND_DERIVED(TCls, TBase)				\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() { return binder.class_; } \
	void TCls##ConstructInstance(void *d) { new(d) TCls; } \
	void TCls##DestructInstance(void *d) { ((TCls*)d)->~TCls(); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_AllowLocal, &TBase::binder, &TCls::memberRegistrator, sizeof(TCls), TCls##ConstructInstance, TCls##DestructInstance);

// Bind a class not derived from CObject
#define CR_BIND(TCls)								\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() { return binder.class_; } \
	void TCls##ConstructInstance(void *d) { new(d) TCls; } \
	void TCls##DestructInstance(void *d) { ((TCls*)d)->~TCls(); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_AllowLocal, 0, &TCls::memberRegistrator, sizeof(TCls), TCls##ConstructInstance, TCls##DestructInstance);

// Bind an abstract derived class
#define CR_BIND_INTERFACE(TCls, TBase)				\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() { return binder.class_; } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_Abstract, &TBase::binder, &TCls::memberRegistrator, sizeof(TCls), 0, 0);

// Variable registration, also goes in the implementation
#define CR_BIND_MEMBERS(TCls, Members)				\
	struct TCls##MemberRegistrator : creg::IMemberRegistrator {\
	TCls##MemberRegistrator() {				\
		TCls::memberRegistrator=this;				\
	}												\
	void RegisterMembers(creg::Class* class_) {		\
		TCls* null=(TCls*)0;						\
		Members; }									\
	} static TCls##mreg;

	// Macro to register a member within the CR_BIND_MEMBERS macro - the offset is a bit weird to prevent compiler warnings
#define CR_MEMBER(Member)	\
	class_->AddMember ( #Member, creg::GetType (null->Member), (unsigned int)(((char*)&null->Member)-((char*)0)))
#define CR_ENUM_MEMBER(Member) \
	class_->AddMember ( #Member, creg::IType::CreateEnumeratedType(sizeof(null->Member)), (unsigned int)(((char*)&null->Member)-((char*)0)))

};

#endif

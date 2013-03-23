/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// creg - Code compoment registration system

#ifndef _CREG_H
#define _CREG_H

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include "ISerializer.h"
#include "System/Sync/SyncedPrimitive.h"


namespace creg {

	class IType;
	class Class;
	class ClassBinder;

	typedef unsigned int uint;

	// Fundamental/basic types
	enum BasicTypeID
	{
		crInt,		crUInt,
		crShort,	crUShort,
		crChar,		crUChar,
		crInt64,
		crFloat,
		crDouble,
		crBool,
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
		crSyncedSint,   crSyncedUint,
		crSyncedSshort, crSyncedUshort,
		crSyncedSchar,  crSyncedUchar,
		crSyncedFloat,
		crSyncedDouble,
		crSyncedBool,
#endif
	};

	class IType
	{
	public:
		// Type interface can go here...
		virtual ~IType();

		virtual void Serialize(ISerializer* s, void* instance) = 0;
		virtual std::string GetName() = 0;
		virtual size_t GetSize() = 0;

		static boost::shared_ptr<IType> CreateBasicType(BasicTypeID t);
		static boost::shared_ptr<IType> CreateStringType();
		static boost::shared_ptr<IType> CreateObjInstanceType(Class* objectType);
		static boost::shared_ptr<IType> CreateEnumeratedType(size_t size);
	};

	class IMemberRegistrator
	{
	public:
		virtual ~IMemberRegistrator();
		virtual void RegisterMembers(Class* cls) = 0;
	};

	enum ClassFlags {
		CF_None = 0,
		CF_Abstract = 4,
		CF_Synced = 8,
	};

	struct _DummyStruct {};

	/** Class member flags to use with CR_MEMBER_SETFLAG */
	enum ClassMemberFlag {
		CM_NoSerialize = 1, /// Make the serializers skip the member
		CM_Config = 2,  // Exposed in config
	};

/**
 * Stores class bindings such as constructor/destructor
 */
	class ClassBinder
	{
	public:
		ClassBinder(const char* className, unsigned int cf, ClassBinder* base,
				IMemberRegistrator** mreg, int instanceSize, int instanceAlignment, bool hasVTable,
				void (*constructorProc)(void* instance),
				void (*destructorProc)(void* instance));

		Class* class_;
		ClassBinder* base;
		ClassFlags flags;
		IMemberRegistrator** memberRegistrator;
		const char* name;
		int size; // size of an instance in bytes
		int alignment;
		bool hasVTable;

		void (*constructor)(void* instance);
		/**
		 * Needed for classes without virtual destructor.
		 * (classes/structs declared with CR_DECLARE_STRUCT)
		 */
		void (*destructor)(void* instance);

		ClassBinder* nextBinder;
	};

	class System
	{
	public:
		/// Return the global list of classes
		static const std::vector<Class*>& GetClasses() { return classes; }

		/**
		 * Initialization of creg, collects all the classes and initializes
		 * metadata.
		 */
		static void InitializeClasses();

		/// Shutdown of creg
		static void FreeClasses();

		/// Find a class by name
		static Class* GetClass(const std::string& name);

		static void AddClassBinder(ClassBinder* cb);

	protected:
		static ClassBinder* binderList;
		static std::vector<Class*> classes;
	};

	/**
	 * Represents a C++ class or struct, declared with
	 * CR_DECLARE or CR_DECLARE_STRUCT.
	 */
	class Class
	{
	public:
		struct Member
		{
			const char* name;
			boost::shared_ptr<IType> type;
			unsigned int offset;
			int alignment;
			int flags; // combination of ClassMemberFlag's
		};

		Class();
		~Class();

		/// Returns true if this class is equal to or derived from other
		bool IsSubclassOf(Class* other);
		void DeleteInstance(void* inst);
		/// Allocate an instance of the class
		void* CreateInstance();
		/// Calculate a checksum from the class metadata
		void CalculateChecksum(unsigned int& checksum);
		bool AddMember(const char* name, boost::shared_ptr<IType> type, unsigned int offset, int alignment);
		bool AddMember(const char* name, IType* type, unsigned int offset, int alignment);
		void SetMemberFlag(const char* name, ClassMemberFlag f);
		Member* FindMember(const char* name);

		void BeginFlag(ClassMemberFlag flag);
		void EndFlag(ClassMemberFlag flag);

		void SetFlag(ClassFlags flag);

		bool IsAbstract() const { return (binder->flags & CF_Abstract) != 0; }

		/// Returns all concrete classes that implement this class
		std::vector<Class*> GetImplementations();

		std::vector <Member*> members;
		/// all classes that derive from this class
		std::vector <Class*> derivedClasses;
		ClassBinder* binder;
		std::string name;
		int size;
		int alignment;
		Class* base;
		void (_DummyStruct::*serializeProc)(ISerializer& s);
		void (_DummyStruct::*postLoadProc)();

		friend class ClassBinder;
	};

// -------------------------------------------------------------------
// Container Type templates
// -------------------------------------------------------------------

	// vector,deque container
	template<typename T>
	class DynamicArrayType : public IType
	{
	public:
		typedef typename T::iterator iterator;
		typedef typename T::value_type ElemT;

		boost::shared_ptr<IType> elemType;

		DynamicArrayType(boost::shared_ptr<IType> et)
			: elemType(et) {}
		~DynamicArrayType() {}

		void Serialize(ISerializer* s, void* inst) {
			T& ct = *(T*)inst;
			if (s->IsWriting()) {
				int size = (int)ct.size();
				s->SerializeInt(&size, sizeof(int));
				for (int a = 0; a < size; a++) {
					elemType->Serialize(s, &ct[a]);
				}
			} else {
				int size;
				s->SerializeInt(&size, sizeof(int));
				ct.resize(size);
				for (int a = 0; a < size; a++) {
					elemType->Serialize(s, &ct[a]);
				}
			}
		}
		std::string GetName() { return elemType->GetName() + "[]"; }
		size_t GetSize() { return sizeof(T); }
	};

	class StaticArrayBaseType : public IType
	{
	public:
		boost::shared_ptr<IType> elemType;
		int size, elemSize;

		StaticArrayBaseType(boost::shared_ptr<IType> et, int Size, int ElemSize)
			: elemType(et), size(Size), elemSize(ElemSize) {}
		~StaticArrayBaseType() {}

		std::string GetName();
		size_t GetSize() { return size * elemSize; }
	};

	template<typename T, int Size>
	class StaticArrayType : public StaticArrayBaseType
	{
	public:
		typedef T ArrayType[Size];
		StaticArrayType(boost::shared_ptr<IType> et)
			: StaticArrayBaseType(et, Size, sizeof(ArrayType)/Size) {}
		void Serialize(ISerializer* s, void* instance)
		{
			T* array = (T*)instance;
			for (int a = 0; a < Size; a++)
				elemType->Serialize(s, &array[a]);
		}
	};

	template<typename T>
	class BitArrayType : public IType
	{
	public:
		boost::shared_ptr<IType> elemType;

		BitArrayType(boost::shared_ptr<IType> et)
			: elemType(et) {}
		~BitArrayType() {}

		void Serialize(ISerializer* s, void* inst) {
			T* ct = (T*)inst;
			if (s->IsWriting()) {
				int size = (int)ct->size();
				s->SerializeInt(&size, sizeof(int));
				for (int a = 0; a < size; a++) {
					bool b = (*ct)[a];
					elemType->Serialize(s, &b);
				}
			} else {
				int size;
				s->SerializeInt(&size, sizeof(int));
				ct->resize(size);
				for (int a = 0; a < size; a++) {
					bool b;
					elemType->Serialize(s, &b);
					(*ct)[a] = b;
				}
			}
		}
		std::string GetName() { return elemType->GetName() + "[]"; }
		size_t GetSize() { return sizeof(T); }
	};

	class EmptyType : public IType
	{
	public:
		int size;
		EmptyType(int Size) {size=Size;}
		~EmptyType() {}

		void Serialize(ISerializer* s, void* instance)
		{
			for (int a=0;a<size;a++) {
				char c=0;
				s->Serialize(&c,1);
			}
		}
		std::string GetName()
		{
			return "void";
		}
		size_t GetSize() { return 0; /* size*/ } //FIXME used by CR_RESERVED(), ignored by now
	};

	class IgnoredType : public IType
	{
	public:
		int size;
		IgnoredType(int Size) {size=Size;}
		~IgnoredType() {}

		void Serialize(ISerializer* s, void* instance)
		{
			for (int a=0;a<size;a++) {
				char c=0;
				s->Serialize(&c,1);
			}
		}
		std::string GetName()
		{
			return "ignored";
		}
		size_t GetSize() { return size; }
	};
};

#include "TypeDeduction.h"

// detect if c++11
#if __cplusplus <= 199711L
	template<typename t>
	size_t alignof_() {
		return (sizeof(t) > sizeof(int*)) ? sizeof(int*) : sizeof(t);
	}
	#define alignof(t) alignof_<t>()

	template<typename T>
	size_t alignofv(const T& v) {
		return alignof(T);
	}

	template<typename T>
	size_t alignofv(T& v) {
		return alignof(T);
	}
#else
	#define alignofv(v) alignof(v)
#endif


namespace creg {

/** @def CR_DECLARE
 * Add the definitions for creg binding to the class
 * this should be put within the class definition
 */
#define CR_DECLARE(TCls)	public:					\
	static creg::ClassBinder binder;				\
	typedef TCls MyType;							\
	static creg::IMemberRegistrator* memberRegistrator;	 \
	static void _ConstructInstance(void* d);			\
	static void _DestructInstance(void* d);			\
	friend struct TCls##MemberRegistrator;			\
	inline static creg::Class* StaticClass() { return binder.class_; } \
	virtual creg::Class* GetClass() const; \
	static const bool hasVTable = true;

/** @def CR_DECLARE_STRUCT
 * Use this to declare a structure
 * this should be put in the class definition, instead of CR_DECLARE
 * For creg, the only difference between a class and a structure is having a
 * vtable or not.
 */
#define CR_DECLARE_STRUCT(TStr)		public:			\
	static creg::ClassBinder binder;				\
	typedef TStr MyType;							\
	static creg::IMemberRegistrator* memberRegistrator;	\
	static void _ConstructInstance(void* d);			\
	static void _DestructInstance(void* d);			\
	friend struct TStr##MemberRegistrator;			\
	inline static creg::Class* StaticClass() { return binder.class_; } \
	creg::Class* GetClass() const; \
	static const bool hasVTable = false;

/** @def CR_DECLARE_SUB
 * Use this to declare a sub class. This should be put in the class definition
 * of the superclass, alongside CR_DECLARE(CSuperClass) (or CR_DECLARE_STRUCT).
 */
#define CR_DECLARE_SUB(cl) \
	struct cl##MemberRegistrator;

/** @def CR_BIND_DERIVED
 * Bind a derived class declared with CR_DECLARE to creg
 * Should be used in the source file
 * @param TCls class to bind
 * @param TBase base class of TCls
 * @param ctor_args constructor arguments
 */
#define CR_BIND_DERIVED(TCls, TBase, ctor_args) \
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() const { return binder.class_; } \
	void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	creg::ClassBinder TCls::binder(#TCls, 0, &TBase::binder, &TCls::memberRegistrator, sizeof(TCls), alignof(TCls), TCls::hasVTable, TCls::_ConstructInstance, TCls::_DestructInstance);

/** @def CR_BIND_DERIVED_SUB
 * Bind a derived class inside another class to creg
 * Should be used in the source file
 * @param TSuper class that contains the class to register
 * @param TCls subclass to bind
 * @param TBase base class of TCls
 * @param ctor_args constructor arguments
 */
#define CR_BIND_DERIVED_SUB(TSuper, TCls, TBase, ctor_args) \
	creg::IMemberRegistrator* TSuper::TCls::memberRegistrator=0;	 \
	creg::Class* TSuper::TCls::GetClass() const { return binder.class_; }  \
	void TSuper::TCls::_ConstructInstance(void* d) { new(d) TCls ctor_args; }  \
	void TSuper::TCls::_DestructInstance(void* d) { ((TCls*)d)->~TCls(); }  \
	creg::ClassBinder TSuper::TCls::binder(#TSuper "::" #TCls, 0, &TBase::binder, &TSuper::TCls::memberRegistrator, sizeof(TSuper::TCls), alignof(TCls), TCls::hasVTable, TSuper::TCls::_ConstructInstance, TSuper::TCls::_DestructInstance);

/** @def CR_BIND
 * Bind a class not derived from CObject
 * should be used in the source file
 * @param TCls class to bind
 * @param ctor_args constructor arguments
 */
#define CR_BIND(TCls, ctor_args) \
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() const { return binder.class_; } \
	void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	creg::ClassBinder TCls::binder(#TCls, 0, 0, &TCls::memberRegistrator, sizeof(TCls), alignof(TCls), TCls::hasVTable, TCls::_ConstructInstance, TCls::_DestructInstance);
// Stupid GCC likes this template<> crap very much
#define CR_BIND_TEMPLATE(TCls, ctor_args) \
	template<> creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	template<> creg::Class* TCls::GetClass() const { return binder.class_; } \
	template<> void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	template<> void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	template<> creg::ClassBinder TCls::binder(#TCls, 0, 0, &TCls::memberRegistrator, sizeof(TCls), alignof(TCls), TCls::hasVTable, TCls::_ConstructInstance, TCls::_DestructInstance);

/** @def CR_BIND_DERIVED_INTERFACE
 * Bind an abstract derived class
 * should be used in the source file
 * @param TCls abstract class to bind
 * @param TBase base class of TCls
 */
#define CR_BIND_DERIVED_INTERFACE(TCls, TBase)	\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() const { return binder.class_; } \
	creg::ClassBinder TCls::binder(#TCls, (unsigned int)creg::CF_Abstract, &TBase::binder, &TCls::memberRegistrator, sizeof(TCls), alignof(TCls), TCls::hasVTable, 0, 0);

/** @def CR_BIND_INTERFACE
 * Bind an abstract class
 * should be used in the source file
 * This simply does not register a constructor to creg, so you can bind
 * non-abstract class as abstract classes as well.
 * @param TCls abstract class to bind
 */
#define CR_BIND_INTERFACE(TCls)	\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() const { return binder.class_; } \
	creg::ClassBinder TCls::binder(#TCls, (unsigned int)creg::CF_Abstract, 0, &TCls::memberRegistrator, sizeof(TCls), alignof(TCls), TCls::hasVTable, 0, 0);

/** @def CR_REG_METADATA
 * Binds the class metadata to the class itself
 * should be used in the source file
 * @param TClass class to register the info to
 * @param Members the metadata of the class\n
 * should consist of a series of single expression of metadata macros\n
 * for example: (CR_MEMBER(a),CR_POSTLOAD(PostLoadCallback))
 * @see CR_MEMBER
 * @see CR_ENUM_MEMBER
 * @see CR_SERIALIZER
 * @see CR_POSTLOAD
 * @see CR_MEMBER_SETFLAG
 */
#define CR_REG_METADATA(TClass, Members)				\
	struct TClass##MemberRegistrator : creg::IMemberRegistrator {\
	typedef TClass Type;						\
	TClass##MemberRegistrator() {				\
		Type::memberRegistrator=this;				\
	}												\
	void RegisterMembers(creg::Class* class_) {		\
		TClass* null=(Type*)0;						\
		(void)null; /*suppress compiler warning if this isn't used*/	\
		Members; }									\
	} static TClass##mreg;

/** @def CR_REG_METADATA_SUB
 * Just like CR_REG_METADATA, but for a subclass.
 *  @see CR_REG_METADATA
 */
#define CR_REG_METADATA_SUB(TSuperClass, TSubClass, Members)				\
	struct TSuperClass::TSubClass##MemberRegistrator : creg::IMemberRegistrator {\
	typedef TSuperClass::TSubClass Type;						\
	TSubClass##MemberRegistrator() {				\
		Type::memberRegistrator=this;				\
	}												\
	void RegisterMembers(creg::Class* class_) {		\
		Type* null=(Type*)0;						\
		Members; }									\
	} static TSuperClass##TSubClass##mreg;

/** @def CR_MEMBER
 * Registers a class/struct member variable, of a type that is:
 * - a struct registered with CR_DECLARE_STRUCT/CR_BIND_STRUCT
 * - a class registered with CR_DECLARE/CR_BIND*
 * - an int,short,char,long,double,float or bool, or any of the unsigned
 *   variants of those
 * - a std::set/multiset included with STL_Set.h
 * - a std::list included with STL_List.h
 * - a std::deque included with STL_Deque.h
 * - a std::map/multimap included with STL_Map.h
 * - a std::vector
 * - a std::string
 * - an array
 * - a pointer/reference to a creg registered struct or class instance
 * For enumerated type members, @see CR_ENUM_MEMBER
 */
#define CR_MEMBER(Member) \
	class_->AddMember( #Member, creg::GetType(null->Member), (unsigned int)(((char*)&null->Member)-((char*)0)), alignofv(null->Member))

/** @def CR_ENUM_MEMBER
 * Registers a class/struct member variable with an enumerated type
 */
#define CR_ENUM_MEMBER(Member) \
	class_->AddMember( #Member, creg::IType::CreateEnumeratedType(sizeof(null->Member)), (unsigned int)(((char*)&null->Member)-((char*)0)), alignofv(null->Member))

/** @def CR_IGNORED
 * Registers a member variable that isn't saved/loaded
 */
#define CR_IGNORED(Member) \
	class_->AddMember( #Member, new creg::IgnoredType(sizeof(null->Member)), (unsigned int)(((char*)&null->Member)-((char*)0)), alignofv(null->Member))


/** @def CR_MEMBER_UN
 * Registers a member variable that is unsynced.
 * It may be saved depending on the purpose.
 * Currently works as CR_IGNORED.
 */
#define CR_MEMBER_UN(Member) \
    CR_IGNORED( Member )


/** @def CR_RESERVED
 *  @author Victor Muraviev
 * Registers a unused space for compatibility
 * Size = 1:
 * - char, synced char, bool, synced bool
 * - pointer
 * - enum
 * Size = 2:
 * - short, synced short
 * - enum
 * Size = 4:
 * - int, synced int, long, synced long, float, synced float
 * - std::set/multiset
 * - std::list
 * - std::deque
 * - std::map/multimap
 * - std::vector
 * - std::string
 * - enum
 * Size = 8:
 * - double, synced double
 */
#define CR_RESERVED(Size) \
	class_->AddMember("Reserved", new creg::EmptyType(Size), 0, 0)

/** @def CR_SETFLAG
 * Set a flag for a class/struct.
 * @param Flag the class flag @see ClassFlag
 */
#define CR_SETFLAG(Flag) \
	class_->SetFlag(creg::Flag)

/** @def CR_MEMBER_SETFLAG
 * Set a flag for a class/struct member
 * This should come after the CR_MEMBER or CR_ENUM_MEMBER for the member
 * @param Member the class member variable
 * @param Flag the class member flag @see ClassMemberFlag
 * @see ClassMemberFlag
 */
#define CR_MEMBER_SETFLAG(Member, Flag) \
	class_->SetMemberFlag(#Member, creg::Flag)

#define CR_MEMBER_BEGINFLAG(Flag) \
	class_->BeginFlag(creg::Flag)

#define CR_MEMBER_ENDFLAG(Flag) \
	class_->EndFlag(creg::Flag)

/** @def CR_SERIALIZER
 * Registers a custom serialization method for the class/struct
 * this function will be called when an instance is serialized.
 * There can only be one serialize method per class/struct.
 * On serialization, the registered members will be serialized first,
 * and then this function will be called if specified
 *
 * @param SerializeFunc the serialize method, should be a member function of the
 *   class
 */
#define CR_SERIALIZER(SerializeFunc) \
	(class_->serializeProc = (void(creg::_DummyStruct::*)(creg::ISerializer&))&Type::SerializeFunc)

/** @def CR_POSTLOAD
 * Registers a custom post-loading method for the class/struct
 * this function will be called during package loading when all serialization is
 * finished.
 * There can only be one postload method per class/struct
 */
#define CR_POSTLOAD(PostLoadFunc) \
	(class_->postLoadProc = (void(creg::_DummyStruct::*)())&Type::PostLoadFunc)
};

#endif // _CREG_H

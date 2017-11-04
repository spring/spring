/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// creg - Code compoment registration system

#ifndef _CREG_H
#define _CREG_H

#include <vector>
#include <string>
#include <type_traits> // std::is_polymorphic
#include <memory>

#include "ISerializer.h"
#include "System/Sync/SyncedPrimitive.h"


namespace creg {

	class IType;
	class Class;
	class ClassBinder;

	typedef unsigned int uint;

	enum ClassFlags {
		CF_None = 0,
		CF_Abstract = 4,
		CF_Synced = 8,
	};

	/** Class member flags to use with CR_MEMBER_SETFLAG */
	enum ClassMemberFlag {
		CM_NoSerialize = 1, /// Make the serializers skip the member
		CM_Config = 2,  // Exposed in config
	};

	// Fundamental/basic types
	enum BasicTypeID
	{
		crInt,
		crFloat,
#if defined(SYNCDEBUG) || defined(SYNCCHECK)
		crSyncedInt,
		crSyncedFloat
#endif
	};

	class IType
	{
	public:
		// Type interface can go here...
		virtual ~IType() {}

		virtual void Serialize(ISerializer* s, void* instance) = 0;
		virtual std::string GetName() const = 0;
		virtual size_t GetSize() const = 0;

		static std::shared_ptr<IType> CreateBasicType(BasicTypeID t, size_t size);
		static std::shared_ptr<IType> CreateStringType();
		static std::shared_ptr<IType> CreateObjInstanceType(Class* objectType);
		static std::shared_ptr<IType> CreateIgnoredType(size_t size);
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
			std::shared_ptr<IType> type;
			unsigned int offset;
			int alignment;
			int flags; // combination of ClassMemberFlag's
		};

		Class(const char* name);

		/// Returns true if this class is equal to or derived from other
		bool IsSubclassOf(Class* other) const;

		/// Allocate an instance of the class
		void* CreateInstance();
		void DeleteInstance(void* inst);

		/// Calculate a checksum from the class metadata
		void CalculateChecksum(unsigned int& checksum);
		void AddMember(const char* name, std::shared_ptr<IType> type, unsigned int offset, int alignment);
		void SetMemberFlag(const char* name, ClassMemberFlag f);
		Member* FindMember(const char* name, const bool inherited = true);

		void BeginFlag(ClassMemberFlag flag);
		void EndFlag(ClassMemberFlag flag);
		void SetFlag(ClassFlags flag);

		inline bool IsAbstract() const;
		inline Class* base() const;

		/// Returns all concrete classes that implement this class
		std::vector<Class*> GetImplementations();

		/// all classes that derive from this class
		const std::vector<Class*>& GetDerivedClasses() const;

		// generic member function pointer
		typedef void(Class::*FuncPTR)();
		typedef void(*CastAndSerializeProc)(void* object, ISerializer* s, FuncPTR sp);
		typedef void(*CastAndPostLoadProc)(void* object, FuncPTR pp);


		template<class T, typename TF, int bla = sizeof(TF), int bli = sizeof(FuncPTR)>
		void SetSerialize(T* foo, TF proc) {
			static_assert(sizeof(serializeProc) == sizeof(proc), "function pointers have different sizes");
			serializeProc = *reinterpret_cast<FuncPTR*>(&proc);
			castAndSerializeProc = [](void* object, ISerializer* s, FuncPTR sp) {
				(static_cast<T*>(object)->**(reinterpret_cast<TF*>(&sp)))(s);
			};
		}
		template<class T, typename TF, int bla = sizeof(TF), int bli = sizeof(FuncPTR)>
		void SetPostLoad(T* foo, TF proc) {
			static_assert(sizeof(postLoadProc) == sizeof(proc), "function pointers have different sizes");
			postLoadProc = *reinterpret_cast<FuncPTR*>(&proc);
			castAndPostLoadProc = [](void* object, FuncPTR pp) {
				(static_cast<T*>(object)->**(reinterpret_cast<TF*>(&pp)))();
			};
		}
		bool HasSerialize() const { return (serializeProc != nullptr); }
		bool HasPostLoad() const { return (postLoadProc != nullptr); }
		void CallSerializeProc(void* object, ISerializer* s) { castAndSerializeProc(object, s, serializeProc); }
		void CallPostLoadProc(void* object)                  { castAndPostLoadProc(object, postLoadProc); }

	public:
		std::vector<Member> members;

		ClassBinder* binder;
		std::string name;
		int size; // size of an instance in bytes
		int alignment;

		FuncPTR serializeProc;
		FuncPTR postLoadProc;
		CastAndSerializeProc castAndSerializeProc;
		CastAndPostLoadProc castAndPostLoadProc;

	private:
		int currentMemberFlags;
	};


	/**
	 * Stores class bindings such as constructor/destructor
	 */
	class ClassBinder //TODO merge with `class Class`
	{
	public:
		ClassBinder(const char* className, ClassFlags cf, ClassBinder* base,
				void (*memberRegistrator)(creg::Class*),
				int instanceSize, int instanceAlignment, bool hasVTable, bool isCregStruct,
				void (*constructorProc)(void* instance), void (*destructorProc)(void* instance),
				void* (*allocProc)(), void (*freeProc)(void* instance));

		// propagates poolAlloc and poolFree to derivatives
		void PropagatePoolFuncs();

		Class class_;
		ClassBinder* base;
		ClassFlags flags;
		bool hasVTable;
		bool isCregStruct;

		void (*constructor)(void* instance);
		/**
		 * Needed for classes without virtual destructor.
		 * (classes/structs declared with CR_DECLARE_STRUCT)
		 */
		void (*destructor)(void* instance);


		/**
		 * Needed for objects using pools.
		 */
		void* (*poolAlloc)();

		void (*poolFree)(void* instance);
	};


	/**
	 * CREG main interface for outside code
	 */
	class System
	{
	public:
		/// Return the global list of classes
		static const std::vector<Class*>& GetClasses();

		/// Find a class by name
		static Class* GetClass(const std::string& name);

		static void AddClass(Class* c);
	};


	//Note: has to be defined after `class ClassBinder`, else we get a compile error
	bool Class::IsAbstract() const { return (binder->flags & CF_Abstract) != 0; }
	Class* Class::base() const { return binder->base ? &binder->base->class_ : nullptr; }
}




// -------------------------------------------------------------------
// Container Type templates
// -------------------------------------------------------------------

#include "BasicTypes.h"
#include "TypeDeduction.h"


//FIXME: defined cause gcc4.8 still doesn't support c++11's offsetof for non-static members
#define offsetof_creg(type, member) (std::size_t)(((char*)&null->member) - ((char*)null))


#define CR_DECLARE_BASE(TCls, isStr, VIRTUAL, OVERRIDE)	\
public: \
	static creg::ClassBinder binder; \
	static const bool creg_isStruct = isStr; \
	typedef TCls MyType; \
	static creg::Class* StaticClass() { return &binder.class_; } \
	VIRTUAL creg::Class* GetClass() const OVERRIDE { return &binder.class_; } \
	static void _ConstructInstance(void* d); \
	static void _DestructInstance(void* d); \
	static void* _Alloc(); \
	static void _Free(void* d); \
	static void _CregRegisterMembers(creg::Class* class_);


/** @def CR_DECLARE
 * Add the definitions for creg binding to the class
 * this should be put within the class definition
 */
#define CR_DECLARE(TCls) CR_DECLARE_BASE(TCls, false, virtual, )


/** @def CR_DECLARE_DERIVED
 * Use this to declare a derived class
 * this should be put in the class definition, instead of CR_DECLARE
 */
#define CR_DECLARE_DERIVED(TCls) CR_DECLARE_BASE(TCls, false, virtual, override)

/** @def CR_DECLARE_STRUCT
 * Use this to declare a structure
 * this should be put in the class definition, instead of CR_DECLARE
 * For creg, the only difference between a class and a structure is having a
 * vtable or not.
 */
#define CR_DECLARE_STRUCT(TStr)	CR_DECLARE_BASE(TStr ,true, , )

/** @def CR_DECLARE_SUB
 * Use this to declare a sub class. This should be put in the class definition
 * of the superclass, alongside CR_DECLARE(CSuperClass) (or CR_DECLARE_STRUCT).
 */
#define CR_DECLARE_SUB(cl) \
	struct cl##MemberRegistrator;

/** @def CR_BIND
 * Bind a class not derived from CObject
 * should be used in the source file
 * @param TCls class to bind
 * @param ctor_args constructor arguments
 */
#define CR_BIND(TCls, ctor_args) \
	void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_None, nullptr, &TCls::_CregRegisterMembers, sizeof(TCls), alignof(TCls), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, TCls::_ConstructInstance, TCls::_DestructInstance, nullptr, nullptr);

/** @def CR_BIND_DERIVED
 * Bind a derived class declared with CR_DECLARE to creg
 * Should be used in the source file
 * @param TCls class to bind
 * @param TBase base class of TCls
 * @param ctor_args constructor arguments
 */
#define CR_BIND_DERIVED(TCls, TBase, ctor_args) \
	void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_None, &TBase::binder, &TCls::_CregRegisterMembers, sizeof(TCls), alignof(TCls), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, TCls::_ConstructInstance, TCls::_DestructInstance, nullptr, nullptr);

/** @def CR_BIND_DERIVED_SUB
 * Bind a derived class inside another class to creg
 * Should be used in the source file
 * @param TSuper class that contains the class to register
 * @param TCls subclass to bind
 * @param TBase base class of TCls
 * @param ctor_args constructor arguments
 */
#define CR_BIND_DERIVED_SUB(TSuper, TCls, TBase, ctor_args) \
	void TSuper::TCls::_ConstructInstance(void* d) { new(d) TCls ctor_args; }  \
	void TSuper::TCls::_DestructInstance(void* d) { ((TCls*)d)->~TCls(); }  \
	creg::ClassBinder TSuper::TCls::binder(#TSuper "::" #TCls, creg::CF_None, &TBase::binder, &TSuper::TCls::_CregRegisterMembers, sizeof(TSuper::TCls), alignof(TCls), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, TSuper::TCls::_ConstructInstance, TSuper::TCls::_DestructInstance, nullptr, nullptr);

/** @def CR_BIND_DERIVED
 * Bind a derived class declared with CR_DECLARE to creg
 * Should be used in the source file
 * @param TCls class to bind
 * @param TBase base class of TCls
 * @param ctor_args constructor arguments
 * @param poolAlloc pool function used to allocate
 * @param poolFree pool function used to allocate
 */
#define CR_BIND_DERIVED_POOL(TCls, TBase, ctor_args, poolAlloc, poolFree) \
	void* TCls::_Alloc() { return poolAlloc(); } \
	void TCls::_Free(void* d) { poolFree(d); } \
	void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_None, &TBase::binder, &TCls::_CregRegisterMembers, sizeof(TCls), alignof(TCls), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, TCls::_ConstructInstance, TCls::_DestructInstance, TCls::_Alloc, TCls::_Free);

/** @def CR_BIND_TEMPLATE
 *  @see CR_BIND
 */
#define CR_BIND_TEMPLATE(TCls, ctor_args) \
	template<> void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	template<> void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	template<> void TCls::_CregRegisterMembers(creg::Class* class_); \
	template<> creg::ClassBinder TCls::binder(#TCls, creg::CF_None, nullptr, &TCls::_CregRegisterMembers, sizeof(TCls), alignof(TCls), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, TCls::_ConstructInstance, TCls::_DestructInstance, nullptr, nullptr);


/** @def CR_BIND_DERIVED_INTERFACE
 * Bind an abstract derived class
 * should be used in the source file
 * @param TCls abstract class to bind
 * @param TBase base class of TCls
 */
#define CR_BIND_DERIVED_INTERFACE(TCls, TBase)	\
	creg::ClassBinder TCls::binder(#TCls, creg::CF_Abstract, &TBase::binder, &TCls::_CregRegisterMembers, sizeof(TCls&), alignof(TCls&), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, nullptr, nullptr, nullptr, nullptr);


	/** @def CR_BIND_DERIVED_INTERFACE_POOL
 * Bind an abstract derived class
 * should be used in the source file
 * @param TCls abstract class to bind
 * @param TBase base class of TCls
 * @param poolAlloc pool function used to allocate
 * @param poolFree pool function used to allocate
 */
#define CR_BIND_DERIVED_INTERFACE_POOL(TCls, TBase, poolAlloc, poolFree)	\
	void* TCls::_Alloc() { return poolAlloc(); } \
	void TCls::_Free(void* d) { poolFree(d); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_Abstract, &TBase::binder, &TCls::_CregRegisterMembers, sizeof(TCls&), alignof(TCls&), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, nullptr, nullptr, TCls::_Alloc, TCls::_Free);

/** @def CR_BIND_INTERFACE
 * Bind an abstract class
 * should be used in the source file
 * This simply does not register a constructor to creg, so you can bind
 * non-abstract class as abstract classes as well.
 * @param TCls abstract class to bind
 */
#define CR_BIND_INTERFACE(TCls)	\
	creg::ClassBinder TCls::binder(#TCls, creg::CF_Abstract, nullptr, &TCls::_CregRegisterMembers, sizeof(TCls&), alignof(TCls&), std::is_polymorphic<TCls>::value, TCls::creg_isStruct, nullptr, nullptr, nullptr, nullptr);

/** @def CR_REG_METADATA
 * Binds the class metadata to the class itself
 * should be used in the source file
 * @param TClass class to register the info to
 * @param Members the metadata of the class\n
 * should consist of a series of single expression of metadata macros\n
 * for example: (CR_MEMBER(a),CR_POSTLOAD(PostLoadCallback))
 * @see CR_MEMBER
 * @see CR_SERIALIZER
 * @see CR_POSTLOAD
 * @see CR_MEMBER_SETFLAG
 */
#define CR_REG_METADATA(TClass, Members) \
	void TClass::_CregRegisterMembers(creg::Class* class_) { \
		typedef TClass Type; \
		Type* null=nullptr; \
		(void)null; /*suppress compiler warning if this isn't used*/ \
		Members; \
	}

/** @def CR_REG_METADATA_TEMPLATE
 * Just like CR_REG_METADATA, but for a template.
 *  @see CR_REG_METADATA
 */
#define CR_REG_METADATA_TEMPLATE(TCls, Members) \
	template<> void TCls::_CregRegisterMembers(creg::Class* class_) { \
		typedef TCls Type; \
		Type* null=nullptr; \
		(void)null; /*suppress compiler warning if this isn't used*/ \
		Members; \
	}

/** @def CR_REG_METADATA_SUB
 * Just like CR_REG_METADATA, but for a subclass.
 *  @see CR_REG_METADATA
 */
#define CR_REG_METADATA_SUB(TSuperClass, TSubClass, Members) \
	void TSuperClass::TSubClass::_CregRegisterMembers(creg::Class* class_) { \
		typedef TSuperClass::TSubClass Type; \
		Type* null=nullptr; \
		(void)null; /*suppress compiler warning if this isn't used*/ \
		Members; \
	}

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
 * - an enum
 */
#define CR_MEMBER(Member) \
	class_->AddMember( #Member, creg::GetType(null->Member), offsetof_creg(Type, Member), alignof(decltype(Type::Member)))

/** @def CR_IGNORED
 * Registers a member variable that isn't saved/loaded
 */
#define CR_IGNORED(Member) \
	class_->AddMember( #Member, creg::IType::CreateIgnoredType(sizeof(Type::Member)), offsetof_creg(Type, Member), alignof(decltype(Type::Member)))


/** @def CR_MEMBER_UN
 * Registers a member variable that is unsynced.
 * It may be saved depending on the purpose.
 * Currently works as CR_IGNORED.
 */
#define CR_MEMBER_UN(Member) \
	CR_IGNORED( Member )

/** @def CR_SETFLAG
 * Set a flag for a class/struct.
 * @param Flag the class flag @see ClassFlag
 */
#define CR_SETFLAG(Flag) \
	class_->SetFlag(creg::Flag)

/** @def CR_MEMBER_SETFLAG
 * Set a flag for a class/struct member
 * This should come after the CR_MEMBER for the member
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
	(class_->SetSerialize(null, &Type::SerializeFunc))

/** @def CR_POSTLOAD
 * Registers a custom post-loading method for the class/struct
 * this function will be called during package loading when all serialization is
 * finished.
 * There can only be one postload method per class/struct
 */
#define CR_POSTLOAD(PostLoadFunc) \
	(class_->SetPostLoad(null, &Type::PostLoadFunc))

#endif // _CREG_H

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// creg - Code compoment registration system

#ifndef _CREG_H
#define _CREG_H

#include <vector>
#include <string>
#include <type_traits> // std::is_polymorphic
#include <boost/shared_ptr.hpp>
#include <functional>

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

		static boost::shared_ptr<IType> CreateBasicType(BasicTypeID t, size_t size);
		static boost::shared_ptr<IType> CreateStringType();
		static boost::shared_ptr<IType> CreateObjInstanceType(Class* objectType);
		static boost::shared_ptr<IType> CreateIgnoredType(size_t size);
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

		Class(const char* name);

		/// Returns true if this class is equal to or derived from other
		bool IsSubclassOf(Class* other) const;

		/// Allocate an instance of the class
		void* CreateInstance();
		void DeleteInstance(void* inst);

		/// Calculate a checksum from the class metadata
		void CalculateChecksum(unsigned int& checksum);
		void AddMember(const char* name, boost::shared_ptr<IType> type, unsigned int offset, int alignment);
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

		template<class T, typename TF>
		void SetSerialize(T* foo, TF proc) {
			serializeProc = [=](void* object, ISerializer* s) {
				(static_cast<T*>(object)->*proc)(s);
			};
		}
		template<class T, typename TF>
		void SetPostLoad(T* foo, TF proc) {
			postLoadProc = [=](void* object) {
				(static_cast<T*>(object)->*proc)();
			};
		}
		bool HasSerialize() const { return (bool)serializeProc; }
		bool HasPostLoad() const { return (bool)postLoadProc; }
		void CallSerializeProc(void* object, ISerializer* s) { serializeProc(object, s); }
		void CallPostLoadProc(void* object)                  { postLoadProc(object); }

	public:
		std::vector<Member> members;

		ClassBinder* binder;
		std::string name;
		int size;
		int alignment;

		std::function<void(void* object, ISerializer* s)> serializeProc;
		std::function<void(void* object)> postLoadProc;

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
				void (*constructorProc)(void* instance), void (*destructorProc)(void* instance));

		Class class_;
		ClassBinder* base;
		ClassFlags flags;
		const char* name;
		int size; // size of an instance in bytes
		int alignment;
		bool hasVTable;
		bool isCregStruct;

		void (*constructor)(void* instance);
		/**
		 * Needed for classes without virtual destructor.
		 * (classes/structs declared with CR_DECLARE_STRUCT)
		 */
		void (*destructor)(void* instance);
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
	static bool creg_hasVTable; \
	static const bool creg_isStruct = isStr; \
	typedef TCls MyType; \
	static creg::Class* StaticClass() { return &binder.class_; } \
	VIRTUAL creg::Class* GetClass() const OVERRIDE { return &binder.class_; } \
	static void _ConstructInstance(void* d); \
	static void _DestructInstance(void* d); \
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
	bool TCls::creg_hasVTable = std::is_polymorphic<TCls>::value; \
	void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_None, nullptr, &TCls::_CregRegisterMembers, sizeof(TCls), alignof(TCls), TCls::creg_hasVTable, TCls::creg_isStruct, TCls::_ConstructInstance, TCls::_DestructInstance);

/** @def CR_BIND_DERIVED
 * Bind a derived class declared with CR_DECLARE to creg
 * Should be used in the source file
 * @param TCls class to bind
 * @param TBase base class of TCls
 * @param ctor_args constructor arguments
 */
#define CR_BIND_DERIVED(TCls, TBase, ctor_args) \
	bool TCls::creg_hasVTable = std::is_polymorphic<TCls>::value; \
	void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_None, &TBase::binder, &TCls::_CregRegisterMembers, sizeof(TCls), alignof(TCls), TCls::creg_hasVTable, TCls::creg_isStruct, TCls::_ConstructInstance, TCls::_DestructInstance);

/** @def CR_BIND_DERIVED_SUB
 * Bind a derived class inside another class to creg
 * Should be used in the source file
 * @param TSuper class that contains the class to register
 * @param TCls subclass to bind
 * @param TBase base class of TCls
 * @param ctor_args constructor arguments
 */
#define CR_BIND_DERIVED_SUB(TSuper, TCls, TBase, ctor_args) \
	bool TSuper::TCls::creg_hasVTable = std::is_polymorphic<TSuper::TCls>::value; \
	void TSuper::TCls::_ConstructInstance(void* d) { new(d) TCls ctor_args; }  \
	void TSuper::TCls::_DestructInstance(void* d) { ((TCls*)d)->~TCls(); }  \
	creg::ClassBinder TSuper::TCls::binder(#TSuper "::" #TCls, creg::CF_None, &TBase::binder, &TSuper::TCls::_CregRegisterMembers, sizeof(TSuper::TCls), alignof(TCls), TCls::creg_hasVTable, TCls::creg_isStruct, TSuper::TCls::_ConstructInstance, TSuper::TCls::_DestructInstance);


/** @def CR_BIND_TEMPLATE
 *  @see CR_BIND
 */
#define CR_BIND_TEMPLATE(TCls, ctor_args) \
	template<> bool TCls::creg_hasVTable = std::is_polymorphic<TCls>::value; \
	template<> void TCls::_ConstructInstance(void* d) { new(d) MyType ctor_args; } \
	template<> void TCls::_DestructInstance(void* d) { ((MyType*)d)->~MyType(); } \
	template<> void TCls::_CregRegisterMembers(creg::Class* class_); \
	template<> creg::ClassBinder TCls::binder(#TCls, creg::CF_None, nullptr, &TCls::_CregRegisterMembers, sizeof(TCls), alignof(TCls), TCls::creg_hasVTable, TCls::creg_isStruct, TCls::_ConstructInstance, TCls::_DestructInstance);


/** @def CR_BIND_DERIVED_INTERFACE
 * Bind an abstract derived class
 * should be used in the source file
 * @param TCls abstract class to bind
 * @param TBase base class of TCls
 */
#define CR_BIND_DERIVED_INTERFACE(TCls, TBase)	\
	bool TCls::creg_hasVTable = std::is_polymorphic<TCls>::value; \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_Abstract, &TBase::binder, &TCls::_CregRegisterMembers, sizeof(TCls&), alignof(TCls&), TCls::creg_hasVTable, TCls::creg_isStruct, 0, 0);

/** @def CR_BIND_INTERFACE
 * Bind an abstract class
 * should be used in the source file
 * This simply does not register a constructor to creg, so you can bind
 * non-abstract class as abstract classes as well.
 * @param TCls abstract class to bind
 */
#define CR_BIND_INTERFACE(TCls)	\
	bool TCls::creg_hasVTable = std::is_polymorphic<TCls>::value; \
	creg::ClassBinder TCls::binder(#TCls, creg::CF_Abstract, nullptr, &TCls::_CregRegisterMembers, sizeof(TCls&), alignof(TCls&), TCls::creg_hasVTable, TCls::creg_isStruct, 0, 0);

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

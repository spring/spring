/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 
*/

#ifndef jc_CR_HEADER
#define jc_CR_HEADER

#include <vector>
#include <string>
#include <map>
#include <deque>
#include <list>
#include <set>
#include "float3.h"

namespace creg {

	class IType;
	class Class;

	class IMemberRegistrator 
	{
	public:
		virtual void RegisterMembers (Class *cls) = 0;
	};


// -------------------------------------------------------------------
// ClassBinder: Registers the class to the class list
// -------------------------------------------------------------------

	class ClassBinder
	{
	public:
		ClassBinder (const char *className, ClassBinder* base, IMemberRegistrator **mreg, void* (*instanceCreateProc)());

		Class *class_;
		ClassBinder *base;
		IMemberRegistrator **memberRegistrator;
		const char *name;
		void* (*createProc)();  // returns a new class instance 

		static ClassBinder* GetList ();
		static void InitializeClasses ();
		static Class* GetClass(const std::string& name);

	protected:
		ClassBinder* nextBinder;

		static std::map<std::string, Class*> nameToClass;
		static ClassBinder *binderList;
	};

// Use this within the class declaration
#define CR_DECLARE(TCls)							\
	static creg::ClassBinder binder;				\
	static creg::IMemberRegistrator *memberRegistrator;	 \
	virtual creg::Class* GetClass();				\
	inline static creg::Class *StaticClass() { return binder.class_; }

// To be used in the .cpp implementation:
// Bind a class derived from another class
#define CR_BIND_DERIVED(TCls, TBase)				\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() { return binder.class_; } \
	void* TCls##CreateInstanceProc() { return new TCls; } \
	creg::ClassBinder TCls::binder(#TCls, &TBase::binder, &TCls::memberRegistrator, TCls##CreateInstanceProc);

// Bind a class not derived from CObject
#define CR_BIND(TCls)								\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() { return binder.class_; } \
	void* TCls##CreateInstanceProc() { return new TCls; } \
	creg::ClassBinder TCls::binder(#TCls, 0, &TCls::memberRegistrator, TCls##CreateInstanceProc);

// Bind an abstract derived class
#define CR_BIND_INTERFACE(TCls, TBase)				\
	creg::IMemberRegistrator* TCls::memberRegistrator=0;	\
	creg::Class* TCls::GetClass() { return binder.class_; } \
	creg::ClassBinder TCls::binder(#TCls, &TBase::binder, &TCls::memberRegistrator, 0);

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

	// Base object class - all objects should derive from this if they should be referencable by pointers 
	class Object
	{
	public:
		CR_DECLARE(Object)

		virtual ~Object() {}
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

		std::vector <Member*> members;
		ClassBinder* binder;
		std::string name;
		Class *base;
	};

// -------------------------------------------------------------------
// Type handlers for a number of types
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
		crFloat3
	};

	class IType
	{
	public:
		// Type interface can go here...
		virtual ~IType() {}

		static IType* CreateBasicType (BasicTypeID t);
		static IType* CreateStringType ();
		static IType* CreatePointerToObjType (Class *objectType);
		static IType* CreateStaticArrayType (IType *elemType, unsigned int size);
	};

	struct Iterator; // Undefined Iterator struct to denote a container iterator

	// Container interface - defines a generic runtime callback to containers of any type
	class IContainerType : public IType
	{
	public:
		IType* elemType;

		IContainerType (IType *elemType) : elemType(elemType) {}
		~IContainerType();

		virtual void Add (void *container, void *elem) = 0;
		virtual void Erase (void *container, Iterator *elem) = 0;
		virtual Iterator* Begin (void *container) = 0;
		virtual void* Element(void *container, Iterator *i) = 0;
		virtual Iterator* Next (void *container, Iterator *i) = 0; // returns 0 on end
	};

	// set/multiset
	template<typename T>
	class OrderedContainer : public IContainerType
	{
	public:
		OrderedContainer (IType *elemType) : IContainerType(elemType) {}
		typedef typename T ct;
		typedef typename T::value_type ElemT;

		void Add (void *container, void *elem) { 
			((ct*)container)->insert(*(ElemT*)elem); }

		void Erase (void *container, Iterator *elem) { 
			((ct*)container)->erase( *(ct::iterator*)elem ); }

		// TODO: mempool for iterators?
		Iterator* Begin (void *container) { 
			if (((ct*)container)->empty()) return 0;
			return new ct::iterator ( ((ct*)container)->begin() ); }

		void* Element(void *container, Iterator *i) { 
			return &**(ct::iterator*)i; }

		Iterator* Next (void *container, Iterator *i) { 
			ct::iterator *it = (ct::iterator*)i;
			++(*it);
			if ( *it == ((ct*)container)->end() ) {
				delete it;
				return 0;
			}
			return i;
		}
	};

	// vector,deque,list container
	template<typename T>
	class UnorderedContainer : public IContainerType
	{
	public:
		typedef typename T ct;
		typedef typename T::value_type ElemT;

		UnorderedContainer (IType *elemType) : IContainerType(elemType) {}

		void Add (void *container, void *elem) {
			((ct*)container)->push_back ( *(ElemT*)elem ); }

		void Erase (void *container, Iterator *elem) {
			((ct*)container)->erase ( *(T::iterator*)elem ); }

		Iterator* Begin (void *container) {
			if (((ct*)container)->empty()) return 0;
			return (Iterator*)new ct::iterator ( ((ct*)container)->begin() ); }

		void* Element(void *container, Iterator *i) {
			return &**(ct::iterator*)i; }

		Iterator* Next (void *container, Iterator *i) {
			ct::iterator *it = (ct::iterator*)i;
			++(*it);
			if ( *it == ((ct*)container)->end() ) {
				delete it;
				return 0;
			}
			return i;
		}
	};

#include "TypeDeduction.h"

};


#endif

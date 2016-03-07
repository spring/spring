/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SERIALIZER_IMPL_H
#define SERIALIZER_IMPL_H

#include "ISerializer.h"
#include "creg_cond.h"

#ifdef USING_CREG

#include <map>
#include <vector>
#include <deque>
#include <istream>

namespace creg {

	/**
	 * Output stream serializer
	 * Usage: create an instance of this class and call SavePackage
	 * @see SavePackage
	 */
	class COutputStreamSerializer : public ISerializer
	{
	protected:
		struct ObjectMember {
			Class::Member* member;
			int memberId;
			int size;
		};
		struct ObjectMemberGroup {
			Class* membersClass;
			std::vector<COutputStreamSerializer::ObjectMember> members;
			int size;
		};
		struct ObjectRef {
			ObjectRef() {
				ptr = 0;
				id=0;
				classIndex=0;
				isEmbedded=false;
				class_=0;
			}
			ObjectRef(void* ptr, int id, bool isEmbedded, Class* class_) {
				this->ptr = ptr;
				this->id=id;
				classIndex=0;
				this->isEmbedded=isEmbedded;
				this->class_=class_;
			}
			ObjectRef(const ObjectRef&src) :memberGroups(src.memberGroups){
				ptr=src.ptr;
				id=src.id;
				classIndex=src.classIndex;
				isEmbedded=src.isEmbedded;
				class_=src.class_;
			}
			void* ptr;
			int id, classIndex;
			bool isEmbedded;
			Class* class_;
			std::vector<COutputStreamSerializer::ObjectMemberGroup> memberGroups;
			bool isThisObject(void* objPtr, Class* objClass, bool objEmbedded) const
			{
				if (ptr != objPtr) return false;
				if (class_ == objClass) return true;
				if (isEmbedded && objEmbedded) return false;
				if (!objEmbedded) {
					for (Class* base=class_->base(); base; base=base->base())
						if (base == objClass) return true;
				}
				if (!isEmbedded) {
					for (Class* base=objClass->base(); base; base=base->base())
						if (base == class_) return true;
				}
				return false;
			}
		};

		// Temporary class reference
		struct ClassRef;

		std::ostream* stream;
		std::map<void*,std::vector<ObjectRef*> > ptrToId;
		std::deque<ObjectRef> objects;
		std::vector<ObjectRef*> pendingObjects; // these objects still have to be saved
		std::map<Class*, int> classSizes;
		std::map<Class*, int> classCounts;

		// Serialize all class names
		void WriteObjectInfo();
		// Helper for instance/ptr saving
		void WriteObjectRef(void* inst, Class* cls, bool embedded);

		ObjectRef* FindObjectRef(void* inst, Class* objClass, bool isEmbedded);

		void SerializeObject(Class* c, void* ptr, ObjectRef* objr);

	public:
		COutputStreamSerializer();

		/** Create a package of the given root object and all the objects that it references
		 * @param s stream to serialize the data to
		 * @param rootObj the rootObj: the starting point for finding all the objects to save
		 * @param cls the class of the root object
		 * This method throws an std::runtime_error when something goes wrong
		 */
		void SavePackage(std::ostream* s, void* rootObj, Class* cls);

		/** @see ISerializer::IsWriting */
		bool IsWriting();

		/** @see ISerializer::SerializeObjectPtr */
		void SerializeObjectPtr(void** ptr, Class* cls);

		/** @see ISerializer::SerializeObjectInstance */
		void SerializeObjectInstance(void* inst, Class* cls);

		/** @see ISerializer::Serialize */
		void Serialize(void* data, int byteSize);

		/** @see ISerializer::SerializeInt */
		void SerializeInt(void* data, int byteSize);

		/** Empty function, only applies to loading */
		void AddPostLoadCallback(void (*cb)(void* d), void* d) {}
	};

	/** Input stream serializer
	 * Usage: Create an instance of this class and call LoadPackage
	 * @see LoadPackage
	 */
	class CInputStreamSerializer : public ISerializer
	{
	protected:
		std::istream* stream;
		std::vector<Class*> classRefs;

		struct UnfixedPtr {
			void** ptrAddr;
			int objID;
		};
		std::vector<UnfixedPtr> unfixedPointers; // pointers to embedded objects that haven't been filled yet, because they are not yet loaded

		struct StoredObject
		{
			void* obj;
			int classRef;
			bool isEmbedded;
		};
		std::vector<StoredObject> objects;

		struct PostLoadCallback
		{
			void (*cb)(void* d);
			void* userdata;
		};
		std::vector<PostLoadCallback> callbacks;

		void SerializeObject(Class* c, void* ptr);
	public:
		CInputStreamSerializer();
		~CInputStreamSerializer();

		/** @see ISerializer::IsWriting */
		bool IsWriting();

		/** @see ISerializer::SerializeObjectPtr */
		void SerializeObjectPtr(void** ptr, Class* cls);

		/** @see ISerializer::SerializeObjectInstance */
		void SerializeObjectInstance(void* inst, Class* cls);

		/** @see ISerializer::Serialize */
		void Serialize(void* data, int byteSize);

		/** @see ISerializer::SerializeInt */
		void SerializeInt(void* data, int byteSize);

		/** @see ISerializer::AddPostLoadCallback */
		void AddPostLoadCallback(void (*cb)(void* userdata), void* userdata);

		/** Load a package that is saved by CInputStreamSerializer
		 * @param s the input stream to read from
		 * @param root the root object address will be assigned to this
		 * @param rootCls the root object class will be assigned to this
		 * This method throws an std::runtime_error when something goes wrong */
		void LoadPackage(std::istream* s, void*& root, Class*& rootCls);
	};

}

#endif //USING_CREG


#endif //SERIALIZER_IMPL_H

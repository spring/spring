
#ifndef SERIALIZER_IMPL_H
#define SERIALIZER_IMPL_H

#include "ISerializer.h"
#include SPRING_HASH_MAP_H
#include <vector>

#ifdef __GNUG__
// This is needed as gnu doesn't offer specialization for other pointer types other that char*
// (look in ext/hash_fun.h for the types supported out of the box)
namespace __gnu_cxx
{

  template<> struct hash<void*>
  {
    size_t operator()(const void* __s) const
    { return (size_t)(__s); }
  };
}
#endif

namespace creg {
	/**
	 * Output stream serializer
	 * Usage: create an instance of this class and call SavePackage
	 * @see SavePackage
	 */
	class COutputStreamSerializer : public ISerializer
	{
	protected:
		struct ObjectID {
			int id, classIndex;
			bool isEmbedded;
			creg::Class *class_;
		};

		// Temporary class reference
		struct ClassRef;

		typedef std::map<void*, ObjectID> ObjIDmap;
		ObjIDmap ptrToID; // maps pointers to object IDs that can be serialized
		std::ostream *stream;

		std::vector <ObjectID*> objects;
		std::vector <ObjIDmap::iterator> pendingObjects; // these objects still have to be saved

		// Serialize all class names
		void WriteObjectInfo ();
		// Helper for instance/ptr saving
		void WriteObjectRef (void *inst, creg::Class *cls, bool embedded);

	public:
		COutputStreamSerializer ();

		/** Create a package of the given root object and all the objects that it references
		 * @param s stream to serialize the data to
		 * @param rootObj the rootObj: the starting point for finding all the objects to save
		 * @param cls the class of the root object
		 * This method throws an std::runtime_error when something goes wrong
		 */
		void SavePackage (std::ostream *s, void *rootObj, creg::Class *cls);

		/** @see ISerializer::IsWriting */
		bool IsWriting ();

		/** @see ISerializer::SerializeObjectPtr */
		void SerializeObjectPtr (void **ptr, creg::Class *cls);
		
		/** @see ISerializer::SerializeObjectInstance */
		void SerializeObjectInstance (void *inst, creg::Class *cls);

		/** @see ISerializer::Serialize */
		void Serialize (void *data, int byteSize);
	};

	/** Input stream serializer
	 * Usage: Create an instance of this class and call LoadPackage
	 * @see LoadPackage
	 */
	class CInputStreamSerializer : public ISerializer
	{
	protected:
		std::istream* stream;
		std::vector <creg::Class *> classRefs;

		struct UnfixedPtr {
			void **ptrAddr;
			int objID;
		};
		std::vector <UnfixedPtr> unfixedPointers; // pointers to embedded objects that haven't been filled yet, because they are not yet loaded

		struct StoredObject
		{
			void *obj;
			int classRef;
			bool isEmbedded;
		};
		std::vector <StoredObject> objects;

	public:
		CInputStreamSerializer ();
		~CInputStreamSerializer ();

		/** @see ISerializer::IsWriting */
		bool IsWriting ();

		/** @see ISerializer::SerializeObjectPtr */
		void SerializeObjectPtr (void **ptr, creg::Class *cls);
		
		/** @see ISerializer::SerializeObjectInstance */
		void SerializeObjectInstance (void *inst, creg::Class *cls);

		/** @see ISerializer::Serialize */
		void Serialize (void *data, int byteSize);

		/** Load a package that is saved by CInputStreamSerializer
		 * @param s the input stream to read from
		 * @param root the root object address will be assigned to this
		 * @param rootCls the root object class will be assigned to this
		 * This method throws an std::runtime_error when something goes wrong */
		void LoadPackage (std::istream *s, void *&root, creg::Class *&rootCls);
	};

};


#endif

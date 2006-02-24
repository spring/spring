
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
//-------------------------------------------------------------------------
// Base output serializer
//-------------------------------------------------------------------------

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

		typedef SPRING_HASH_MAP<void*, ObjectID> ObjIDmap;
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

		void SavePackage (std::ostream *s, void *rootObj, creg::Class *cls);
		bool IsWriting ();

		// Serialize a pointer to an object.
		void SerializeObjectPtr (void **ptr, creg::Class *cls);
		
		// Serialize an instance of an object, could be a structure embedded into another object
		void SerializeObjectInstance (void *inst, creg::Class *cls);

		void Serialize (void *data, int byteSize);
	};

//-------------------------------------------------------------------------
// Base input serializer
//-------------------------------------------------------------------------

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

		void Serialize (void *data, int byteSize);
		bool IsWriting ();
		// Serialize a pointer to an object.
		void SerializeObjectPtr (void **ptr, creg::Class *cls);
		// Serialize an instance of an object, could be a structure embedded into another object
		void SerializeObjectInstance (void *inst, creg::Class *cls);

		void LoadPackage (std::istream *s, void *&root, creg::Class *&rootCls);
	};

};


#endif

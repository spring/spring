
#ifndef SERIALIZER_IMPL_H
#define SERIALIZER_IMPL_H

#include "ISerializer.h"
#include <map>
#include <vector>

namespace creg {
//-------------------------------------------------------------------------
// Base output serializer
//-------------------------------------------------------------------------

	class COutputStreamSerializer : public ISerializer
	{
	public:
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
	public:
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

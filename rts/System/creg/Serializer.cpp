/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen 

Classes for serialization of registrated class instances
*/
#include "StdAfx.h"
#include "creg.h"
#include "Serializer.h"
#include "Platform/byteorder.h"
#include <fstream>
#include <assert.h>
#include <stdexcept>

using namespace std;
using namespace creg;

#define CREG_PACKAGE_FILE_ID "CRPK"

// File format structures

#pragma pack(push,1)

struct PackageHeader
{
	char magic[4];
	int objDataOffset;
	int objTableOffset;
	int numObjects;
	int objClassRefOffset; // a class ref is: zero-term class string + checksum DWORD
	int numObjClassRefs; 

	void SwapBytes ()
	{
		objDataOffset = swabdword (objDataOffset);
		objTableOffset = swabdword (objTableOffset);
		objClassRefOffset = swabdword (objClassRefOffset);
		numObjClassRefs = swabdword (numObjClassRefs);
		numObjects = swabdword (numObjects);
	}
};

struct PackageObject
{
	unsigned short classRefIndex;
	char isEmbedded;

	void SwapBytes() {
		classRefIndex = swabword (classRefIndex);
	}
};

static int test_size[sizeof(PackageObject)==3 ? 1 : -1];

#pragma pack(pop)

static string ReadZStr(std::istream& f)
{
	std::string s;
	char c,i=0;
	while(!f.eof ()) {
		f >> c;
		if (!c) break;
		s += c;
	}
	return s;
}

static void WriteZStr(std::ostream& f, const string& s)
{
	int c;
	c = s.length ();
	f.write (s.c_str(),c+1);
}

//-------------------------------------------------------------------------
// Base output serializer
//-------------------------------------------------------------------------
COutputStreamSerializer::COutputStreamSerializer ()
{
	stream = 0;
}

bool COutputStreamSerializer::IsWriting ()
{
	return true;
}

void COutputStreamSerializer::SerializeObjectInstance (void *inst, creg::Class *objClass)
{
	// register the object, and mark it as embedded if a pointer was already referencing it
	ObjIDmap::iterator i = ptrToID.find (inst);
	ObjectID *obj = 0;
	if (i != ptrToID.end()) {
		obj = &i->second;
		assert (objClass == obj->class_);
	}
	else {
		obj = &ptrToID[inst];
		obj->id = objects.size ();
		obj->class_ = objClass;
		objects.push_back (obj);
	}
	obj->isEmbedded = true;

//	printf ("writepos of %s (%d): %d\n", objClass->name.c_str(), obj->id,(int)stream->tellp());

	// write an object ID
	int id = swabdword (obj->id);
	stream->write ((char*)&id, sizeof(int));

	// write the object
	objClass->SerializeInstance (this, inst);
}

void COutputStreamSerializer::SerializeObjectPtr (void **ptr, creg::Class *objClass)
{
//	printf ("writing ptr %s* at %d\n", objClass->name.c_str(), (int)stream->tellp());
	if (*ptr) {
		// valid pointer, write a one and the object ID
		int id;
		ObjIDmap::iterator i = ptrToID.find (*ptr);
		if (i == ptrToID.end()) {
			std::pair<ObjIDmap::iterator, bool> r = ptrToID.insert (std::pair<void*, ObjectID>(*ptr, ObjectID()));
			ObjectID& obj = r.first->second;
			obj.class_ = objClass;
			obj.isEmbedded = false;
			id = obj.id = objects.size();
			objects.push_back (&obj);
			pendingObjects.push_back (r.first);
		} else
			id = i->second.id;

		*stream << (char)1;
		id = swabdword(id);
		stream->write ((char*)&id, sizeof(int));
	} else {
		// null pointer, write a zero
		*stream << (char)0;
	}
}

void COutputStreamSerializer::Serialize (void *data, int byteSize)
{
	stream->write ((char*)data, byteSize);
}


struct COutputStreamSerializer::ClassRef
{
	int index;
	creg::Class* class_;
};


void COutputStreamSerializer::SavePackage (std::ostream *s, void *rootObj, Class *rootObjClass)
{
	PackageHeader ph;

	stream = s;

	stream->seekp (sizeof (PackageHeader));
	ph.objDataOffset = (int)stream->tellp();

	// Insert the first object that will provide references to everything
	ObjectID obj;
	obj.class_ = rootObjClass;
	obj.isEmbedded = false;
	obj.id = 0;
	ptrToID[rootObj] = obj;
	pendingObjects.push_back (ptrToID.find(rootObj));
	objects.push_back (&ptrToID[rootObj]);

	// Save until all the referenced objects have been stored
	while (!pendingObjects.empty ())
	{
		vector <ObjIDmap::iterator> po = pendingObjects;
		pendingObjects.clear ();

		for (vector<ObjIDmap::iterator>::iterator i=po.begin();i!=po.end();++i)
		{
			ObjectID& obj = (*i)->second;
			obj.class_->SerializeInstance (this, (*i)->first);
		}
	}

	// Collect a set of all used classes
	map<creg::Class *,ClassRef> classMap;
	vector <ClassRef*> classRefs;
	for (ObjIDmap::iterator oi = ptrToID.begin(); oi != ptrToID.end(); ++oi) {
//		printf ("Obj: %s\n", oi->second.class_->name.c_str());
		map<creg::Class*,ClassRef>::iterator cr = classMap.find (oi->second.class_);
		if (cr == classMap.end()) {
			ClassRef *pRef = &classMap[oi->second.class_];
			pRef->index = classRefs.size();
			pRef->class_ = oi->second.class_;

			classRefs.push_back (pRef);
			oi->second.classIndex = pRef->index;
		} else 
			oi->second.classIndex = cr->second.index;
	}

	// Write the class references
	ph.numObjClassRefs = classRefs.size();
	ph.objClassRefOffset = (int)stream->tellp();
	for (int a=0;a<classRefs.size();a++) {
		WriteZStr (*stream, classRefs[a]->class_->name);
		// write a checksum (unused atm)
		int checksum = swabdword(0);
		stream->write ((char*)&checksum, sizeof(int));
	}

	// Write object info
//	printf ("Writing %d objects (at %d)\n", objects.size(), (int)stream->tellp());
	ph.objTableOffset = (int)stream->tellp();
	ph.numObjects = objects.size();
	for (int a=0;a<objects.size();a++)
	{
		ObjectID *o = objects[a];
		PackageObject d;

		d.classRefIndex = o->classIndex;
		d.isEmbedded = o->isEmbedded ? 1 : 0;

		d.SwapBytes ();
		stream->write ((char*)&d, sizeof(PackageObject));
	}

	stream->seekp (0);
	memcpy(ph.magic, CREG_PACKAGE_FILE_ID, 4);
	ph.SwapBytes ();
	stream->write ((const char *)&ph, sizeof(PackageHeader));

	objects.clear();
	ptrToID.clear();
}

//-------------------------------------------------------------------------
// CInputStreamSerializer
//-------------------------------------------------------------------------

CInputStreamSerializer::CInputStreamSerializer()
{
	stream = 0;
}
CInputStreamSerializer::~CInputStreamSerializer()
{}

bool CInputStreamSerializer::IsWriting ()
{
	return false;
}

void CInputStreamSerializer::Serialize (void *data, int byteSize)
{
	stream->read ((char*)data, byteSize);
}

void CInputStreamSerializer::SerializeObjectPtr (void **ptr, creg::Class *cls)
{
	char v;
//	printf ("reading ptr %s* at %d\n", cls->name.c_str(), (int)stream->tellg());
	*stream >> v;
	if (v) {
		int id;
		stream->read ((char*)&id, sizeof(int));
		id = swabdword (id);

		StoredObject &o = objects [id];
		if (o.obj) *ptr = o.obj;
		else {
			// The object is not yet avaiable, so it needs fixing afterwards
//			printf ("Ref to embedded obj (%d)\n", id);
			UnfixedPtr ufp;
			ufp.objID = id;
			ufp.ptrAddr = ptr;
			unfixedPointers.push_back (ufp);
		}
	} else 
		*ptr = NULL;
}

// Serialize an instance of an object embedded into another object
void CInputStreamSerializer::SerializeObjectInstance (void *inst, creg::Class *cls)
{
	int id;
	stream->read((char*)&id, sizeof(int));

//	printf ("readpos of embedded %s (%d): %d\n", cls->name.c_str(), id, ((int)stream->tellg())-4);

	StoredObject& o = objects[swabdword(id)];
	assert (!o.obj);
	assert (o.isEmbedded);

	o.obj = inst;
	cls->SerializeInstance (this, inst);
}


void CInputStreamSerializer::LoadPackage (std::istream *s, void*& root, creg::Class *& rootCls)
{
	PackageHeader ph;

	stream = s;
	s->read((char *)&ph, sizeof(PackageHeader));

	if (memcmp (ph.magic, CREG_PACKAGE_FILE_ID, 4)) 
		throw std::runtime_error ("Incorrect object package file ID");

	// Load references
	classRefs.resize (ph.numObjClassRefs);
	s->seekg (ph.objClassRefOffset);
	for (int a=0;a<ph.numObjClassRefs;a++)
	{
		string className = ReadZStr (*s);
		int checksum;
		s->read ((char*)&checksum, sizeof(int));
		checksum = swabdword(checksum);
		creg::Class *class_ = ClassBinder::GetClass (className);

		if (!class_) 
			throw std::runtime_error ("Package file contains reference to unknown class " + className);

		classRefs[a] = class_;
	}

	// Create all non-embedded objects
	s->seekg (ph.objTableOffset);
	objects.resize (ph.numObjects);
	for (int a=0;a<ph.numObjects;a++)
	{
		PackageObject d;
		s->read ((char*)&d, sizeof(PackageObject));
		d.SwapBytes ();

		if (!d.isEmbedded) {
			// Allocate and construct
			ClassBinder *binder = classRefs[d.classRefIndex]->binder;
			void *inst = new char[binder->size];
			if (binder->constructor) binder->constructor (inst);
			objects [a].obj = inst;
		} else objects[a].obj = 0;
		objects [a].isEmbedded = d.isEmbedded;
		objects [a].classRef = d.classRefIndex;
	}

//	printf ("Loading %d objects (at %d)\n", objects.size(), (int)stream->tellg());

	// Read the object data using serialization
	s->seekg (ph.objDataOffset);
	for (int a=0;a<objects.size();a++)
	{
		if (!objects[a].isEmbedded) {
			creg::Class *cls = classRefs[objects[a].classRef];
			cls->SerializeInstance (this, objects[a].obj);
		}
	}

	for (int a=0;a<objects.size();a++) {
		StoredObject& o = objects[a];
		assert (o.obj);
	}

	// Fix pointers to embedded objects
	for (int a=0;a<unfixedPointers.size();a++) {
		void *p = objects [unfixedPointers[a].objID].obj;
		*unfixedPointers[a].ptrAddr = p;
	}
	
	// The first object is the root object
	root = objects[0].obj;
	rootCls = classRefs[objects[0].classRef];

	unfixedPointers.clear();
	objects.clear();
}


/* Testing..

class EmbeddedObj {
	CR_DECLARE(EmbeddedObj);
	int value;
};
CR_BIND(EmbeddedObj);
CR_BIND_MEMBERS(EmbeddedObj, CR_MEMBER(value));

struct TestObj {
	CR_DECLARE(TestObj);

	TestObj() {
		intvar=0;
		for(int a=0;a<5;a++) sarray[a]=0;
		childs[0]=childs[1]=0;
		embeddedPtr=0;
	}
	~TestObj() {
		if (childs[0]) delete childs[0];
		if (childs[1]) delete childs[1];
	}

	int intvar;
	string str;
	int sarray[5];
	EmbeddedObj *embeddedPtr;
	vector<int> darray;
	TestObj *childs[2];
	EmbeddedObj embedded;
};

CR_BIND(TestObj);

CR_BIND_MEMBERS(TestObj, (
	CR_MEMBER(childs),
	CR_MEMBER(intvar),
	CR_MEMBER(str),
	CR_MEMBER(sarray),
	CR_MEMBER(embedded),
	CR_MEMBER(embeddedPtr),
	CR_MEMBER(darray)
));

static void savetest()
{
	TestObj *o = new TestObj;
	o->darray.push_back(3);
	o->intvar = 1;
	o->str = "Hi!";
	for (int a=0;a<5;a++) o->sarray[a]=a+10;

	TestObj *c = new TestObj;
	c->intvar = 144;
	o->childs [0] = c;
	o->childs [1] = c;
	c->embeddedPtr = &o->embedded;
	o->embeddedPtr = &c->embedded;
	creg::COutputStreamSerializer ss;
	std::ofstream f ("test.p", ios::out | ios::binary);
	ss.SavePackage (&f, o, o->GetClass());
}

static void loadtest()
{
	creg::CInputStreamSerializer ss;
	std::ifstream f ("test.p", ios::in | ios::binary);

	void *root;
	creg::Class *rootCls;
	ss.LoadPackage (&f, root, rootCls);

	assert (rootCls == TestObj::StaticClass());
	TestObj *obj = (TestObj*)root;

	assert (obj->childs[0]->embeddedPtr == &obj->embedded);
	assert (obj->embeddedPtr == &obj->childs[0]->embedded);
	assert (obj->childs[0]==obj->childs[1]);
	assert (obj->childs[0]->intvar == 144);
	assert (obj->darray.size()==1 && obj->darray[0]==3);
	assert (obj->intvar == 1);
	assert (obj->str == "Hi!");
	for (int a=0;a<5;a++) printf("%d\n",obj->sarray[a]);
}

void test_creg_serializer()
{
	savetest();
	loadtest();

	remove("test.p");
}
*/

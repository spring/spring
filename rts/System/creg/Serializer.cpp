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
#include <map>
#include <string.h>
//#include "LogOutput.h"

using namespace std;
using namespace creg;

#ifndef swabdword
	#define swabdword(d) (d)
#endif
#ifndef swabword
	#define swabword(d) (d)
#endif

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
	unsigned int metadataChecksum;

	void SwapBytes ()
	{
		objDataOffset = swabdword (objDataOffset);
		objTableOffset = swabdword (objTableOffset);
		objClassRefOffset = swabdword (objClassRefOffset);
		numObjClassRefs = swabdword (numObjClassRefs);
		numObjects = swabdword (numObjects);
		metadataChecksum = swabdword (metadataChecksum);
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

COMPILE_TIME_ASSERT(sizeof(PackageObject) == 3, test_size);

#pragma pack(pop)

static string ReadZStr(std::istream& f)
{
	std::string s;
	char c;
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

COutputStreamSerializer::ObjectRef* COutputStreamSerializer::FindObjectRef(void *inst, creg::Class *objClass, bool isEmbedded)
{
	std::vector<ObjectRef*> *refs = &(ptrToId[inst]);
	for (std::vector<ObjectRef*>::iterator i=refs->begin();i!=refs->end();i++) {
		if ((*i)->isThisObject(inst,objClass,isEmbedded))
			return *i;
	}
	return 0;
}

void COutputStreamSerializer::SerializeObjectInstance (void *inst, creg::Class *objClass)
{
	// register the object, and mark it as embedded if a pointer was already referencing it
	ObjectRef *obj = FindObjectRef(inst,objClass,true);
	if (!obj) {
		obj = &*objects.insert(objects.end(),ObjectRef(inst,objects.size (),true,objClass));
		ptrToId[inst].push_back(obj);
	} else if (obj->isEmbedded) 
		throw content_error("Reserialization of embedded object");
	else {
		std::vector<ObjectRef*>::iterator pos;
		for (pos=pendingObjects.begin();pos!=pendingObjects.end() && (*pos)!=obj;pos++) ;
		if (pos==pendingObjects.end())
			throw content_error("Object pointer was serialized");
		else {
			pendingObjects.erase(pos);
		}
	}
	obj->class_ = objClass;
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
	if (*ptr) {
		// valid pointer, write a one and the object ID
		int id;
		ObjectRef *obj = FindObjectRef(*ptr,objClass,false);
		if (!obj) {
			obj = &*objects.insert(objects.end(),ObjectRef(*ptr,objects.size (),false,objClass));
			ptrToId[*ptr].push_back(obj);
			id = obj->id;
			pendingObjects.push_back (obj);
		} else
			id = obj->id;

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
	unsigned startOffset = stream->tellp();

	stream->seekp (startOffset + sizeof (PackageHeader));
	ph.objDataOffset = (int)stream->tellp();

	// Insert the first object that will provide references to everything
	ObjectRef *obj = &*objects.insert(objects.end(),ObjectRef(rootObj,objects.size(),false,rootObjClass));
	ptrToId[rootObj].push_back(obj);
	pendingObjects.push_back (obj);

	map<creg::Class *,int> classSizes;
	// Save until all the referenced objects have been stored
	while (!pendingObjects.empty ())
	{
		std::vector <ObjectRef*> po = pendingObjects;
		pendingObjects.clear ();

		for (std::vector <ObjectRef*> ::iterator i=po.begin();i!=po.end();++i)
		{
			ObjectRef* obj = *i;
			unsigned objstart = stream->tellp();
			obj->class_->SerializeInstance (this, obj->ptr);
			unsigned objend = stream->tellp();
			int sz = objend-objstart;
			classSizes[obj->class_]+=sz;
		}
	}

	// Collect a set of all used classes
	map<creg::Class *,ClassRef> classMap;
	vector <ClassRef*> classRefs;
	map<int,int> classObjects;
	for (std::list <ObjectRef>::iterator i=objects.begin();i!=objects.end();i++) {
		//printf ("Obj: %s\n", oi->second.class_->name.c_str());
		map<creg::Class*,ClassRef>::iterator cr = classMap.find (i->class_);
		if (cr == classMap.end()) {
			ClassRef *pRef = &classMap[i->class_];
			pRef->index = classRefs.size();
			pRef->class_ = i->class_;

			classRefs.push_back (pRef);
			i->classIndex = pRef->index;
		} else
			i->classIndex = cr->second.index;
		classObjects[i->classIndex]++;
	}

//	for (map<int,int>::iterator i=classObjects.begin();i!=classObjects.end();i++) {
//		logOutput.Print("%20s %10u %10u",classRefs[i->first]->class_->name.c_str(),i->second,classSizes[classRefs[i->first]->class_]);
//	}

	// Write the class references
	ph.numObjClassRefs = classRefs.size();
	ph.objClassRefOffset = (int)stream->tellp();
	for (uint a=0;a<classRefs.size();a++) {
		WriteZStr (*stream, classRefs[a]->class_->name);
		// write a checksum (unused atm)
		int checksum = swabdword(0);
		stream->write ((char*)&checksum, sizeof(int));
	}

	// Write object info
	ph.objTableOffset = (int)stream->tellp();
	ph.numObjects = objects.size();
	for (std::list <ObjectRef>::iterator i=objects.begin();i!=objects.end();i++) {
		PackageObject d;

		d.classRefIndex = i->classIndex;
		d.isEmbedded = i->isEmbedded ? 1 : 0;

		d.SwapBytes ();
		stream->write ((char*)&d, sizeof(PackageObject));
	}

	// Calculate a checksum for metadata verification
	ph.metadataChecksum = 0;
	for (uint a=0;a<classRefs.size();a++)
	{
		Class *c = classRefs[a]->class_;
		c->CalculateChecksum (ph.metadataChecksum);
	}
//	printf("Checksum: %d\n", ph.metadataChecksum);

	int endOffset = stream->tellp();
	stream->seekp (startOffset);
	memcpy(ph.magic, CREG_PACKAGE_FILE_ID, 4);
	ph.SwapBytes ();
	stream->write ((const char *)&ph, sizeof(PackageHeader));

//	logOutput.Print("Number of objects saved: %d\nNumber of classes involved: %d\n", objects.size(), classRefs.size());

	stream->seekp (endOffset);
	ptrToId.clear();
	pendingObjects.clear();
	objects.clear();
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

void CInputStreamSerializer::AddPostLoadCallback(void (*cb)(void*), void *ud)
{
	PostLoadCallback plcb;

	plcb.cb = cb;
	plcb.userdata = ud;

	callbacks.push_back (plcb);
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
		checksum = swabdword(checksum); // ignored for now
		creg::Class *class_ = System::GetClass (className);

		if (!class_)
			throw std::runtime_error ("Package file contains reference to unknown class " + className);

		classRefs[a] = class_;
	}

	// Calculate metadata checksum and compare with stored checksum
	unsigned int checksum = 0;
	for (uint a=0;a<classRefs.size();a++)
		classRefs[a]->CalculateChecksum (checksum);
	if (checksum != ph.metadataChecksum)
		throw std::runtime_error ("Metadata checksum error: Package file was saved with a different version");

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
			void *inst = binder->class_->CreateInstance ();
			objects [a].obj = inst;
		} else objects[a].obj = 0;
		objects [a].isEmbedded = !!d.isEmbedded;
		objects [a].classRef = d.classRefIndex;
	}

	int endOffset = s->tellg();

//	printf ("Loading %d objects (at %d)\n", objects.size(), (int)stream->tellg());

	// Read the object data using serialization
	s->seekg (ph.objDataOffset);
	for (uint a=0;a<objects.size();a++)
	{
		if (!objects[a].isEmbedded) {
			creg::Class *cls = classRefs[objects[a].classRef];
			cls->SerializeInstance (this, objects[a].obj);
		}
	}

	// Fix pointers to embedded objects
	for (uint a=0;a<unfixedPointers.size();a++) {
		void *p = objects [unfixedPointers[a].objID].obj;
		*unfixedPointers[a].ptrAddr = p;
	}

	// Run all registered post load callbacks
	for (uint a=0;a<callbacks.size();a++) {
		callbacks[a].cb (callbacks[a].userdata);
	}

	// Run post load functions on all objects
	for (uint a=0;a<objects.size();a++) {
		StoredObject& o = objects[a];
		Class *c = classRefs[objects[a].classRef];
		std::vector<Class*> hierarchy;
		for (Class *c2=c;c2;c2=c2->base)
			hierarchy.insert(hierarchy.end(),c2);
		for (std::vector<Class*>::reverse_iterator i=hierarchy.rbegin();i!=hierarchy.rend();i++) {
			if ((*i)->postLoadProc) {
				_DummyStruct *ds = (_DummyStruct*)o.obj;
				(ds->*(*i)->postLoadProc)();
			}
		}
/*		
		if (c->postLoadProc) {
			_DummyStruct *ds = (_DummyStruct*)o.obj;
			(ds->*c->postLoadProc)();
		}
*/
	}

	// The first object is the root object
	root = objects[0].obj;
	rootCls = classRefs[objects[0].classRef];

	s->seekg (endOffset);
	unfixedPointers.clear();
	objects.clear();
}

ISerializer::~ISerializer() {
}

/* Testing..

class EmbeddedObj {
	CR_DECLARE(EmbeddedObj);
	int value;
};
CR_BIND(EmbeddedObj);
CR_REG_METADATA(EmbeddedObj, CR_MEMBER(value));

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

CR_REG_METADATA(TestObj, (
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
	TestObj *o = SAFE_NEW TestObj;
	o->darray.push_back(3);
	o->intvar = 1;
	o->str = "Hi!";
	for (int a=0;a<5;a++) o->sarray[a]=a+10;

	TestObj *c = SAFE_NEW TestObj;
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

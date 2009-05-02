/*
creg - Code compoment registration system
Copyright 2005 Jelmer Cnossen

Classes for serialization of registrated class instances
*/
#include "StdAfx.h"
#include "creg_cond.h"
#include "Serializer.h"
#include "Platform/byteorder.h"
#include <fstream>
#include <assert.h>
#include <stdexcept>
#include <map>
#include <string.h>
//#include "LogOutput.h"

#include "Exceptions.h"

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

static int MakeStrHash(const char *str)
{
	int result = 0xDA38E7AB;
	for (const char *pos=str;*pos;pos++) {
		int rotcnt = *pos%31;
		result = (result<<rotcnt) | (result>>(32-rotcnt));
		result*=(0xE66ED34E + *pos);
		result^=0x3D7E5ED5;
		result+=0x1BF942A - *pos;
	}
	return result;
}

void WriteVarSizeUInt(std::ostream *stream, unsigned int val)
{
	if (val<0x80) {
		unsigned char a = val;
		stream->write ((char*)&a, sizeof(char));
	} else if (val<0x4000) {
		unsigned char a = (val & 0x7F) | 0x80;
		unsigned char b = val>>7;
		stream->write ((char*)&a, sizeof(char));
		stream->write ((char*)&b, sizeof(char));
	} else if (val<0x40000000) {
		unsigned char a = (val & 0x7F) | 0x80;
		unsigned char b = ((val>>7) & 0x7F) | 0x80;
		unsigned short c = swabword(val>>14);
		stream->write ((char*)&a, sizeof(char));
		stream->write ((char*)&b, sizeof(char));
		stream->write ((char*)&c, sizeof(short));
	} else throw "Cannot save varible-size int";
}

void ReadVarSizeUInt(std::istream *stream, unsigned int *buf)
{
	unsigned char a;
	stream->read ((char*)&a, sizeof(char));
	if (a&0x80) {
		unsigned char b;
		stream->read ((char*)&b, sizeof(char));
		if (b&0x80) {
			unsigned short c;
			stream->read ((char*)&c, sizeof(short));
			*buf = (a&0x7F) | ((b&0x7F)<<7) | (c<<14);
		} else {
			*buf = (a&0x7F) | ((b&0x7F)<<7);
		}
	} else {
		*buf = a&0x7F;
	}
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

void COutputStreamSerializer::SerializeObject (Class *c, void *ptr, ObjectRef *objr)
{
	if (c->base)
		SerializeObject(c->base, ptr, objr);

	ObjectMemberGroup omg;
	omg.membersClass = c;

	for (uint a=0;a<c->members.size();a++)
	{
		creg::Class::Member* m = c->members [a];
		if (m->flags & CM_NoSerialize)
			continue;

		ObjectMember om;
		om.member = m;
		om.memberId = a;
		void *memberAddr = ((char*)ptr) + m->offset;
		unsigned mstart = stream->tellp();
		m->type->Serialize (this, memberAddr);
		unsigned mend = stream->tellp();
		om.size = mend-mstart;
		omg.members.push_back(om);
		omg.size+=om.size;
	}


	if (c->serializeProc) {
		ObjectMember om;
		om.member = NULL;
		om.memberId = -1;
		unsigned mstart = stream->tellp();
		_DummyStruct *obj = (_DummyStruct*)ptr;
		(obj->*(c->serializeProc))(*this);
		unsigned mend = stream->tellp();
		om.size = mend-mstart;
		omg.members.push_back(om);
		omg.size+=om.size;
	}

	objr->memberGroups.push_back(omg);
}

void COutputStreamSerializer::SerializeObjectInstance (void *inst, creg::Class *objClass)
{
	// register the object, and mark it as embedded if a pointer was already referencing it
	ObjectRef *obj = FindObjectRef(inst,objClass,true);
	if (!obj) {
		obj = &*objects.insert(objects.end(),ObjectRef(inst,objects.size (),true,objClass));
		ptrToId[inst].push_back(obj);
	} else if (obj->isEmbedded) 
		throw "Reserialization of embedded object";
	else {
		std::vector<ObjectRef*>::iterator pos;
		for (pos=pendingObjects.begin();pos!=pendingObjects.end() && (*pos)!=obj;pos++) ;
		if (pos==pendingObjects.end())
			throw "Object pointer was serialized";
		else {
			pendingObjects.erase(pos);
		}
	}
	obj->class_ = objClass;
	obj->isEmbedded = true;

//	printf ("writepos of %s (%d): %d\n", objClass->name.c_str(), obj->id,(int)stream->tellp());

	// write an object ID
	WriteVarSizeUInt(stream,obj->id);

	// write the object
	SerializeObject(objClass, inst, obj);
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

//		*stream << (char)1;
		WriteVarSizeUInt(stream,id);
	} else {
		// null pointer, write a zero
		WriteVarSizeUInt(stream,0);
//		*stream << (char)0;
	}
}

void COutputStreamSerializer::Serialize (void *data, int byteSize)
{
	stream->write ((char*)data, byteSize);
}

void COutputStreamSerializer::SerializeInt (void *data, int byteSize)
{
	char buf[4];
	switch (byteSize) {
		case 1:{
			*(char *)buf = *(char *) data;
			break;
		}
		case 2:{
			*(short *)buf = swabword(*(short *) data);
			break;
		}
		case 4:{
			*(long *)buf = swabdword(*(long *) data);
			break;
		}
		default: throw "Unknown int type";
	}
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

	// Insert dummy object with id 0
	ObjectRef *obj = &*objects.insert(objects.end(),ObjectRef(0,0,true,0));
	obj->classIndex = 0;

	// Insert the first object that will provide references to everything
	obj = &*objects.insert(objects.end(),ObjectRef(rootObj,objects.size(),false,rootObjClass));
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
			SerializeObject(obj->class_, obj->ptr, obj);
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
		if (i->ptr==NULL) continue; //Object with id 0 - dummy
		//printf ("Obj: %s\n", oi->second.class_->name.c_str());
		map<creg::Class*,ClassRef>::iterator cr = classMap.find (i->class_);
		if (cr == classMap.end()) {
			ClassRef *pRef = &classMap[i->class_];
			pRef->index = classRefs.size();
			pRef->class_ = i->class_;

			classRefs.push_back (pRef);
			i->classIndex = pRef->index;

			for (creg::Class *c = i->class_->base;c;c = c->base) {
				map<creg::Class*,ClassRef>::iterator cr = classMap.find (c);
				if (cr == classMap.end()) {
					ClassRef *pRef = &classMap[c];
					pRef->index = classRefs.size();
					pRef->class_ = c;

					classRefs.push_back (pRef);
				}
			}
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
//		int checksum = swabdword(0);
//		stream->write ((char*)&checksum, sizeof(int));
		int cnt = classRefs[a]->class_->members.size();
		WriteVarSizeUInt(stream,cnt);
		for (int b=0;b<cnt;b++) {
			creg::Class::Member* m = classRefs[a]->class_->members [b];
			int namehash = swabdword(MakeStrHash(m->name));
			std::string typeName = m->type->GetName();
			int typehash1 = MakeStrHash(typeName.c_str());
			char typehash2 = ((typehash1>>0)&0xFF) ^ ((typehash1>>8)&0xFF) ^ ((typehash1>>16)&0xFF) ^ ((typehash1>>24)&0xFF);
			stream->write ((char*)&namehash, sizeof(int));
			stream->write ((char*)&typehash2, sizeof(char));
		}
	}

	// Write object info
	ph.objTableOffset = (int)stream->tellp();
	ph.numObjects = objects.size();
	for (std::list <ObjectRef>::iterator i=objects.begin();i!=objects.end();i++) {
		int classRefIndex = i->classIndex;
		char isEmbedded = i->isEmbedded ? 1 : 0;
		WriteVarSizeUInt(stream,classRefIndex);
		stream->write ((char*)&isEmbedded, sizeof(char));
		char mgcnt = i->memberGroups.size();
		WriteVarSizeUInt(stream,mgcnt);
		for (std::vector<COutputStreamSerializer::ObjectMemberGroup>::iterator j=i->memberGroups.begin();j!=i->memberGroups.end();j++) {
			map<creg::Class*,ClassRef>::iterator cr = classMap.find(j->membersClass);
			if (cr == classMap.end()) throw "Cannot find member class ref";
			int cid = cr->second.index;
			WriteVarSizeUInt(stream,cid);
			unsigned int mcnt = j->members.size();
			WriteVarSizeUInt(stream,mcnt);
			bool hasSerializerMember = false;
			char groupFlags = 0;
			if (!j->members.empty() && j->members.back().memberId==-1) {
				groupFlags|=0x01;
				hasSerializerMember = true;
			}
			stream->write ((char*)&groupFlags, sizeof(char));
			int midx=0;
			for (std::vector<COutputStreamSerializer::ObjectMember>::iterator k=j->members.begin();k!=j->members.end();k++,midx++) {
				if (k->memberId!=midx && (!hasSerializerMember || k!=j->members.end()-1))
					throw "Invalid member id";
				WriteVarSizeUInt(stream,k->size);
			}
		}
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

void CInputStreamSerializer::SerializeObject (Class *c, void *ptr)
{
	if (c->base)
		SerializeObject(c->base, ptr);

	for (uint a=0;a<c->members.size();a++)
	{
		creg::Class::Member* m = c->members [a];
		if (m->flags & CM_NoSerialize)
			continue;

		void *memberAddr = ((char*)ptr) + m->offset;
		m->type->Serialize (this, memberAddr);
	}

	if (c->serializeProc) {
		_DummyStruct *obj = (_DummyStruct*)ptr;
		(obj->*(c->serializeProc))(*this);
	}
}

void CInputStreamSerializer::Serialize (void *data, int byteSize)
{
	stream->read ((char*)data, byteSize);
}

void CInputStreamSerializer::SerializeInt (void *data, int byteSize)
{
	stream->read ((char*)data, byteSize);
	switch (byteSize) {
		case 2:{
			*(short *) data = swabword(*(short *) data);
			break;
		}
		case 4:{
			*(long *) data = swabdword(*(long *) data);
			break;
		}
		default: throw "Unknown int type";
	}
}

void CInputStreamSerializer::SerializeObjectPtr (void **ptr, creg::Class *cls)
{
//	char v;
//	printf ("reading ptr %s* at %d\n", cls->name.c_str(), (int)stream->tellg());
//	*stream >> v;
	unsigned int id;
	ReadVarSizeUInt(stream, &id);
	if (id) {
		StoredObject &o = objects [id];
		if (o.obj) *ptr = o.obj;
		else {
			// The object is not yet avaiable, so it needs fixing afterwards
			*ptr = (void*) 1;
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
	unsigned int id;
	ReadVarSizeUInt(stream, &id);

	if (id==0)
		return; // this is old save game and it has not this object - skip it

//	printf ("readpos of embedded %s (%d): %d\n", cls->name.c_str(), id, ((int)stream->tellg())-4);
	StoredObject& o = objects[id];
	assert (!o.obj);
	assert (o.isEmbedded);

	o.obj = inst;
	SerializeObject(cls, inst);
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
		creg::Class *class_ = System::GetClass (className);
		if (!class_)
			throw std::runtime_error ("Package file contains reference to unknown class " + className);
		unsigned int cnt;
		ReadVarSizeUInt(stream, &cnt);
		for (unsigned int b=0;b<cnt;b++) {
			int namehash;
			stream->read ((char*)&namehash, sizeof(int));
			char typehash;
			stream->read ((char*)&typehash, sizeof(char));
		}

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
		unsigned int classRefIndex;
		char isEmbedded;
		ReadVarSizeUInt(stream,&classRefIndex);
		stream->read ((char*)&isEmbedded, sizeof(char));
		unsigned int mgcnt;
		ReadVarSizeUInt(stream,&mgcnt);
		for (unsigned int b=0;b<mgcnt;b++) {
			unsigned int cid,mcnt;
			char groupFlags;
			ReadVarSizeUInt(stream,&cid);
			ReadVarSizeUInt(stream,&mcnt);
			stream->read ((char*)&groupFlags, sizeof(char));
			for (unsigned int c=0;c<mcnt;c++) {
				unsigned int size;
				ReadVarSizeUInt(stream,&size);
			}
		}

		if (!isEmbedded) {
			// Allocate and construct
			ClassBinder *binder = classRefs[classRefIndex]->binder;
			void *inst = binder->class_->CreateInstance ();
			objects [a].obj = inst;
		} else objects[a].obj = 0;
		objects [a].isEmbedded = !!isEmbedded;
		objects [a].classRef = classRefIndex;
	}

	int endOffset = s->tellg();

//	printf ("Loading %d objects (at %d)\n", objects.size(), (int)stream->tellg());

	// Read the object data using serialization
	s->seekg (ph.objDataOffset);
	for (uint a=0;a<objects.size();a++)
	{
		if (!objects[a].isEmbedded) {
			creg::Class *cls = classRefs[objects[a].classRef];
			SerializeObject(cls, objects[a].obj);
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
	for (uint a=1;a<objects.size();a++) {
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
	root = objects[1].obj;
	rootCls = classRefs[objects[1].classRef];

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

/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * creg - Code compoment registration system
 * Classes for serialization of registrated class instances
 */

#define LOG_SECTION_CREG_SERIALIZER "CregSerializer"

#include "creg_cond.h"
#include "Serializer.h"

#include "System/Log/ILog.h"
#include "System/Platform/byteorder.h"
#include "System/Exceptions.h"

#include <fstream>
#include <assert.h>
#include <stdexcept>
#include <map>
#include <vector>
#include <string>
#include <string.h>
#include <boost/cstdint.hpp>

using namespace creg;
using std::string;
using std::map;
using std::vector;

LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_CREG_SERIALIZER)

//
#define CREG_PACKAGE_FILE_ID "CRPK"

// File format structures
struct PackageHeader
{
	char magic[4];
	int objDataOffset;
	int objTableOffset;
	int numObjects;
	int objClassRefOffset; // a class ref is: zero-term class string + checksum DWORD
	int numObjClassRefs;
	unsigned int metadataChecksum;

	void SwapBytes()
	{
		swabDWordInPlace(objDataOffset);
		swabDWordInPlace(objTableOffset);
		swabDWordInPlace(objClassRefOffset);
		swabDWordInPlace(numObjClassRefs);
		swabDWordInPlace(numObjects);
		swabDWordInPlace(metadataChecksum);
	}
	PackageHeader():
		objDataOffset(0),
		objTableOffset(0),
		numObjects(0),
		objClassRefOffset(0),
		numObjClassRefs(0),
		metadataChecksum(0)
	{
		magic[0] = 0;
		magic[1] = 0;
		magic[2] = 0;
		magic[3] = 0;
	}
};



static std::string ReadZStr(std::istream& file)
{
	char cstr[1024];
	file.getline(cstr, sizeof(cstr), 0);
	return std::string(cstr);
}

static void WriteZStr(std::ostream& file, const std::string& str)
{
	assert(str.length() < 1024); // check func above!
	file.write(str.c_str(), str.length() + 1);
}

void WriteVarSizeUInt(std::ostream* stream, unsigned int val)
{
	if (val < 0x80) {
		unsigned char a = val;
		stream->write((char*)&a, sizeof(char));
	} else if (val < 0x4000) {
		unsigned char a = (val & 0x7F) | 0x80;
		unsigned char b = val >> 7;
		stream->write((char*)&a, sizeof(char));
		stream->write((char*)&b, sizeof(char));
	} else if (val < 0x40000000) {
		unsigned char a = (val & 0x7F) | 0x80;
		unsigned char b = ((val >> 7) & 0x7F) | 0x80;
		unsigned short c = swabWord(val >> 14);
		stream->write((char*)&a, sizeof(char));
		stream->write((char*)&b, sizeof(char));
		stream->write((char*)&c, sizeof(short));
	} else throw "Cannot save varible-size int";
}

void ReadVarSizeUInt(std::istream* stream, unsigned int* buf)
{
	unsigned char a;
	stream->read((char*)&a, sizeof(char));
	if (a & 0x80) {
		unsigned char b;
		stream->read((char*)&b, sizeof(char));
		if (b & 0x80) {
			unsigned short c;
			stream->read((char*)&c, sizeof(short));
			*buf = (a & 0x7F) | ((b & 0x7F) << 7) | (c << 14);
		} else {
			*buf = (a & 0x7F) | ((b & 0x7F) << 7);
		}
	} else {
		*buf = a & 0x7F;
	}
}

//-------------------------------------------------------------------------
// Base output serializer
//-------------------------------------------------------------------------
COutputStreamSerializer::COutputStreamSerializer()
{
	stream = nullptr;
}

bool COutputStreamSerializer::IsWriting()
{
	return true;
}

COutputStreamSerializer::ObjectRef* COutputStreamSerializer::FindObjectRef(void* inst, creg::Class* objClass, bool isEmbedded)
{
	std::vector<ObjectRef*>* refs = &(ptrToId[inst]);
	for (std::vector<ObjectRef*>::iterator i = refs->begin(); i != refs->end(); ++i) {
		if ((*i)->isThisObject(inst, objClass, isEmbedded))
			return *i;
	}
	return nullptr;
}

void COutputStreamSerializer::SerializeObject(Class* c, void* ptr, ObjectRef* objr)
{
	const unsigned objstart = stream->tellp();

	if (c->base())
		SerializeObject(c->base(), ptr, objr);

	ObjectMemberGroup omg;
	omg.membersClass = c;

	for (uint a = 0; a < c->members.size(); a++)
	{
		creg::Class::Member* m = &c->members[a];
		if (m->flags & CM_NoSerialize)
			continue;

		ObjectMember om;
		om.member = m;
		om.memberId = a;
		void* memberAddr = ((char*)ptr) + m->offset;
		unsigned mstart = stream->tellp();
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Serialized %s::%s type:%s", c->name.c_str(), m->name, m->type->GetName().c_str());
		m->type->Serialize(this, memberAddr);
		unsigned mend = stream->tellp();
		om.size = mend - mstart;
		omg.members.push_back(om);
		omg.size += om.size;
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Serialized %s::%s type:%s size:%d", c->name.c_str(), m->name, m->type->GetName().c_str(), om.size);
	}


	if (c->HasSerialize()) {
		ObjectMember om;
		om.member = nullptr;
		om.memberId = -1;
		unsigned mstart = stream->tellp();
		c->CallSerializeProc(ptr, this);
		unsigned mend = stream->tellp();
		om.size = mend - mstart;
		omg.members.push_back(om);
		omg.size += om.size;
	}

	objr->memberGroups.push_back(omg);

	const unsigned objend = stream->tellp();
	const int sz = objend - objstart;
	classSizes[c] += sz;
	classCounts[c]++;
}

void COutputStreamSerializer::SerializeObjectInstance(void* inst, creg::Class* objClass)
{
	// register the object, and mark it as embedded if a pointer was already referencing it
	ObjectRef* obj = FindObjectRef(inst, objClass, true);
	if (!obj) {
		objects.emplace_back(inst, objects.size(), true, objClass);
		obj = &objects.back();
		ptrToId[inst].push_back(obj);
	} else if (obj->isEmbedded) {
		throw "Reserialization of embedded object (" + objClass->name + ")";
	} else {
		std::vector<ObjectRef*>::iterator it = std::find(pendingObjects.begin(), pendingObjects.end(), obj);
		if (it == pendingObjects.end()) {
			throw "Object pointer was serialized (" + objClass->name + ")";
		} else {
			pendingObjects.erase(it);
		}
	}
	obj->class_ = objClass;
	obj->isEmbedded = true;

	// write an object ID
	WriteVarSizeUInt(stream, obj->id);

	// write the object
	SerializeObject(objClass, inst, obj);
}

void COutputStreamSerializer::SerializeObjectPtr(void** ptr, creg::Class* objClass)
{
	if (*ptr) {
		// valid pointer, write a one and the object ID
		int id;
		ObjectRef* obj = FindObjectRef(*ptr, objClass, false);
		if (!obj) {
			objects.push_back(ObjectRef(*ptr, objects.size(), false, objClass));
			obj = &objects.back();
			ptrToId[*ptr].push_back(obj);
			pendingObjects.push_back(obj);
		}
		id = obj->id;

		WriteVarSizeUInt(stream, id);
	} else {
		// null pointer, write a zero
		WriteVarSizeUInt(stream, 0);
	}
}

void COutputStreamSerializer::Serialize(void* data, int byteSize)
{
	stream->write((char*)data, byteSize);
}

void COutputStreamSerializer::SerializeInt(void* data, int byteSize)
{
	// always save ints as 64bit
	// cause of int-types might differ in size depending on platforms
	// to make savegames compatible between those we need to so
	boost::int64_t x = 0;
	switch (byteSize) {
		case 1: { x = *(boost::int8_t* )data; break; }
		case 2: { x = *(boost::int16_t*)data; break; }
		case 4: { x = *(boost::int32_t*)data; break; }
		case 8: { x = *(boost::int64_t*)data; break; }
		default: {
			throw "Unknown int type";
		}
	}
	stream->write((char*)&x, 8);
}


struct COutputStreamSerializer::ClassRef
{
	int index;
	creg::Class* class_;
};

void COutputStreamSerializer::SavePackage(std::ostream* s, void* rootObj, Class* rootObjClass)
{
	PackageHeader ph;

	stream = s;
	unsigned startOffset = stream->tellp();
	stream->write((char*)&ph, sizeof(PackageHeader));
	stream->seekp(startOffset + sizeof(PackageHeader));
	ph.objDataOffset = (int)stream->tellp();

	// Insert dummy object with id 0
	objects.push_back(ObjectRef(0, 0, true, 0));
	ObjectRef* obj = &objects.back();
	obj->classIndex = 0;

	// Insert the first object that will provide references to everything
	objects.push_back(ObjectRef(rootObj, objects.size(), false, rootObjClass));
	obj = &objects.back();
	ptrToId[rootObj].push_back(obj);
	pendingObjects.push_back(obj);

	// Save until all the referenced objects have been stored
	while (!pendingObjects.empty())
	{
		const std::vector<ObjectRef*> po = pendingObjects;
		pendingObjects.clear();

		for (std::vector<ObjectRef*>::const_iterator i = po.begin(); i != po.end(); ++i)
		{
			ObjectRef* obj = *i;
			SerializeObject(obj->class_, obj->ptr, obj);
			//LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Serialized %s size:%i", obj->class_->name.c_str(), sz);
		}
	}

	// Collect a set of all used classes
	std::map<creg::Class*, ClassRef> classMap;
	std::vector<ClassRef*> classRefs;
	for (ObjectRef& oRef: objects) {
		if (oRef.ptr == nullptr)
			continue;

		creg::Class* c = oRef.class_;
		while (c) {
			std::map<creg::Class*, ClassRef>::iterator cr = classMap.find(c);
			if (cr == classMap.end()) {
				ClassRef* pRef = &classMap[c];
				pRef->index = classRefs.size();
				pRef->class_ = c;
				classRefs.push_back(pRef);
			}
			c = c->base();
		}

		std::map<creg::Class*, ClassRef>::iterator cr = classMap.find(oRef.class_);
		oRef.classIndex = cr->second.index;
	}


	if (LOG_IS_ENABLED(L_DEBUG)) {
		for (auto &it: classSizes) {
			LOG_L(L_DEBUG, "%30s %10u %10u",
					it.first->name.c_str(),
					classCounts[it.first],
					it.second);
		}
	}


	// Write the class references & calc their checksum
	ph.numObjClassRefs = classRefs.size();
	ph.objClassRefOffset = (int)stream->tellp();
	for (uint a = 0; a < classRefs.size(); a++) {
		creg::Class* c =  classRefs[a]->class_;
		WriteZStr(*stream, c->name);
	};

	// Write object info
	ph.objTableOffset = (int)stream->tellp();
	ph.numObjects = objects.size();
	for (ObjectRef& oRef: objects) {
		int classRefIndex = oRef.classIndex;
		char isEmbedded = oRef.isEmbedded ? 1 : 0;
		WriteVarSizeUInt(stream, classRefIndex);
		stream->write((char*)&isEmbedded, sizeof(char));

		char mgcnt = oRef.memberGroups.size();
		WriteVarSizeUInt(stream, mgcnt);

		std::vector<COutputStreamSerializer::ObjectMemberGroup>::iterator j;
		for (j = oRef.memberGroups.begin(); j != oRef.memberGroups.end(); ++j) {
			std::map<creg::Class*, ClassRef>::iterator cr = classMap.find(j->membersClass);
			if (cr == classMap.end()) throw "Cannot find member class ref";
			int cid = cr->second.index;
			WriteVarSizeUInt(stream, cid);

			unsigned int mcnt = j->members.size();
			WriteVarSizeUInt(stream, mcnt);

			bool hasSerializerMember = false;
			char groupFlags = 0;
			if (!j->members.empty() && (j->members.back().memberId == -1)) {
				groupFlags |= 0x01;
				hasSerializerMember = true;
			}
			stream->write((char*)&groupFlags, sizeof(char));

			int midx = 0;
			std::vector<COutputStreamSerializer::ObjectMember>::iterator k;
			for (k = j->members.begin(); k != j->members.end(); ++k, ++midx) {
				if ((k->memberId != midx) && (!hasSerializerMember || k != (j->members.end() - 1))) {
					throw "Invalid member id";
				}
				WriteVarSizeUInt(stream, k->size);
			}
		}
	}

	// Calculate a checksum for metadata verification
	ph.metadataChecksum = 0;
	for (uint a = 0; a < classRefs.size(); a++)
	{
		Class* c = classRefs[a]->class_;
		c->CalculateChecksum(ph.metadataChecksum);
	}

	int endOffset = stream->tellp();
	stream->seekp(startOffset);
	memcpy(ph.magic, CREG_PACKAGE_FILE_ID, 4);
	ph.SwapBytes();
	stream->write((const char*)&ph, sizeof(PackageHeader));

	LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG,
			"Checksum: %X\nNumber of objects saved: %i\nNumber of classes involved: %i",
			ph.metadataChecksum, int(objects.size()), int(classRefs.size()));

	stream->seekp(endOffset);
	ptrToId.clear();
	pendingObjects.clear();
	objects.clear();
}

//-------------------------------------------------------------------------
// CInputStreamSerializer
//-------------------------------------------------------------------------

CInputStreamSerializer::CInputStreamSerializer()
	: stream(NULL)
{
}

CInputStreamSerializer::~CInputStreamSerializer()
{
	for (StoredObject& o: objects) {
		if (o.obj) {
			classRefs[o.classRef]->DeleteInstance(o.obj);
		}
	}
}

bool CInputStreamSerializer::IsWriting()
{
	return false;
}

void CInputStreamSerializer::SerializeObject(Class* c, void* ptr)
{
	if (c->base())
		SerializeObject(c->base(), ptr);

	for (uint a = 0; a < c->members.size(); a++)
	{
		creg::Class::Member* m = &c->members[a];
		if (m->flags & CM_NoSerialize)
			continue;

		const unsigned oldPos = stream->tellg();
		void* memberAddr = ((char*)ptr) + m->offset;
		m->type->Serialize(this, memberAddr);
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Deserialized %s::%s type:%s size:%u", c->name.c_str(), m->name, m->type->GetName().c_str(), unsigned(stream->tellg()) - oldPos);
	}

	if (c->HasSerialize()) {
		c->CallSerializeProc(ptr, this);
	}
}

void CInputStreamSerializer::Serialize(void* data, int byteSize)
{
	stream->read((char*)data, byteSize);
}

void CInputStreamSerializer::SerializeInt(void* data, int byteSize)
{
	// always save ints as 64bit
	// cause of int-types might differ in size depending on platforms
	// to make savegames compatible between those we need to so
	boost::int64_t x = 0;
	stream->read((char*)&x, 8);
	switch (byteSize) {
		case 1: { *(boost::int8_t* )data = x; break; }
		case 2: { *(boost::int16_t*)data = x; break; }
		case 4: { *(boost::int32_t*)data = x; break; }
		case 8: { *(boost::int64_t*)data = x; break; }
		default: {
			throw "Unknown int type";
		}
	}
}

void CInputStreamSerializer::SerializeObjectPtr(void** ptr, creg::Class* cls)
{
	unsigned int id;
	ReadVarSizeUInt(stream, &id);
	if (id) {
		StoredObject& o = objects [id];
		if (o.obj) *ptr = o.obj;
		else {
			// The object is not yet available, so it needs fixing afterwards
			*ptr = (void*) 1;
			UnfixedPtr ufp;
			ufp.objID = id;
			ufp.ptrAddr = ptr;
			unfixedPointers.push_back(ufp);
		}
	} else
		*ptr = NULL;
}

// Serialize an instance of an object embedded into another object
void CInputStreamSerializer::SerializeObjectInstance(void* inst, creg::Class* cls)
{
	unsigned int id;
	ReadVarSizeUInt(stream, &id);

	if (id == 0)
		return; // this is old save game and it has not this object - skip it

	StoredObject& o = objects[id];
	assert(!o.obj);
	assert(o.isEmbedded);

	o.obj = inst;
	SerializeObject(cls, inst);
}

void CInputStreamSerializer::AddPostLoadCallback(void (*cb)(void*), void* ud)
{
	PostLoadCallback plcb;

	plcb.cb = cb;
	plcb.userdata = ud;

	callbacks.push_back(plcb);
}

void CallPostLoad(creg::Class* c, creg::Class* oc, void* obj)
{
	if (c->base() != nullptr)
		CallPostLoad(c->base(), oc, obj);

	if (c->HasPostLoad()) {
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Run PostLoad of %s::%s", oc->name.c_str(), c->name.c_str());
		c->CallPostLoadProc(obj);
	}
}

void CInputStreamSerializer::LoadPackage(std::istream* s, void*& root, creg::Class*& rootCls)
{
	PackageHeader ph;

	stream = s;
	s->read((char*)&ph, sizeof(PackageHeader));

	if (memcmp(ph.magic, CREG_PACKAGE_FILE_ID, 4))
		throw std::runtime_error("Incorrect object package file ID");

	// Load references
	classRefs.resize(ph.numObjClassRefs);
	s->seekg(ph.objClassRefOffset);
	for (int a = 0; a < ph.numObjClassRefs; a++)
	{
		const std::string className = ReadZStr(*s);
		creg::Class* class_ = System::GetClass(className);
		if (!class_)
			throw std::runtime_error("Package file contains reference to unknown class " + className);
		classRefs[a] = class_;
	}

	// Calculate metadata checksum and compare with stored checksum
	unsigned int checksum = 0;
	for (uint a = 0; a < classRefs.size(); a++)
		classRefs[a]->CalculateChecksum(checksum);
	LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Checksum: %X (savegame: %X)\n", checksum, ph.metadataChecksum);
	if (checksum != ph.metadataChecksum)
		throw std::runtime_error("Metadata checksum error: Package file was saved with a different version");

	// Create all non-embedded objects
	s->seekg(ph.objTableOffset);
	objects.resize(ph.numObjects);
	for (int a = 0; a < ph.numObjects; a++)
	{
		unsigned int classRefIndex;
		char isEmbedded;
		unsigned int mgcnt;
		ReadVarSizeUInt(stream, &classRefIndex);
		stream->read((char*)&isEmbedded, sizeof(char));
		ReadVarSizeUInt(stream, &mgcnt);

		for (unsigned int b = 0; b < mgcnt; b++) {
			unsigned int cid, mcnt;
			char groupFlags;
			ReadVarSizeUInt(stream, &cid);
			ReadVarSizeUInt(stream, &mcnt);
			stream->read((char*)&groupFlags, sizeof(char));
			for (unsigned int c = 0; c < mcnt; c++) {
				unsigned int size;
				ReadVarSizeUInt(stream, &size);
			}
		}

		objects[a].obj = NULL;
		if (!isEmbedded) {
			// Allocate and construct
			ClassBinder* binder = classRefs[classRefIndex]->binder;
			void* inst = binder->class_.CreateInstance();
			objects[a].obj = inst;
		}
		objects[a].isEmbedded = !!isEmbedded;
		objects[a].classRef = classRefIndex;
	}

	int endOffset = s->tellg();

	// Read the object data using serialization
	s->seekg(ph.objDataOffset);
	for (uint a = 0; a < objects.size(); a++)
	{
		if (!objects[a].isEmbedded) {
			creg::Class* cls = classRefs[objects[a].classRef];
			SerializeObject(cls, objects[a].obj);
			LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Deserialized %s size:%i", cls->name.c_str(), cls->size);
		}
	}

	// Fix pointers to embedded objects
	for (uint a = 0; a < unfixedPointers.size(); a++) {
		void* p = objects[unfixedPointers[a].objID].obj;
		*unfixedPointers[a].ptrAddr = p;
	}

	// Run all registered post load callbacks
	for (uint a = 0; a < callbacks.size(); a++) {
		callbacks[a].cb(callbacks[a].userdata);
	}

	// Run post load functions on `all` objects (exclude root object)
	for (uint a = 1; a < objects.size(); a++) {
		StoredObject& o = objects[a];
		creg::Class* oc = classRefs[objects[a].classRef];
		creg::Class* c = oc;
		CallPostLoad(c, oc, o.obj);
	}

	// The first object is the root object
	root = objects[1].obj;
	rootCls = classRefs[objects[1].classRef];

	LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG,
			"SaveGame loaded.\nNumber of objects loaded: %i\nNumber of classes involved: %i\n",
			int(objects.size()), int(classRefs.size()));

	s->seekg(endOffset);
	unfixedPointers.clear();
	objects.clear();
}

ISerializer::~ISerializer() {
}

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

#include <algorithm>
#include <fstream>
#include <cassert>
#include <stdexcept>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <cinttypes>

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
	int objDataOffset = 0;
	int objTableOffset = 0;
	int numObjects = 0;
	int objClassRefOffset = 0; // a class ref is: zero-term class string + checksum DWORD
	int numObjClassRefs = 0;
	unsigned int metadataChecksum = 0;

	void SwapBytes()
	{
		swabDWordInPlace(objDataOffset);
		swabDWordInPlace(objTableOffset);
		swabDWordInPlace(objClassRefOffset);
		swabDWordInPlace(numObjClassRefs);
		swabDWordInPlace(numObjects);
		swabDWordInPlace(metadataChecksum);
	}
	PackageHeader()
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


template<typename T>
void ReadVarSizeUInt(std::istream* stream, T* buf)
{
	std::uint64_t val = 0;
	unsigned offset = 0;
	while (true) {
		unsigned char a;
		stream->read((char*)&a, sizeof(char));

		val += ((std::uint64_t)(a & 0x7F)) << offset;
		if ((a & 0x80) == 0)
			break;

		offset += 7;
	}

	*buf = val;
}


template<typename T>
void WriteVarSizeUInt(std::ostream* stream, T val)
{
	std::uint64_t v = val;
	do {
		unsigned char a = v & 0x7F;
		v >>= 7;

		if (v > 0)
			a |= 0x80;

		stream->write((char*)&a, sizeof(char));
	} while (v > 0);
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
	std::vector<ObjectRef*>& refs = ptrToId[inst];
	for (auto& obj: refs) {
		if (obj->isThisObject(inst, objClass, isEmbedded))
			return obj;
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
	omg.size = 0;

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
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Serialized %s::%s type:%s", c->name, m->name, m->type->GetName().c_str());
		m->type->Serialize(this, memberAddr);
		unsigned mend = stream->tellp();
		om.size = mend - mstart;
		omg.members.push_back(om);
		omg.size += om.size;
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Serialized %s::%s type:%s size:%d", c->name, m->name, m->type->GetName().c_str(), om.size);
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
		throw std::string("Reserialization of embedded object (") + objClass->name + ")";
	} else {
		std::vector<ObjectRef*>::iterator it = std::find(pendingObjects.begin(), pendingObjects.end(), obj);
		if (it == pendingObjects.end()) {
			throw std::string("Object pointer was serialized (") + objClass->name + ")";
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
			objects.emplace_back(*ptr, objects.size(), false, objClass);
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
	std::uint64_t x = 0;
	switch (byteSize) {
		case 1: { x = *(std::uint8_t* )data; break; }
		case 2: { x = *(std::uint16_t*)data; break; }
		case 4: { x = *(std::uint32_t*)data; break; }
		case 8: { x = *(std::uint64_t*)data; break; }
		default: {
			throw "Unknown int type";
		}
	}
	WriteVarSizeUInt(stream, x);
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
	objects.emplace_back(nullptr, 0, true, nullptr);
	ObjectRef* obj = &objects.back();
	obj->classIndex = 0;

	// Insert the first object that will provide references to everything
	objects.emplace_back(rootObj, objects.size(), false, rootObjClass);
	obj = &objects.back();
	ptrToId[rootObj].push_back(obj);
	pendingObjects.push_back(obj);

	// Save until all the referenced objects have been stored
	while (!pendingObjects.empty())
	{
		const std::vector<ObjectRef*> po = pendingObjects;
		pendingObjects.clear();

		for (ObjectRef* obj: po) {
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
					it.first->name,
					classCounts[it.first],
					it.second);
			it.second = 0;
			classCounts[it.first] = 0;
		}
	}


	// Write the class references & calc their checksum
	ph.numObjClassRefs = classRefs.size();
	ph.objClassRefOffset = (int)stream->tellp();
	for (auto& classRef: classRefs) {
		Class* c = classRef->class_;
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
		if (!isEmbedded && oRef.class_ != nullptr && oRef.class_->HasGetSize())
			WriteVarSizeUInt(stream, oRef.class_->CallGetSizeProc(oRef.ptr));
	}

	// Calculate a checksum for metadata verification
	ph.metadataChecksum = 0;
	for (auto& classRef: classRefs) {
		Class* c = classRef->class_;
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
	classSizes.clear();
}

//-------------------------------------------------------------------------
// CInputStreamSerializer
//-------------------------------------------------------------------------

CInputStreamSerializer::CInputStreamSerializer()
	: stream(nullptr)
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
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Deserialized %s::%s type:%s size:%u", c->name, m->name, m->type->GetName().c_str(), unsigned(stream->tellg()) - oldPos);
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
	std::uint64_t x = 0;
	ReadVarSizeUInt(stream, &x);
	switch (byteSize) {
		case 1: { *(std::uint8_t* )data = x; break; }
		case 2: { *(std::uint16_t*)data = x; break; }
		case 4: { *(std::uint32_t*)data = x; break; }
		case 8: { *(std::uint64_t*)data = x; break; }
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
	} else {
		*ptr = nullptr;
	}
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
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Run PostLoad of %s::%s", oc->name, c->name);
		c->CallPostLoadProc(obj);
	}
}

void CInputStreamSerializer::LoadPackage(std::istream* s, void*& root, creg::Class*& rootCls)
{
	PackageHeader ph;

	stream = s;
	s->read((char*)&ph, sizeof(PackageHeader));

	if (memcmp(ph.magic, CREG_PACKAGE_FILE_ID, 4) != 0)
		throw content_error("Incorrect object package file ID");

	// Load references
	classRefs.resize(ph.numObjClassRefs);
	s->seekg(ph.objClassRefOffset);

	for (int a = 0; a < ph.numObjClassRefs; a++) {
		const std::string className = ReadZStr(*s);

		if ((classRefs[a] = System::GetClass(className)) == nullptr)
			throw content_error("Package file contains reference to unknown class " + className);
	}

	{
		// Calculate metadata checksum and compare with stored checksum
		unsigned int checksum = 0;

		for (const auto& classRef: classRefs)
			classRef->CalculateChecksum(checksum);

		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Checksum: %X (savegame: %X)\n", checksum, ph.metadataChecksum);

		if (checksum != ph.metadataChecksum)
			throw content_error("Metadata checksum error: Package file was saved with a different version");
	}

	// Create all non-embedded objects
	s->seekg(ph.objTableOffset);
	objects.resize(ph.numObjects);

	for (int a = 0; a < ph.numObjects; a++) {
		unsigned int classRefIndex;
		char isEmbedded;

		ReadVarSizeUInt(stream, &classRefIndex);
		stream->read((char*)&isEmbedded, sizeof(char));
		Class* c = classRefs[classRefIndex];

		objects[a].obj = nullptr;

		if (!isEmbedded) {
			size_t size = c->size;

			if (c->HasGetSize())
				ReadVarSizeUInt(stream, &size);

			// Allocate and construct
			objects[a].obj = c->CreateInstance(size);
		}

		objects[a].isEmbedded = !!isEmbedded;
		objects[a].classRef = classRefIndex;
	}

	const int endOffset = s->tellg();

	// Read the object data using serialization
	s->seekg(ph.objDataOffset);
	for (const auto& object: objects) {
		if (object.isEmbedded)
			continue;

		creg::Class* cls = classRefs[object.classRef];
		SerializeObject(cls, object.obj);
		LOG_SL(LOG_SECTION_CREG_SERIALIZER, L_DEBUG, "Deserialized %s size:%i", cls->name, cls->size);
	}

	// Fix pointers to embedded objects
	for (const auto& unfixedPointer: unfixedPointers) {
		*unfixedPointer.ptrAddr = objects[unfixedPointer.objID].obj;
	}

	// Run all registered PostLoad callbacks
	for (const auto& callback: callbacks) {
		callback.cb(callback.userdata);
	}

	// Run PostLoad functions on `all` objects (exclude root object)
	for (const StoredObject& o: objects) {
		creg::Class* oc = classRefs[o.classRef];
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

ISerializer::~ISerializer() = default;

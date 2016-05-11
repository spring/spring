/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandDescription.h"

#include <cassert>
#include "System/Sync/HsiehHash.h"

CR_BIND(SCommandDescription, )
CR_REG_METADATA(SCommandDescription, (
	CR_MEMBER(id),
	CR_MEMBER(type),

	CR_MEMBER(name),
	CR_MEMBER(action),
	CR_MEMBER(iconname),
	CR_MEMBER(mouseicon),
	CR_MEMBER(tooltip),

	CR_MEMBER(queueing),
	CR_MEMBER(hidden),
	CR_MEMBER(disabled),
	CR_MEMBER(showUnique),
	CR_MEMBER(onlyTexture),
	CR_MEMBER(params),
	CR_MEMBER(refCount)
))


bool SCommandDescription::operator != (const SCommandDescription& cd) const
{
	return id          != cd.id          ||
		   type        != cd.type        ||
		   name        != cd.name        ||
		   action      != cd.action      ||
		   iconname    != cd.iconname    ||
		   mouseicon   != cd.mouseicon   ||
		   tooltip     != cd.tooltip     ||
		   queueing    != cd.queueing    ||
		   hidden      != cd.hidden      ||
		   disabled    != cd.disabled    ||
		   showUnique  != cd.showUnique  ||
		   onlyTexture != cd.onlyTexture ||
		   params      != cd.params;
}

CCommandDescriptionCache* commandDescriptionCache = nullptr;

CR_BIND(CCommandDescriptionCache, )
CR_REG_METADATA(CCommandDescriptionCache, (
	CR_MEMBER(cache)
))

int CCommandDescriptionCache::CalcHash(const SCommandDescription& cd) const
{
	int hash = HsiehHash(&cd.id             , sizeof(cd.id)         , 0   );
	hash     = HsiehHash(&cd.type           , sizeof(cd.type)       , hash);
	hash     = HsiehHash(cd.name.data()     , cd.name.size()        , hash);
	hash     = HsiehHash(cd.action.data()   , cd.action.size()      , hash);
	hash     = HsiehHash(cd.iconname.data() , cd.iconname.size()    , hash);
	hash     = HsiehHash(cd.mouseicon.data(), cd.mouseicon.size()   , hash);
	hash     = HsiehHash(cd.tooltip.data()  , cd.tooltip.size()     , hash);
	hash     = HsiehHash(&cd.queueing       , sizeof(cd.queueing)   , hash);
	hash     = HsiehHash(&cd.hidden         , sizeof(cd.hidden)     , hash);
	hash     = HsiehHash(&cd.disabled       , sizeof(cd.disabled)   , hash);
	hash     = HsiehHash(&cd.showUnique     , sizeof(cd.showUnique) , hash);
	hash     = HsiehHash(&cd.onlyTexture    , sizeof(cd.onlyTexture), hash);
	for (const std::string& s: cd.params) {
		hash = HsiehHash(s.data()           , s.size()              , hash);
	}
	return hash;
}


const SCommandDescription* CCommandDescriptionCache::GetPtr(const SCommandDescription& cd)
{
	int hash = CalcHash(cd);
	auto it = cache.find(hash);
	while (it != cache.end()) {
		if (it->second != cd) {
			++it;
			continue;
		}

		++(it->second.refCount);
		return &(it->second);
	}
	it = cache.emplace(hash, cd);
	it->second.refCount = 1;
	return &(it->second);
}


void CCommandDescriptionCache::DecRef(const SCommandDescription& cd)
{
	int hash = CalcHash(cd);
	auto it = cache.find(hash);
	while (it != cache.end()) {
		if (&(it->second) != &cd) {
			++it;
			continue;
		}

		if (it->second.refCount == 1) {
			cache.erase(it);
		} else {
			--(it->second.refCount);
		}
		return;
	}

	assert(false);
}


void CCommandDescriptionCache::DecRef(std::vector<const SCommandDescription*>& cmdDescs)
{
	for (const SCommandDescription* cd: cmdDescs) {
		DecRef(*cd);
	}
	cmdDescs.clear();
}

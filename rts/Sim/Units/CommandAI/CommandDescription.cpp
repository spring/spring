/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <algorithm>

#include "CommandDescription.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"

CR_BIND(SCommandDescription, )
CR_REG_METADATA(SCommandDescription, (
	CR_MEMBER(id),
	CR_MEMBER(type),

	CR_MEMBER(refCount),

	CR_MEMBER(queueing),
	CR_MEMBER(hidden),
	CR_MEMBER(disabled),
	CR_MEMBER(showUnique),
	CR_MEMBER(onlyTexture),

	CR_MEMBER(name),
	CR_MEMBER(action),
	CR_MEMBER(iconname),
	CR_MEMBER(mouseicon),
	CR_MEMBER(tooltip),

	CR_MEMBER(params)
))

CR_BIND(CCommandDescriptionCache, )
CR_REG_METADATA(CCommandDescriptionCache, (
	// FIXME: std::pair type deduction
	CR_IGNORED(index),
	CR_MEMBER(slots),
	CR_MEMBER(cache),

	CR_MEMBER(numCmdDescrs),
	CR_MEMBER(numFreeSlots)
))



bool SCommandDescription::operator != (const SCommandDescription& cd) const
{
	return id          != cd.id          ||
	       type        != cd.type        ||

	       queueing    != cd.queueing    ||
	       hidden      != cd.hidden      ||
	       disabled    != cd.disabled    ||
	       showUnique  != cd.showUnique  ||
	       onlyTexture != cd.onlyTexture ||

	       name        != cd.name        ||
	       action      != cd.action      ||
	       iconname    != cd.iconname    ||
	       mouseicon   != cd.mouseicon   ||
	       tooltip     != cd.tooltip     ||
	       params      != cd.params;
}


CCommandDescriptionCache commandDescriptionCache;


void CCommandDescriptionCache::Init() {
	index.fill({0, 0});
	slots.fill(-1u);

	for (SCommandDescription& cd: cache) {
		cd = std::move(SCommandDescription());
	}

	// generate cache-indices pool, reverse order since GetPtr pops from the back
	std::for_each(slots.begin(), slots.end(), [&](const unsigned int& i) { slots[&i - &slots[0]] = &i - &slots[0]; });
	std::reverse(slots.begin(), slots.end());

	numCmdDescrs = 0;
	numFreeSlots = slots.size();
}


int CCommandDescriptionCache::CalcHash(const SCommandDescription& cd) const
{
	int hash = 0;

	hash = HsiehHash(&cd.id             , sizeof(cd.id)         , hash);
	hash = HsiehHash(&cd.type           , sizeof(cd.type)       , hash);
	hash = HsiehHash(&cd.queueing       , sizeof(cd.queueing)   , hash);
	hash = HsiehHash(&cd.hidden         , sizeof(cd.hidden)     , hash);
	hash = HsiehHash(&cd.disabled       , sizeof(cd.disabled)   , hash);
	hash = HsiehHash(&cd.showUnique     , sizeof(cd.showUnique) , hash);
	hash = HsiehHash(&cd.onlyTexture    , sizeof(cd.onlyTexture), hash);
	hash = HsiehHash(cd.name.data()     , cd.name.size()        , hash);
	hash = HsiehHash(cd.action.data()   , cd.action.size()      , hash);
	hash = HsiehHash(cd.iconname.data() , cd.iconname.size()    , hash);
	hash = HsiehHash(cd.mouseicon.data(), cd.mouseicon.size()   , hash);
	hash = HsiehHash(cd.tooltip.data()  , cd.tooltip.size()     , hash);

	for (const std::string& s: cd.params) {
		hash = HsiehHash(s.data(), s.size(), hash);
	}

	return hash;
}


const SCommandDescription* CCommandDescriptionCache::GetPtr(SCommandDescription&& cd)
{
	using P = decltype(index)::value_type;

	const int cdHash = CalcHash(cd);

	const auto ibeg = index.begin();
	const auto iend = index.begin() + numCmdDescrs;
	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(ibeg, iend, P{cdHash, -1u}, pred);

	if (iter == iend || iter->first != cdHash) {
		if (numFreeSlots == 0) {
			LOG_L(L_WARNING, "[CmdDescrCache::%s] too many unique command-descriptions", __func__);
			return &cache[cache.size() - 1];
		}

		const unsigned int cdIndex = slots[--numFreeSlots];

		cache[cdIndex] = std::move(cd);
		cache[cdIndex].refCount = 1;

		index[numCmdDescrs] = {cdHash, cdIndex};

		// swap into position
		for (unsigned int i = numCmdDescrs++; i > 0; i--) {
			if (index[i - 1].first < index[i].first)
				break;

			std::swap(index[i - 1], index[i]);
		}

		return &cache[cdIndex];
	}

	for (auto it = iter; (it != iend && it->first == cdHash); ++it) {
		if (cache[it->second] != cd)
			continue;

		cache[it->second].refCount += 1;
		return &cache[it->second];
	}

	// dummy
	return &cache[cache.size() - 1];
}


void CCommandDescriptionCache::DecRef(const SCommandDescription& cd)
{
	using P = decltype(index)::value_type;

	const int cdHash = CalcHash(cd);

	const auto ibeg = index.begin();
	const auto iend = index.begin() + numCmdDescrs;
	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(ibeg, iend, P{cdHash, -1u}, pred);

	if (iter == iend || iter->first != cdHash) {
		assert(false);
		return;
	}

	assert(numFreeSlots < slots.size());

	for (auto it = iter; (it != iend && it->first == cdHash); ++it) {
		SCommandDescription& icd = cache[it->second];

		if (&icd != &cd)
			continue;

		if (icd.refCount == 1) {
			// return free slot
			slots[numFreeSlots++] = it->second;

			// delete from index
			for (unsigned int j = it - ibeg, n = --numCmdDescrs; j < n; j++) {
				std::swap(index[j], index[j + 1]);
			}
		} else {
			icd.refCount -= 1;
		}

		break;
	}
}


void CCommandDescriptionCache::DecRef(std::vector<const SCommandDescription*>& cmdDescs)
{
	for (const SCommandDescription* cd: cmdDescs) {
		DecRef(*cd);
	}

	cmdDescs.clear();
}


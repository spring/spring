/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_DESCRIPTION_H
#define COMMAND_DESCRIPTION_H

#include "Command.h"

#include <array>
#include <vector>

struct SCommandDescription {
	CR_DECLARE_STRUCT(SCommandDescription)

public:
	SCommandDescription() = default;
	SCommandDescription(const SCommandDescription& cd) = default;
	SCommandDescription(SCommandDescription&& cd) { *this = std::move(cd); }

	SCommandDescription& operator = (const SCommandDescription& cd) = default;
	SCommandDescription& operator = (SCommandDescription&& cd) {
		id = cd.id;
		type = cd.type;

		refCount = cd.refCount;

		queueing = cd.queueing;
		hidden = cd.hidden;
		disabled = cd.disabled;
		showUnique = cd.showUnique;
		onlyTexture = cd.onlyTexture;

		name = std::move(cd.name);
		action = std::move(cd.action);
		iconname = std::move(cd.iconname);
		mouseicon = std::move(cd.mouseicon);
		tooltip = std::move(cd.tooltip);

		params = std::move(cd.params);
		return *this;
	}

	bool operator != (const SCommandDescription& cd) const;

public:
	/// CMD_xxx code (custom codes can also be used)
	int id = 0;
	/// CMDTYPE_xxx code
	int type = CMDTYPE_ICON;

	mutable int refCount = 1;

	bool queueing = true;
	/// if true dont show a button for the command
	bool hidden = false;
	/// for greying-out commands
	bool disabled = false;
	/// command only applies to single units
	bool showUnique = false;
	/// do not draw the name if the texture is available
	bool onlyTexture = false;

	/// command name
	std::string name;
	/// the associated command action binding name
	std::string action;
	/// button texture
	std::string iconname;
	/// mouse cursor
	std::string mouseicon;
	/// tooltip text
	std::string tooltip;

	std::vector<std::string> params;
};


class CCommandDescriptionCache {
	CR_DECLARE_STRUCT(CCommandDescriptionCache)

public:
	void Init();
	void Kill() {}
	void Dump(bool forced);

	const SCommandDescription& GetRef(SCommandDescription&& cd) { return *GetPtr(std::move(cd)); }
	const SCommandDescription* GetPtr(SCommandDescription&& cd);

	void DecRef(std::vector<const SCommandDescription*>& cmdDescs);
	void DecRef(const SCommandDescription& cd);

private:
	int CalcHash(const SCommandDescription& cd) const;

private:
	// maps hashes to cache-indices (sorted)
	std::array< std::pair<int, unsigned int>, 1024 > index;
	// tracks free slots in cache (stack)
	std::array<unsigned int, 1024> slots;
	// includes one dummy description
	std::array<SCommandDescription, 1024 + 1> cache;

	unsigned int numCmdDescrs = 0;
	unsigned int numFreeSlots = 0;
	unsigned int cacheFullCtr = 0;
};

extern CCommandDescriptionCache commandDescriptionCache;

#endif // COMMAND_DESCRIPTION_H


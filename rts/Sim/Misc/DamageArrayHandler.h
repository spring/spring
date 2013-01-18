/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DAMAGE_ARRAY_HANDLER_H
#define _DAMAGE_ARRAY_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include <boost/noncopyable.hpp>
#include "System/creg/creg_cond.h"

class CDamageArrayHandler : public boost::noncopyable
{
	CR_DECLARE_STRUCT(CDamageArrayHandler);

public:
	CDamageArrayHandler();
	~CDamageArrayHandler();

	int GetTypeFromName(std::string name) const;

	int GetNumTypes() const { return typeList.size(); }
	const std::vector<std::string>& GetTypeList() const { return typeList; }

private:
	std::map<std::string, int> name2type;
	std::vector<std::string> typeList;
};

extern CDamageArrayHandler* damageArrayHandler;

#endif // _DAMAGE_ARRAY_HANDLER_H

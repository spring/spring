#ifndef __DAMAGE_ARRAY_HANDLER_H__
#define __DAMAGE_ARRAY_HANDLER_H__

#include <string>
#include <vector>
#include <map>
#include <boost/noncopyable.hpp>
#include "creg/creg_cond.h"

class CDamageArrayHandler : public boost::noncopyable
{
	CR_DECLARE(CDamageArrayHandler);

public:
	CDamageArrayHandler(void);
	~CDamageArrayHandler(void);

	int GetTypeFromName(std::string name);

	int GetNumTypes() const { return numTypes; }
	const std::vector<std::string>& GetTypeList() const { return typeList; }

private:
	int numTypes;
	std::map<std::string, int> name2type;
	std::vector<std::string> typeList;
};

extern CDamageArrayHandler* damageArrayHandler;

#endif // __DAMAGE_ARRAY_HANDLER_H__

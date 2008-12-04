/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _RESOURCEHANDLER_H
#define _RESOURCEHANDLER_H

#include <vector>
#include <boost/noncopyable.hpp>
#include "creg/creg.h"
#include "Resource.h"

class CResourceHandler : public boost::noncopyable
{
	CR_DECLARE(CResourceHandler);

public:
	static CResourceHandler* GetInstance();
	static void FreeInstance();

	/**
	 * @brief	resource
	 * @param	resourceId index to fetch
	 * @return	the searched resource
	 *
	 * Accesses a CResource instance at a given index
	 */
	const CResource* GetResource(int resourceId) const;
	/**
	 * @brief	resource by name
	 * @param	resourceName name of the resource to fetch
	 * @return	the searched resource
	 *
	 * Accesses a CResource instance by name
	 */
	const CResource* GetResourceByName(const std::string& resourceName) const;
	/**
	 * @brief	resource index by name
	 * @param	resourceName name of the resource to fetch
	 * @return	index of the searched resource
	 *
	 * Accesses a resource by name
	 */
	int GetResourceId(const std::string& resourceName) const;

	unsigned int GetNumResources() const;

//	bool IsMetal(int resourceId) const;
//	bool IsEnergy(int resourceId) const;
	int GetMetalId() const;
	int GetEnergyId() const;

private:
	static CResourceHandler* instance;

	CResourceHandler();
	~CResourceHandler();

	std::vector<CResource> resources;
};

#define rh CResourceHandler::GetInstance()

#endif // _RESOURCEHANDLER_H

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
#include <map>
#include <boost/noncopyable.hpp>
#include "creg/creg_cond.h"
#include "Resource.h"

class CResourceMapAnalyzer;

class CResourceHandler : public boost::noncopyable
{
	CR_DECLARE(CResourceHandler);

public:
	static CResourceHandler* GetInstance();
	static void FreeInstance();

	/**
	 * @brief	add a resource
	 * @param	resource the resource to add
	 * @return	the id of the resource just added
	 *
	 * Adds a CResource to the pool and retun its resourceId.
	 */
	int AddResource(const CResource& resource);
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

	/**
	 * @brief	resource map
	 * @param	resourceId index of the resource whichs map to fetch
	 * @return	the resource values for all the pixels of the map
	 *
	 * Returns a resource map by index.
	 */
	const unsigned char* GetResourceMap(int resourceId) const;
	/**
	 * @brief	resource map size
	 * @param	resourceId index of the resource whichs map size to fetch
	 * @return	the number of values in the resource map
	 *
	 * Returns the resource map size by index.
	 */
	size_t GetResourceMapSize(int resourceId) const;
	/**
	 * @brief	resource map width
	 * @param	resourceId index of the resource whichs map width to fetch
	 * @return	width of values in the resource map
	 *
	 * Returns the resource map width by index.
	 */
	size_t GetResourceMapWidth(int resourceId) const;
	/**
	 * @brief	resource map height
	 * @param	resourceId index of the resource whichs map height to fetch
	 * @return	height of values in the resource map
	 *
	 * Returns the resource map height by index.
	 */
	size_t GetResourceMapHeight(int resourceId) const;
	/**
	 * @brief	resource map analyzer
	 * @param	resourceId index of the resource whichs map analyzer to fetch
	 * @return	resource map analyzer
	 *
	 * Returns the resource map analyzer by index.
	 */
	const CResourceMapAnalyzer* GetResourceMapAnalyzer(int resourceId);

	size_t GetNumResources() const;

//	bool IsMetal(int resourceId) const;
//	bool IsEnergy(int resourceId) const;
	int GetMetalId() const;
	int GetEnergyId() const;

	bool IsValidId(int resourceId) const;

private:
	static CResourceHandler* instance;

	CResourceHandler();
	~CResourceHandler();

	std::vector<CResource> resources;
	std::map<int, CResourceMapAnalyzer*> resourceMapAnalyzers;

	int metalResourceId;
	int energyResourceId;
};

#define resourceHandler CResourceHandler::GetInstance()

#endif // _RESOURCEHANDLER_H

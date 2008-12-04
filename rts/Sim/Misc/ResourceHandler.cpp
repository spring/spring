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

#include "ResourceHandler.h"

#include "Sync/Syncify.h"

CR_BIND(CResourceHandler,);

CR_REG_METADATA(CResourceHandler, (
	CR_MEMBER(resources)
));

CResourceHandler* CResourceHandler::instance;

CResourceHandler* CResourceHandler::GetInstance()
{
	if (instance == NULL) {
		instance = SAFE_NEW CResourceHandler();
	}
	return instance;
}
void CResourceHandler::FreeInstance()
{
	delete instance;
	instance = NULL;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static const int METAL_INDEX = 0;
static const int ENERGY_INDEX = 1;
CResourceHandler::CResourceHandler()
{
	CResource rMetal;
	rMetal.name = "Metal";
	resources.push_back(rMetal);
	// Metal will have index 0

	CResource rEnergy;
	rEnergy.name = "Energy";
	resources.push_back(rEnergy);
	// Energy will have index 1
}

CResourceHandler::~CResourceHandler()
{
}


const CResource* CResourceHandler::GetResource(int resourceId) const
{
	if (resourceId >= (int)resources.size()) {
		return NULL;
	} else {
		return &resources[resourceId];
	}
}

const CResource* CResourceHandler::GetResourceByName(const std::string& resourceName) const
{
	return GetResource(GetResourceId(resourceName));
}

int CResourceHandler::GetResourceId(const std::string& resourceName) const
{
	int size_res = resources.size();
	for (int r=0; r < size_res; ++r) {
		if (resources[r].name == resourceName) {
			return r;
		}
	}
	return -1;
}


unsigned int CResourceHandler::GetNumResources() const
{
	return resources.size();
}


//bool CResourceHandler::IsMetal(int resourceId) const
//{
//	const CResource* res = GetResource(resourceId);
//	return res && (res->name == "Metal");
//}
//
//bool CResourceHandler::IsEnergy(int resourceId) const
//{
//	const CResource* res = GetResource(resourceId);
//	return res && (res->name == "Energy");
//}

int CResourceHandler::GetMetalId() const
{
	return METAL_INDEX;
}

int CResourceHandler::GetEnergyId() const
{
	return ENERGY_INDEX;
}

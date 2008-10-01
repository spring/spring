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

// The structs in this files relate to *Infos.lua files
// They are used for AIs eg (-> AIInfo.lua)

#ifndef _SINFO_H
#define	_SINFO_H

#ifdef	__cplusplus
extern "C" {
#endif

struct InfoItem {
	const char* key;
	const char* value;
	const char* desc;
};

InfoItem copyInfoItem(const struct InfoItem* const orig);
void deleteInfoItem(const struct InfoItem* const info);

#if	defined(__cplusplus) && !defined(BUILDING_AI)
unsigned int ParseInfos(
		const char* fileName,
		const char* fileModes,
		const char* accessModes,
		InfoItem infos[], unsigned int max);
unsigned int ParseInfosRawFileSystem(
		const char* fileName,
		InfoItem infos[], unsigned int max);
#endif	/* defined(__cplusplus) && !defined(BUILDING_AI) */

#ifdef	__cplusplus
}
#endif

//#ifdef	__cplusplus
//#include <vector>
//#include <string>
//std::vector<InfoItem> ParseInfos(
//		const std::string& fileName,
//		const std::string& fileModes,
//		const std::string& accessModes);
//#endif

#endif	/* _SINFO_H */


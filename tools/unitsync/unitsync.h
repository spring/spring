#ifndef UNITSYNC_H
#define UNITSYNC_H

#include <string>
#include "exportdefines.h"
#include "ExternalAI/Interface/ELevelOfSupport.h"
#include "ExternalAI/Interface/SOption.h"
#include "ExternalAI/Interface/SInfo.h"

#define STRBUF_SIZE 100000


/**
 * @addtogroup unitsync_api Unitsync API
 * @{
*/

/**
 * @brief 2d vector storing a map defined starting position
 * @sa MapInfo
 */
struct StartPos
{
	int x; ///< X component
	int z; ///< Z component
};


/**
 * @brief Metadata of a map
 * @sa GetMapInfo GetMapInfoEx
 */
struct MapInfo
{
	char* description;   ///< Description (max 255 chars)
	int tidalStrength;   ///< Tidal strength
	int gravity;         ///< Gravity
	float maxMetal;      ///< Metal scale factor
	int extractorRadius; ///< Extractor radius (of metal extractors)
	int minWind;         ///< Minimum wind speed
	int maxWind;         ///< Maximum wind speed

	// 0.61b1+
	int width;              ///< Width of the map
	int height;             ///< Height of the map
	int posCount;           ///< Number of defined start positions
	StartPos positions[16]; ///< Start positions defined by the map (max 16)

	// VERSION>=1
	char* author;   ///< Creator of the map (max 200 chars)
};


/**
 * @brief Available bitmap typeHints
 * @sa GetInfoMap
 */
enum BitmapType {
	bm_grayscale_8  = 1, ///< 8 bits per pixel grayscale bitmap
	bm_grayscale_16 = 2  ///< 16 bits per pixel grayscale bitmap
};

/** @} */


const char *GetStr(std::string str);


#ifdef WIN32
	#include <windows.h>
#else
	#include <iostream>
	#define MB_OK 0
	static inline void MessageBox(void*, const char* msg, const char* capt, unsigned int)
	{
		std::cerr << "unitsync: " << capt << ": " << msg << std::endl;
	}
#endif	/* WIN32 */

/**
 * @brief returns the version of spring this was compiled with
 *
 * Returns a const char* string specifying the version of spring used to build this library with.
 * It was added to aid in lobby creation, where checks for updates to spring occur.
 */
EXPORT(const char*) GetSpringVersion();
/**
 * @brief Creates a messagebox with said message
 * @param p_szMessage const char* string holding the message
 *
 * Creates a messagebox with the title "Message from DLL", an OK button, and the specified message
 */
EXPORT(void) Message(const char* p_szMessage);
EXPORT(void) UnInit();
EXPORT(int) Init(bool isServer, int id);
/**
 * @brief process another unit and return how many are left to process
 * @return int The number of unprocessed units to be handled
 *
 * Call this function repeatedly untill it returns 0 before calling any other function related to units.
 */
EXPORT(int) ProcessUnits(void);
/**
 * @brief process another unit and return how many are left to process without checksumming
 * @return int The number of unprocessed units to be handled
 *
 * Call this function repeatedly untill it returns 0 before calling any other function related to units.
 * This function performs the same operations as ProcessUnits() but it does not generate checksums.
 */
EXPORT(int) ProcessUnitsNoChecksum(void);
EXPORT(const char*) GetCurrentList();
EXPORT(void) AddClient(int id, const char *unitList);
EXPORT(void) RemoveClient(int id);
EXPORT(const char*) GetClientDiff(int id);
EXPORT(void) InstallClientDiff(const char *diff);
/**
 * @brief returns the number of units
 * @return int number of units processed and available
 *
 * Will return the number of units. Remember to call processUnits() beforehand untill it returns 0
 * As ProcessUnits is called the number of processed units goes up, and so will the value returned
 * by this function.
 *
 * while(processUnits()){}
 * int unit_number = GetUnitCount();
 */
EXPORT(int) GetUnitCount();
/**
 * @brief returns the units internal mod name
 * @param int the units id number
 * @return const char* The units internal modname
 *
 * This function returns the units internal mod name. For example it would return armck and not
 * Arm Construction kbot.
 */
EXPORT(const char*) GetUnitName(int unit);
/**
 * @brief returns The units human readable name
 * @param int The units id number
 * @return const char* The Units human readable name
 *
 * This function returns the units human name. For example it would return Arm Construction kbot
 * and not armck.
 */
EXPORT(const char*) GetFullUnitName(int unit);
EXPORT(int) IsUnitDisabled(int unit);
EXPORT(int) IsUnitDisabledByClient(int unit, int clientId);
EXPORT(void) AddArchive(const char* name);
EXPORT(void) AddAllArchives(const char* root);
EXPORT(unsigned int) GetArchiveChecksum(const char* arname);
EXPORT(const char*) GetArchivePath(const char* arname);

EXPORT(int) GetMapCount();
EXPORT(const char*) GetMapName(int index);
EXPORT(int) GetMapInfoEx(const char* name, MapInfo* outInfo, int version);
EXPORT(int) GetMapInfo(const char* name, MapInfo* outInfo);
EXPORT(int) GetMapArchiveCount(const char* mapName);
EXPORT(const char*) GetMapArchiveName(int index);
EXPORT(unsigned int) GetMapChecksum(int index);
EXPORT(unsigned int) GetMapChecksumFromName(const char* mapName);
/**
 * @brief Retrieves a minimap image for a map.
 * @param filename The name of the map, including extension.
 * @param miplevel Which miplevel of the minimap to extract from the file.
 * Set miplevel to 0 to get the largest, 1024x1024 minimap. Each increment
 * divides the width and height by 2. The maximum miplevel is 8, resulting in a
 * 4x4 image.
 * @return A pointer to a static memory area containing the minimap as a 16 bit
 * packed RGB-565 (MSB to LSB: 5 bits red, 6 bits green, 5 bits blue) linear
 * bitmap.
 *
 * An example usage would be GetMinimap("SmallDivide.smf", 2).
 * This would return a 16 bit packed RGB-565 256x256 (= 1024/2^2) bitmap.
 *
 * Be sure you've made a calls to Init prior to using this.
 */
EXPORT(void*) GetMinimap(const char* filename, int miplevel);

/**
 * @brief Retrieves the name of this mod
 * @return int The number of mods
 *
 * Returns the name of the mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(int) GetPrimaryModCount();
/**
 * @brief Retrieves the name of this mod
 * @param index in The mods index/id
 * @return const char* The mods name
 *
 * Returns the name of the mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(const char*) GetPrimaryModName(int index);
/**
 * @brief Retrieves the shortened name of this mod
 * @param index in The mods index/id
 * @return const char* The mods abbrieviated name
 *
 * Returns the shortened name of the mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(const char*) GetPrimaryModShortName(int index);
/**
 * @brief Retrieves the version string of this mod
 * @param index in The mods index/id
 * @return const char* The mods version string
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(const char*) GetPrimaryModVersion(int index);
/**
 * @brief Retrieves the mutator name of this mod
 * @param index in The mods index/id
 * @return const char* The mods mutator name
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(const char*) GetPrimaryModMutator(int index);
/**
 * @brief Retrieves the game name of this mod
 * @param index in The mods index/id
 * @return const char* The mods game
 *
 * Returns the name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(const char*) GetPrimaryModGame(int index);
/**
 * @brief Retrieves the short game name of this mod
 * @param index in The mods index/id
 * @return const char* The mods abbrieviated game name
 *
 * Returns the abbrieviated name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(const char*) GetPrimaryModShortGame(int index);
/**
 * @brief Retrieves the description of this mod
 * @param index in The mods index/id
 * @return const char* The mods description
 *
 * Returns a description for the specified mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
EXPORT(const char*) GetPrimaryModDescription(int index);
EXPORT(const char*) GetPrimaryModArchive(int index);
/**
 * @brief Retrieves the number of archives a mod requires
 * @param index int The index of the mod
 * @return the number of archives this mod depends on.
 *
 * This is used to get the entire list of archives that a mod requires.
 * Call GetPrimaryModArchiveCount() with selected mod first to get number of
 * archives, and then use GetPrimaryModArchiveList() for 0 to count-1 to get the
 * name of each archive.
 */
EXPORT(int) GetPrimaryModArchiveCount(int index);
/**
 * @brief Retrieves the name of the current mod's archive.
 * @param archiveNr The mod's archive index/id.
 * @return the name of the archive
 * @see GetPrimaryModArchiveCount()
 */
EXPORT(const char*) GetPrimaryModArchiveList(int archiveNr);
EXPORT(int) GetPrimaryModIndex(const char* name);
EXPORT(unsigned int) GetPrimaryModChecksum(int index);
EXPORT(unsigned int) GetPrimaryModChecksumFromName(const char* name);
EXPORT(int) GetSideCount();
EXPORT(const char*) GetSideName(int side);
EXPORT(const char*) GetSideStartUnit(int side);
EXPORT(int) GetLuaAICount();
EXPORT(const char*) GetLuaAIName(int aiIndex);
EXPORT(const char*) GetLuaAIDesc(int aiIndex);

/**
 * Returns the number of available Skirmish AIs.
 * A call to this function will load AI information.
 */
EXPORT(int) GetSkirmishAICount();
/**
 * Returns the specifier of a Skirmish AI.
 * It contians all the info needed to specify an AI in script.txt eg.
 */
EXPORT(struct SSAISpecifier) GetSkirmishAISpecifier(int index);
/**
 * Returns a list of static properties of a skirmish AI.
 * For a list of standard properties, see the SKIRMISH_AI_PROPERTY_* defines
 * in rts/ExternalAI/Interface/SSAILibrary.h.
 */
EXPORT(int) GetSkirmishAIInfoCount(int index);
EXPORT(const char*) GetInfoKey(int index);
EXPORT(const char*) GetInfoValue(int index);
EXPORT(const char*) GetInfoDescription(int index);
/**
 * Returns the level of support we can avait from a Skirmish AI, concerning a
 * specific Spring and AI Interface version.
 */
//EXPORT(struct LevelOfSupport) GetSkirmishAILevelOfSupport(int index, const char* engineVersionString, int engineVersionNumber, const char* aiInterfaceShortName, const char* aiInterfaceVersion);

EXPORT(int) GetMapOptionCount(const char* name);
EXPORT(int) GetModOptionCount();
EXPORT(int) GetSkirmishAIOptionCount(int index);

// Common Parameters
EXPORT(const char*) GetOptionKey(int optIndex);
EXPORT(const char*) GetOptionName(int optIndex);
EXPORT(const char*) GetOptionDesc(int optIndex);
EXPORT(int) GetOptionType(int optIndex);

// Bool Options
EXPORT(int) GetOptionBoolDef(int optIndex);

// Number Options
EXPORT(float) GetOptionNumberDef(int optIndex);
EXPORT(float) GetOptionNumberMin(int optIndex);
EXPORT(float) GetOptionNumberMax(int optIndex);
EXPORT(float) GetOptionNumberStep(int optIndex);

// String Options
EXPORT(const char*) GetOptionStringDef(int optIndex);
EXPORT(int) GetOptionStringMaxLen(int optIndex);

// List Options
EXPORT(int) GetOptionListCount(int optIndex);
EXPORT(const char*) GetOptionListDef(int optIndex);
EXPORT(const char*) GetOptionListItemKey(int optIndex, int itemIndex);
EXPORT(const char*) GetOptionListItemName(int optIndex, int itemIndex);
EXPORT(const char*) GetOptionListItemDesc(int optIndex, int itemIndex);

/**
 * A return value of 0 means that any map can be selected
 * Map names should be complete  (including the .smf or .sm3 extension)
 */
EXPORT(int) GetModValidMapCount();
EXPORT(const char*) GetModValidMap(int index);

// Map the wanted archives into the VFS with AddArchive before using these functions
EXPORT(int) OpenFileVFS(const char* name);
EXPORT(void) CloseFileVFS(int handle);
EXPORT(void) ReadFileVFS(int handle, void* buf, int length);
EXPORT(int) FileSizeVFS(int handle);
/**
 * Does not currently support more than one call at a time
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int) InitFindVFS(const char* pattern);
/**
 * Does not currently support more than one call at a time
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int) InitDirListVFS(const char* path, const char* pattern, const char* modes);
/**
 * Does not currently support more than one call at a time
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int) InitSubDirsVFS(const char* path, const char* pattern, const char* modes);
/**
 * On first call, pass handle from initfind. pass the return value of this
 * function on subsequent calls until 0 is returned.
 * size should be set to max namebuffer size on call.
 */
EXPORT(int) FindFilesVFS(int handle, char* nameBuf, int size);
/** returns 0 on error, a handle otherwise */
EXPORT(int) OpenArchive(const char* name);
/** returns 0 on error, a handle otherwise */
EXPORT(int) OpenArchiveType(const char* name, const char* type);
EXPORT(void) CloseArchive(int archive);
EXPORT(int) FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
EXPORT(int) OpenArchiveFile(int archive, const char* name);
EXPORT(int) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
EXPORT(void) CloseArchiveFile(int archive, int handle);
EXPORT(int) SizeArchiveFile(int archive, int handle);
/**
 * @brief get string from Spring configuration
 * @param name name of key to get
 * @param defvalue default string value to use if key is not found, may not be NULL
 * @return string value
 */
EXPORT(const char*) GetSpringConfigString( const char* name, const char* defValue);
/**
 * @brief get integer from Spring configuration
 * @param name name of key to get
 * @param defvalue default integer value to use if key is not found
 * @return integer value
 */
EXPORT(int) GetSpringConfigInt( const char* name, const int defValue);
/**
 * @brief get float from Spring configuration
 * @param name name of key to get
 * @param defvalue default float value to use if key is not found
 * @return float value
 */
EXPORT(float) GetSpringConfigFloat( const char* name, const float defValue);
/**
 * @brief set string in Spring configuration
 * @param name name of key to set
 * @param value string value to set
 */
EXPORT(void) SetSpringConfigString(const char* name, const char* value);
/**
 * @brief set integer in Spring configuration
 * @param name name of key to set
 * @param value integer value to set
 */
EXPORT(void) SetSpringConfigInt(const char* name, const int value);
/**
 * @brief set float in Spring configuration
 * @param name name of key to set
 * @param value float value to set
 */
EXPORT(void) SetSpringConfigFloat(const char* name, const float value);

#endif // UNITSYNC_H

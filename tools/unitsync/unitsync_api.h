#ifndef UNITSYNC_API_H
#define UNITSYNC_API_H

#ifdef PLAIN_API_STRUCTURE
	// This is useful when parsing/wrapping this file with preprocessor support.
	// Do NOT use this when compiling!
	#define EXPORT(type) type
	#warning PLAIN_API_STRUCTURE is defined -> functions will NOT be properly exported!
#else
	#include "unitsync.h"
	#include "System/exportdefines.h"
#endif


// from unitsync.cpp:

/** @addtogroup unitsync_api
	@{ */

/**
 * @brief Retrieve next error in queue of errors and removes this error from queue
 * @return An error message, or NULL if there are no more errors in the queue
 *
 * Use this method to get a (short) description of errors that occurred in any
 * other unitsync methods. Call this in a loop until it returns NULL to get all
 * errors.
 *
 * The error messages may be varying in detail etc.; nothing is guaranteed about
 * them, not even whether they have terminating newline or not.
 *
 * Example:
 *		@code
 *		const char* err;
 *		while ((err = GetNextError()) != NULL)
 *			printf("unitsync error: %s\n", err);
 *		@endcode
 */
EXPORT(const char* ) GetNextError();
/**
 * @brief Retrieve the version of Spring this unitsync was compiled with
 * @return The Spring/unitsync version string
 *
 * Returns a const char* string specifying the version of spring used to build this library with.
 * It was added to aid in lobby creation, where checks for updates to spring occur.
 */
EXPORT(const char* ) GetSpringVersion();

/**
 * @brief Initialize the unitsync library
 * @return Zero on error; non-zero on success
 *
 * Call this function before calling any other function in unitsync.
 * In case unitsync was already initialized, it is uninitialized and then
 * reinitialized.
 *
 * Calling this function is currently the only way to clear the VFS of the
 * files which are mapped into it.  In other words, after using AddArchive() or
 * AddAllArchives() you have to call Init when you want to remove the archives
 * from the VFS and start with a clean state.
 *
 * The config handler won't be reset, it will however be initialised if it wasn't before (with SetSpringConfigFile())
 */
EXPORT(int         ) Init(bool isServer, int id, bool enable_logging = true);
/**
 * @brief Uninitialize the unitsync library
 *
 * also resets the config handler
 */
EXPORT(void        ) UnInit();

/**
 * @brief Get the main data directory that's used by unitsync and Spring
 * @return NULL on error; the data directory path on success
 *
 * This is the data directory which is used to write logs, screenshots, demos, etc.
 */
EXPORT(const char* ) GetWritableDataDirectory();

/**
 * @brief Process another unit and return how many are left to process
 * @return The number of unprocessed units to be handled
 *
 * Call this function repeatedly until it returns 0 before calling any other
 * function related to units.
 *
 * Because of risk for infinite loops, this function can not return any error code.
 * It is advised to poll GetNextError() after calling this function.
 *
 * Before any units are available, you'll first need to map a mod's archives
 * into the VFS using AddArchive() or AddAllArchives().
 */
EXPORT(int         ) ProcessUnits();
/**
 * @brief Identical to ProcessUnits(), neither generates checksum anymore
 * @see ProcessUnits
 */
EXPORT(int         ) ProcessUnitsNoChecksum();

/**
 * @brief Get the number of units
 * @return Zero on error; the number of units available on success
 *
 * Will return the number of units. Remember to call ProcessUnits() beforehand
 * until it returns 0.  As ProcessUnits() is called the number of processed
 * units goes up, and so will the value returned by this function.
 *
 * Example:
 *		@code
 *		while (ProcessUnits() != 0) {}
 *		int unit_number = GetUnitCount();
 *		@endcode
 */
EXPORT(int         ) GetUnitCount();
/**
 * @brief Get the units internal mod name
 * @param unit The units id number
 * @return The units internal modname or NULL on error
 *
 * This function returns the units internal mod name. For example it would
 * return 'armck' and not 'Arm Construction kbot'.
 */
EXPORT(const char* ) GetUnitName(int unit);
/**
 * @brief Get the units human readable name
 * @param unit The units id number
 * @return The units human readable name or NULL on error
 *
 * This function returns the units human name. For example it would return
 * 'Arm Construction kbot' and not 'armck'.
 */
EXPORT(const char* ) GetFullUnitName(int unit);

/**
 * @brief Adds an archive to the VFS (Virtual File System)
 *
 * After this, the contents of the archive are available to other unitsync functions,
 * for example: ProcessUnits(), OpenFileVFS(), ReadFileVFS(), FileSizeVFS(), etc.
 *
 * Don't forget to call RemoveAllArchives() before proceeding with other archives.
 */
EXPORT(void        ) AddArchive(const char* name);
/**
 * @brief Adds an achive and all it's dependencies to the VFS
 * @see AddArchive
 */
EXPORT(void        ) AddAllArchives(const char* root);
/**
 * @brief Removes all archives from the VFS (Virtual File System)
 *
 * After this, the contents of the archives are not available to other unitsync functions anymore,
 * for example: ProcessUnits(), OpenFileVFS(), ReadFileVFS(), FileSizeVFS(), etc.
 *
 * In a lobby client, this may be used instead of Init() when switching mod archive.
 */
EXPORT(void        ) RemoveAllArchives();
/**
 * @brief Get checksum of an archive
 * @return Zero on error; the checksum on success
 *
 * This checksum depends only on the contents from the archive itself, and not
 * on the contents from dependencies of this archive (if any).
 */
EXPORT(unsigned int) GetArchiveChecksum(const char* arname);
/**
 * @brief Gets the real path to the archive
 * @return NULL on error; a path to the archive on success
 */
EXPORT(const char* ) GetArchivePath(const char* arname);

#if       !defined(PLAIN_API_STRUCTURE)
/**
 * @brief Retrieve map info
 * @param mapName name of the map, e.g. "SmallDivide"
 * @param outInfo pointer to structure which is filled with map info
 * @param version this determines which fields of the MapInfo structure are filled
 * @return Zero on error; non-zero on success
 * @deprecated
 * @see GetMapCount
 *
 * If version >= 1, then the author field is filled.
 *
 * Important: the description and author fields must point to a valid, and sufficiently long buffer
 * to store their contents.  Description is max 255 chars, and author is max 200 chars. (including
 * terminating zero byte).
 *
 * If an error occurs (return value 0), the description is set to an error message.
 * However, using GetNextError() is the recommended way to get the error message.
 *
 * Example:
 *		@code
 *		char description[255];
 *		char author[200];
 *		MapInfo mi;
 *		mi.description = description;
 *		mi.author = author;
 *		if (GetMapInfoEx("somemap.smf", &mi, 1)) {
 *			// now mi contains map data
 *		} else {
 *			// handle the error
 *		}
 *		@endcode
 */
EXPORT(int         ) GetMapInfoEx(const char* mapName, MapInfo* outInfo, int version);
/**
 * @brief Retrieve map info, equivalent to GetMapInfoEx(name, outInfo, 0)
 * @param mapName name of the map, e.g. "SmallDivide"
 * @param outInfo pointer to structure which is filled with map info
 * @return Zero on error; non-zero on success
 * @deprecated
 * @see GetMapCount
 */
EXPORT(int         ) GetMapInfo(const char* mapName, MapInfo* outInfo);
#endif // !defined(PLAIN_API_STRUCTURE)

/**
 * @brief Get the number of maps available
 * @return Zero on error; the number of maps available on success
 *
 * Call this before any of the map functions which take a map index as parameter.
 * This function actually performs a relatively costly enumeration of all maps,
 * so you should resist from calling it repeatedly in a loop.  Rather use:
 *		@code
 *		int map_count = GetMapCount();
 *		for (int index = 0; index < map_count; ++index) {
 *			printf("map name: %s\n", GetMapName(index));
 *		}
 *		@endcode
 * Then:
 *		@code
 *		for (int index = 0; index < GetMapCount(); ++index) { ... }
 *		@endcode
 */
EXPORT(int         ) GetMapCount();
/**
 * @brief Get the name of a map
 * @return NULL on error; the name of the map (e.g. "SmallDivide") on success
 */
EXPORT(const char* ) GetMapName(int index);
/**
 * @brief Get the filename of a map
 * @return NULL on error; the filename of the map (e.g. "maps/SmallDivide.smf") on success
 */
EXPORT(const char* ) GetMapFileName(int index);
/**
 * @brief Get the description of a map
 * @return NULL on error; the description of the map
 *         (e.g. "Lot of metal in middle") on success
 */
EXPORT(const char* ) GetMapDescription(int index);
/**
 * @brief Get the name of the author of a map
 * @return NULL on error; the name of the author of a map on success
 */
EXPORT(const char* ) GetMapAuthor(int index);
/**
 * @brief Get the width of a map
 * @return -1 on error; the width of a map
 */
EXPORT(int         ) GetMapWidth(int index);
/**
 * @brief Get the height of a map
 * @return -1 on error; the height of a map
 */
EXPORT(int         ) GetMapHeight(int index);
/**
 * @brief Get the tidal speed of a map
 * @return -1 on error; the tidal speed of the map on success
 */
EXPORT(int         ) GetMapTidalStrength(int index);
/**
 * @brief Get the minimum wind speed on a map
 * @return -1 on error; the minimum wind speed on a map
 */
EXPORT(int         ) GetMapWindMin(int index);
/**
 * @brief Get the maximum wind strenght on a map
 * @return -1 on error; the maximum wind strenght on a map
 */
EXPORT(int         ) GetMapWindMax(int index);
/**
 * @brief Get the gravity of a map
 * @return -1 on error; the gravity of the map on success
 */
EXPORT(int         ) GetMapGravity(int index);
/**
 * @brief Get the number of resources supported available
 * @return -1 on error; the number of resources supported available on success
 */
EXPORT(int         ) GetMapResourceCount(int index);
/**
 * @brief Get the name of a map resource
 * @return NULL on error; the name of a map resource (e.g. "Metal") on success
 */
EXPORT(const char* ) GetMapResourceName(int index, int resourceIndex);
/**
 * @brief Get the scale factor of a resource map
 * @return 0.0f on error; the scale factor of a resource map on success
 */
EXPORT(float       ) GetMapResourceMax(int index, int resourceIndex);
/**
 * @brief Get the extractor radius for a map resource
 * @return -1 on error; the extractor radius for a map resource on success
 */
EXPORT(int         ) GetMapResourceExtractorRadius(int index, int resourceIndex);

/**
 * @brief Get the number of defined start positions for a map
 * @return -1 on error; the number of defined start positions for a map
 *         on success
 */
EXPORT(int         ) GetMapPosCount(int index);
/**
 * @brief Get the position on the x-axis for a start position on a map
 * @return -1.0f on error; the position on the x-axis for a start position
 *         on a map on success
 */
EXPORT(float       ) GetMapPosX(int index, int posIndex);
/**
 * @brief Get the position on the z-axis for a start position on a map
 * @return -1.0f on error; the position on the z-axis for a start position
 *         on a map on success
 */
EXPORT(float       ) GetMapPosZ(int index, int posIndex);

/**
 * @brief return the map's minimum height
 * @param mapName name of the map, e.g. "SmallDivide"
 *
 * Together with maxHeight, this determines the
 * range of the map's height values in-game. The
 * conversion formula for any raw 16-bit height
 * datum <code>h</code> is
 *
 *    <code>minHeight + (h * (maxHeight - minHeight) / 65536.0f)</code>
 */
EXPORT(float       ) GetMapMinHeight(const char* mapName);
/**
 * @brief return the map's maximum height
 * @param mapName name of the map, e.g. "SmallDivide"
 *
 * Together with minHeight, this determines the
 * range of the map's height values in-game. See
 * GetMapMinHeight() for the conversion formula.
 */
EXPORT(float       ) GetMapMaxHeight(const char* mapName);

/**
 * @brief Retrieves the number of archives a map requires
 * @param mapName name of the map, e.g. "SmallDivide"
 * @return Zero on error; the number of archives on success
 *
 * Must be called before GetMapArchiveName()
 */
EXPORT(int         ) GetMapArchiveCount(const char* mapName);
/**
 * @brief Retrieves an archive a map requires
 * @param index the index of the archive
 * @return NULL on error; the name of the archive on success
 */
EXPORT(const char* ) GetMapArchiveName(int index);

/**
 * @brief Get map checksum given a map index
 * @param index the index of the map
 * @return Zero on error; the checksum on success
 *
 * This checksum depends on Spring internals, and as such should not be expected
 * to remain stable between releases.
 *
 * (It is ment to check sync between clients in lobby, for example.)
 */
EXPORT(unsigned int) GetMapChecksum(int index);
/**
 * @brief Get map checksum given a map name
 * @param mapName name of the map, e.g. "SmallDivide"
 * @return Zero on error; the checksum on success
 * @see GetMapChecksum
 */
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
 * bitmap on success; NULL on error.
 *
 * An example usage would be GetMinimap("SmallDivide", 2).
 * This would return a 16 bit packed RGB-565 256x256 (= 1024/2^2) bitmap.
 */
EXPORT(unsigned short*) GetMinimap(const char* filename, int miplevel);
/**
 * @brief Retrieves dimensions of infomap for a map.
 * @param mapName  The name of the map, e.g. "SmallDivide".
 * @param name     Of which infomap to retrieve the dimensions.
 * @param width    This is set to the width of the infomap, or 0 on error.
 * @param height   This is set to the height of the infomap, or 0 on error.
 * @return Non-zero when the infomap was found with a non-zero size; zero on error.
 * @see GetInfoMap
 */
EXPORT(int         ) GetInfoMapSize(const char* mapName, const char* name, int* width, int* height);
/**
 * @brief Retrieves infomap data of a map.
 * @param mapName  The name of the map, e.g. "SmallDivide".
 * @param name     Which infomap to extract from the file.
 * @param data     Pointer to memory location with enough room to hold the infomap data.
 * @param typeHint One of bm_grayscale_8 (or 1) and bm_grayscale_16 (or 2).
 * @return Non-zero if the infomap was successfully extracted (and optionally
 * converted), or zero on error (map wasn't found, infomap wasn't found, or
 * typeHint could not be honoured.)
 *
 * This function extracts an infomap from a map. This can currently be one of:
 * "height", "metal", "grass", "type". The heightmap is natively in 16 bits per
 * pixel, the others are in 8 bits pixel. Using typeHint one can give a hint to
 * this function to convert from one format to another. Currently only the
 * conversion from 16 bpp to 8 bpp is implemented.
 */
EXPORT(int         ) GetInfoMap(const char* mapName, const char* name, unsigned char* data, int typeHint);

// TODO documentation
EXPORT(int         ) GetSkirmishAICount();
// TODO documentation
EXPORT(int         ) GetSkirmishAIInfoCount(int index);
// TODO documentation
EXPORT(const char* ) GetInfoKey(int index);
// TODO documentation
EXPORT(const char* ) GetInfoValue(int index);
// TODO documentation
EXPORT(const char* ) GetInfoDescription(int index);
// TODO documentation
EXPORT(int         ) GetSkirmishAIOptionCount(int index);

/**
 * @brief Retrieves the number of mods available
 * @return int Zero on error; The number of mods available on success
 * @see GetMapCount
 */
EXPORT(int         ) GetPrimaryModCount();
/**
 * @brief Retrieves the name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods name on success
 *
 * Returns the name of the mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModName(int index);
/**
 * @brief Retrieves the shortened name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods abbrieviated name on success
 *
 * Returns the shortened name of the mod usually found in modinfo.tdf.
 * Be sure you've made a call GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModShortName(int index);
/**
 * @brief Retrieves the version string of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods version string on success
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModVersion(int index);
/**
 * @brief Retrieves the mutator name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods mutator name on success
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModMutator(int index);
/**
 * @brief Retrieves the game name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods game name on success
 *
 * Returns the name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModGame(int index);
/**
 * @brief Retrieves the short game name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods abbrieviated game name on success
 *
 * Returns the abbrieviated name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModShortGame(int index);
/**
 * @brief Retrieves the description of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods description on success
 *
 * Returns a description for the specified mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModDescription(int index);
/**
 * @brief Retrieves the mod's first/primary archive
 * @param index The mods index/id
 * @return NULL on error; The mods primary archive on success
 *
 * Returns the name of the primary archive of the mod.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char* ) GetPrimaryModArchive(int index);
/**
 * @brief Retrieves the number of archives a mod requires
 * @param index The index of the mod
 * @return Zero on error; the number of archives this mod depends on otherwise
 *
 * This is used to get the entire list of archives that a mod requires.
 * Call GetPrimaryModArchiveCount() with selected mod first to get number of
 * archives, and then use GetPrimaryModArchiveList() for 0 to count-1 to get the
 * name of each archive.  In code:
 *		@code
 *		int count = GetPrimaryModArchiveCount(mod_index);
 *		for (int arnr = 0; arnr < count; ++arnr) {
 *			printf("primary mod archive: %s\n", GetPrimaryModArchiveList(arnr));
 *		}
 *		@endcode
 */
EXPORT(int         ) GetPrimaryModArchiveCount(int index);
/**
 * @brief Retrieves the name of the current mod's archive.
 * @param archiveNr The archive's index/id.
 * @return NULL on error; the name of the archive on success
 * @see GetPrimaryModArchiveCount
 */
EXPORT(const char* ) GetPrimaryModArchiveList(int arnr);
/**
 * @brief The reverse of GetPrimaryModName()
 * @param name The name of the mod
 * @return -1 if the mod can not be found; the index of the mod otherwise
 */
EXPORT(int         ) GetPrimaryModIndex(const char* name);
/**
 * @brief Get checksum of mod
 * @param index The mods index/id
 * @return Zero on error; the checksum on success.
 * @see GetMapChecksum
 */
EXPORT(unsigned int) GetPrimaryModChecksum(int index);
/**
 * @brief Get checksum of mod given the mod's name
 * @param name The name of the mod
 * @return Zero on error; the checksum on success.
 * @see GetMapChecksum
 */
EXPORT(unsigned int) GetPrimaryModChecksumFromName(const char* name);

/**
 * @brief Retrieve the number of available sides
 * @return Zero on error; the number of sides on success
 *
 * This function parses the mod's side data, and returns the number of sides
 * available. Be sure to map the mod into the VFS using AddArchive() or
 * AddAllArchives() prior to using this function.
 */
EXPORT(int         ) GetSideCount();
/**
 * @brief Retrieve a side's name
 * @return NULL on error; the side's name on success
 *
 * Be sure you've made a call to GetSideCount() prior to using this.
 */
EXPORT(const char* ) GetSideName(int side);
/**
 * @brief Retrieve a side's default starting unit
 * @return NULL on error; the side's starting unit name on success
 *
 * Be sure you've made a call to GetSideCount() prior to using this.
 */
EXPORT(const char* ) GetSideStartUnit(int side);

/**
 * @brief Retrieve the number of map options available
 * @param mapName  the name of the map, e.g. "SmallDivide"
 * @return Zero on error; the number of map options available on success
 */
EXPORT(int         ) GetMapOptionCount(const char* mapName);
/**
 * @brief Retrieve the number of mod options available
 * @return Zero on error; the number of mod options available on success
 *
 * Be sure to map the mod into the VFS using AddArchive() or AddAllArchives()
 * prior to using this function.
 */
EXPORT(int         ) GetModOptionCount();
// TODO documentation
EXPORT(int         ) GetCustomOptionCount(const char* filename);
/**
 * @brief Retrieve an option's key
 * @param optIndex option index/id
 * @return NULL on error; the option's key on success
 *
 * The key of an option is the name it should be given in the start script's
 * MODOPTIONS or MAPOPTIONS section.
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionKey(int optIndex);
/**
 * @brief Retrieve an option's scope
 * @param optIndex option index/id
 * @return NULL on error; the option's scope on success
 *
 * Will be either "global" (default), "player", "team" or "allyteam"
 */
EXPORT(const char* ) GetOptionScope(int optIndex);
/**
 * @brief Retrieve an option's name
 * @param optIndex option index/id
 * @return NULL on error; the option's user visible name on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionName(int optIndex);
/**
 * @brief Retrieve an option's section
 * @param optIndex option index/id
 * @return NULL on error; the option's section name on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionSection(int optIndex);
/**
 * @brief Retrieve an option's style
 * @param optIndex option index/id
 * @return NULL on error; the option's style on success
 *
 * The format of an option style string is currently undecided.
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionStyle(int optIndex);
/**
 * @brief Retrieve an option's description
 * @param optIndex option index/id
 * @return NULL on error; the option's description on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionDesc(int optIndex);
/**
 * @brief Retrieve an option's type
 * @param optIndex option index/id
 * @return opt_error on error; the option's type on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int         ) GetOptionType(int optIndex);

/**
 * @brief Retrieve an opt_bool option's default value
 * @param optIndex option index/id
 * @return Zero on error; the option's default value (0 or 1) on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int         ) GetOptionBoolDef(int optIndex);

/**
 * @brief Retrieve an opt_number option's default value
 * @param optIndex option index/id
 * @return Zero on error; the option's default value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float       ) GetOptionNumberDef(int optIndex);
/**
 * @brief Retrieve an opt_number option's minimum value
 * @param optIndex option index/id
 * @return -1.0e30 on error; the option's minimum value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float       ) GetOptionNumberMin(int optIndex);
/**
 * @brief Retrieve an opt_number option's maximum value
 * @param optIndex option index/id
 * @return +1.0e30 on error; the option's maximum value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float       ) GetOptionNumberMax(int optIndex);
/**
 * @brief Retrieve an opt_number option's step value
 * @param optIndex option index/id
 * @return Zero on error; the option's step value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float       ) GetOptionNumberStep(int optIndex);

/**
 * @brief Retrieve an opt_string option's default value
 * @param optIndex option index/id
 * @return NULL on error; the option's default value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionStringDef(int optIndex);
/**
 * @brief Retrieve an opt_string option's maximum length
 * @param optIndex option index/id
 * @return Zero on error; the option's maximum length on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int         ) GetOptionStringMaxLen(int optIndex);

/**
 * @brief Retrieve an opt_list option's number of available items
 * @param optIndex option index/id
 * @return Zero on error; the option's number of available items on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int         ) GetOptionListCount(int optIndex);
/**
 * @brief Retrieve an opt_list option's default value
 * @param optIndex option index/id
 * @return NULL on error; the option's default value (list item key) on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionListDef(int optIndex);
/**
 * @brief Retrieve an opt_list option item's key
 * @param optIndex option index/id
 * @param itemIndex list item index/id
 * @return NULL on error; the option item's key (list item key) on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionListItemKey(int optIndex, int itemIndex);
/**
 * @brief Retrieve an opt_list option item's name
 * @param optIndex option index/id
 * @param itemIndex list item index/id
 * @return NULL on error; the option item's name on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionListItemName(int optIndex, int itemIndex);
/**
 * @brief Retrieve an opt_list option item's description
 * @param optIndex option index/id
 * @param itemIndex list item index/id
 * @return NULL on error; the option item's description on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char* ) GetOptionListItemDesc(int optIndex, int itemIndex);

/**
 * @brief Retrieve the number of valid maps for the current mod
 * @return 0 on error; the number of valid maps on success
 *
 * A return value of 0 means that any map can be selected.
 * Be sure to map the mod into the VFS using AddArchive() or AddAllArchives()
 * prior to using this function.
 */
EXPORT(int         ) GetModValidMapCount();
/**
 * @brief Retrieve the name of a map valid for the current mod
 * @return NULL on error; the name of the map on success
 *
 * Map names should be complete  (including the .smf or .sm3 extension.)
 * Be sure you've made a call to GetModValidMapCount() prior to using this.
 */
EXPORT(const char* ) GetModValidMap(int index);

/**
 * @brief Open a file from the VFS
 * @param name the name of the file
 * @return Zero on error; a non-zero file handle on success.
 *
 * The returned file handle is needed for subsequent calls to CloseFileVFS(),
 * ReadFileVFS() and FileSizeVFS().
 *
 * Map the wanted archives into the VFS with AddArchive() or AddAllArchives()
 * before using this function.
 */
EXPORT(int         ) OpenFileVFS(const char* name);
/**
 * @brief Close a file in the VFS
 * @param handle the file handle as returned by OpenFileVFS()
 */
EXPORT(void        ) CloseFileVFS(int handle);
/**
 * @brief Read some data from a file in the VFS
 * @param handle the file handle as returned by OpenFileVFS()
 * @param buf output buffer, must be at least length bytes
 * @param length how many bytes to read from the file
 * @return -1 on error; the number of bytes read on success
 * (if this is less than length you reached the end of the file.)
 */
EXPORT(int         ) ReadFileVFS(int handle, unsigned char* buf, int length);
/**
 * @brief Retrieve size of a file in the VFS
 * @param handle the file handle as returned by OpenFileVFS()
 * @return -1 on error; the size of the file on success
 */
EXPORT(int         ) FileSizeVFS(int handle);

/**
 * Does not currently support more than one call at a time.
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int         ) InitFindVFS(const char* pattern);
/**
 * Does not currently support more than one call at a time.
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int         ) InitDirListVFS(const char* path, const char* pattern, const char* modes);
/**
 * Does not currently support more than one call at a time.
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int         ) InitSubDirsVFS(const char* path, const char* pattern, const char* modes);
/**
 * Find the next file.
 * On first call, pass handle from InitFindVFS().
 * On subsequent calls, pass the return value of this function.
 * @param size should be set to max namebuffer size on call
 * @return new handle or 0
 */
EXPORT(int         ) FindFilesVFS(int handle, char* nameBuf, int size);

/**
 * @brief Open an archive
 * @param name the name of the archive (*.sdz, *.sd7, ...)
 * @return Zero on error; a non-zero archive handle on success.
 * @sa OpenArchiveType
 */
EXPORT(int         ) OpenArchive(const char* name);
/**
 * @brief Open an archive
 * @param name the name of the archive (*.sd7, *.sdz, *.sdd, *.ccx, *.hpi, *.ufo, *.gp3, *.gp4, *.swx)
 * @param type the type of the archive (sd7, 7z, sdz, zip, sdd, dir, ccx, hpi, ufo, gp3, gp4, swx)
 * @return Zero on error; a non-zero archive handle on success.
 * @sa OpenArchive
 *
 * The list of supported types and recognized extensions may change at any time.
 * (But this list will always be the same as the file types recognized by the engine.)
 *
 * This function is pointless, because OpenArchive() does the same and automatically
 * detects the file type based on it's extension.  Who added it anyway?
 */
EXPORT(int         ) OpenArchiveType(const char* name, const char* type);
/**
 * @brief Close an archive in the VFS
 * @param archive the archive handle as returned by OpenArchive()
 */
EXPORT(void        ) CloseArchive(int archive);
// TODO documentation
EXPORT(int         ) FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
/**
 * @brief Open an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param name the name of the file
 * @return Zero on error; a non-zero file handle on success.
 *
 * The returned file handle is needed for subsequent calls to ReadArchiveFile(),
 * CloseArchiveFile() and SizeArchiveFile().
 */
EXPORT(int         ) OpenArchiveFile(int archive, const char* name);
/**
 * @brief Read some data from an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param handle the file handle as returned by OpenArchiveFile()
 * @param buffer output buffer, must be at least numBytes bytes
 * @param numBytes how many bytes to read from the file
 * @return -1 on error; the number of bytes read on success
 * (if this is less than numBytes you reached the end of the file.)
 */
EXPORT(int         ) ReadArchiveFile(int archive, int handle, unsigned char* buffer, int numBytes);
/**
 * @brief Close an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param handle the file handle as returned by OpenArchiveFile()
 */
EXPORT(void        ) CloseArchiveFile(int archive, int handle);
/**
 * @brief Retrieve size of an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param handle the file handle as returned by OpenArchiveFile()
 * @return -1 on error; the size of the file on success
 */
EXPORT(int         ) SizeArchiveFile(int archive, int handle);

// TODO documentation
EXPORT(void        ) SetSpringConfigFile(const char* filenameAsAbsolutePath);
// TODO documentation
EXPORT(const char* ) GetSpringConfigFile();
/**
 * @brief get string from Spring configuration
 * @param name name of key to get
 * @param defValue default string value to use if key is not found, may not be NULL
 * @return string value
 */
EXPORT(const char* ) GetSpringConfigString(const char* name, const char* defvalue);
/**
 * @brief get integer from Spring configuration
 * @param name name of key to get
 * @param defValue default integer value to use if key is not found
 * @return integer value
 */
EXPORT(int         ) GetSpringConfigInt(const char* name, const int defvalue);
/**
 * @brief get float from Spring configuration
 * @param name name of key to get
 * @param defValue default float value to use if key is not found
 * @return float value
 */
EXPORT(float       ) GetSpringConfigFloat( const char* name, const float defvalue );
/**
 * @brief set string in Spring configuration
 * @param name name of key to set
 * @param value string value to set
 */
EXPORT(void        ) SetSpringConfigString(const char* name, const char* value);
/**
 * @brief set integer in Spring configuration
 * @param name name of key to set
 * @param value integer value to set
 */
EXPORT(void        ) SetSpringConfigInt(const char* name, const int value);
/**
 * @brief set float in Spring configuration
 * @param name name of key to set
 * @param value float value to set
 */
EXPORT(void        ) SetSpringConfigFloat(const char* name, const float value);


// from LuaParserAPI.cpp:

EXPORT(void       ) lpClose();
EXPORT(int        ) lpOpenFile(const char* filename, const char* fileModes, const char* accessModes);
EXPORT(int        ) lpOpenSource(const char* source, const char* accessModes);
EXPORT(int        ) lpExecute();
EXPORT(const char*) lpErrorLog();

EXPORT(void       ) lpAddTableInt(int key, int override);
EXPORT(void       ) lpAddTableStr(const char* key, int override);
EXPORT(void       ) lpEndTable();
EXPORT(void       ) lpAddIntKeyIntVal(int key, int val);
EXPORT(void       ) lpAddStrKeyIntVal(const char* key, int val);
EXPORT(void       ) lpAddIntKeyBoolVal(int key, int val);
EXPORT(void       ) lpAddStrKeyBoolVal(const char* key, int val);
EXPORT(void       ) lpAddIntKeyFloatVal(int key, float val);
EXPORT(void       ) lpAddStrKeyFloatVal(const char* key, float val);
EXPORT(void       ) lpAddIntKeyStrVal(int key, const char* val);
EXPORT(void       ) lpAddStrKeyStrVal(const char* key, const char* val);

EXPORT(int        ) lpRootTable();
EXPORT(int        ) lpRootTableExpr(const char* expr);
EXPORT(int        ) lpSubTableInt(int key);
EXPORT(int        ) lpSubTableStr(const char* key);
EXPORT(int        ) lpSubTableExpr(const char* expr);
EXPORT(void       ) lpPopTable();

EXPORT(int        ) lpGetKeyExistsInt(int key);
EXPORT(int        ) lpGetKeyExistsStr(const char* key);

EXPORT(int        ) lpGetIntKeyType(int key);
EXPORT(int        ) lpGetStrKeyType(const char* key);

EXPORT(int        ) lpGetIntKeyListCount();
EXPORT(int        ) lpGetIntKeyListEntry(int index);
EXPORT(int        ) lpGetStrKeyListCount();
EXPORT(const char*) lpGetStrKeyListEntry(int index);

EXPORT(int        ) lpGetIntKeyIntVal(int key, int defVal);
EXPORT(int        ) lpGetStrKeyIntVal(const char* key, int defVal);
EXPORT(int        ) lpGetIntKeyBoolVal(int key, int defVal);
EXPORT(int        ) lpGetStrKeyBoolVal(const char* key, int defVal);
EXPORT(float      ) lpGetIntKeyFloatVal(int key, float defVal);
EXPORT(float      ) lpGetStrKeyFloatVal(const char* key, float defVal);
EXPORT(const char*) lpGetIntKeyStrVal(int key, const char* defVal);
EXPORT(const char*) lpGetStrKeyStrVal(const char* key, const char* defVal);

/** @} */

#endif // UNITSYNC_API_H

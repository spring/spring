/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_AI_CALLBACK_H
#define I_AI_CALLBACK_H

#include "ExternalAI/AILegacySupport.h"
#include "Sim/Misc/GlobalConstants.h" // needed for MAX_UNITS
#include "Sim/Units/CommandAI/Command.h"
#include "System/float3.h"

#include <string>
#include <vector>
#include <map>


struct SCommandDescription;

namespace springLegacyAI {

class CCommandQueue;
struct UnitDef;
struct FeatureDef;
struct WeaponDef;

/// Generalized callback interface, used by Global AIs
class IAICallback
{
public:
	virtual void SendTextMsg(const char* text, int zone) = 0;
	virtual void SetLastMsgPos(float3 pos) = 0;
	virtual void AddNotification(float3 pos, float3 color, float alpha) = 0;

	/**
	 * Give \<mAmount\> of metal and \<eAmount\> of energy to \<receivingTeam\>
	 * - the amounts are capped to the AI team's resource levels
	 * - does not check for alliance with \<receivingTeam\>
	 * - LuaRules might not allow resource transfers,
	 *   AI's must verify the deduction
	 */
	virtual bool SendResources(float mAmount, float eAmount, int receivingTeam) = 0;
	/**
	 * Give units to \<receivingTeam\>, the return value represents how many
	 * actually were transferred (make sure this always matches the size of
	 * the vector you pass in, if not then some unitID's were filtered out)
	 * - does not check for alliance with \<receivingTeam\>
	 * - AI's should check each unit if it is still under control of their
	 *   team after the transaction via UnitTaken() and UnitGiven(), since
	 *   LuaRules might block part of it
	 */
	virtual int SendUnits(const std::vector<int>& unitIds, int receivingTeamId) = 0;

	/**
	 * Checks if pos is within view of the current camera,
	 * using radius as a margin.
	 */
	virtual bool PosInCamera(float3 pos, float radius) = 0;

	/**
	 * Returns the current game time measured in frames (the
	 * simulation runs at 30 frames per second at normal
	 * speed)
	 */
	virtual int GetCurrentFrame() = 0;

	virtual int GetMySkirmishAIId() = 0;
	virtual int GetMyTeam() = 0;
	virtual int GetMyAllyTeam() = 0;
	virtual int GetPlayerTeam(int playerId) = 0;
	/**
	 * Returns the number of active teams participating
	 * in the currently running game.
	 * A return value of 6 for example, would mean that teams 0 till 5
	 * take part in the game.
	 */
	virtual int GetTeams() = 0;
	/**
	 * Returns the name of the side of a team in the game.
	 *
	 * This should not be used, as it may be "",
	 * and as the AI should rather rely on the units it has,
	 * which will lead to a more stable and versatile AI.
	 * @deprecated
	 *
	 * @return eg. "ARM" or "CORE"; may be "",
	 *         depending on how the game was setup
	 */
	virtual const char* GetTeamSide(int teamId) = 0;
	virtual int GetTeamAllyTeam(int teamId) = 0;

	virtual float GetTeamMetalCurrent(int teamId) = 0;
	virtual float GetTeamMetalIncome(int teamId) = 0;
	virtual float GetTeamMetalUsage(int teamId) = 0;
	virtual float GetTeamMetalStorage(int teamId) = 0;

	virtual float GetTeamEnergyCurrent(int teamId) = 0;
	virtual float GetTeamEnergyIncome(int teamId) = 0;
	virtual float GetTeamEnergyUsage(int teamId) = 0;
	virtual float GetTeamEnergyStorage(int teamId) = 0;

	/// Returns true, if the two supplied ally-teams are currently allied
	virtual bool IsAllied(int firstAllyTeamId, int secondAllyTeamId) = 0;

	/**
	 * Returns the size of the created area, this is initialized to all 0 if not
	 * previously created set something to !0 to tell other AI's that the area
	 * is already initialized when they try to create it (the exact internal
	 * format of the memory areas is up to the AI's to keep consistent).
	 * @deprecated
	 */
	virtual void* CreateSharedMemArea(char* name, int size) = 0;
	/**
	 * Releases your reference to a memory area.
	 * @deprecated
	 */
	virtual void ReleasedSharedMemArea(char* name) = 0;

	/// Creates a group and returns the id it was given, returns -1 on failure
	virtual int CreateGroup() = 0;
	/// Erases a specified group
	virtual void EraseGroup(int groupId) = 0;
	/**
	 * @brief Adds a unit to a specific group.
	 * If it was previously in a group, it is removed from that.
	 * Returns false if the group did not exist or did not accept the unit.
	 */
	virtual bool AddUnitToGroup(int unitId, int groupId) = 0;
	/// Removes a unit from its group
	virtual bool RemoveUnitFromGroup(int unitId) = 0;
	/// Returns the group a unit belongs to, -1 if none
	virtual int GetUnitGroup(int unitId) = 0;
	/**
	 * The commands that this group can understand, other commands will be
	 * ignored.
	 */
	virtual const std::vector<SCommandDescription>* GetGroupCommands(int groupId) = 0;
	virtual int GiveGroupOrder(int unitId, Command* c) = 0;

	virtual int GiveOrder(int unitId, Command* c) = 0;

	/**
	 * The commands that this unit can understand, other commands will be
	 * ignored.
	 */
	virtual const std::vector<SCommandDescription>* GetUnitCommands(int unitId) = 0;
	virtual const CCommandQueue* GetCurrentUnitCommands(int unitId) = 0;

	/*
	 * These functions always work on allied units, but for
	 * enemies only when you have LOS on them (so watch out
	 * when calling GetUnitDef)
	 */

	/**
	 * Returns the maximum number of units to be in-game.
	 * The maximum unit-ID is this vaue -1.
	 */
	virtual int GetMaxUnits() = 0;

	virtual int GetUnitTeam(int unitId) = 0;
	virtual int GetUnitAllyTeam(int unitId) = 0;
	/// the unit's current health
	virtual float GetUnitHealth(int unitId) = 0;
	/// the unit's maximum health
	virtual float GetUnitMaxHealth(int unitId) = 0;
	/// the unit's maximum speed
	virtual float GetUnitSpeed(int unitId) = 0;
	/// sort of the measure of the units overall power
	virtual float GetUnitPower(int unitId) = 0;
	/// how experienced the unit is (0.0f-1.0f)
	virtual float GetUnitExperience(int unitId) = 0;
	/// the furthest any weapon of the unit can fire
	virtual float GetUnitMaxRange(int unitId) = 0;
	virtual bool IsUnitActivated (int unitId) = 0;
	/// true if the unit is currently being built
	virtual bool UnitBeingBuilt(int unitId) = 0;
	/**
	 * Returns the unit's unitdef struct from which you can read all
	 * the statistics of the unit, do NOT try to change any values in it.
	 */
	virtual const UnitDef* GetUnitDef(int unitId) = 0;

	virtual float3 GetUnitPos(int unitId) = 0; //! current unit position vector
	virtual float3 GetUnitVel(int unitId) = 0; //! current unit velocity vector

	/// returns the unit's build facing (0-3)
	virtual int GetBuildingFacing(int unitId) = 0;
	virtual bool IsUnitCloaked(int unitId) = 0;
	virtual bool IsUnitParalyzed(int unitId) = 0;
	virtual bool IsUnitNeutral(int unitId) = 0;
	virtual bool GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) = 0;

	virtual const UnitDef* GetUnitDef(const char* unitName) = 0;

	/**
	 * the following functions allow the dll to use the built-in pathfinder
	 * - call InitPath and you get a pathid back
	 * - use this to call GetNextWaypoint to get subsequent waypoints,
	 *   the waypoints are centered on 8*8 squares
	 * - note that the pathfinder calculates the waypoints as needed so do not
	 *   retrieve them until they are needed
	 * - the waypoint's x and z coordinates are returned in x and z while y is
	 *   used for status codes
	 * y =  0: legal path waypoint IFF x >= 0 and z >= 0
	 * y = -1: temporary waypoint, path not yet available
	 */
	virtual int InitPath(float3 start, float3 end, int pathType, float goalRadius = 8) = 0;
	virtual float3 GetNextWaypoint(int pathId) = 0;
	virtual void FreePath(int pathId) = 0;

	/**
	 * returns the approximate path cost between two points (note that
	 * it needs to calculate the complete path so somewhat expensive)
	 */
	virtual float GetPathLength(float3 start, float3 end, int pathType, float goalRadius = 8) = 0;

	/*
	 * The following function return the units into arrays that must be
	 * allocated by the dll
	 * - 30000 is currently the max amount of units (June 2011) so that should
	 *   be a safe size for the array
	 * - the return value indicates how many units were returned, the rest of
	 *   the array is unchanged
	 * - all forms of GetEnemyUnits and GetFriendlyUnits filter out any
	 *   neutrals, use the GetNeutral callbacks to retrieve them
	 */

	/// returns all known (in LOS) enemy units
	virtual int GetEnemyUnits(int* unitIds, int unitIds_max = MAX_UNITS) = 0;
	/// returns all known enemy units within radius from pos
	virtual int GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = MAX_UNITS) = 0;
	/// returns all enemy units in radar and los
	virtual int GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max = MAX_UNITS) = 0;
	/// returns all friendly units
	virtual int GetFriendlyUnits(int* unitIds, int unitIds_max = MAX_UNITS) = 0;
	/// returns all friendly units within radius from pos
	virtual int GetFriendlyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = MAX_UNITS) = 0;
	/// returns all known (in LOS) neutral units
	virtual int GetNeutralUnits(int* unitIds, int unitIds_max = MAX_UNITS) = 0;
	/// returns all known neutral units within radius from pos
	virtual int GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = MAX_UNITS) = 0;

	/*
	 * The following functions are used to get information about the map
	 * - do NOT modify or delete any of the pointers returned
	 * - the maps are stored from top left and each data position is 8*8 in size
	 * - to get info about a position (x, y) look at location
	 *   (int(y / 8)) * GetMapWidth() + (int(x / 8))
	 * - note that some of the type-maps are stored in a lower resolution than
	 *   this
	 */

	virtual int GetMapWidth() = 0;
	virtual int GetMapHeight() = 0;
	/**
	 * This is the height for the center of the squares.
	 * This differs slightly from the drawn map since
	 * that one uses the height at the corners.
	 * Use this one if you are unsure whether you need corners or centers.
	 * @see GetCornerHeightMap()
	 */
	virtual const float* GetHeightMap() = 0;
	/**
	 * This is the height for the corners of the squares.
	 * This is the same like the drawn map.
	 * It is one unit wider and one higher then the centers height map.
	 * @see GetHeightMap()
	 */
	virtual const float* GetCornersHeightMap() = 0;
	/// readmap->minHeight
	virtual float GetMinHeight() = 0;
	/// readmap->maxHeight
	virtual float GetMaxHeight() = 0;
	/**
	 * @brief the slope map
	 * The values are 1 minus the y-component of the (average) facenormal of the
	 * square.
	 *
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 2*2 in size
	 * - the value for the full resolution position (x, z) is at index
	 *   ((z * width + x) / 2)
	 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
	 */
	virtual const float* GetSlopeMap() = 0;
	/**
	 * A square with value zero means you don't have LOS coverage on it.
	 * This has the resolution returned by GetLosMapResolution().
	 */
	virtual const unsigned short* GetLosMap() = 0;
	virtual int GetLosMapResolution() = 0;
	/**
	 * A square with value zero means you don't have radar coverage on it,
	 * 1/8 the resolution of the standard map
	 */
	virtual const unsigned short* GetRadarMap() = 0;
	/**
	 * A square with value zero means you don't have radar jamming coverage
	 * on it, 1/8 the resolution of the standard map.
	 */
	virtual const unsigned short* GetJammerMap() = 0;
	/**
	 * This map shows the metal density on the map, half the resolution
	 * of the standard map
	 */
	virtual const unsigned char* GetMetalMap() = 0;
	/// Use this one for reference (eg. in cache-file names)
	virtual int GetMapHash() = 0;
	/// Use this one for reference (eg. in config-file names)
	virtual const char* GetMapName() = 0;
	virtual const char* GetMapHumanName() = 0;
	/// Use this one for reference (eg. in cache-file names)
	virtual int GetModHash() = 0;
	/// archive name @deprecated
	virtual const char* GetModName() = 0;
	/// Use this one for reference (eg. in config-file names)
	virtual const char* GetModHumanName() = 0;
	/**
	 * Returns the short name of the mod, which does not include the version.
	 * Use this for reference to the mod in general, eg. as version independent
	 * reference.
	 * Be aware though, that this still contain special characters and spaces,
	 * and may not be used as a file name without checks and replaces.
	 * Tip: replace every char matching [^0-9a-zA-Z_-.] with '_'
	 */
	virtual const char* GetModShortName() = 0;
	virtual const char* GetModVersion() = 0;

	/// Gets the elevation of the map at position (x, z)
	virtual float GetElevation(float x, float z) = 0;


	/// Returns what metal value 255 in the metal map is worth
	virtual float GetMaxMetal() const = 0;
	/// Returns extraction radius for metal extractors
	virtual float GetExtractorRadius() const = 0;
	virtual float GetMinWind() const = 0;
	virtual float GetMaxWind() const = 0;
	virtual float GetCurWind() const = 0;
	virtual float GetTidalStrength() const = 0;
	virtual float GetGravity() const = 0;


	/*
	 * Line-Drawer interface functions.
	 * - these allow you to draw command-like lines and figures
	 * - use these only from within CGroupAI::DrawCommands()
	 */

	virtual void LineDrawerStartPath(const float3& pos, const float* color) = 0;
	virtual void LineDrawerFinishPath() = 0;
	virtual void LineDrawerDrawLine(const float3& endPos, const float* color) = 0;
	virtual void LineDrawerDrawLineAndIcon(int commandId, const float3& endPos, const float* color) = 0;
	virtual void LineDrawerDrawIconAtLastPos(int commandId) = 0;
	virtual void LineDrawerBreak(const float3& endPos, const float* color) = 0;
	virtual void LineDrawerRestart() = 0;
	virtual void LineDrawerRestartSameColor() = 0;

	/*
	 * The following functions allow the AI to draw figures in the world
	 * - each figure is part of a figureGroup
	 * - when creating figures use 0 as figureGroupId to get a new figureGroup,
	 *   the return value is the new figureGroup
	 * - the lifeTime is how many frames a figure should live before being
	 *   auto-removed, 0 means no removal
	 * - arrow != 0 means that the figure will get an arrow at the end
	 */

	/**
	 * Creates a cubic Bezier spline figure from pos1 to pos4 with control
	 * points pos2 and pos3).
	 */
	virtual int CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3, float3 pos4, float width, int arrow, int lifeTime, int figureGroupId) = 0;
	virtual int CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifeTime, int figureGroupId) = 0;
	virtual void SetFigureColor(int figureGroupId, float red, float green, float blue, float alpha) = 0;
	virtual void DeleteFigureGroup(int figureGroupId) = 0;

	/**
	 * Draws units on the map.
	 * - they only show up on the local player's screen
	 * - they will be drawn in the "standard pose"
	 *   (as if before any COB scripts are run)
	 * - the rotation is in radians, team affects the color of the unit
	 */
	virtual void DrawUnit(const char* unitName, float3 pos, float rotation, int lifeTime, int teamId, bool transparent, bool drawBorder, int facing = 0) = 0;

	/*
	 * The following functions allow AI's to plot real-time
	 * performance graphs (useful for basic visual profiling)
	 *
	 * - position and size are specified in relative screen-space
	 *   (ie. position must be in [-1.0, 1.0], size in [0.0, 2.0])
	 * - position refers to the bottom-left corner of the graph
	 * - data-points are automatically normalized, but must not
	 *   exceed 1E9 (1000^3) in absolute value
	 * - you must be a spectator and watching the team of an AI
	 *   that has called AddDebugGraphPoint() to see these graphs
	 *   note: they are drawn IIF IsDebugDrawerEnabled())
	 */

	virtual bool IsDebugDrawerEnabled() const = 0;
	virtual void DebugDrawerAddGraphPoint(int lineId, float x, float y) = 0;
	virtual void DebugDrawerDelGraphPoints(int lineId, int numPoints) = 0;
	virtual void DebugDrawerSetGraphPos(float x, float y) = 0;
	virtual void DebugDrawerSetGraphSize(float w, float h) = 0;
	virtual void DebugDrawerSetGraphLineColor(int lineId, const float3& color) = 0;
	virtual void DebugDrawerSetGraphLineLabel(int lineId, const char* label) = 0;

	/*
	 * The following functions allow AI's to visualize overlay
	 * maps as textures (useful for analyzing threat-maps and
	 * the like in real-time)
	 *
	 * - position and size are specified as for graphs
	 * - AI's are responsible for normalizing the data
	 * - the data must be stored in a one-dimensional
	 *   array of floats of length (w * h); updating
	 *   a texture sub-region must be done in absolute
	 *   pixel) coordinates
	 */

	virtual int DebugDrawerAddOverlayTexture(const float* texData, int w, int h) = 0;
	virtual void DebugDrawerUpdateOverlayTexture(int texHandle, const float* texData, int x, int y, int w, int h) = 0;
	virtual void DebugDrawerDelOverlayTexture(int texHandle) = 0;
	virtual void DebugDrawerSetOverlayTexturePos(int texHandle, float x, float y) = 0;
	virtual void DebugDrawerSetOverlayTextureSize(int texHandle, float w, float h) = 0;
	virtual void DebugDrawerSetOverlayTextureLabel(int texHandle, const char* label) = 0;


	virtual bool CanBuildAt(const UnitDef* unitDef, float3 pos, int facing = 0) = 0;
	/**
	 * Returns the closest position from a given position that a building can be
	 * built at.
	 * @param unitdef      unitdef to search for a build position
	 * @param pos          position to search for
	 * @param searchRadius radius to search for a build position
	 * @param minDist the distance in squares that the building must keep to
	 *                other buildings, to make it easier to keep free paths
	 *                through a base
	 * @param facing facing of the building (0-3)
	 */
	virtual float3 ClosestBuildSite(const UnitDef* unitdef, float3 pos, float searchRadius, int minDist, int facing = 0) = 0;

	/// For certain future callback extensions
	virtual bool GetProperty(int unitId, int propertyId, void* dst) = 0;
	virtual bool GetValue(int valueId, void* dst) = 0;
	virtual int HandleCommand(int commandId, void* data) = 0;

	/// Return -1 when the file does not exist
	virtual int GetFileSize(const char* filename) = 0;
	/// Returns false when file does not exist or the buffer is too small
	virtual bool ReadFile(const char* filename, void* buffer, int bufferLen) = 0;

	virtual int GetSelectedUnits(int* unitIds, int unitIds_max = MAX_UNITS) = 0;
	virtual float3 GetMousePos() = 0;
	/**
	 * Returns all points drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
	virtual int GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies) = 0;
	/**
	 * Returns all lines drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
	virtual int GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies) = 0;

	virtual float GetMetal() = 0;         ///< current metal level for team
	virtual float GetMetalIncome() = 0;   ///< current metal income for team
	virtual float GetMetalUsage() = 0;    ///< current metal usage for team
	virtual float GetMetalStorage() = 0;  ///< curent metal storage capacity for team

	virtual float GetEnergy() = 0;        ///< current energy level for team
	virtual float GetEnergyIncome() = 0;  ///< current energy income for team
	virtual float GetEnergyUsage() = 0;   ///< current energy usage for team
	virtual float GetEnergyStorage() = 0; ///< curent energy storage capacity for team

	virtual int GetFeatures(int *featureIds, int max) = 0;
	virtual int GetFeatures(int *featureIds, int max, const float3& pos, float radius) = 0;
	virtual const FeatureDef* GetFeatureDef(int featureId) = 0;
	virtual float GetFeatureHealth(int featureId) = 0;
	virtual float GetFeatureReclaimLeft(int featureId) = 0;
	virtual float3 GetFeaturePos(int featureId) = 0;

	virtual int GetNumUnitDefs() = 0;
	virtual void GetUnitDefList(const UnitDef** list) = 0;
	virtual float GetUnitDefHeight(int unitDefId) = 0; ///< forces loading of the unit model
	virtual float GetUnitDefRadius(int unitDefId) = 0; ///< forces loading of the unit model

	virtual const WeaponDef* GetWeapon(const char* weaponName) = 0;

	virtual const float3* GetStartPos() = 0;

	/**
	 * Returns the categories bit field value.
	 * @return the categories bit field value or 0,
	 *         in case of empty name or too many categories
	 * @see #GetCategoryName
	 */
	unsigned int GetCategoryFlag(const char* categoryName);
	/**
	 * Returns the bitfield values of a list of category names.
	 * @see #GetCategoryFlag
	 */
	unsigned int GetCategoriesFlag(const char* categoryNames);
	/**
	 * Return the name of the category described by a category flag.
	 * @see #GetCategoryFlag
	 */
	void GetCategoryName(int categoryFlag, char* name, int name_sizeMax);

	/**
	 * 1. 'inData' can be setup to NULL to skip passing in a string
	 * 2. if inSize is less than 0, the data size is calculated using strlen()
	 */
	virtual std::string CallLuaRules(const char* inData, int inSize = -1) = 0;
	virtual std::string CallLuaUI(const char* inData, int inSize = -1) = 0;

	virtual std::map<std::string, std::string> GetMyInfo() = 0;
	virtual std::map<std::string, std::string> GetMyOptionValues() = 0;

	// use virtual instead of pure virtual,
	// because pure virtual is not well supported
	// among different OSs and compilers,
	// and pure virtual has no advantage
	// if we have other pure virtual functions
	// in the class
	virtual ~IAICallback() {}
};

} // namespace springLegacyAI

#endif // I_AI_CALLBACK_H

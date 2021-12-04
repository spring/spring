--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    callins.lua
--  brief:   array and map of call-ins
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--  functions listed here are reserved and not allowed to be RegisterGlobal()'ed
--  they are set to 'nil' if not used by any gadget, see gadgetHandler:UpdateCallIn
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

CALLIN_MAP = {}
CALLIN_LIST = {
	-- Save/Load
	"Save",
	"Load",

	"Pong",

	-- called when Lua is disabled or this client exits
	"Shutdown",

	-- game callins
	"GameSetup",
	"GamePreload",
	"GameStart",
	"GameOver",
	"GameFrame",
	"GamePaused",
	"GameProgress",
	"GameID",

	-- player callins
	"PlayerChanged",
	"PlayerAdded",
	"PlayerRemoved",

	-- team callins
	"TeamDied",
	"TeamChanged",

	-- unit callins
	"UnitCreated",
	"UnitFinished",
	"UnitFromFactory",
	"UnitReverseBuilt",
	"UnitDestroyed",
	"RenderUnitDestroyed",
	"UnitExperience",
	"UnitIdle",
	"UnitCmdDone",
	"UnitPreDamaged",
	"UnitDamaged",
	"UnitStunned",
	"UnitTaken",
	"UnitGiven",
	"UnitEnteredRadar",
	"UnitEnteredLos",
	"UnitLeftRadar",
	"UnitLeftLos",
	"UnitSeismicPing",
	"UnitLoaded",
	"UnitUnloaded",
	"UnitCloaked",
	"UnitDecloaked",
	"UnitUnitCollision",
	"UnitFeatureCollision",
	"UnitMoveFailed",
	"UnitMoved",               -- FIXME: not exposed to Lua yet (as of 95.0)
	"UnitEnteredAir",
	"UnitLeftAir",
	"UnitEnteredWater",
	"UnitLeftWater",
	"UnitCommand",
	"UnitHarvestStorageFull",

	-- weapon callins
	"StockpileChanged",

	-- feature callins
	"FeatureCreated",
	"FeatureDestroyed",
	"FeatureDamaged",
	"FeatureMoved",            -- FIXME: not exposed to Lua yet (as of 95.0)
	"FeaturePreDamaged",

	-- projectile callins
	"ProjectileCreated",
	"ProjectileDestroyed",

	-- shield callins
	"ShieldPreDamaged",

	-- misc synced LuaRules callins
	"AllowCommand",
	"AllowStartPosition",
	"AllowUnitCreation",
	"AllowUnitTransfer",
	"AllowUnitBuildStep",
	"AllowUnitTransport",
	"AllowUnitTransportLoad",
	"AllowUnitTransportUnload",
	"AllowUnitCloak",
	"AllowUnitDecloak",
	"AllowUnitKamikaze",
	"AllowFeatureBuildStep",
	"AllowFeatureCreation",
	"AllowResourceLevel",
	"AllowResourceTransfer",
	"AllowDirectUnitControl",
	"AllowBuilderHoldFire",
	"AllowWeaponTargetCheck",
	"AllowWeaponTarget",
	"AllowWeaponInterceptTarget",

	"Explosion",
	"CommandFallback",
	"MoveCtrlNotify",
	"TerraformComplete",

	-- synced message callins
	"GotChatMsg",
	"RecvLuaMsg",

	-- unsynced callins
	"Update",
	"UnsyncedHeightMapUpdate", -- FIXME: not implemented by base GH

	"DrawGenesis",
	"DrawWater",
	"DrawSky",
	"DrawSun",
	"DrawGrass",
	"DrawTrees",
	"DrawWorld",
	"DrawWorldPreUnit",
	"DrawWorldPreParticles",
	"DrawWorldShadow",
	"DrawWorldReflection",
	"DrawWorldRefraction",
	"DrawGroundPreForward",
	"DrawGroundPostForward",
	"DrawGroundPreDeferred",
	"DrawGroundPostDeferred",
	"DrawUnitsPostDeferred",
	"DrawFeaturesPostDeferred",
	"DrawScreenEffects",
	"DrawScreenPost",
	"DrawScreen",
	"DrawInMiniMap",

	"DrawUnit",
	"DrawFeature",
	"DrawShield",
	"DrawProjectile",
	"DrawMaterial",

	"SunChanged",

	-- unsynced message callins
	"RecvFromSynced",
	"RecvSkirmishAIMessage",

	"DefaultCommand",
	"CommandNotify",

	"ViewResize", -- FIXME ?
	"LayoutButtons",
	"ConfigureLayout",

	"AddConsoleLine",
	"GroupChanged",

	-- moved from LuaUI
	"KeyPress",
	"KeyRelease",
	"TextInput",
	"TextEditing",
	"MousePress",
	"MouseRelease",
	"MouseMove",
	"MouseWheel",

	"IsAbove",
	"GetTooltip",

	"WorldTooltip",            -- FIXME: not implemented by base GH
	"MapDrawCmd",
	"ShockFront",              -- FIXME: not implemented by base GH

	"DownloadQueued",
	"DownloadStarted",
	"DownloadFinished",
	"DownloadFailed",
	"DownloadProgress",
}

for callinIdx, callinName in ipairs(CALLIN_LIST) do
	CALLIN_MAP[callinName] = callinIdx
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

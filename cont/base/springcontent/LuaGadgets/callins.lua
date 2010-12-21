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
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

CallInsList = {
	"Shutdown",
	"LayoutButtons",
	"ConfigureLayout",
	"CommandNotify",

	"KeyPress",
	"KeyRelease",
	"MouseMove",
	"MousePress",
	"MouseRelease",
	"IsAbove",
	"GetTooltip",
	"AddConsoleLine",
	"GroupChanged",

	"GamePreload",
	"GameStart",
	"GameOver",
	"TeamDied",

	"UnitCreated",
	"UnitFinished",
	"UnitFromFactory",
	"UnitDestroyed",
	"UnitTaken",
	"UnitGiven",
	"UnitIdle",
	"UnitSeismicPing",
	"UnitEnteredRadar",
	"UnitEnteredLos",
	"UnitLeftRadar",
	"UnitLeftLos",
	"UnitLoaded",
	"UnitUnloaded",
	"UnitCloaked",
	"UnitDecloaked",
	-- "UnitUnitCollision",
	-- "UnitFeatureCollision",
	-- "UnitMoveFailed",

	"FeatureCreated",
	"FeatureDestroyed",

	"ProjectileCreated",
	"ProjectileDestroyed",

	"DrawGenesis",
	"DrawWorld",
	"DrawWorldPreUnit",
	"DrawWorldShadow",
	"DrawWorldReflection",
	"DrawWorldRefraction",
	"DrawScreenEffects",
	"DrawScreen",
	"DrawInMiniMap",
	"DrawUnit",
	"DrawFeature",

	"Explosion",
	"ShockFront",

	"GameFrame",
	"CobCallback",
	"AllowCommand",
	"CommandFallback",
	"AllowUnitCreation",
	"AllowUnitTransfer",
	"AllowUnitBuildStep",
	"AllowFeatureCreation",
	"AllowFeatureBuildStep",
	"AllowResourceLevel",
	"AllowResourceTransfer",
	"MoveCtrlNotify",
	"TerraformComplete",
	"AllowWeaponTargetCheck",
	"AllowWeaponTarget",
}


-- make the map
CallInsMap = {}
for _, callin in ipairs(CallInsList) do
	CallInsMap[callin] = true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

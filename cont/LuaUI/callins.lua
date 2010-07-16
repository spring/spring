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
  "JoyAxis",
  "JoyHat",
  "JoyButtonDown",
  "JoyButtonUp",
  "IsAbove",
  "GetTooltip",
  "AddConsoleLine",
  "GroupChanged",
  "WorldTooltip",

  "GameLoadLua",
  "GameStartPlaying",
  "GameOver",
  "TeamDied",

  "UnitCreated",
  "UnitFinished",
  "UnitFromFactory",
  "UnitDestroyed",
  "UnitTaken",
  "UnitGiven",
  "UnitIdle",
  "UnitCommand",
  "UnitSeismicPing",
  "UnitEnteredRadar",
  "UnitEnteredLos",
  "UnitLeftRadar",
  "UnitLeftLos",
  "UnitLoaded",
  "UnitUnloaded",

  "UnitEnteredWater",
  "UnitEnteredAir",
  "UnitLeftWater",
  "UnitLeftAir",

  "FeatureCreated",
  "FeatureDestroyed",

  "DrawGenesis",
  "DrawWorld",
  "DrawWorldPreUnit",
  "DrawWorldShadow",
  "DrawWorldReflection",
  "DrawWorldRefraction",
  "DrawScreenEffects",
  "DrawScreen",
  "DrawInMiniMap",

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
}


-- make the map
CallInsMap = {}
for _, callin in ipairs(CallInsList) do
  CallInsMap[callin] = true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

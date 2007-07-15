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

  "FeatureCreated",
  "FeatureDestroyed",

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

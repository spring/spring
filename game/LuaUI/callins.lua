--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    callins.lua
--  brief:   wrapper to make sure that all call-ins exist after initialization
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function OneShot(name)
  _G[name] = function()
    print("LuaUI: removed unused call-in: " .. name .. "()")
    _G[name] = nil
  end
end

OneShot("Shutdown")
OneShot("LayoutButtons")
OneShot("UpdateLayout")
OneShot("ConfigureLayout")
OneShot("CommandNotify")

OneShot("DrawWorldItems")
OneShot("DrawScreenItems")

OneShot("KeyPress")
OneShot("KeyRelease")
OneShot("MouseMove")
OneShot("MousePress")
OneShot("MouseRelease")
OneShot("IsAbove")
OneShot("GetTooltip")
OneShot("AddConsoleLine")
OneShot("GroupChanged")

OneShot("GameOver")
OneShot("TeamDied")

OneShot("UnitCreated")
OneShot("UnitFinished")
OneShot("UnitFromFactory")
OneShot("UnitDestroyed")
OneShot("UnitTaken")
OneShot("UnitGiven")
OneShot("UnitIdle")
OneShot("UnitSeismicPing")
OneShot("UnitEnteredRadar")
OneShot("UnitEnteredLos")
OneShot("UnitLeftRadar")
OneShot("UnitLeftLos")
OneShot("UnitLoaded")
OneShot("UnitUnloaded")

OneShot("FeatureCreated")
OneShot("FeatureDestroyed")

OneShot("DrawWorld")
OneShot("DrawWorldShadow")
OneShot("DrawWorldReflection")
OneShot("DrawWorldRefraction")
OneShot("DrawScreen")
OneShot("DrawInMiniMap")


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    system.lua
--  brief:   defines the global entries placed into a widget's table
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (System == nil) then

  System = {
    --
    --  Custom LuaUI variables
    --
    Commands = Commands,
    LUAUI_DIRNAME = LUAUI_DIRNAME,

    --
    --  Custom Spring tables
    --
    Game = Game,
    Spring = Spring,
    gl = Spring.Draw,
    UnitDefs = UnitDefs,
    WeaponDefs = WeaponDefs,

    --
    --  Standard libraries
    --
    io = io,
    math = math,
    debug = debug,
    table = table,
    string = string,
    coroutine = coroutine,
    
    --  
    --  Standard functions and variables
    --
    assert         = assert,
    error          = error,

    print          = print,
    
    next           = next,
    pairs          = pairs,
    ipairs         = ipairs,

    tonumber       = tonumber,
    tostring       = tostring,
    type           = type,

    collectgarbage = collectgarbage,
    gcinfo         = gcinfo,

    unpack         = unpack,

    dofile         = dofile,
    loadfile       = loadfile,
    loadlib        = loadlib,
    loadstring     = loadstring,
    require        = require,
    
    getmetatable   = getmetatable,
    setmetatable   = setmetatable,

    rawequal       = rawequal,
    rawget         = rawget,
    rawset         = rawset,

    getfenv        = getfenv,
    setfenv        = setfenv,

    pcall          = pcall,
    xpcall         = xpcall,

    _VERSION       = _VERSION
  }

end

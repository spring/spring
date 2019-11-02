--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gadgets.lua
--  brief:   the gadget manager, a call-in router
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  TODO:  - get rid of the ':'/self referencing, it's a waste of cycles
--         - (De)RegisterCOBCallback(data) + callin?
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local SAFEWRAP = 0
-- 0: disabled
-- 1: enabled, but can be overriden by gadget.GetInfo().unsafe
-- 2: always enabled


local HANDLER_DIR = 'LuaGadgets/'
local GADGETS_DIR = Script.GetName():gsub('US$', '') .. '/Gadgets/'
local LOG_SECTION = "" -- FIXME: "LuaRules" section is not registered anywhere


local VFSMODE = VFS.ZIP_ONLY -- FIXME: ZIP_FIRST ?
if (Spring.IsDevLuaEnabled()) then
  VFSMODE = VFS.RAW_ONLY
end


VFS.Include(HANDLER_DIR .. 'setupdefs.lua', nil, VFSMODE)
VFS.Include(HANDLER_DIR .. 'system.lua',    nil, VFSMODE)
VFS.Include(HANDLER_DIR .. 'callins.lua',   nil, VFSMODE)

local actionHandler = VFS.Include(HANDLER_DIR .. 'actions.lua', nil, VFSMODE)


--------------------------------------------------------------------------------

function pgl() -- (print gadget list)  FIXME: move this into a gadget
  for k,v in ipairs(gadgetHandler.gadgets) do
    Spring.Log(LOG_SECTION, LOG.ERROR,
      string.format("%3i  %3i  %s", k, v.ghInfo.layer, v.ghInfo.name)
    )
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  the gadgetHandler object
--

gadgetHandler = {

  gadgets = {},

  orderList = {},

  knownGadgets = {},
  knownCount = 0,
  knownChanged = true,

  GG = {}, -- shared table for gadgets

  globals = {}, -- global vars/funcs

  CMDIDs = {},

  xViewSize    = 1,
  yViewSize    = 1,
  xViewSizeOld = 1,
  yViewSizeOld = 1,

  actionHandler = actionHandler,
  mouseOwner = nil,
}


-- initialize the call-in lists
do
  for _,listname in ipairs(CALLIN_LIST) do
    gadgetHandler[listname .. 'List'] = {}
  end
end


-- Utility call
local isSyncedCode = (SendToUnsynced ~= nil)
local function IsSyncedCode()
  return isSyncedCode
end



--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  array-table reverse iterator
--
--  all callin handlers use this so that gadgets can
--  RemoveGadget() themselves (during iteration over
--  a callin list) without causing a miscount
--
--  c.f. Array{Insert,Remove}
--
local function r_ipairs(tbl)
  local function r_iter(tbl, key)
    if (key <= 1) then
      return nil
    end

    -- next idx, next val
    return (key - 1), tbl[key - 1]
  end

  return r_iter, tbl, (1 + #tbl)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  returns:  basename, dirname
--

local function Basename(fullpath)
  local _,_,base = string.find(fullpath, "([^\\/:]*)$")
  local _,_,path = string.find(fullpath, "(.*[\\/:])[^\\/:]*$")
  if (path == nil) then path = "" end
  return base, path
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadgetHandler:Initialize()
  local syncedHandler = Script.GetSynced()

  local unsortedGadgets = {}
  -- get the gadget names
  local gadgetFiles = VFS.DirList(GADGETS_DIR, "*.lua", VFSMODE)
--  table.sort(gadgetFiles)

--  for k,gf in ipairs(gadgetFiles) do
--    Spring.Echo('gf1 = ' .. gf) -- FIXME
--  end

  -- stuff the gadgets into unsortedGadgets
  for k,gf in ipairs(gadgetFiles) do
--    Spring.Echo('gf2 = ' .. gf) -- FIXME
    local gadget = self:LoadGadget(gf)
    if (gadget) then
      table.insert(unsortedGadgets, gadget)
    end
  end

  -- sort the gadgets
  table.sort(unsortedGadgets, function(g1, g2)
    local l1 = g1.ghInfo.layer
    local l2 = g2.ghInfo.layer
    if (l1 ~= l2) then
      return (l1 < l2)
    end
    local n1 = g1.ghInfo.name
    local n2 = g2.ghInfo.name
    local o1 = self.orderList[n1]
    local o2 = self.orderList[n2]
    if (o1 ~= o2) then
      return (o1 < o2)
    else
      return (n1 < n2)
    end
  end)

  -- add the gadgets
  for _,g in ipairs(unsortedGadgets) do
    gadgetHandler:InsertGadget(g)

    local gtype = ((syncedHandler and "SYNCED") or "UNSYNCED")
    local gname = g.ghInfo.name
    local gbasename = g.ghInfo.basename

    Spring.Log(LOG_SECTION, LOG.INFO, string.format("Loaded %s gadget:  %-18s  <%s>", gtype, gname, gbasename))
  end
end


function gadgetHandler:LoadGadget(filename)
  local basename = Basename(filename)
  local text = VFS.LoadFile(filename, VFSMODE)
  if (text == nil) then
    Spring.Log(LOG_SECTION, LOG.ERROR, 'Failed to load: ' .. filename)
    return nil
  end
  local chunk, err = loadstring(text, filename)
  if (chunk == nil) then
    Spring.Log(LOG_SECTION, LOG.ERROR, 'Failed to load: ' .. basename .. '  (' .. err .. ')')
    return nil
  end

  local gadget = gadgetHandler:NewGadget()

  setfenv(chunk, gadget)
  local success, err = pcall(chunk)
  if (not success) then
    Spring.Log(LOG_SECTION, LOG.ERROR, 'Failed to load: ' .. basename .. '  (' .. tostring(err) .. ')')
    return nil
  end
  if (err == false) then
    return nil -- gadget asked for a quiet death
  end

  -- raw access to gadgetHandler
  if (gadget.GetInfo and gadget:GetInfo().handler) then
    gadget.gadgetHandler = self
  end

  self:FinalizeGadget(gadget, filename, basename)
  local name = gadget.ghInfo.name

  err = self:ValidateGadget(gadget)
  if (err) then
    Spring.Log(LOG_SECTION, LOG.ERROR, 'Failed to load: ' .. basename .. '  (' .. err .. ')')
    return nil
  end

  local knownInfo = self.knownGadgets[name]
  if (knownInfo) then
    if (knownInfo.active) then
      Spring.Log(LOG_SECTION, LOG.ERROR, 'Failed to load: ' .. basename .. '  (duplicate name)')
      return nil
    end
  else
    -- create a knownInfo table
    knownInfo = {}
    knownInfo.desc     = gadget.ghInfo.desc
    knownInfo.author   = gadget.ghInfo.author
    knownInfo.basename = gadget.ghInfo.basename
    knownInfo.filename = gadget.ghInfo.filename
    self.knownGadgets[name] = knownInfo
    self.knownCount = self.knownCount + 1
    self.knownChanged = true
  end
  knownInfo.active = true

  local info  = gadget.GetInfo and gadget:GetInfo()
  local order = self.orderList[name]
  if (((order ~= nil) and (order > 0)) or
      ((order == nil) and ((info == nil) or info.enabled))) then
    -- this will be an active gadget
    if (order == nil) then
      self.orderList[name] = 12345  -- back of the pack
    else
      self.orderList[name] = order
    end
  else
    self.orderList[name] = 0
    self.knownGadgets[name].active = false
    return nil
  end

  return gadget
end


function gadgetHandler:NewGadget()
  local gadget = {}
  -- load the system calls into the gadget table
  for k,v in pairs(System) do
    gadget[k] = v
  end
  gadget._G = _G         -- the global table
  gadget.GG = self.GG    -- the shared table
  gadget.gadget = gadget -- easy self referencing

  -- wrapped calls (closures)
  gadget.gadgetHandler = {}
  local gh = gadget.gadgetHandler
  local self = self

  gadget.include  = function (f)
    return VFS.Include(f, gadget, VFSMODE)
  end

  gh.RaiseGadget  = function (_) self:RaiseGadget(gadget)      end
  gh.LowerGadget  = function (_) self:LowerGadget(gadget)      end
  gh.RemoveGadget = function (_) self:RemoveGadget(gadget)     end
  gh.GetViewSizes = function (_) return self:GetViewSizes()    end
  gh.GetHourTimer = function (_) return self:GetHourTimer()    end
  gh.IsSyncedCode = function (_) return IsSyncedCode()         end

  gh.UpdateCallIn = function (_, name)
    self:UpdateGadgetCallIn(name, gadget)
  end
  gh.RemoveCallIn = function (_, name)
    self:RemoveGadgetCallIn(name, gadget)
  end

  gh.RegisterCMDID = function(_, id)
    self:RegisterCMDID(gadget, id)
  end

  gh.RegisterGlobal = function(_, name, value)
    return self:RegisterGlobal(gadget, name, value)
  end
  gh.DeregisterGlobal = function(_, name)
    return self:DeregisterGlobal(gadget, name)
  end
  gh.SetGlobal = function(_, name, value)
    return self:SetGlobal(gadget, name, value)
  end

  gh.AddChatAction = function (_, cmd, func, help)
    return actionHandler.AddChatAction(gadget, cmd, func, help)
  end
  gh.RemoveChatAction = function (_, cmd)
    return actionHandler.RemoveChatAction(gadget, cmd)
  end

  if (not IsSyncedCode()) then
    gh.AddSyncAction = function(_, cmd, func, help)
      return actionHandler.AddSyncAction(gadget, cmd, func, help)
    end
    gh.RemoveSyncAction = function(_, cmd)
      return actionHandler.RemoveSyncAction(gadget, cmd)
    end
  end

  -- for proxied call-ins
  gh.IsMouseOwner = function (_)
    return (self.mouseOwner == gadget)
  end
  gh.DisownMouse  = function (_)
    if (self.mouseOwner == gadget) then
      self.mouseOwner = nil
    end
  end

  return gadget
end


function gadgetHandler:FinalizeGadget(gadget, filename, basename)
  local gi = {}

  gi.filename = filename
  gi.basename = basename
  if (gadget.GetInfo == nil) then
    gi.name  = basename
    gi.layer = 0
  else
    local info = gadget:GetInfo()
    gi.name      = info.name    or basename
    gi.layer     = info.layer   or 0
    gi.desc      = info.desc    or ""
    gi.author    = info.author  or ""
    gi.license   = info.license or ""
    gi.enabled   = info.enabled or false
  end

  gadget.ghInfo = {}  --  a proxy table
  local mt = {
    __index = gi,
    __newindex = function() error("ghInfo tables are read-only") end,
    __metatable = "protected"
  }
  setmetatable(gadget.ghInfo, mt)
end


function gadgetHandler:ValidateGadget(gadget)
  if (gadget.GetTooltip and not gadget.IsAbove) then
    return "Gadget has GetTooltip() but not IsAbove()"
  end
  if (gadget.TweakGetTooltip and not gadget.TweakIsAbove) then
    return "Gadget has TweakGetTooltip() but not TweakIsAbove()"
  end
  return nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function SafeWrap(func, funcName)
  local gh = gadgetHandler
  return function(g, ...)
    local r = { pcall(func, g, ...) }
    if (r[1]) then
      table.remove(r, 1)
      return unpack(r)
    else
      if (funcName ~= 'Shutdown') then
        gadgetHandler:RemoveGadget(g)
      else
        Spring.Log(LOG_SECTION, LOG.ERROR, 'Error in Shutdown')
      end
      local name = g.ghInfo.name
      Spring.Log(LOG_SECTION, LOG.INFO, r[2])
      Spring.Log(LOG_SECTION, LOG.INFO, 'Removed gadget: ' .. name)
      return nil
    end
  end
end


local function SafeWrapGadget(gadget)
  if (SAFEWRAP <= 0) then
    return
  elseif (SAFEWRAP == 1) then
    if (gadget.GetInfo and gadget.GetInfo().unsafe) then
      Spring.Log(LOG_SECTION, LOG.ERROR, 'LuaUI: loaded unsafe gadget: ' .. gadget.ghInfo.name)
      return
    end
  end

  for _,ciName in ipairs(CALLIN_LIST) do
    if (gadget[ciName]) then
      gadget[ciName] = SafeWrap(gadget[ciName], ciName)
    end
    if (gadget.Initialize) then
      gadget.Initialize = SafeWrap(gadget.Initialize, 'Initialize')
    end
  end
end


--------------------------------------------------------------------------------

local function ArrayInsert(t, g)
  local layer = g.ghInfo.layer
  local index = 1

  for i,v in ipairs(t) do
    if (v == g) then
      return -- already in the table
    end

    -- insert-sort the gadget based on its layer
    -- note: reversed value ordering, highest to lowest
    -- iteration over the callin lists is also reversed
    if (layer < v.ghInfo.layer) then
      index = i + 1
    end
  end

  table.insert(t, index, g)
end


local function ArrayRemove(t, g)
  for k,v in ipairs(t) do
    if (v == g) then
      table.remove(t, k)
      -- break
    end
  end
end


function gadgetHandler:InsertGadget(gadget)
  if (gadget == nil) then
    return
  end

  ArrayInsert(self.gadgets, gadget)
  for _,listname in ipairs(CALLIN_LIST) do
    local func = gadget[listname]
    if (func ~= nil and type(func) == 'function') then
      ArrayInsert(self[listname .. 'List'], gadget)
    end
  end

  self:UpdateCallIns()
  if (gadget.Initialize) then
    gadget:Initialize()
  end
  self:UpdateCallIns()
end


function gadgetHandler:RemoveGadget(gadget)
  if (gadget == nil) then
    return
  end

  local name = gadget.ghInfo.name
  self.knownGadgets[name].active = false
  if (gadget.Shutdown) then
    gadget:Shutdown()
  end

  ArrayRemove(self.gadgets, gadget)
  self:RemoveGadgetGlobals(gadget)
  actionHandler.RemoveGadgetActions(gadget)
  for _,listname in ipairs(CALLIN_LIST) do
    ArrayRemove(self[listname..'List'], gadget)
  end

  for id,g in pairs(self.CMDIDs) do
    if (g == gadget) then
      self.CMDIDs[id] = nil
    end
  end

  self:UpdateCallIns()
end


--------------------------------------------------------------------------------

function gadgetHandler:UpdateCallIn(name)
  local listName = name .. 'List'
  local forceUpdate = (name == 'GotChatMsg' or name == 'RecvFromSynced') -- redundant?

  _G[name] = nil

  if (forceUpdate or #self[listName] > 0) then
    local selffunc = self[name]

    if (selffunc ~= nil) then
      _G[name] = function(...)
        return selffunc(self, ...)
      end
    else
      Spring.Log(LOG_SECTION, LOG.ERROR, "UpdateCallIn: " .. name .. " is not implemented")
    end
  end

  Script.UpdateCallIn(name)
end


function gadgetHandler:UpdateGadgetCallIn(name, g)
  local listName = name .. 'List'
  local ciList = self[listName]
  if (ciList) then
    local func = g[name]
    if (func ~= nil and type(func) == 'function') then
      ArrayInsert(ciList, g)
    else
      ArrayRemove(ciList, g)
    end
    self:UpdateCallIn(name)
  else
    Spring.Log(LOG_SECTION, LOG.ERROR, 'UpdateGadgetCallIn: bad name: ' .. name)
  end
end


function gadgetHandler:RemoveGadgetCallIn(name, g)
  local listName = name .. 'List'
  local ciList = self[listName]
  if (ciList) then
    ArrayRemove(ciList, g)
    self:UpdateCallIn(name)
  else
    Spring.Log(LOG_SECTION, LOG.ERROR, 'RemoveGadgetCallIn: bad name: ' .. name)
  end
end


function gadgetHandler:UpdateCallIns()
  for _,name in ipairs(CALLIN_LIST) do
    self:UpdateCallIn(name)
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadgetHandler:EnableGadget(name)
  local ki = self.knownGadgets[name]
  if (not ki) then
    Spring.Log(LOG_SECTION, LOG.ERROR, "EnableGadget(), could not find gadget: " .. tostring(name))
    return false
  end
  if (not ki.active) then
    Spring.Log(LOG_SECTION, LOG.INFO, 'Loading:  '..ki.filename)
    local order = gadgetHandler.orderList[name]
    if (not order or (order <= 0)) then
      self.orderList[name] = 1
    end
    local w = self:LoadGadget(ki.filename)
    if (not w) then return false end
    self:InsertGadget(w)
  end
  return true
end


function gadgetHandler:DisableGadget(name)
  local ki = self.knownGadgets[name]
  if (not ki) then
    Spring.Log(LOG_SECTION, LOG.ERROR, "DisableGadget(), could not find gadget: " .. tostring(name))
    return false
  end
  if (ki.active) then
    local w = self:FindGadget(name)
    if (not w) then return false end
    Spring.Log(LOG_SECTION, LOG.INFO, 'Removed:  '..ki.filename)
    self:RemoveGadget(w)     -- deactivate
    self.orderList[name] = 0 -- disable
  end
  return true
end


function gadgetHandler:ToggleGadget(name)
  local ki = self.knownGadgets[name]
  if (not ki) then
    Spring.Log(LOG_SECTION, LOG.ERROR, "ToggleGadget(), could not find gadget: " .. tostring(name))
    return
  end
  if (ki.active) then
    return self:DisableGadget(name)
  elseif (self.orderList[name] <= 0) then
    return self:EnableGadget(name)
  else
    -- the gadget is not active, but enabled; disable it
    self.orderList[name] = 0
  end
  return true
end


--------------------------------------------------------------------------------

local function FindGadgetIndex(t, w)
  for k,v in ipairs(t) do
    if (v == w) then
      return k
    end
  end
  return nil
end




function gadgetHandler:RaiseGadget(gadget)
  if (gadget == nil) then
    return
  end

  local function FindLowestIndex(t, i, layer)
    local n = #t
    for x = (i + 1), n, 1 do
      if (t[x].ghInfo.layer < layer) then
        return (x - 1)
      end
    end
    return n
  end

  local function Raise(callinList, gadget)
    local gadgetIdx = FindGadgetIndex(callinList, gadget)
    if (gadgetIdx == nil) then
      return
    end

    -- starting from gIdx and counting up, find the index
    -- of the first gadget whose layer is lower** than g's
    -- and move g to right before (lowestIdx - 1) it
    -- ** lists are in reverse layer order, lowest at back
    local lowestIdx = FindLowestIndex(callinList, gadgetIdx, gadget.ghInfo.layer)

    if (lowestIdx > gadgetIdx) then
      -- insert first since lowestIdx is larger
      table.insert(callinList, lowestIdx, gadget)
      table.remove(callinList, gadgetIdx)
    end
  end

  Raise(self.gadgets, gadget)

  for _,listname in ipairs(CALLIN_LIST) do
    if (gadget[listname] ~= nil) then
      Raise(self[listname .. 'List'], gadget)
    end
  end
end


function gadgetHandler:LowerGadget(gadget)
  if (gadget == nil) then
    return
  end

  local function FindHighestIndex(t, i, layer)
    for x = (i - 1), 1, -1 do
      if (t[x].ghInfo.layer > layer) then
        return (x + 1)
      end
    end
    return 1
  end

  local function Lower(callinList, gadget)
    local gadgetIdx = FindGadgetIndex(callinList, gadget)
    if (gadgetIdx == nil) then
      return
    end

    -- starting from gIdx and counting down, find the index
    -- of the first gadget whose layer is higher** than g's
    -- and move g to right after (highestIdx + 1) it
    -- ** lists are in reverse layer order, highest at front
    local highestIdx = FindHighestIndex(callinList, gadgetIdx, gadget.ghInfo.layer)

    if (highestIdx < gadgetIdx) then
      -- remove first since highestIdx is smaller
      table.remove(callinList, gadgetIdx)
      table.insert(callinList, highestIdx, gadget)
    end
  end

  Lower(self.gadgets, gadget)

  for _,listname in ipairs(CALLIN_LIST) do
    if (gadget[listname] ~= nil) then
      Lower(self[listname .. 'List'], gadget)
    end
  end
end


function gadgetHandler:FindGadget(name)
  if (type(name) ~= 'string') then
    return nil
  end
  for k,v in ipairs(self.gadgets) do
    if (name == v.ghInfo.name) then
      return v,k
    end
  end
  return nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Global var/func management
--

function gadgetHandler:RegisterGlobal(owner, name, value)
  if (name == nil) then return false end
  if (_G[name] ~= nil) then return false end
  if (self.globals[name] ~= nil) then return false end
  if (CALLIN_MAP[name] ~= nil) then return false end

  _G[name] = value
  self.globals[name] = owner
  return true
end


function gadgetHandler:DeregisterGlobal(owner, name)
  if (name == nil) then
    return false
  end
  _G[name] = nil
  self.globals[name] = nil
  return true
end


function gadgetHandler:SetGlobal(owner, name, value)
  if ((name == nil) or (self.globals[name] ~= owner)) then
    return false
  end
  _G[name] = value
  return true
end


function gadgetHandler:RemoveGadgetGlobals(owner)
  local count = 0
  for name, o in pairs(self.globals) do
    if (o == owner) then
      _G[name] = nil
      self.globals[name] = nil
      count = count + 1
    end
  end
  return count
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Helper facilities
--

local hourTimer = 0


function gadgetHandler:GetHourTimer()
  return hourTimer
end


function gadgetHandler:GetViewSizes()
  return self.xViewSize, self.yViewSize
end


function gadgetHandler:RegisterCMDID(gadget, id)
  if (id <= 1000) then
    Spring.Log(LOG_SECTION, LOG.ERROR, 'Gadget (' .. gadget.ghInfo.name .. ') ' ..
                'tried to register a reserved CMD_ID')
    Script.Kill('Reserved CMD_ID code: ' .. id)
  end

  if (self.CMDIDs[id] ~= nil) then
    Spring.Log(LOG_SECTION, LOG.ERROR, 'Gadget (' .. gadget.ghInfo.name .. ') ' ..
                'tried to register a duplicated CMD_ID')
    Script.Kill('Duplicate CMD_ID code: ' .. id)
  end

  self.CMDIDs[id] = gadget
end



--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  The call-in distribution routines
--
function gadgetHandler:Shutdown()
  for _,g in r_ipairs(self.ShutdownList) do
    g:Shutdown()
  end
end

function gadgetHandler:GameSetup(state, ready, playerStates)
  local success, newReady = false, ready
  for _,g in r_ipairs(self.GameSetupList) do
    success, newReady = g:GameSetup(state, ready, playerStates)
  end
  return success, newReady
end

function gadgetHandler:GamePreload()
  for _,g in r_ipairs(self.GamePreloadList) do
    g:GamePreload()
  end
end

function gadgetHandler:GameStart()
  for _,g in r_ipairs(self.GameStartList) do
    g:GameStart()
  end
end

function gadgetHandler:GameOver(winningAllyTeams)
  for _,g in r_ipairs(self.GameOverList) do
    g:GameOver(winningAllyTeams)
  end
end

function gadgetHandler:GameFrame(frameNum)
  for _,g in r_ipairs(self.GameFrameList) do
    g:GameFrame(frameNum)
  end
end

function gadgetHandler:GamePaused(playerID, paused)
  for _,g in r_ipairs(self.GamePausedList) do
    g:GamePaused(playerID, paused)
  end
end

function gadgetHandler:GameProgress(serverFrameNum)
  for _,g in r_ipairs(self.GameProgressList) do
    g:GameProgress(serverFrameNum)
  end
end

function gadgetHandler:GameID(gameID)
  for _,g in r_ipairs(self.GameIDList) do
    g:GameID(gameID)
  end
end


function gadgetHandler:RecvFromSynced(...)
  if (actionHandler.RecvFromSynced(...)) then
    return
  end
  for _,g in r_ipairs(self.RecvFromSyncedList) do
    if (g:RecvFromSynced(...)) then
      return
    end
  end
end


function gadgetHandler:GotChatMsg(msg, player)
  if ((player == 0) and Spring.IsCheatingEnabled()) then
    local sp = '^%s*'    -- start pattern
    local ep = '%s+(.*)' -- end pattern
    local s, e, match
    s, e, match = string.find(msg, sp..'togglegadget'..ep)
    if (match) then
      self:ToggleGadget(match)
      return true
    end
    s, e, match = string.find(msg, sp..'enablegadget'..ep)
    if (match) then
      self:EnableGadget(match)
      return true
    end
    s, e, match = string.find(msg, sp..'disablegadget'..ep)
    if (match) then
      self:DisableGadget(match)
      return true
    end
  end

  if (actionHandler.GotChatMsg(msg, player)) then
    return true
  end

  for _,g in r_ipairs(self.GotChatMsgList) do
    if (g:GotChatMsg(msg, player)) then
      return true
    end
  end

  return false
end


function gadgetHandler:RecvLuaMsg(msg, player)
  for _,g in r_ipairs(self.RecvLuaMsgList) do
    if (g:RecvLuaMsg(msg, player)) then
      return true
    end
  end
  return false
end


--------------------------------------------------------------------------------
--
--  Drawing call-ins
--

-- generates ViewResize() calls for the gadgets
function gadgetHandler:SetViewSize(vsx, vsy)
  self.xViewSize = vsx
  self.yViewSize = vsy
  if ((self.xViewSizeOld ~= vsx) or
      (self.yViewSizeOld ~= vsy)) then
    gadgetHandler:ViewResize(vsx, vsy)
    self.xViewSizeOld = vsx
    self.yViewSizeOld = vsy
  end
end


function gadgetHandler:ViewResize(vsx, vsy)
  for _,g in r_ipairs(self.ViewResizeList) do
    g:ViewResize(vsx, vsy)
  end
end


--------------------------------------------------------------------------------
--
--  Team and player call-ins
--

function gadgetHandler:TeamDied(teamID)
  for _,g in r_ipairs(self.TeamDiedList) do
    g:TeamDied(teamID)
  end
end

function gadgetHandler:TeamChanged(teamID)
  for _,g in r_ipairs(self.TeamChangedList) do
    g:TeamChanged(teamID)
  end
end


function gadgetHandler:PlayerChanged(playerID)
  for _,g in r_ipairs(self.PlayerChangedList) do
    g:PlayerChanged(playerID)
  end
end


function gadgetHandler:PlayerAdded(playerID)
  for _,g in r_ipairs(self.PlayerAddedList) do
    g:PlayerAdded(playerID)
  end
end


function gadgetHandler:PlayerRemoved(playerID, reason)
  for _,g in r_ipairs(self.PlayerRemovedList) do
    g:PlayerRemoved(playerID, reason)
  end
end


--------------------------------------------------------------------------------
--
--  LuaRules Game call-ins
--

function gadgetHandler:DrawUnit(unitID, drawMode)
  for _,g in r_ipairs(self.DrawUnitList) do
    if (g:DrawUnit(unitID, drawMode)) then
      return true
    end
  end
  return false
end

function gadgetHandler:DrawFeature(featureID, drawMode)
  for _,g in r_ipairs(self.DrawFeatureList) do
    if (g:DrawFeature(featureID, drawMode)) then
      return true
    end
  end
  return false
end

function gadgetHandler:DrawShield(unitID, weaponID, drawMode)
  for _,g in r_ipairs(self.DrawShieldList) do
    if (g:DrawShield(unitID, weaponID, drawMode)) then
      return true
    end
  end
  return false
end

function gadgetHandler:DrawProjectile(projectileID, drawMode)
  for _,g in r_ipairs(self.DrawProjectileList) do
    if (g:DrawProjectile(projectileID, drawMode)) then
      return true
    end
  end
  return false
end

function gadgetHandler:DrawMaterial(materialID, drawMode)
  for _,g in r_ipairs(self.DrawMaterialList) do
    if (g:DrawMaterial(materialID, drawMode)) then
      return true
    end
  end
  return false
end



function gadgetHandler:RecvSkirmishAIMessage(aiTeam, dataStr)
  for _,g in r_ipairs(self.RecvSkirmishAIMessageList) do
    local dataRet = g:RecvSkirmishAIMessage(aiTeam, dataStr)
    if (dataRet) then
      return dataRet
    end
  end
end


function gadgetHandler:CommandFallback(unitID, unitDefID, unitTeam,
                                       cmdID, cmdParams, cmdOptions, cmdTag)
  for _,g in r_ipairs(self.CommandFallbackList) do
    local used, remove = g:CommandFallback(unitID, unitDefID, unitTeam,
                                           cmdID, cmdParams, cmdOptions, cmdTag)
    if (used) then
      return remove
    end
  end
  return true  -- remove the command
end


function gadgetHandler:AllowCommand(
	unitID, unitDefID, unitTeam,
	cmdID, cmdParams, cmdOptions, cmdTag,
	playerID, fromSynced, fromLua
)
  for _,g in r_ipairs(self.AllowCommandList) do
    if (not g:AllowCommand(
		unitID, unitDefID, unitTeam,
		cmdID, cmdParams, cmdOptions, cmdTag,
		playerID, fromSynced, fromLua)
	) then
      return false
    end
  end
  return true
end

function gadgetHandler:AllowStartPosition(playerID, teamID, readyState, cx, cy, cz, rx, ry, rz)
  for _,g in r_ipairs(self.AllowStartPositionList) do
    if (not g:AllowStartPosition(playerID, teamID, readyState, cx, cy, cz, rx, ry, rz)) then
      return false
    end
  end
  return true
end

function gadgetHandler:AllowUnitCreation(unitDefID, builderID, builderTeam, x, y, z, facing)
  for _,g in r_ipairs(self.AllowUnitCreationList) do
    if (not g:AllowUnitCreation(unitDefID, builderID, builderTeam, x, y, z, facing)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowUnitTransfer(unitID, unitDefID, oldTeam, newTeam, capture)
  for _,g in r_ipairs(self.AllowUnitTransferList) do
    if (not g:AllowUnitTransfer(unitID, unitDefID, oldTeam, newTeam, capture)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowUnitBuildStep(builderID, builderTeam, unitID, unitDefID, part)
  for _,g in r_ipairs(self.AllowUnitBuildStepList) do
    if (not g:AllowUnitBuildStep(builderID, builderTeam, unitID, unitDefID, part)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowUnitTransport(
  transporterID, transporterUnitDefID, transporterTeam,
  transporteeID, transporteeUnitDefID, transporteeTeam
)
  for _,g in r_ipairs(self.AllowUnitTransportList) do
    if (not g:AllowUnitTransport(
      transporterID, transporterUnitDefID, transporterTeam,
      transporteeID, transporteeUnitDefID, transporteeTeam
    )) then
      return false
    end
  end
  return true
end

function gadgetHandler:AllowUnitTransportLoad(
  transporterID, transporterUnitDefID, transporterTeam,
  transporteeID, transporteeUnitDefID, transporteeTeam,
  loadPosX, loadPosY, loadPosZ
)
  for _,g in r_ipairs(self.AllowUnitTransportLoadList) do
    if (not g:AllowUnitTransportLoad(
      transporterID, transporterUnitDefID, transporterTeam,
      transporteeID, transporteeUnitDefID, transporteeTeam,
      loadPosX, loadPosY, loadPosZ
    )) then
      return false
    end
  end
  return true
end

function gadgetHandler:AllowUnitTransportUnload(
  transporterID, transporterUnitDefID, transporterTeam,
  transporteeID, transporteeUnitDefID, transporteeTeam,
  unloadPosX, unloadPosY, unloadPosZ
)
  for _,g in r_ipairs(self.AllowUnitTransportUnloadList) do
    if (not g:AllowUnitTransportUnload(
      transporterID, transporterUnitDefID, transporterTeam,
      transporteeID, transporteeUnitDefID, transporteeTeam,
      unloadPosX, unloadPosY, unloadPosZ
    )) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowUnitCloak(unitID, enemyID)
  for _,g in r_ipairs(self.AllowUnitCloakList) do
    if (not g:AllowUnitCloak(unitID, enemyID)) then
      return false
    end
  end

  return true
end

function gadgetHandler:AllowUnitDecloak(unitID, objectID, weaponID)
  for _,g in r_ipairs(self.AllowUnitDecloakList) do
    if (not g:AllowUnitDecloak(unitID, objectID, weaponID)) then
      return false
    end
  end

  return true
end


function gadgetHandler:AllowUnitKamikaze(unitID, targetID)
  for _,g in r_ipairs(self.AllowUnitKamikazeList) do
    if (not g:AllowUnitKamikaze(unitID, targetID)) then
      return false
    end
  end

  return true
end


function gadgetHandler:AllowFeatureBuildStep(builderID, builderTeam, featureID, featureDefID, part)
  for _,g in r_ipairs(self.AllowFeatureBuildStepList) do
    if (not g:AllowFeatureBuildStep(builderID, builderTeam, featureID, featureDefID, part)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowFeatureCreation(featureDefID, teamID, x, y, z)
  for _,g in r_ipairs(self.AllowFeatureCreationList) do
    if (not g:AllowFeatureCreation(featureDefID, teamID, x, y, z)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowResourceLevel(teamID, res, level)
  for _,g in r_ipairs(self.AllowResourceLevelList) do
    if (not g:AllowResourceLevel(teamID, res, level)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowResourceTransfer(oldTeamID, newTeamID, res, amount)
  for _,g in r_ipairs(self.AllowResourceTransferList) do
    if (not g:AllowResourceTransfer(oldTeamID, newTeamID, res, amount)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowDirectUnitControl(unitID, unitDefID, unitTeam, playerID)
  for _,g in r_ipairs(self.AllowDirectUnitControlList) do
    if (not g:AllowDirectUnitControl(unitID, unitDefID, unitTeam, playerID)) then
      return false
    end
  end
  return true
end


function gadgetHandler:AllowBuilderHoldFire(unitID, unitDefID, action)
  for _,g in r_ipairs(self.AllowBuilderHoldFireList) do
    if (not g:AllowBuilderHoldFire(unitID, unitDefID, action)) then
      return false
    end
  end
  return true
end


function gadgetHandler:MoveCtrlNotify(unitID, unitDefID, unitTeam, data)
  local state = false
  for _,g in r_ipairs(self.MoveCtrlNotifyList) do
    if (g:MoveCtrlNotify(unitID, unitDefID, unitTeam, data)) then
      state = true
    end
  end
  return state
end


function gadgetHandler:TerraformComplete(unitID, unitDefID, unitTeam, buildUnitID, buildUnitDefID, buildUnitTeam)
  for _,g in r_ipairs(self.TerraformCompleteList) do
    if (g:TerraformComplete(unitID, unitDefID, unitTeam, buildUnitID, buildUnitDefID, buildUnitTeam)) then
      return true
    end
  end
  return false
end



function gadgetHandler:AllowWeaponTargetCheck(attackerID, attackerWeaponNum, attackerWeaponDefID)
	local ignore = true
	for _, g in r_ipairs(self.AllowWeaponTargetCheckList) do
		local allowCheck, ignoreCheck = g:AllowWeaponTargetCheck(attackerID, attackerWeaponNum, attackerWeaponDefID)
		if not ignoreCheck then
			ignore = false
			if not allowCheck then
				return 0
			end
		end
	end

	return ((ignore and -1) or 1)
end

function gadgetHandler:AllowWeaponTarget(attackerID, targetID, attackerWeaponNum, attackerWeaponDefID, defPriority)
	local allowed = true
	local priority = 1.0

	for _, g in r_ipairs(self.AllowWeaponTargetList) do
		local targetAllowed, targetPriority = g:AllowWeaponTarget(attackerID, targetID, attackerWeaponNum, attackerWeaponDefID, defPriority)

		if (not targetAllowed) then
			allowed = false; break
		end

		priority = math.max(priority, targetPriority)
	end

	return allowed, priority
end

function gadgetHandler:AllowWeaponInterceptTarget(interceptorUnitID, interceptorWeaponNum, interceptorTargetID)
	for _, g in r_ipairs(self.AllowWeaponInterceptTargetList) do
		if (not g:AllowWeaponInterceptTarget(interceptorUnitID, interceptorWeaponNum, interceptorTargetID)) then
			return false
		end
	end

	return true
end


--------------------------------------------------------------------------------
--
--  Unit call-ins
--

function gadgetHandler:UnitCreated(unitID, unitDefID, unitTeam, builderID)
  for _,g in r_ipairs(self.UnitCreatedList) do
    g:UnitCreated(unitID, unitDefID, unitTeam, builderID)
  end
end


function gadgetHandler:UnitFinished(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitFinishedList) do
    g:UnitFinished(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitFromFactory(
  unitID, unitDefID, unitTeam,
  factID, factDefID, userOrders
)
  for _,g in r_ipairs(self.UnitFromFactoryList) do
    g:UnitFromFactory(unitID, unitDefID, unitTeam,
                      factID, factDefID, userOrders)
  end
end


function gadgetHandler:UnitReverseBuilt(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitReverseBuiltList) do
    g:UnitReverseBuilt(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitDestroyed(
  unitID,     unitDefID,     unitTeam,
  attackerID, attackerDefID, attackerTeam
)
  for _,g in r_ipairs(self.UnitDestroyedList) do
    g:UnitDestroyed(
      unitID,     unitDefID,     unitTeam,
      attackerID, attackerDefID, attackerTeam
    )
  end
end


function gadgetHandler:RenderUnitDestroyed(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.RenderUnitDestroyedList) do
    g:RenderUnitDestroyed(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitExperience(unitID, unitDefID, unitTeam,
                                      experience, oldExperience)
  for _,g in r_ipairs(self.UnitExperienceList) do
    g:UnitExperience(unitID, unitDefID, unitTeam, experience, oldExperience)
  end
end


function gadgetHandler:UnitIdle(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitIdleList) do
    g:UnitIdle(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitCmdDone(unitID, unitDefID, unitTeam, cmdID, cmdParams, cmdOpts, cmdTag)
  for _,g in r_ipairs(self.UnitCmdDoneList) do
    g:UnitCmdDone(unitID, unitDefID, unitTeam, cmdID, cmdParams, cmdOpts, cmdTag)
  end
end

function gadgetHandler:UnitCommand(
	unitID, unitDefID, unitTeam,
	cmdID, cmdParams, cmdOpts, cmdTag,
	playerID, fromSynced, fromLua
)
  for _,g in r_ipairs(self.UnitCommandList) do
    g:UnitCommand(
		unitID, unitDefID, unitTeam,
		cmdID, cmdParams, cmdOpts, cmdTag,
		playerID, fromSynced, fromLua
	)
  end
end

function gadgetHandler:UnitPreDamaged(
  unitID,
  unitDefID,
  unitTeam,
  damage,
  paralyzer,
  weaponDefID,
  projectileID,
  attackerID,
  attackerDefID,
  attackerTeam
)
  local retDamage = damage
  local retImpulse = 1.0

  for _,g in r_ipairs(self.UnitPreDamagedList) do
    dmg, imp = g:UnitPreDamaged(
      unitID, unitDefID, unitTeam,
      retDamage, paralyzer,
      weaponDefID, projectileID,
      attackerID, attackerDefID, attackerTeam
    )

    if (dmg ~= nil) then retDamage = dmg end
    if (imp ~= nil) then retImpulse = imp end
  end

  return retDamage, retImpulse
end


function gadgetHandler:UnitDamaged(
  unitID,
  unitDefID,
  unitTeam,
  damage,
  paralyzer,
  weaponDefID,
  projectileID,
  attackerID,
  attackerDefID,
  attackerTeam
)
  for _,g in r_ipairs(self.UnitDamagedList) do
    g:UnitDamaged(unitID, unitDefID, unitTeam,
                  damage, paralyzer, weaponDefID, projectileID,
                  attackerID, attackerDefID, attackerTeam)
  end
end

function gadgetHandler:UnitStunned(unitID, unitDefID, unitTeam, stunned)
  for _,g in r_ipairs(self.UnitStunnedList) do
    g:UnitStunned(unitID, unitDefID, unitTeam, stunned)
  end
end


function gadgetHandler:UnitTaken(unitID, unitDefID, unitTeam, newTeam)
  for _,g in r_ipairs(self.UnitTakenList) do
    g:UnitTaken(unitID, unitDefID, unitTeam, newTeam)
  end
end


function gadgetHandler:UnitGiven(unitID, unitDefID, unitTeam, oldTeam)
  for _,g in r_ipairs(self.UnitGivenList) do
    g:UnitGiven(unitID, unitDefID, unitTeam, oldTeam)
  end
end


function gadgetHandler:UnitEnteredRadar(unitID, unitTeam, allyTeam, unitDefID)
  for _,g in r_ipairs(self.UnitEnteredRadarList) do
    g:UnitEnteredRadar(unitID, unitTeam, allyTeam, unitDefID)
  end
end


function gadgetHandler:UnitEnteredLos(unitID, unitTeam, allyTeam, unitDefID)
  for _,g in r_ipairs(self.UnitEnteredLosList) do
    g:UnitEnteredLos(unitID, unitTeam, allyTeam, unitDefID)
  end
end


function gadgetHandler:UnitLeftRadar(unitID, unitTeam, allyTeam, unitDefID)
  for _,g in r_ipairs(self.UnitLeftRadarList) do
    g:UnitLeftRadar(unitID, unitTeam, allyTeam, unitDefID)
  end
end


function gadgetHandler:UnitLeftLos(unitID, unitTeam, allyTeam, unitDefID)
  for _,g in r_ipairs(self.UnitLeftLosList) do
    g:UnitLeftLos(unitID, unitTeam, allyTeam, unitDefID)
  end
end


function gadgetHandler:UnitEnteredWater(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitEnteredWaterList) do
    g:UnitEnteredWater(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitLeftWater(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitLeftWaterList) do
    g:UnitLeftWater(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitEnteredAir(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitEnteredAirList) do
    g:UnitEnteredAir(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitLeftAir(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitLeftAirList) do
    g:UnitLeftAir(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitSeismicPing(x, y, z, strength,
                                       allyTeam, unitID, unitDefID)
  for _,g in r_ipairs(self.UnitSeismicPingList) do
    g:UnitSeismicPing(x, y, z, strength,
                      allyTeam, unitID, unitDefID)
  end
end


function gadgetHandler:UnitLoaded(unitID, unitDefID, unitTeam,
                                  transportID, transportTeam)
  for _,g in r_ipairs(self.UnitLoadedList) do
    g:UnitLoaded(unitID, unitDefID, unitTeam,
                 transportID, transportTeam)
  end
end


function gadgetHandler:UnitUnloaded(unitID, unitDefID, unitTeam,
                                    transportID, transportTeam)
  for _,g in r_ipairs(self.UnitUnloadedList) do
    g:UnitUnloaded(unitID, unitDefID, unitTeam,
                   transportID, transportTeam)
  end
end


function gadgetHandler:UnitCloaked(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitCloakedList) do
    g:UnitCloaked(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitDecloaked(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitDecloakedList) do
    g:UnitDecloaked(unitID, unitDefID, unitTeam)
  end
end


function gadgetHandler:UnitUnitCollision(colliderID, collideeID)
	for _,g in r_ipairs(self.UnitUnitCollisionList) do
		if (g:UnitUnitCollision(colliderID, collideeID)) then
			return true
		end
	end

	return false
end

function gadgetHandler:UnitFeatureCollision(colliderID, collideeID)
	for _,g in r_ipairs(self.UnitFeatureCollisionList) do
		if (g:UnitFeatureCollision(colliderID, collideeID)) then
			return true
		end
	end

	return false
end


function gadgetHandler:StockpileChanged(unitID, unitDefID, unitTeam,
                                        weaponNum, oldCount, newCount)
  for _,g in r_ipairs(self.StockpileChangedList) do
    g:StockpileChanged(unitID, unitDefID, unitTeam,
                       weaponNum, oldCount, newCount)
  end
end

function gadgetHandler:UnitHarvestStorageFull(unitID, unitDefID, unitTeam)
  for _,g in r_ipairs(self.UnitHarvestStorageFullList) do
    g:UnitHarvestStorageFull(unitID, unitDefID, unitTeam)
  end
end

--------------------------------------------------------------------------------
--
--  Feature call-ins
--

function gadgetHandler:FeatureCreated(featureID, allyTeam)
  for _,g in r_ipairs(self.FeatureCreatedList) do
    g:FeatureCreated(featureID, allyTeam)
  end
end


function gadgetHandler:FeatureDestroyed(featureID, allyTeam)
  for _,g in r_ipairs(self.FeatureDestroyedList) do
    g:FeatureDestroyed(featureID, allyTeam)
  end
end

function gadgetHandler:FeatureDamaged(
  featureID,
  featureDefID,
  featureTeam,
  damage,
  weaponDefID,
  projectileID,
  attackerID,
  attackerDefID,
  attackerTeam
)
  for _,g in r_ipairs(self.FeatureDamagedList) do
    g:FeatureDamaged(featureID, featureDefID, featureTeam,
                    damage, weaponDefID, projectileID,
                    attackerID, attackerDefID, attackerTeam)
  end
end

function gadgetHandler:FeaturePreDamaged(
  featureID,
  featureDefID,
  featureTeam,
  damage,
  weaponDefID,
  projectileID,
  attackerID,
  attackerDefID,
  attackerTeam
)
  local retDamage = damage
  local retImpulse = 1.0

  for _,g in r_ipairs(self.FeaturePreDamagedList) do
    dmg, imp = g:FeaturePreDamaged(
      featureID, featureDefID, featureTeam,
      retDamage,
      weaponDefID, projectileID,
      attackerID, attackerDefID, attackerTeam
    )

    if (dmg ~= nil) then retDamage = dmg end
    if (imp ~= nil) then retImpulse = imp end
  end

  return retDamage, retImpulse
end


--------------------------------------------------------------------------------
--
--  Projectile call-ins
--

function gadgetHandler:ProjectileCreated(proID, proOwnerID, proWeaponDefID)
  for _,g in r_ipairs(self.ProjectileCreatedList) do
    g:ProjectileCreated(proID, proOwnerID, proWeaponDefID)
  end
end

function gadgetHandler:ProjectileDestroyed(proID)
  for _,g in r_ipairs(self.ProjectileDestroyedList) do
    g:ProjectileDestroyed(proID)
  end
end


--------------------------------------------------------------------------------
--
--  Shield call-ins
--

function gadgetHandler:ShieldPreDamaged(
  proID,
  proOwnerID,
  shieldEmitterWeapNum,
  shieldCarrierUnitID,
  bounceProj,
  beamEmitterWeapNum,
  beamEmitterUnitID,
  spx,
  spy,
  spz,
  hpx,
  hpy,
  hpz
)
  for _,g in r_ipairs(self.ShieldPreDamagedList) do
    -- first gadget to handle this consumes the event
    if (g:ShieldPreDamaged(proID, proOwnerID, shieldEmitterWeapNum, shieldCarrierUnitID, bounceProj, beamEmitterWeapNum, beamEmitterUnitID, spx, spy, spz, hpx, hpy, hpz)) then
      return true
    end
  end

  return false
end


--------------------------------------------------------------------------------
--
--  Misc call-ins
--

function gadgetHandler:Explosion(weaponID, px, py, pz, ownerID, projectileID)
	-- "noGfx = noGfx or ..." short-circuits, so equivalent to this
	for _,g in r_ipairs(self.ExplosionList) do
		if (g:Explosion(weaponID, px, py, pz, ownerID, projectileID)) then
			return true
		end
	end

	return false
end


--------------------------------------------------------------------------------
--
--  Draw call-ins
--

function gadgetHandler:Update()
  for _,g in r_ipairs(self.UpdateList) do
    g:Update()
  end
end


function gadgetHandler:DefaultCommand(type, id, cmd)
  for _,g in r_ipairs(self.DefaultCommandList) do
    local id = g:DefaultCommand(type, id, cmd)
    if (id) then
      return id
    end
  end
end

function gadgetHandler:CommandNotify(id, params, options)
  for _,g in r_ipairs(self.CommandNotifyList) do
    if (g:CommandNotify(id, params, options)) then
      return true
    end
  end
  return false
end


function gadgetHandler:DrawGenesis()
  for _,g in r_ipairs(self.DrawGenesisList) do
    g:DrawGenesis()
  end
end

function gadgetHandler:DrawWater()
  for _,g in r_ipairs(self.DrawWaterList) do
    g:DrawWater()
  end
end

function gadgetHandler:DrawSky()
  for _,g in r_ipairs(self.DrawSkyList) do
    g:DrawSky()
  end
end

function gadgetHandler:DrawSun()
  for _,g in r_ipairs(self.DrawSunList) do
    g:DrawSun()
  end
end

function gadgetHandler:DrawGrass()
  for _,g in r_ipairs(self.DrawGrassList) do
    g:DrawGrass()
  end
end

function gadgetHandler:DrawTrees()
  for _,g in r_ipairs(self.DrawTreesList) do
    g:DrawTrees()
  end
end

function gadgetHandler:DrawWorld()
  for _,g in r_ipairs(self.DrawWorldList) do
    g:DrawWorld()
  end
end

function gadgetHandler:DrawWorldPreUnit()
  for _,g in r_ipairs(self.DrawWorldPreUnitList) do
    g:DrawWorldPreUnit()
  end
end

function gadgetHandler:DrawWorldPreParticles()
  for _,g in r_ipairs(self.DrawWorldPreParticlesList) do
    g:DrawWorldPreParticles()
  end
end

function gadgetHandler:DrawWorldShadow()
  for _,g in r_ipairs(self.DrawWorldShadowList) do
    g:DrawWorldShadow()
  end
end

function gadgetHandler:DrawWorldReflection()
  for _,g in r_ipairs(self.DrawWorldReflectionList) do
    g:DrawWorldReflection()
  end
end

function gadgetHandler:DrawWorldRefraction()
  for _,g in r_ipairs(self.DrawWorldRefractionList) do
    g:DrawWorldRefraction()
  end
end


function gadgetHandler:DrawGroundPreForward()
  for _,g in r_ipairs(self.DrawGroundPreForwardList) do
    g:DrawGroundPreForward()
  end
end

function gadgetHandler:DrawGroundPostForward()
  for _,g in r_ipairs(self.DrawGroundPostForwardList) do
    g:DrawGroundPostForward()
  end
end

function gadgetHandler:DrawGroundPreDeferred()
  for _,g in r_ipairs(self.DrawGroundPreDeferredList) do
    g:DrawGroundPreDeferred()
  end
end

function gadgetHandler:DrawGroundPostDeferred()
  for _,g in r_ipairs(self.DrawGroundPostDeferredList) do
    g:DrawGroundPostDeferred()
  end
end


function gadgetHandler:DrawUnitsPostDeferred()
  for _,g in r_ipairs(self.DrawUnitsPostDeferredList) do
    g:DrawUnitsPostDeferred()
  end
end

function gadgetHandler:DrawFeaturesPostDeferred()
  for _,g in r_ipairs(self.DrawFeaturesPostDeferredList) do
    g:DrawFeaturesPostDeferred()
  end
end


function gadgetHandler:DrawScreenEffects(vsx, vsy)
  for _,g in r_ipairs(self.DrawScreenEffectsList) do
    g:DrawScreenEffects(vsx, vsy)
  end
end

function gadgetHandler:DrawScreenPost(vsx, vsy)
  for _,g in r_ipairs(self.DrawScreenPostList) do
    g:DrawScreenPost(vsx, vsy)
  end
end

function gadgetHandler:DrawScreen(vsx, vsy)
  for _,g in r_ipairs(self.DrawScreenList) do
    g:DrawScreen(vsx, vsy)
  end
end


function gadgetHandler:DrawInMiniMap(mmsx, mmsy)
  for _,g in r_ipairs(self.DrawInMiniMapList) do
    g:DrawInMiniMap(mmsx, mmsy)
  end
end


function gadgetHandler:SunChanged()
  for _,g in r_ipairs(self.SunChangedList) do
    g:SunChanged()
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadgetHandler:KeyPress(key, mods, isRepeat, label, unicode)
  for _,g in r_ipairs(self.KeyPressList) do
    if (g:KeyPress(key, mods, isRepeat, label, unicode)) then
      return true
    end
  end
  return false
end


function gadgetHandler:KeyRelease(key, mods, label, unicode)
  for _,g in r_ipairs(self.KeyReleaseList) do
    if (g:KeyRelease(key, mods, label, unicode)) then
      return true
    end
  end
  return false
end


function gadgetHandler:TextInput(utf8, ...)
  if (self.tweakMode) then
    return true
  end

  for _,g in r_ipairs(self.TextInputList) do
    if (g:TextInput(utf8, ...)) then
      return true
    end
  end
  return false
end


function gadgetHandler:MousePress(x, y, button)
  local mo = self.mouseOwner
  if (mo) then
    mo:MousePress(x, y, button)
    return true  --  already have an active press
  end
  for _,g in r_ipairs(self.MousePressList) do
    if (g:MousePress(x, y, button)) then
      self.mouseOwner = g
      return true
    end
  end
  return false
end


function gadgetHandler:MouseMove(x, y, dx, dy, button)
  local mo = self.mouseOwner
  if (mo and mo.MouseMove) then
    return mo:MouseMove(x, y, dx, dy, button)
  end
end


function gadgetHandler:MouseRelease(x, y, button)
  local mo = self.mouseOwner
  local mx, my, lmb, mmb, rmb = Spring.GetMouseState()
  if (not (lmb or mmb or rmb)) then
    self.mouseOwner = nil
  end
  if (mo and mo.MouseRelease) then
    return mo:MouseRelease(x, y, button)
  end
  return -1
end


function gadgetHandler:MouseWheel(up, value)
  for _,g in r_ipairs(self.MouseWheelList) do
    if (g:MouseWheel(up, value)) then
      return true
    end
  end
  return false
end


function gadgetHandler:IsAbove(x, y)
  for _,g in r_ipairs(self.IsAboveList) do
    if (g:IsAbove(x, y)) then
      return true
    end
  end
  return false
end


function gadgetHandler:GetTooltip(x, y)
  for _,g in r_ipairs(self.GetTooltipList) do
    if (g:IsAbove(x, y)) then
      local tip = g:GetTooltip(x, y)
      if (string.len(tip) > 0) then
        return tip
      end
    end
  end
  return ''
end


function gadgetHandler:MapDrawCmd(playerID, cmdType, px, py, pz, labelText)
  for _,g in r_ipairs(self.MapDrawCmdList) do
    if (g:MapDrawCmd(playerID, cmdType, px, py, pz, labelText)) then
      return true
    end
  end
  return false
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadgetHandler:DownloadStarted(id)
  for _,g in r_ipairs(self.DownloadStartedList) do
    g:DownloadStarted(id)
  end
end

function gadgetHandler:DownloadQueued(id)
  for _,g in r_ipairs(self.DownloadQueuedList) do
    g:DownloadQueued(id)
  end
end

function gadgetHandler:DownloadFinished(id)
  for _,g in r_ipairs(self.DownloadFinishedList) do
    g:DownloadFinished(id)
  end
end

function gadgetHandler:DownloadFailed(id, errorid)
  for _,g in r_ipairs(self.DownloadFailedList) do
    g:DownloadFailed(id, errorid)
  end
end

function gadgetHandler:DownloadProgress(id, downloaded, total)
  for _,g in r_ipairs(self.DownloadProgressList) do
    g:DownloadProgress(id, downloaded, total)
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadgetHandler:Save(zip)
  for _,g in r_ipairs(self.SaveList) do
    g:Save(zip)
  end
end


function gadgetHandler:Load(zip)
  for _,g in r_ipairs(self.LoadList) do
    g:Load(zip)
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadgetHandler:Pong(pingTag, pktSendTime, pktRecvTime)
  for _,g in r_ipairs(self.PongList) do
    g:Pong(pingTag, pktSendTime, pktRecvTime)
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

gadgetHandler:Initialize()

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

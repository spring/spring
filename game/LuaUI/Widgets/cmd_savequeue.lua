--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    cmd_savequeue.lua
--  brief:   adds command to save and append a unit's queued commands
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "SaveQueue",
    desc      = "Adds '/luaui savequeue' and '/luaui loadqueue' commands",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local savedQueue = {}


function widget:TextCommand(command)
  if (command == "savequeue") then
    SaveQueue()
    return true
  elseif (command == "loadqueue") then
    LoadQueue()
    return true
  end
  return false
end   


--------------------------------------------------------------------------------

local function SelectedString()
  local s = ""
  uidTable = Spring.GetSelectedUnits()
  uidTable.n = nil  --  or use ipairs
  for k,v in pairs(uidTable) do
     s = s .. ' +' .. v
  end
  return s
end


local function GiveUnitOrders(unitID, func)
  local selstr = SelectedString()
  Spring.SendCommands({ "selectunits clear +" .. unitID })
  func(unitID)
  Spring.SendCommands({ "selectunits clear" .. selstr })
end


function SaveQueue()
  local uTable = Spring.GetSelectedUnits()
  uTable.n = nil
  for x,uid in pairs(uTable) do
    savedQueue[uid] = Spring.GetCommandQueue(uid)
  end
end


function LoadQueue()
  local reselect = false
  local selstr = SelectedString()
  local uTable = Spring.GetSelectedUnits()
  uTable.n = nil
  for x,uid in pairs(uTable) do
    local queue = savedQueue[uid]
    if (queue ~= nil) then
      for k,v in ipairs(queue) do  --  in order
        if (not v.options.internal) then
          local opts = {}
          table.insert(opts, "shift") -- appending
          if (v.options.alt)   then table.insert(opts, "alt")   end
          if (v.options.ctrl)  then table.insert(opts, "ctrl")  end
          if (v.options.right) then table.insert(opts, "right") end
          Spring.SendCommands({ "selectunits clear +" .. uid })
          Spring.GiveOrder( v.id, v.params, opts )
          reselect = true
        end
      end
    end
  end
  if (reselect) then
    Spring.SendCommands({"selectunits clear" .. selstr})
  end
end

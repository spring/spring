--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    hook_copyqueue.lua
--  brief:   CTRL+GUARD copies a unit's command queue
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "CopyQueue",
    desc      = "CTRL+GUARD copies a units commands instead of guarding",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:CommandNotify(id, params, options)
  -- GUARD + CTRL = copy command queue
  if ((id ~= CMD.GUARD) or (not options.ctrl)) then
    return false
  end

  local targetID = params[1];
  local queue = Spring.GetCommandQueue(targetID);
  if (queue == nil) then
    return false  --  might not be an ally
  end

  -- copy states
  local states = Spring.GetUnitStates(targetID)
  if (states ~= nil) then
    Spring.GiveOrder(CMD.FIRE_STATE, { states.firestate }, 0)
    Spring.GiveOrder(CMD.MOVE_STATE, { states.movestate }, 0)
    Spring.GiveOrder(CMD.REPEAT,     { states['repeat']  and 1 or 0 }, 0)
    Spring.GiveOrder(CMD.ONOFF,      { states.active     and 1 or 0 }, 0)
    Spring.GiveOrder(CMD.CLOAK,      { states.cloak      and 1 or 0 }, 0)
    Spring.GiveOrder(CMD.TRAJECTORY, { states.trajectory and 1 or 0 }, 0)
  end

  -- copy commands
  local firstShift = false 
  if (not options.shift) then
    Spring.GiveOrder( CMD.STOP, {}, {} )
    firstShift = true
  end
  for k,v in ipairs(queue) do  --  in order
    local opts = v.options
    if (not opts.internal) then
      local newopts = {}
      if (opts.alt)   then table.insert(newopts, "alt")   end
      if (opts.ctrl)  then table.insert(newopts, "ctrl")  end
      if (opts.right) then table.insert(newopts, "right") end
      if (not firstShift) then
        table.insert(newopts, "shift")
      end
      Spring.GiveOrder(v.id, v.params, opts.coded)
      firstShift = false
    end
  end

  return true  --  do not add the original GUARD command
end

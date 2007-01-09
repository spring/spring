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
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


include("spring.h.lua")


function widget:CommandNotify(id, params, options)
  -- GUARD + CTRL = copy command queue
  if ((id == CMD_GUARD) and options.ctrl) then
    local targetID = params[1];
    local queue = Spring.GetCommandQueue(targetID);
    if (queue ~= nil) then  --  might not be an ally
      if (not options.shift) then
        Spring.GiveOrder( CMD_STOP, {}, {} )
      end
      local first = next(queue, nil)
      for k,v in ipairs(queue) do  --  in order
        local opts = v.options
        if (not opts.internal) then
          local newopts = {}
          if (opts.alt)   then table.insert(newopts, "alt")   end
          if (opts.ctrl)  then table.insert(newopts, "ctrl")  end
          if (opts.shift) then table.insert(newopts, "shift") end
          if (opts.right) then table.insert(newopts, "right") end
          if (k == first) then
            if (options.ctrl) then
              table.insert(newopts, "shift")
            end
          end
          Spring.GiveOrder( v.id, v.params, newopts )
        end
      end  
      return true  --  do not add the original GUARD command
    end
  end  
  return false
end

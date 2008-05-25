--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    messages.lua
--  brief:   messages.tdf lua parser
--  author:  Craig Lawrence, Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = VFS.Include('gamedata/parse_tdf.lua')

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not VFS.FileExists('gamedata/messages.tdf')) then
  return false
end

local tdfMsgs, err = TDF.Parse('gamedata/messages.tdf')
if (tdfMsgs == nil) then
  error('Error parsing messages.tdf: ' .. err)
end
if (type(tdfMsgs.messages) ~= 'table') then
  error('Missing "messages" table')
end

local luaMsgs = {}

for label, msgs in pairs(tdfMsgs.messages) do
  if ((type(label) == 'string') and
      (type(msgs)  == 'table')) then
    local msgArray = {}
    for _, msg in pairs(msgs) do
      if (type(msg) == 'string') then
        table.insert(msgArray, msg)
      end
    end
    luaMsgs[label:lower()] = msgArray -- lower case keys
  end
end


--------------------------------------------------------------------------------

return luaMsgs

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

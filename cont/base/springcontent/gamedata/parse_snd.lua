--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    parse_snd.lua
--  brief:   sounds.tdf parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = VFS.Include('gamedata/parse_tdf.lua')

local function SetupSounds(soundTable)
  local newTbl = {}
  for k, v in pairs(soundTable) do
    if ((type(k) == 'string') and (type(v) == 'string')) then
      local lower = string.lower(k)
      local s, e, name, num = string.find(lower, '([^%d]+)(%d*)')
      if (name) then
        num = tonumber(num)
        if (num) then
          newTbl[name] = newTbl[name] or {}
          newTbl[name][num] = string.lower(v)
        else
          newTbl[name] = string.lower(v)
        end
      end
    end
  end
  return newTbl
end


local function ParseSND(filename)
  local snds, err = TDF.Parse(filename)
  if (snds == nil) then
    return nil, err
  end

  for k,v in pairs(snds) do
--    print('sound.tdf:  ' .. tostring(k) .. ' = ' .. tostring(v))

    local luaSnds = SetupSounds(v)
    snds[k] = luaSnds

    for kx,vx in pairs(luaSnds) do
--      print(tostring(kx), tostring(vx))
      if (type(vx) == 'table') then
        for kt,vt in pairs(vx) do
--          print(kt, vt)
        end
      end
    end
  end

  return snds
end


return { Parse = ParseSND }

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

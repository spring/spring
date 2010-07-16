--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    parsetdf.lua
--  brief:   lua tdf parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (TDFparser) then
  return TDFparser
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local pos = 1

local text = ''

local tdfName = ''

local keyFilter = string.lower

local _ -- junk value


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function GetLineNumber(x)
  x = x and x or pos
  local t = text  
  local p = 1
  local n = 1
  local l = ''
  while (true) do
    _, _, l, p = t:find('([^\n]*)\n()', p)
    if ((not p) or (p > x)) then
      break
    end
    n = n + 1
  end
  return n, l
end


local function Crash(msg)
  local num, line = GetLineNumber(pos)
  line = line and ('  "' .. line:gsub('\r', '') .. '"') or ''
  local name = (#tdfName > 0) and (tdfName .. ' ') or ''
  msg = ('%s (%sline %i)%s'):format(msg, name, num, line)
  error(msg)
end


--------------------------------------------------------------------------------

local function CurrChar()
  if (not pos) then
    return nil
  end
  return text:sub(pos, pos)
end


local function EatWhite()
  if (not pos) then
    return
  end
  repeat
    local ppos = pos
    local s, e, p -- start, end, pos
    if (text:sub(pos, pos) ~= '/') then
      -- whitespace
      s, e, p = text:find('^%s*()', pos)
      if (p) then pos = p end
    else
      -- line comments
      s, e, p = text:find('^//[^\n]*()', pos)
      if (p) then pos = p end
      -- block comments
      s, e, p = text:find('^/%*.-*/()', pos)
      if (p) then pos = p end
    end
  until (ppos == pos)
end


--------------------------------------------------------------------------------

local ParseKey
local ParseValue
local ParseSection
local ParseElements


ParseKey = function()
  local s, e, k, p = text:find('^([^%s=]+)[ \t]*=[ \t]*()', pos)
  if (not k) then
    Crash('could not find key')
  end
  pos = p
  return k
end


ParseValue = function()
  -- look for quoted string
  local s, e, v, p = text:find('^"([^\n"]*)"[ \t]*;+()', pos)
  if (v) then
    pos = p
    return v
  end
  -- unquoted string (can not contain ';')
  local s, e, v, p = text:find('^([^\n;]*);+()', pos)
  if (v) then
    pos = p
    return v
  end
  Crash('could not find value, missing ";" ?')
end


ParseSection = function()
  local s, e, h, p = text:find('^%[([^%]]+)%]()', pos)
  if (not h) then
    Crash('missing section')
  end
  pos = p

  EatWhite()
  if (CurrChar() ~= '{') then
    Crash('missing section start brace')
  end
  pos = pos + 1

  local t, c = ParseElements()
  if (c ~= '}') then
    Crash('missing section end brace')
  end

  return h, t
end


-- elements can be either sections, or key/value pairs

ParseElements = function()
  local t = {}

  while (true) do
    EatWhite()

    local c = CurrChar()
    if (c == '') then
      return t, nil
    elseif (c == '}') then
      pos = pos + 1
      return t, c
    else
      local k, v
      if (c == '[') then
        k, v = ParseSection()
      else
        k, v = ParseKey(), ParseValue()
      end
      t[keyFilter(k)] = v
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function Parse(newText)
  text = newText
  pos = 1

  local success, data = pcall(function()
    local d, char = ParseElements()
    if (char ~= nil) then
      Crash('extra junk')
    end
    return d
  end)

  if (success) then
    return data
  else
    return nil, data
  end
end


local function ParseFile(filename)
  local fileText, err = VFS.LoadFile(filename)
  if (fileText == nil) then
    return nil, err
  end
  tdfName = filename
  return Parse(fileText)
end


local function ParseText(srcText)
  tdfName = ''
  return Parse(srcText)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function GetKeyFilter()
  return keyFilter
end


local function SetKeyFilter(filter)
  if (type(filter) == 'function') then
    keyFilter = filter
  else
    error('TDF.SetKeyFilter(): filter must be a function')
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

TDFparser = {
  ParseText       = ParseText,
  Parse           = ParseFile,
  ParseFile       = ParseFile,
  GetKeyFilter    = GetKeyFilter,
  SetKeyFilter    = SetKeyFilter,
  AllowDuplicates = AllowDuplicates,
}

return TDFparser

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

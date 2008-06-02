--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    parsetdf.lua
--  brief:   lua tdf parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  NOTES:
--
--    - Although this parser is faster then the C++ TDFParser, it still suffers
--      from a few serious slowdowns. Running global substitutions for dos2unix,
--      line comments, and block comments is probably not the best way to go.
--
--    - Add more complete error checking / handling
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (TDFparser) then
  return TDFparser
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local keyFilter = string.lower

local noDuplicates = false

local debug = false


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function FindLineNum(pos, text)
  local lineNum = 1
  local p = 1
  while (p ~= nil) do
    local s, e = string.find(text, '^[^\n]*\n', p) 
    if (s == nil) then
      return lineNum
    end
    p = e + 1
    if (p > pos) then
      return lineNum
    end
    lineNum = lineNum + 1
  end
  return lineNum
end


--------------------------------------------------------------------------------

local function DosToUnix(text)
  return string.gsub(text, '\r', '')
end


local function ReplaceWithSpaces(text)
  return string.gsub(text, '[^\n]', ' ')
end


local function StripLineComments(text)
  return string.gsub(text, '//[^\n]*[\n]?', ReplaceWithSpaces)
end


local function StripBlockComments(text)
  return string.gsub(text, '/%*.-%*/', ReplaceWithSpaces)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local cleanText

local stack = { {} }
local current = stack[1]


--------------------------------------------------------------------------------

local function ParseElement(text, pos)
  if (debug) then
    print('pos = ' .. tostring(pos))
  end

  local sectionPattern = '^%s*%[([^%]]+)%]%s*{'
  local s, e, sec = string.find(text, sectionPattern, pos)
  if (sec) then
    sec = keyFilter(sec)
    if (noDuplicates and current[sec]) then
      return nil, 'duplicate entry: ' .. sec
    end
    local newSection = {}
    table.insert(stack, newSection)
    current[sec] = newSection
    current = newSection
    if (debug) then
      print('  SECTION', sec, e + 1)
      print(type(e))
    end
    return e + 1
  end

  local endSecPattern = '^(%s*})'
  local s, e, endText = string.find(text, endSecPattern, pos)
  if (s) then
    local stackSize = #stack
    if (stackSize <= 1) then
      return nil, 'table underrun'
    else
      table.remove(stack)
      current = stack[stackSize - 1]
    end
    if (debug) then
      print('  ENDSECTION', endText, e + 1)
    end
    return e + 1
  end

  local entryPattern = '^%s*([^%s=]*)%s*=%s*([^;\n]*);'
  local s, e, k, v = string.find(text, entryPattern, pos)
  if (k and v) then
    k = keyFilter(k)
    if (noDuplicates and current[k]) then
      return nil, 'duplicate entry: ' .. k
    end

    local _, _, v = string.find(v, '(.-)%s*$') -- remove trailing space
    current[k] = v
    if (debug) then
      print('  PAIR:  ' .. k .. ' = ' .. v,
            (e + 1) .. '/' .. FindLineNum(pos, cleanText))
    end
    return e + 1
  end

  if (string.find(text, '^%s*$', pos)) then
    return nil
  end

  return nil, 'Bad TDF text at line ' .. FindLineNum(pos, cleanText)
end


--------------------------------------------------------------------------------

local function Parse(text)

  text = DosToUnix(text)
  text = StripLineComments(text)
  text = StripBlockComments(text)

  cleanText = text

  stack = { {} }
  current = stack[1]

  local pos = 1

  while (pos) do
    pos, err = ParseElement(text, pos)
    if (err) then
      return nil, err
    end
    if ((pos == nil) or (pos > #text)) then
      if (#stack == 1) then
        break;
      else
        return nil, 'missing } table terminations'
      end
    end
  end

  return stack[1]
end


local function ParseFile(filename)
  if (debug) then
    print('TDF.Parse: ' .. tostring(filename))
  end
  local text, err = VFS.LoadFile(filename)
  if (text == nil) then
    return nil, err
  end
  return Parse(text)
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


local function AllowDuplicates(state)
  if (type(state) == 'boolean') then
    noDuplicates = not state
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

TDFparser = {
  Parse           = ParseFile,
  ParseText       = Parse,
  GetKeyFilter    = GetKeyFilter,
  SetKeyFilter    = SetKeyFilter,
  AllowDuplicates = AllowDuplicates,
}

return TDFparser

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

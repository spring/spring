--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    fonts.lua
--  brief:   font handler, uses texture atlases from BZFlag
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (fontHandler ~= nil) then
  return fontHandler
end


local DefaultFontName = LUAUI_DIRNAME .. "Fonts/ProFont_12"
local DefaultFontName = LUAUI_DIRNAME .. "Fonts/test"


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Local speedups
--

local strlen   = string.len
local strsub   = string.sub
local strbyte  = string.byte
local strchar  = string.char
local strfind  = string.find
local strgfind = string.gfind


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local fonts = {}
local activeFont  = nil
local defaultFont = nil

local caching = true

local timeStamp  = 0
local lastUpdate = 0

local debug = true
local origPrint = print
local print = function(...)
  if (debug) then
    origPrint(unpack(arg))
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function LoadFontSpecs(fontName)
  local chunk = loadfile(fontName .. ".lua")
  if (not chunk) then
    return nil
  end
  local fontSpecs = chunk()

  print('fontSpecs.srcFile  = ' .. fontSpecs.srcFile)
  print('fontSpecs.family   = ' .. fontSpecs.family)
  print('fontSpecs.style    = ' .. fontSpecs.style)
  print('fontSpecs.height   = ' .. fontSpecs.height)
  print('fontSpecs.yStep    = ' .. fontSpecs.yStep)
  print('fontSpecs.xTexSize = ' .. fontSpecs.xTexSize)
  print('fontSpecs.yTexSize = ' .. fontSpecs.yTexSize)

  return fontSpecs
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function MakeDisplayLists(fontSpecs)
  local lists = {}
  local xs = fontSpecs.xTexSize
  local ys = fontSpecs.yTexSize
  for _,gi in pairs(fontSpecs.glyphs) do
    local list = gl.ListCreate(function ()
      gl.TexRect(gi.oxn, gi.oyn, gi.oxp, gi.oyp,
                 gi.txn / xs, 1.0 - (gi.tyn / ys),
                 gi.txp / xs, 1.0 - (gi.typ / ys))
      gl.Translate(gi.adv, 0, 0)
    end)
    lists[gi.num] = list
  end
  return lists
end


local function MakeOutlineDisplayLists(fontSpecs)
  local lists = {}
  local tw = fontSpecs.xTexSize
  local th = fontSpecs.yTexSize

  for _,gi in pairs(fontSpecs.glyphs) do
    local w = gi.xmax - gi.xmin
    local h = gi.ymax - gi.ymin
    local txn = gi.xmin / tw
    local tyn = gi.ymax / th
    local txp = gi.xmax / tw
    local typ = gi.ymin / th
    
    local list = gl.ListCreate(function ()
      gl.Translate(gi.initDist, 0, 0)

      gl.Color(0, 0, 0, 0.75)
      local o = 2
      gl.TexRect( o,  o, w, h, txn, tyn, txp, typ)
      gl.TexRect(-o,  o, w, h, txn, tyn, txp, typ)
      gl.TexRect( o,  0, w, h, txn, tyn, txp, typ)
      gl.TexRect(-o,  0, w, h, txn, tyn, txp, typ)
      gl.TexRect( o, -o, w, h, txn, tyn, txp, typ)
      gl.TexRect(-o, -o, w, h, txn, tyn, txp, typ)
      gl.TexRect( 0,  o, w, h, txn, tyn, txp, typ)
      gl.TexRect( 0, -o, w, h, txn, tyn, txp, typ)

      gl.Color(1, 1, 1, 1)
      gl.TexRect( 0,  0, w, h, txn, tyn, txp, typ)

      gl.Translate(gi.width + gi.whitespace, 0, 0)
    end)

    lists[gi.num] = list
  end
  return lists
end


--------------------------------------------------------------------------------

local function CalcTextWidth(text)
  local specs = activeFont.specs
  local w = 0
  for i = 1, strlen(text) do
    local c = strbyte(text, i)
    local glyphInfo = specs.glyphs[c]
    if (not glyphInfo) then
      glyphInfo = specs.glyphs[32]
    end
    if (glyphInfo) then
      w = w + glyphInfo.adv
    end
  end
  return w
end


local function CalcTextHeight(text)
  return height
end


--------------------------------------------------------------------------------

local function StripColorCodes(text)
  local stripped = ""
  for txt, color in strgfind(text, "([^\255]*)(\255?.?.?.?)") do
    if (strlen(txt) > 0) then
      stripped = stripped .. txt
    end
  end
  return stripped
end


--------------------------------------------------------------------------------

local function RawDraw(text)
  local lists = activeFont.lists
  for i = 1, strlen(text) do
    local c = strbyte(text, i)
    local list = lists[c]
    if (list) then
      gl.ListRun(list)
    else
      gl.ListRun(lists[strbyte(" ", 1)])
    end
  end
end


local function RawColorDraw(text)
  for txt, color in strgfind(text, "([^\255]*)(\255?.?.?.?)") do
    if (strlen(txt) > 0) then
      RawDraw(txt)
    end
    if (strlen(color) == 4) then
      gl.Color(strbyte(color, 2) / 255,
               strbyte(color, 3) / 255,
               strbyte(color, 4) / 255)
    end
  end
end


local function DrawNoCache(text, x, y, size, opts)
--local function Draw(text, x, y, size, opts) -- FIXME
  if (not x) then
    RawDraw(text)
  else
    gl.PushMatrix()
    gl.Translate(x, y, 0)
    gl.Texture(activeFont.image)
    RawColorDraw(text)
    gl.PopMatrix()
  end
end


local function Draw(text, x, y, size, opts) -- FIXME
  if (not caching) then
    DrawNoCache(text, x, y, size, opts)
    return
  end

  local cacheTextData = activeFont.cache[text]
  if (not cacheTextData) then
    local textList = gl.ListCreate(function()
      gl.Texture(activeFont.image)
      RawColorDraw(text)  -- FIXME?
    end)
    cacheTextData = { textList, timeStamp }
    activeFont.cache[text] = cacheTextData
  else
    cacheTextData[2] = timeStamp  --  refresh the timeStamp
  end

  if (not x) then
    gl.ListRun(cacheTextData[1])
  else
    gl.PushMatrix()
    gl.Translate(x, y, 0)
    gl.ListRun(cacheTextData[1])
    gl.PopMatrix()
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function DrawRight(text, x, y)
  local stripped = StripColorCodes(text)
  local width = CalcTextWidth(stripped)  -- use cached widths?
  gl.PushMatrix()
  gl.Translate(-width, 0, 0)
  Draw(text, x, y)
  gl.PopMatrix()
end


local function DrawCentered(text, x, y)
  local stripped = StripColorCodes(text)
  local width = CalcTextWidth(stripped)  -- use cached widths?
  gl.PushMatrix()
  gl.Translate(-width * 0.5, 0, 0)
  Draw(text, x, y)
  gl.PopMatrix()
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function LoadFont(fontName)
  if (fonts[fontName]) then
    return nil  -- already loaded
  end

  local baseName = fontName
  local _,_,options,bn = strfind(fontName, "(:.-:)(.*)")
  if (options) then
    baseName = bn
  else
    options = ""
  end

  local fontSpecs = LoadFontSpecs(baseName)
  if (not fontSpecs) then
    return nil  -- bad specs
  end

  local fontLists
  if (strfind(options, "o")) then
    fontLists = MakeOutlineDisplayLists(fontSpecs)
  else
    fontLists = MakeDisplayLists(fontSpecs)
  end
  if (not fontLists) then
    return nil  -- bad display lists
  end

  local font = {}
  font.name  = fontName
  font.base  = baseName
  font.opts  = options
  font.specs = fontSpecs
  font.lists = fontLists
  font.cache = {}
  font.image = options .. baseName .. ".png"
  font.size  = 12.0  -- FIXME

  return font
end


local function SetFont(fontName)
  local font = fonts[fontName]
  if (font) then
    activeFont = font
    return
  end

  font = LoadFont(fontName)
  if (font) then
    print("Loaded font: " .. fontName)
    activeFont = font
    fonts[fontName] = font
    return
  end
end


local function SetDefaultFont()
  activeFont = defaultFont
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function FreeCache(fontName)
  local font = fonts[fontName]
  if (not font) then
    return
  end
  for text,data in pairs(font.cache) do
    gl.ListDelete(data[1])
  end
end


local function FreeFont(fontName)
  local font = fonts[fontName]
  if (not font) then
    return
  end

  for _,list in pairs(font.lists) do
    gl.ListDelete(list)
  end
  for text,data in pairs(font.cache) do
    gl.ListDelete(data[1])
  end
  gl.FreeTexture(font.image)

  fonts[font.name] = nil
end


local function FreeFonts()
  for fontName in pairs(fonts) do
    FreeFont(fontName)
  end
end


local function Update(time)
  timeStamp = time
  if (timeStamp < (lastUpdate + 3)) then
    return  -- only update every 3 seconds
  end

  local killTime = (time - 30)
  for fontName, font in pairs(fonts) do
    local killList = {}
    for text,data in pairs(font.cache) do
      if (data[2] < killTime) then
        gl.ListDelete(data[1])
        table.insert(killList, text)
        print(fontName .. " removed string list(" .. data[1] .. ") " .. text)
      end
    end
    for _,text in ipairs(killList) do
      font.cache[text] = nil
    end
  end
  lastUpdate = timeStamp
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

SetFont(DefaultFontName)
defaultFont = activeFont


local FH = {}

FH.Update = Update

FH.SetFont        = SetFont
FH.SetDefaultFont = SetDefaultFont

FH.GetActiveFont  = function() return activeFont.name end

FH.FreeFont  = FreeFont
FH.FreeFonts = FreeFonts
FH.FreeCache = FreeCache

FH.Draw         = Draw
FH.DrawRight    = DrawRight
FH.DrawCentered = DrawCentered

FH.StripColors = StripColorCodes

FH.CalcTextWidth  = CalcTextWidth
FH.CalcTextHeight = CalcTextHeight

FH.GetYStep     = function() return activeFont.specs.yStep end

FH.CacheState   = function() return caching  end
FH.EnableCache  = function() caching = true  end
FH.DisableCache = function() caching = false end

fontHandler = FH  -- make it global

return FH

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

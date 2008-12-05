--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    fonts.lua
--  brief:   font handler, with automatic texture atlas generation
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

-- ":n:" sets it to nearest texture filtering
local DefaultFontName = ":n:" .. LUAUI_DIRNAME .. "Fonts/FreeMonoBold_12"

Spring.CreateDir(LUAUI_DIRNAME .. 'Fonts')


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Automatically generated local definitions

local glCallList             = gl.CallList
local glColor                = gl.Color
local glCreateList           = gl.CreateList
local glDeleteList           = gl.DeleteList
local glDeleteTexture        = gl.DeleteTexture
local glPopMatrix            = gl.PopMatrix
local glPushMatrix           = gl.PushMatrix
local glTexRect              = gl.TexRect
local glTexture              = gl.Texture
local glTranslate            = gl.Translate
local spGetLastUpdateSeconds = Spring.GetLastUpdateSeconds


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Local speedups
--

local floor     = math.floor
local strlen    = string.len
local strsub    = string.sub
local strbyte   = string.byte
local strchar   = string.char
local strfind   = string.find
local strgmatch = string.gmatch


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local fonts = {}
local activeFont  = nil
local defaultFont = nil

local caching = true
local useFloor = true

local timeStamp  = 0
local lastUpdate = 0

local debug = false
local origPrint = print
local print = function(...)
  if (debug) then
    origPrint(...)
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function HaveFontFiles(fontName)
  if (VFS.FileExists(fontName .. '.lua') and
      VFS.FileExists(fontName .. '.png')) then
    return true
  end
  return false
end


local function CreateFontFiles(fontName)
  local _, _, name, size = string.find(fontName, '^(.*)_(%d*)$')
  if ((not name) or (not size)) then
    return false
  end

  local fullName = name .. '.ttf'
  local inData = VFS.LoadFile(fullName)
  if (not inData) then
    fullName = name .. '.otf'
    inData = VFS.LoadFile(fullName)
  end
  if (not inData) then
    return false
  end
  
  print('CreateFontFiles = ' .. fullName .. ', ' .. size)

  return
    Spring.MakeFont(fullName, {
      inData = inData,
      height = tonumber(size),
      minChar = 0,
      maxChar = 255,
    --[[
      texWidth = 
      outlineMode = 
      outlineRadius = 
      outlineWeight = 
      padding = 
      spacing = 
      debug =
    --]]
    })
end


local function LoadFontSpecs(fontName)
  local specFile = fontName .. ".lua"
  local text = VFS.LoadFile(specFile)
  if (text == nil) then
    return nil
  end
  local chunk, err = loadstring(text, specFile)
  if (not chunk) then
    return nil
  end
  local fontSpecs = chunk()

  print('fontSpecs.srcFile  = ' .. fontSpecs.srcFile)
  print('fontSpecs.family   = ' .. fontSpecs.family)
  print('fontSpecs.style    = ' .. fontSpecs.style)
  print('fontSpecs.size     = ' .. fontSpecs.height)
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
    local list = glCreateList(function ()
      glTexRect(gi.oxn, gi.oyn, gi.oxp, gi.oyp,
                 gi.txn / xs, 1.0 - (gi.tyn / ys),
                 gi.txp / xs, 1.0 - (gi.typ / ys))
      glTranslate(gi.adv, 0, 0)
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
    
    local list = glCreateList(function ()
      glTranslate(gi.initDist, 0, 0)

      glColor(0, 0, 0, 0.75)
      local o = 2
      glTexRect( o,  o, w, h, txn, tyn, txp, typ)
      glTexRect(-o,  o, w, h, txn, tyn, txp, typ)
      glTexRect( o,  0, w, h, txn, tyn, txp, typ)
      glTexRect(-o,  0, w, h, txn, tyn, txp, typ)
      glTexRect( o, -o, w, h, txn, tyn, txp, typ)
      glTexRect(-o, -o, w, h, txn, tyn, txp, typ)
      glTexRect( 0,  o, w, h, txn, tyn, txp, typ)
      glTexRect( 0, -o, w, h, txn, tyn, txp, typ)

      glColor(1, 1, 1, 1)
      glTexRect( 0,  0, w, h, txn, tyn, txp, typ)

      glTranslate(gi.width + gi.whitespace, 0, 0)
    end)

    lists[gi.num] = list
  end
  return lists
end


--------------------------------------------------------------------------------

local function StripColorCodes(text)
  local stripped = ""
  for txt, color in strgmatch(text, "([^\255]*)(\255?.?.?.?)") do
    if (strlen(txt) > 0) then
      stripped = stripped .. txt
    end
  end
  return stripped
end


--------------------------------------------------------------------------------

local function RawGetTextWidth(text)
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


local function GetTextWidth(text)
  -- return the cached value if available
  local cacheTextData = activeFont.cache[text]
  if (cacheTextData) then
    local width = cacheTextData[3]
    if (width) then
      return width
    end
  end
  local stripped = StripColorCodes(text)
  return RawGetTextWidth(stripped)
end


local function CalcTextHeight(text)
  return activeFont.specs.height
end


--------------------------------------------------------------------------------

local function RawDraw(text)
  local lists = activeFont.lists
  for i = 1, strlen(text) do
    local c = strbyte(text, i)
    local list = lists[c]
    if (list) then
      glCallList(list)
    else
      glCallList(lists[strbyte(" ", 1)])
    end
  end
end


local function RawColorDraw(text)
  for txt, color in strgmatch(text, "([^\255]*)(\255?.?.?.?)") do
    if (strlen(txt) > 0) then
      RawDraw(txt)
    end
    if (strlen(color) == 4) then
      glColor(strbyte(color, 2) / 255,
              strbyte(color, 3) / 255,
              strbyte(color, 4) / 255)
    end
  end
end


local function DrawNoCache(text, x, y)
  if (not x) then
    RawDraw(text)
  else
    glPushMatrix()
    glTranslate(x, y, 0)
    glTexture(activeFont.image)
    RawColorDraw(text)
    glTexture(false)
    glPopMatrix()
  end
end


local function Draw(text, x, y)
  if (not caching) then
    DrawNoCache(text, x, y)
    return
  end

  local cacheTextData = activeFont.cache[text]
  if (not cacheTextData) then
    glTexture(activeFont.image) -- else we would _recreate_ the texture each call to the displaylist!
    local textList = glCreateList(function()
      glTexture(activeFont.image)
      RawColorDraw(text)
      glTexture(false)
    end)
    cacheTextData = { textList, timeStamp }  -- param [3] is the width
    activeFont.cache[text] = cacheTextData
  else
    cacheTextData[2] = timeStamp  --  refresh the timeStamp
  end

  if (not x) then
    glCallList(cacheTextData[1])
  else
    glPushMatrix()
    if (useFloor) then
      glTranslate(floor(x), floor(y), 0)
    else
      glTranslate(x, y, 0)
    end
    glCallList(cacheTextData[1])
    glPopMatrix()
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function DrawRight(text, x, y)
  local width = GetTextWidth(text)
  glPushMatrix()
  glTranslate(-width, 0, 0)
  Draw(text, x, y)
  glPopMatrix()
end


local function DrawCentered(text, x, y)
  local width = GetTextWidth(text)
  local halfWidth
  if (useFloor) then
    halfWidth = floor(width * 0.5)
  else
    halfWidth = width * 0.5
  end
  glPushMatrix()
  glTranslate(-halfWidth, 0, 0)
  Draw(text, x, y)
  glPopMatrix()
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function LoadFont(fontName)
  print('LoadFont:  ' .. fontName)

  if (fonts[fontName]) then
    return nil  -- already loaded
  end

  local baseName = fontName
  local _,_,options,bn = strfind(fontName, "(:.-:)(.*)")
  if (options) then
    baseName = bn
  else
    options = ''
  end

  if (not HaveFontFiles(baseName)) then
    CreateFontFiles(baseName)
  end

  local fontSpecs = LoadFontSpecs(baseName)
  if (not fontSpecs) then
    return nil  -- bad specs
  end

  if (not VFS.FileExists(baseName .. ".png")) then
    return nil  -- missing texture
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

  fonts[fontName] = font

  return font
end


local function UseFont(fontName)
  local font = fonts[fontName]
  if (font) then
    activeFont = font
    return true
  end

  font = LoadFont(fontName)
  if (font) then
    activeFont = font
    print("Loaded font: " .. fontName)
    return true
  end

  return false
end


local function UseDefaultFont()
  activeFont = defaultFont
end


local function SetDefaultFont(name)
  local tmpFont = activeFont
  if (UseFont(name)) then
    DefaultFontName = name
    defaultFont = activeFont
  end
  activeFont = tmpFont
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function FreeCache(fontName)
  local font = (fontName == nil) and activeFont or fonts[fontName]
  if (not font) then
    return
  end
  for text,data in pairs(font.cache) do
    glDeleteList(data[1])
  end
end


local function FreeFont(fontName)
  local font = (fontName == nil) and activeFont or fonts[fontName]
  if (not font) then
    return
  end

  for _,list in pairs(font.lists) do
    glDeleteList(list)
  end
  for text,data in pairs(font.cache) do
    glDeleteList(data[1])
  end
  glDeleteTexture(font.image)

  fonts[font.name] = nil
end


local function FreeFonts()
  for fontName in pairs(fonts) do
    FreeFont(fontName)
  end
end


local function Update()
  timeStamp = timeStamp + spGetLastUpdateSeconds()
  if (timeStamp < (lastUpdate + 1.0)) then
    return  -- only update every 1.0 seconds
  end

  local killTime = (timeStamp - 3.0)
  for fontName, font in pairs(fonts) do
    local killList = {}
    for text,data in pairs(font.cache) do
      if (data[2] < killTime) then
        glDeleteList(data[1])
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

UseFont(DefaultFontName)
defaultFont = activeFont


local FH = {}

FH.Update = Update

FH.UseFont        = UseFont
FH.UseDefaultFont = UseDefaultFont
FH.SetDefaultFont = SetDefaultFont

FH.GetFontName  = function() return activeFont.name         end
FH.GetFontSize  = function() return activeFont.specs.height end
FH.GetFontYStep = function() return activeFont.specs.yStep  end
FH.GetTextWidth = GetTextWidth

FH.Draw         = Draw
FH.DrawRight    = DrawRight
FH.DrawCentered = DrawCentered

FH.StripColors = StripColorCodes

FH.FreeFont  = FreeFont
FH.FreeFonts = FreeFonts
FH.FreeCache = FreeCache

FH.CacheState   = function() return caching  end
FH.EnableCache  = function() caching = true  end
FH.DisableCache = function() caching = false end

fontHandler = FH  -- make it global

return FH

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

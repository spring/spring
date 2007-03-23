#!/usr/bin/env lua

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    obj2lua.lua
--  brief:   converts a WaveFront OBJ/MTL model into a custom Spring LUA format
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  NOTE:  The 'vc' (vertex color) keyword is also recognized,
--         along with its additional parameter for face indexing
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local VERSION = "1.0"

local bool inSpring = (Spring ~= nil)

if (not inSpring) then
  require("string")
  require("table")
  require("io")
  require("os")
end


-- local speedups
local print     = print
local pairs     = pairs
local ipairs    = ipairs
local getn      = table.getn
local insert    = table.insert           
local str_find  = string.find
local str_gfind = string.gfind
local str_gsub  = string.gsub


-- utility
local printf = function(fmt, ...)
  print(string.format(fmt, unpack(arg)))
end


--------------------------------------------------------------------------------

if ((not inSpring) and (not arg[1])) then
  print('Usage:  '..arg[0]..' file.obj')
  return
end


local function Basename(fullpath)
  local _,_,base = str_find(fullpath, "([^\\/:]*)$")
  local _,_,path = str_find(fullpath, "(.*[\\/:])[^\\/:]*$")
  if (path == nil) then path = "" end
  return base, path
end


local filename   = arg[1]
local linenum    = 0
local line       = ""
local base, path = Basename(filename)


local infile = io.open(filename, 'r')
if (not infile) then
  print('Could not open: '..arg[1])
  return
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function LineError(msg)
  print(filename .. ":" .. linenum .. ":  " .. msg)
  print("-->  " .. line)
  return false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local mtls = {} -- map of names to material tables
local objs = {} -- array of objects

local verts  = {} -- array of 3D vertices
local txcds  = {} -- array of 2D texture coordinates
local norms  = {} -- array of 3D normals
local colors = {} -- array of 4D colors (not part of the OBJ spec)

local mCount = 0
local oCount = 0

local vCount = 0
local tCount = 0
local nCount = 0
local cCount = 0

local exts = {
  xmin = 0, xmax = 0,
  ymin = 0, ymax = 0,
  zmin = 0, zmax = 0
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function NewObject(name)
  return {
    name = name,
    mats = {},  -- name = faces{}  map
    tricount = 0
  }
end

local currobj = NewObject("")
local currmat = ""


--------------------------------------------------------------------------------

local function NewMaterial(name)
  return {
    name      = name,
    ambient   = { 0.2, 0.2, 0.2, 1.0 },
    diffuse   = { 0.8, 0.8, 0.8, 1.0 },
    emission  = { 0.0, 0.0, 0.0, 1.0 },
    specular  = { 0.0, 0.0, 0.0, 1.0 },
    alpha     = 1.0,
    shininess = 0.0,
    texture   = "",
    lighting  = true
  }
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function Tokenize(line)
  local words = {}
  local wc = 0
  for w in str_gfind(line, "[^%s]+") do
    words[wc] = w
    wc = wc + 1
  end

  if (wc < 1) then return "" end  --  empty line

  local cmd = words[0]
  if (string.sub(cmd, 1, 1) == '#') then return "" end
  wc = wc - 1

  return cmd, wc, words
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local currmtl = nil


local function ParseColor(words, wc)
  if (wc < 3) then
    return nil
  end
  local c = { tonumber(words[1]), tonumber(words[2]), tonumber(words[3]) }
  if ((c[1] == nil) or (c[2] == nil) or (c[3] == nil)) then
    return nil
  end
  c[4] = 1.0
  return c  
end


local function ParseMtlLine(mtlfile)

  local cmd, wc, words = Tokenize(line)
  if (cmd == "") then
    return true
  end
  if (wc < 1) then
    return LineError("missing arguments")
  end

  if (cmd == "newmtl") then
    if (currmtl) then
      mtls[currmtl.name] = currmtl
    end
    currmtl = NewMaterial(words[1])

  elseif (currmtl == nil) then
    return LineError("missing material definition")

  elseif (cmd == "map_Kd") then
    currmtl.texture = words[1]
    
  elseif (cmd == "Ka") then
    local color = ParseColor(words, wc)
    if (color == nil) then
      return LineError("bad ambient color")
    end
    currmtl.ambient = color

  elseif (cmd == "Kd") then
    local color = ParseColor(words, wc)
    if (color == nil) then
      return LineError("bad diffuse color")
    end
    currmtl.diffuse = color

  elseif (cmd == "Ks") then
    local color = ParseColor(words, wc)
    if (color == nil) then
      return LineError("bad specular color")
    end
    currmtl.specular = color

  elseif (cmd == "Ke") then
    local color = ParseColor(words, wc)
    if (color == nil) then
      return LineError("bad emission color")
    end
    currmtl.emission = color 

  elseif (cmd == "d") then
    currmtl.alpha = tonumber(words[1])
    if (not currmtl.alpha) then
      return LineError("bad alpha (d)")
    end

  elseif (cmd == "Ns") then
    local hardness = tonumber(words[1])
    if (not hardness) then
      return LineError("bad shininess (Ns)")
    end
    currmtl.shininess = hardness * (128 / 100)

  elseif (cmd == "illum") then
    local illum = tonumber(words[1])
    if (not currmtl.alpha) then
      return LineError("bad illum")
    end
    currmtl.lighting = (illum > 0)

  end
  
  return true
end



local function ParseMTL(mtlfile)
  mtlfile = path .. mtlfile
  local infile = io.open(mtlfile, 'r')
  if (not infile) then
    return LineError("could not open mtl file, " .. mtlfile)
  end

  local oldFilename = filename
  local oldLinenum  = linenum
  local oldLine     = line

  for l in infile:lines() do
    line = l
    linenum = linenum + 1
    if (not ParseMtlLine(line)) then
      return false
    end
  end
  if (currmtl ~= nil) then
    mtls[currmtl.name] = currmtl
  end

  io.close(infile)

  line     = oldLine  
  linenum  = oldLinenum
  filename = oldFilename
  return true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function ParseIndex(i, size)
  if (i == nil)   then return nil end
  if (i == "")    then return 0   end
  local num = tonumber(i)
  if (num == nil) then return nil end
  if (num < 0)    then
    num = num + size  + 1
  end
  if (num < 0)    then return nil end
  return num
end


local function ParseIndices(text)
  local _,_,v,t,n,c = str_find(text, "([^/]*)/?([^/]*)/?([^/]*)/?([^/]*)")
  v = ParseIndex(v, getn(verts))
  t = ParseIndex(t, getn(txcds))
  n = ParseIndex(n, getn(norms))
  c = ParseIndex(c, getn(colors))
  if (v == nil) then print("bad v index "..tostring(v)) end
  if (t == nil) then print("bad t index "..tostring(t)) end
  if (n == nil) then print("bad n index "..tostring(n)) end
  if (c == nil) then print("bad c index "..tostring(c)) end
  if ((v == nil) or (t == nil) or (n == nil) or (c == nil)) then
    return nil
  end
  return { v = v, t = t, n = n, c = c }
end


--------------------------------------------------------------------------------

local function ParseObjLine()

  local cmd, wc, words = Tokenize(line)
  if (cmd == "") then
    return true
  end

  -- material library
  if (cmd == 'mtllib') then
    if (wc < 1) then
      return LineError("could not parse MTL file")
    end
    return ParseMTL(words[1])

  -- material
  elseif (cmd == 'usemtl') then
    currmat = words[1] or ""

  elseif (cmd == 'o') then
    if (currobj.tricount > 0) then
      insert(objs, currobj)
    end
    currobj = NewObject(words[1] or "")

  -- smoothing group
  elseif (cmd == 'g') then
    

  -- face
  elseif (cmd == 'f') then
    if (wc < 3) then
      return LineError("face with less then 3 vertices")
    end
    local face = {}
    for _,w in ipairs(words) do
      indices = ParseIndices(w)
      if (indices == nil) then
        return LineError("invalid indices");
      else
        insert(face, indices)
      end
    end
    -- insert the face into the material set
    mat = currobj.mats[currmat]
    if (mat) then
      insert(mat, face)
    else
      currobj.mats[currmat] = { face }
    end
    currobj.tricount = currobj.tricount + (wc - 2)

  -- vertex
  elseif (cmd == 'v') then
    if (wc < 3) then
      return LineError("bad vertex")
    end
    local v = { tonumber(words[1]), tonumber(words[2]), tonumber(words[3]) }
    if ((v[1] == nil) or (v[2] == nil) or (v[3] == nil)) then
      return LineError("bad vertex")
    end
    insert(verts, v)

  -- texcoord
  elseif (cmd == 'vt') then
    if (wc < 2) then
      return LineError("bad texcoord")
    end
    local t = { tonumber(words[1]), tonumber(words[2]) }
    if ((t[1] == nil) or (t[2] == nil)) then
      return LineError("bad texcoord")
    end
    insert(txcds, t)

  -- normal
  elseif (cmd == 'vn') then
    if (wc < 3) then
      return LineError("bad normal")
    end
    local n = { tonumber(words[1]), tonumber(words[2]), tonumber(words[3]) }
    if ((n[1] == nil) or (n[2] == nil) or (n[3] == nil)) then
      return LineError("bad normal")
    end
    insert(norms, n)

  -- color  (not part of the wavefront spec)
  elseif (cmd == 'vc') then
    if (wc < 3) then
      return LineError("bad color")
    end
    local c = { tonumber(words[1]), tonumber(words[2]),
                tonumber(words[3]), tonumber(words[4]) }
    if ((c[1] == nil) or (c[2] == nil) or (c[3] == nil)) then
      return LineError("bad color")
    end
    if (c[4] == nil) then c[4] = 1.0 end
    insert(colors, c)

  end

  return true
end


--------------------------------------------------------------------------------

local function ParseOBJ()
  local infile = io.open(filename, 'r')
  if (not infile) then
    print('Could not open: '..arg[1])
    return false
  end

  for l in infile:lines() do
    line = l
    linenum = linenum + 1
    if (not ParseObjLine(line)) then
      return false
    end
  end
  if (currobj.tricount > 0) then
    insert(objs, currobj)
  end

  io.close(infile)
  
  return true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function CalculateExtents()
  local xmin, ymin, zmin =  1.0e36,  1.0e36,  1.0e36
  local xmax, ymax, zmax = -1.0e36, -1.0e36, -1.0e36
  for _,v in ipairs(verts) do
    if (v[1] < xmin) then xmin = v[1] end
    if (v[2] < ymin) then ymin = v[2] end
    if (v[3] < zmin) then zmin = v[3] end
    if (v[1] > xmax) then xmax = v[1] end
    if (v[2] > ymax) then ymax = v[2] end
    if (v[3] > zmax) then zmax = v[3] end
  end
  exts = {
    xmin = xmin, xmax = xmax,
    ymin = ymin, ymax = ymax,
    zmin = zmin, zmax = zmax
  }
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function SafeName(name)
  return string.format("%q", name)
end


local function ColorString(color, alpha)
  return  string.format("{ %f, %f, %f, %f }",
                        color[1], color[2], color[3], alpha)
end


local function PrintCommentLine()
  print("----------------------------------------" ..
        "----------------------------------------")
end


local function PrintMaterial(mtl)
  print("  name      = " .. SafeName(mtl.name)                   .. ",")
  print("  texture   = " .. SafeName(mtl.texture)                .. ",")
  print("  ambient   = " .. ColorString(mtl.ambient,  1.0)       .. ",")
  print("  diffuse   = " .. ColorString(mtl.diffuse,  mtl.alpha) .. ",")
  print("  emission  = " .. ColorString(mtl.emission, 1.0)       .. ",")
  print("  specular  = " .. ColorString(mtl.specular, 1.0)       .. ",")
  print("  shininess = " .. mtl.shininess                        .. ",")
  print("  lighting  = " .. tostring(mtl.lighting))
end


local function PrintMaterials()
  print()
  print("local mtls = {}")
  for name,mtl in pairs(mtls) do
    print("mtls[" .. SafeName(name) .. "] = {")
    PrintMaterial(mtl)
    print("}")
  end
  print()
end


local function PrintArrays()
  print()
  print("local v = {}")
  for i,d in ipairs(verts) do
    printf("v[%i] = { %f, %f, %f }", i, d[1], d[2], d[3])
  end
  print() PrintCommentLine() print()
  print("local t = {}")
  for i,d in ipairs(txcds) do
    printf("t[%i] = { %f, %f }", i, d[1], d[2])
  end
  print() PrintCommentLine() print()
  print("local n = {}")
  for i,d in ipairs(norms) do
    printf("n[%i] = { %f, %f, %f }", i, d[1], d[2], d[3])
  end
  print() PrintCommentLine() print()
  print("local c = {}")
  for i,d in ipairs(colors) do
    printf("c[%i] = { %f, %f, %f, %f }", i, d[1], d[2], d[3], d[4])
  end
  print()
end


local function PrintFace(face)
  local msg = ""
  for i,e in ipairs(face) do
    if (i > 1) then msg = msg .. ", " end
    msg = msg .. "{" .. e.v .. "," .. e.t .. "," ..e.n
    if (e.c > 0) then
      msg = msg .. "," ..e.c.."}"
    else
      msg = msg .. "}"
    end
  end
  print("      {" .. msg .. "},")
end


local function PrintObject(i, obj)
  print("  name = " .. SafeName(obj.name) .. ",")
  print("  mats = {")
  for m,mat in pairs(obj.mats) do
    print("    [" .. SafeName(m) .. "] = {")
    for i,f in ipairs(mat) do
      PrintFace(f)
    end
    print("    },")
  end
  print("  }")
end


local function PrintObjects()
  local tris = 0
  print()
  print("local objs = {}")
  for o,obj in ipairs(objs) do
    printf("objs[%i] = {  --  %i tris", o, obj.tricount)
    PrintObject(o, obj)
    printf("}", o)
    tris = tris + obj.tricount
  end
  print()
end


local function PrintExtents()
  print()
  print("local exts = {")
  print("  xmin = " .. exts.xmin .. ",")
  print("  xmax = " .. exts.xmax .. ",")
  print("  ymin = " .. exts.ymin .. ",")
  print("  ymax = " .. exts.ymax .. ",")
  print("  zmin = " .. exts.zmin .. ",")
  print("  zmax = " .. exts.zmax)
  print("}")
  print()
end


local function PrintReturn()
  print()
  print("return \"" .. VERSION .. "\", objs, mtls, v, t, n, c, exts")
  print()
end


local function PrintStats()
  local triCount = 0
  for _,obj in ipairs(objs) do
    triCount = triCount + obj.tricount
  end
  local matCount = 0
  for _ in pairs(mtls) do
    matCount = matCount + 1
  end
  print("--")
  print("--  filename:    " .. filename)
  print("--  objects:     " .. getn(objs))
  print("--  triangles:   " .. triCount)
  print("--  materials:   " .. matCount)
  print("--  vertices:    " .. getn(verts))
  print("--  texcoorcds:  " .. getn(txcds))
  print("--  normals:     " .. getn(norms))
  print("--  colors:      " .. getn(colors))
  print("--")
end


local function PrintHeader()
  print("--")
  print("--  Spring LUA model file (V" .. VERSION .. ")")
  print("--")
  print("--  created by trepan's obj2lua.lua")
  print("--")
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not inSpring) then
  if (ParseOBJ(filename)) then
    CalculateExtents()
    PrintCommentLine()
    PrintCommentLine() PrintHeader()
    PrintCommentLine()
    PrintCommentLine() PrintStats()
    PrintCommentLine()
    PrintCommentLine() PrintMaterials()
    PrintCommentLine() PrintArrays()
    PrintCommentLine() PrintObjects()
    PrintCommentLine()
    PrintCommentLine() PrintExtents()
    PrintCommentLine()
    PrintCommentLine() PrintReturn()
    PrintCommentLine()
    PrintCommentLine() PrintStats()
    PrintCommentLine()
    PrintCommentLine()
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (inSpring) then
  return function(filename)
    objs, mtls, verts, txcds, norms, colors = {}, {}, {}, {}, {}, {}
    oCount, mCount, vCount, tCount, nCount, cCount = 0, 0, 0, 0, 0, 0

    if (ParseOBJ(filename)) then
      CalculateExtents()
      return VERSION, objs, mtls, verts, txcds, norms, colors, exts
    else
      return nil
    end
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

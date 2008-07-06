--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    loadmodel.lua
--  brief:   utilities for the custom lua 3d model format
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local insert = table.insert

local debug = 1

--------------------------------------------------------------------------------

local currMat = nil

local defMat = {
  ambient   = { 0.2, 0.2, 0.2, 1.0 },
  diffuse   = { 0.8, 0.8, 0.8, 1.0 },
  emission  = { 0.0, 0.0, 0.0, 1.0 },
  specular  = { 0.0, 0.0, 0.0, 1.0 },
  shininess = 0.0,
  texture   = "",
  lighting  = true
}


local function SanitizeMaterial(mat)
  for k,v in pairs(defMat) do
    if (mat[k] == nil) then
      mat[k] = v
    end
    if ((type(v) == "table") and (mat[k][4] == nil)) then
      mat[k][4] = 1.0 -- make the colors r/g/b/A
    end
  end
end


--------------------------------------------------------------------------------

local function AlmostEqual(a, b)
  return (math.abs(a - b) < 0.001)
end
  

local function SameColor(a, b)
  if (AlmostEqual(a[1], b[1]) and
      AlmostEqual(a[2], b[2]) and
      AlmostEqual(a[3], b[3]) and
      AlmostEqual(a[4], b[4])) then
    return true
  end
  return false  
end


local function CompareColor(a, b)
  if (not AlmostEqual(a[1], b[1])) then return (a[1] < b[1]) and -1 or 1 end
  if (not AlmostEqual(a[2], b[2])) then return (a[2] < b[2]) and -1 or 1 end
  if (not AlmostEqual(a[3], b[3])) then return (a[3] < b[3]) and -1 or 1 end
  if (not AlmostEqual(a[4], b[4])) then return (a[4] < b[4]) and -1 or 1 end
  return 0
end


local function CompareMaterials(matPairA, matPairB)
  local matA = matPairA[1]
  local matB = matPairB[1]
  local alphaA = matA.diffuse[4]
  local alphaB = matB.diffuse[4]
  if (not AlmostEqual(alphaA, alphaB)) then return (alphaA > alphaB) end
  if (matA.texture < matB.texture)     then return false end
  if (matA.texture > matB.texture)     then return true  end
  if (matA.lighting ~= matB.lighting)  then return matA.lighting end
  if (matA.lighting) then
    if (not AlmostEqual(matA.shininess, matB.shininess)) then
      return (matA.shininess < matB.shininess)
    end
    local cmpAmbient = CompareColor(matA.ambient, matB.ambient)
    if (cmpAmbient ~= 0)  then return (cmpAmbient > 0) end
    local cmpDiffuse = CompareColor(matA.diffuse, matB.diffuse)
    if (cmpDiffuse ~= 0)  then return (cmpDiffuse > 0) end
    local cmpSpecular = CompareColor(matA.specular, matB.specular)
    if (cmpSpecular ~= 0) then return (cmpSpecular > 0) end
    local cmpEmission = CompareColor(matA.emission, matB.emission)
    if (cmpEmission ~= 0) then return (cmpEmission > 0) end
  end
  return false
end


local function SortMaterials(obj, mats)
  local sorted = {}
  for matname in pairs(obj.mats) do
    local mat = mats[matname]
    if (mat == nil) then
      mat = defMat
    end
    insert(sorted, { mat, matname })
  end
  table.sort(sorted, CompareMaterials)
  if (debug > 0) then
    for k,v in ipairs(sorted) do
      print(k, v[2])
    end
  end
  return sorted
end


local function SetupMaterial(mat, useDepthMask)
  if (currmat == nil) then
    -- set all our states
    if (mat.texture ~= "") then
      gl.Texture(mat.texture)
    else
      gl.Texture(false)
    end
    if (useDepthMask) then
      gl.DepthMask(mat.diffuse[4] >= 1.0)
    end
    gl.Lighting(mat.lighting)
    if (not mat.lighting) then  
      gl.Color(mat.diffuse)
    else
      gl.Material({
        ambient   = mat.ambient,
        diffuse   = mat.diffuse,
        specular  = mat.specular,
        emission  = mat.emission,
        shininess = mat.shininess
      })
    end
    currmat = mat
    return
  end

  if (mat.texture ~= currmat.texture) then
    if (mat.texture ~= "") then
      gl.Texture(mat.texture)
    else
      gl.Texture(false)
    end
  end

  if (useDepthMask) then
    local next = (mat.diffuse[4]     >= 1.0)
    local curr = (currmat.diffuse[4] >= 1.0)
    if (next ~= curr) then
      gl.DepthMask(next)
    end
  end

  if (mat.lighting ~= currmat.lighting) then
    gl.Lighting(mat.lighting)
  end

  if (not mat.lighting) then
    if ((currmat.lighting) or
        (not SameColor(currmat.diffuse, mat.diffuse))) then
      gl.Color(mat.diffuse)
    end
  else
    changeAll = (not currmat.lighting)
    local changes = {}
    if (changeAll or (not SameColor(currmat.ambient, mat.ambient))) then
      changes.ambient = mat.ambient
    end
    if (changeAll or (not SameColor(currmat.diffuse, mat.diffuse))) then
      changes.diffuse = mat.diffuse
    end
    if (changeAll or (not SameColor(currmat.specular, mat.specular))) then
      changes.specular = mat.specular
    end
    if (changeAll or (not SameColor(currmat.emission, mat.emission))) then
      changes.emission = mat.emission
    end
    if (changeAll or (not AlmostEqual(currmat.shininess, mat.shininess))) then
      changes.shininess = mat.shininess
    end
    gl.Material(changes)
  end

  currmat = mat
end


local function SetParam(index, array, name, elem)
  if ((index == nil) or (index <= 0)) then
    return
  end
  local data = array[index]
  if (data == nil) then
    print("indexing error: " .. index .. "[" .. name .. "]")
  else
    elem[name] = data
  end
end


local function DrawObject(obj, mats, v, t, n, c, noMaterials, noDepthMask)
  currmat = nil

  if (debug > 0) then print("obj.name = " .. obj.name) end

  local sortedMats = SortMaterials(obj, mats)

  for _,matPair in ipairs(sortedMats) do

    local mat     = matPair[1]
    local matname = matPair[2]
    local faces = obj.mats[matname]

    if (not noMaterials) then
      SetupMaterial(mat, not noDepthMask)
    end

    local glBeginEnd = gl.BeginEnd
    local glVertex   = gl.Vertex
    local glNormal   = gl.Normal
    local glTexCoord = gl.TexCoord
    local glColor    = gl.Color

    for fi,face in ipairs(faces) do

      glBeginEnd(GL.TRIANGLE_FAN, function()
        for _,elem in ipairs(face) do
          local vi, ti, ni, ci = elem[1], elem[2], elem[3], elem[4]
          if (ci and (ci > 0)) then glColor    (c[ci]) end
          if (ni and (ni > 0)) then glNormal   (n[ni]) end
          if (ti and (ti > 0)) then glTexCoord (t[ti]) end
          if (vi and (vi > 0)) then glVertex   (v[vi]) end
        end
      end)

      if (debug > 1) then
        print("SHAPE: " .. obj.name .. " || " .. matname .. " || face:" .. fi)
        for k1,v1 in ipairs(faceTbl) do
          print(k1,v1)
          for k2,v2 in pairs(v1) do
            print("  " .. k2,v2)
            for k3,v3 in pairs(v2) do
              print("    " .. k3,v3)
            end
          end
        end
      end
    end
  end
  return
end


--------------------------------------------------------------------------------

local function Append(old, new)
  for _,f in ipairs(new) do
    insert(old, f)
  end
end


local function MergeObjects(objs)
  if (objs == nil)    then return nil end
  if (objs[1] == nil) then return {}  end

  local obj = objs[1]
  for i,nextobj in ipairs(objs) do
    if (i > 1) then
      for matname,faces in pairs(nextobj.mats) do
        local oldfaces = obj.mats[matname]
        if (oldfaces) then
          Append(oldfaces, faces)
        else
          obj.mats[matname] = faces
        end
      end
    end
  end
  return obj
end


--------------------------------------------------------------------------------

local function LoadModel(data)
  local version, objs, mats, v, t, n, c, exts
  if (type(data) == 'table') then
    version, objs, mats, v, t, n, c, exts = unpack(data)
  elseif (type(data) == 'string') then
    version, objs, mats, v, t, n, c, exts = include(data)
  else
    print("bad data passed to LoadModel(string|table)")
    return nil
  end

  if (objs == nil) then
    print("LoadModel() failed to load:  " .. tostring(data))
    return nil
  end

  -- make sure the materials are valid
  for name,mat in pairs(mats) do
    SanitizeMaterial(mat)
  end

  -- cache the counts
  local vCount = #v
  local tCount = #t
  local nCount = #n
  local cCount = #c
  objs.vCount = vCount
  objs.tCount = tCount
  objs.nCount = nCount
  objs.cCount = cCount

  -- setup the draw calls
  for _,obj in ipairs(objs) do
    local obj = obj
    obj.Draw = function(useDepthMask)
      return DrawObject(obj, mats, v, t, n, c, useDepthMask)
    end
  end

  -- setup the merge call
  objs.Merge = MergeObjects

  return objs, mats, v, t, n, c, exts
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return LoadModel

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

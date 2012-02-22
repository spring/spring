--//=============================================================================
--//

TextureHandler = {}


--//=============================================================================
--//TWEAKING

local timeLimit = 0.2/15 --//time per second / desiredFPS


--//=============================================================================
--// SpeedUp

local next = next
local spGetTimer = Spring.GetTimer
local spDiffTimers = Spring.DiffTimers
local glActiveTexture = gl.ActiveTexture
local glCallList = gl.CallList

local weakMetaTable = {__mode="k"}


--//=============================================================================
--// local

local loaded = {}
local requested = {}

local placeholderFilename = theme.skin.icons.imageplaceholder
local placeholderDL = gl.CreateList(gl.Texture,CHILI_DIRNAME .. "Skins/default/empty.png")
--local placeholderDL = gl.CreateList(gl.Texture,placeholderFilename)

local function AddRequest(filename,obj)
  local req = requested
  if (req[filename]) then
    local t = req[filename]
    t[obj] = true
  else
    req[filename] = setmetatable({[obj]=true}, weakMetaTable)
  end
end


--//=============================================================================
--// Destroy

TextureHandler._scream = Script.CreateScream()
TextureHandler._scream.func = function()
  requested = {}
  for filename,tex in pairs(loaded) do
    gl.DeleteList(tex.dl)
    gl.DeleteTexture(filename)
  end
  loaded = {}
end


--//=============================================================================
--//

function TextureHandler.LoadTexture(arg1,arg2,arg3)
  local activeTexID,filename,obj
  if (type(arg1)=='number') then
     activeTexID = arg1
     filename = arg2
     obj = arg3
  else
     activeTexID = 0
     filename = arg1
     obj = arg2
  end

  local tex = loaded[filename]
  if (not tex) then
    AddRequest(filename,obj)
    glActiveTexture(activeTexID,glCallList,placeholderDL)
  else
    glActiveTexture(activeTexID,glCallList,tex.dl)
  end
end


function TextureHandler.DeleteTexture(filename)
  local tex = loaded[filename]
  if (tex) then
    tex.references = tex.references - 1
    if (tex.references==0) then
      gl.DeleteList(tex.dl)
      gl.DeleteTexture(filename)
      loaded[filename] = nil
    end
  end
end


--//=============================================================================
--//

local usedTime = 0
local lastCall = spGetTimer()

function TextureHandler.Update()
  if (usedTime>0) then
    thisCall = spGetTimer()

    usedTime = usedTime - spDiffTimers(thisCall,lastCall)
    lastCall = thisCall

    if (usedTime<0) then usedTime = 0 end
  end

  if (not next(requested)) then return end

  local timerStart = spGetTimer()
  while (usedTime < timeLimit) do
    local filename,objs = next(requested)

    if (not filename) then return end

    gl.Texture(filename)
    gl.Texture(false)

    local texture = {}
    texture.dl = gl.CreateList(gl.Texture,filename)
    texture.references = #objs
    loaded[filename] = texture

    for obj in pairs(objs) do
      obj:Invalidate()
    end

    requested[filename] = nil

    local timerEnd = spGetTimer()
    usedTime = usedTime + spDiffTimers(timerEnd,timerStart)
    timerStart = timerEnd
  end

  lastCall = spGetTimer()
end

--//=============================================================================

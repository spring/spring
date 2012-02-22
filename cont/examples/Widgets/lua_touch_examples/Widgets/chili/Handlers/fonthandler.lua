--//=============================================================================
--// FontSystem

FontHandler = {}


--//=============================================================================
--// cache loaded fonts

local loadedFonts = {}
local refCounts = {}

--//  maximum fontsize difference
--// when we don't find the wanted font rendered with the wanted fontsize
--// (in respect to this threshold) then recreate a new one
local fontsize_threshold = 1 

--//=============================================================================
--// Destroy

FontHandler._scream = Script.CreateScream()
FontHandler._scream.func = function()
  for i=1,#loadedFonts do
    gl.DeleteFont(loadedFonts[i])
  end
  loadedFonts = {}
end


--//=============================================================================
--// API

function FontHandler.UnloadFont(font)
  for i=1,#loadedFonts do
    local font2 = loadedFonts[i]
    if (font == font2) then
      local refCount = refCounts[i]
      if (refCount <= 1) then
        --// the font isn't in use anymore, free it
        local last_idx = #loadedFonts
        gl.DeleteFont(loadedFonts[i])
        loadedFonts[i] = loadedFonts[last_idx]
        loadedFonts[last_idx] = nil
        refCounts[i] = refCounts[last_idx]
        refCounts[last_idx] = nil
      else
        refCounts[i] = refCount - 1
      end
      return
    end
  end
end

function FontHandler.LoadFont(fontname,size,outwidth,outweight)
  for i=1,#loadedFonts do
    local font = loadedFonts[i]
    if
      ((font.path == fontname)or(font.path == 'fonts/'..fontname))
      and(math.abs(font.size - size) <= fontsize_threshold)
      and((not outwidth)or(font.outlinewidth == outwidth))
      and((not outweight)or(font.outlineweight == outweight))
    then
      refCounts[i] = refCounts[i] + 1
      return font
    end
  end

  local idx = #loadedFonts+1
  local font = gl.LoadFont(fontname,size,outwidth,outweight)
  loadedFonts[idx] = font
  refCounts[idx] = 1
  return font
end
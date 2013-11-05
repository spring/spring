local ctrlPanel = {}

local function ParseCtrlPanelTXT(filename)
  local f,it,isFile = nil,nil,false
  f  = io.open(filename,'r')
  if f then
    it = f:lines()
    isFile = true
  else
    f  = VFS.LoadFile(filename)
    it = f:gmatch("%a+.-\n")
  end

  local wp = '%s*([^%s]+)'           -- word pattern
  local wp2 = '%s+([^%s]+)'           -- word pattern
  local cp = '^'..wp..wp2..wp2..wp2..wp2 -- color pattern
  local sp = '^'..wp..wp2             -- single value pattern like queuedLineWidth

  for line in it do
    local _, _, n, r, g, b, a = line:lower():find(cp)

    r = tonumber(r or 1.0)
    g = tonumber(g or 1.0)
    b = tonumber(b or 1.0)
    a = tonumber(a or 1.0)

    if n then
      ctrlPanel[n]= { r,g,b,a }
    else
      _, _, n, r = line:lower():find(sp)
      if n then
        ctrlPanel[n]= r
      end
    end
  end

  if isFile then f:close() end
  f,it,wp,cp,sp=nil,nil,nil,nil,nil
end

ParseCtrlPanelTXT('ctrlpanel.txt')
ParseCtrlPanelTXT('LuaUI/ctrlpanel.txt')

return ctrlPanel
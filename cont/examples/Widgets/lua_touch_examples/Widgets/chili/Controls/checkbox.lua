--//=============================================================================

Checkbox = Control:Inherit{
  classname = "checkbox",
  checked   = true,
  caption   = "text",
  textalign = "left",
  boxalign  = "right",
  boxsize   = 10,

  textColor = {0,0,0,1},
  
  defaultWidth     = 70,
  defaultHeight    = 18,

  OnChange = {}
}

local this = Checkbox
local inherited = this.inherited

--//=============================================================================

function Checkbox:Toggle()
  self:CallListeners(self.OnChange,not self.checked)
  self.checked = not self.checked
  self:Invalidate()
end

--//=============================================================================

function Checkbox:DrawControl()
  --// gets overriden by the skin/theme
end

--//=============================================================================

function Checkbox:HitTest()
  return self
end

function Checkbox:MouseDown()
  self:Toggle()
  return self
end

--//=============================================================================

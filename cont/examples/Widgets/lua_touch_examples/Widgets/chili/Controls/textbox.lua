TextBox = Control:Inherit{
  classname = "textbox",

  padding = {0,0,0,0},

  text      = "line1\nline2",
  autoHeight  = true, --// sets height to text size, useful for embedding in scrollboxes
  autoObeyLineHeight = true, --// (needs autoHeight) if true, autoHeight will obey the lineHeight (-> texts with the same line count will have the same height) 
  fontsize = 12,

  _lines = {},
}

local this = TextBox
local inherited = this.inherited

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function TextBox:SetText(t)
  self.text = t
  self:RequestRealign()
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function Split(s, separator)
  local results = {}
  for part in s:gmatch("[^"..separator.."]+") do
    results[#results + 1] = part
  end
  return results
end

-- remove first n elemets from t, return them
local function Take(t, n)
  local removed = {}
  for i=1, n do
    removed[#removed+1] = table.remove(t, 1)
  end
  return removed
end

-- appends t1 to t2 in-place
local function Append(t1, t2)
  local l = #t1
  for i = 1, #t2 do
    t1[i + l] = t2[i]
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function TextBox:UpdateLayout()
  local font = self.font
  local padding = self.padding
  local width  = self.width - padding[1] - padding[3]
  local height = self.height - padding[2] - padding[4]
  if self.autoHeight then
    height = 1e9
  end

  self._wrappedText = font:WrapText(self.text, width, height)

  if self.autoHeight then
    local textHeight,textDescender,numLines = font:GetTextHeight(self._wrappedText)
    textHeight = textHeight-textDescender

    if (self.autoObeyLineHeight) then
      if (numLines>1) then
        textHeight = numLines * font:GetLineHeight()
      else
        --// AscenderHeight = LineHeight w/o such deep chars as 'g','p',...
        textHeight = math.min( math.max(textHeight, font:GetAscenderHeight()), font:GetLineHeight())
      end
    end

    self:Resize(nil, textHeight, true, true)
  end
end


function TextBox:DrawControl()
  local paddx, paddy = unpack4(self.clientArea)
  local x = math.ceil(self.x + paddx)
  local y = math.ceil(self.y + paddy)

  local font = self.font
  font:Draw(self._wrappedText, x, y)
end

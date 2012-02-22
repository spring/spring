--//=============================================================================

Multiprogressbar = Control:Inherit{
  classname = "multiprogressbar",

  defaultWidth     = 90,
  defaultHeight    = 20,
  
  padding = {0,0,0,0},	
  drawBorder = false,
  borderColor = {1,0,0,1},
  orientation = "horizontal", 
  reverse = false, -- draw in reversed orientation 
	
  scaleFunction = nil, --- function that can be used to rescale graph - takes 0-1 and must return 0-1 

  
  bars = {  -- list of bar components to display
	{
		color1 = {1,0,1,1},
		color2 = {0.7,0,0.7,1},
		percent = 0.2,
		texture = nil, -- texture file name
		s = 1, -- tex coords
		t = 1,
		tileSize = nil, --  if set then main axis texture coord = width / tileSize
	},
  
  }
}

local this = Multiprogressbar
local inherited = this.inherited

--//=============================================================================

function Multiprogressbar:SetCaption(str)
  if (self.caption ~= str) then
    self.caption = str
    self:Invalidate()
  end
end

function Multiprogressbar:SetValue(b, p)
  local oldvalue = self.bars[b].percent
  if (p ~= oldvalue) then
    self.bars[b].percent = p
    self:Invalidate()
  end
end

function Multiprogressbar:SetColor1(b, ...)
  local color1 = _ParseColorArgs(...)
  table.merge(color1,self.bars[b].color1)
  if (not table.iequal(color1,self.bars[b].color1)) then
    self.bars[b].color1 = color1
    self:Invalidate()
  end
end

function Multiprogressbar:SetColor2(b, ...)
  local color2 = _ParseColorArgs(...)
  table.merge(color2,self.bars[b].color2)
  if (not table.iequal(color1,self.bars[b].color2)) then
    self.bars[b].color2 = color2
    self:Invalidate()
  end
end

function Multiprogressbar:SetColor(b, ...)
  self:SetColor2(b, ...)
  self:SetColor1(b, ...)
end


--//=============================================================================

local glVertex = gl.Vertex
local glColor = gl.Color
local glBeginEnd = gl.BeginEnd

local function drawBox(x,y,w,h) 
	glVertex(x,y)
	glVertex(x+w,y)
	glVertex(x+w,y+h)
	glVertex(x,y+h)
end 

local function drawBarH(x,y,w,h,color1, color2)
    glColor(color1)
	glVertex(x,y)
	glVertex(x+w,y)
	glColor(color2)
	glVertex(x+w,y+h)
	glVertex(x,y+h)
end 

local function drawBarV(x,y,w,h,color1, color2) 
	glColor(color1)
	glVertex(x,y)
	glVertex(x,y+h)
	glColor(color2)
	glVertex(x+w,y+h)
	glVertex(x+w,y)
end


function Multiprogressbar:DrawControl()
  local percentDone = 0
  local efp 

	if (self.scaleFunction ~= nil) then  -- if using non linear scale fix the bar
		local totalPercent = 0
		for _,b in ipairs(self.bars) do 
			totalPercent = totalPercent + (b.percent or 0)
		end 
		
		local resize = self.scaleFunction(totalPercent) / totalPercent
		for _,b in ipairs(self.bars) do 
			b._drawPercent = b.percent * resize
		end 
	else 
		for _,b in ipairs(self.bars) do 
			b._drawPercent = b.percent
		end 
	end 

  
  if (self.orientation=="horizontal") then 
	for _,b in ipairs(self.bars) do 
		if b._drawPercent > 0 then
			if (self.reverse) then 
				efp = 1 - percentDone - b._drawPercent
			else 
				efp = percentDone
			end 
			if (b.color1 ~= nil) then glBeginEnd(GL.QUADS, drawBarH, self.x + efp * self.width,self.y, self.width * b._drawPercent, self.height, b.color1, b.color2) end
			percentDone = percentDone + b._drawPercent
		
			if b.texture then 
				TextureHandler.LoadTexture(b.texture,self)    
				glColor(1,1,1,1)
			
				local bs = b.s
				if (b.tileSize) then 
					bs = self.width * b._drawPercent / b.tileSize
				end 
				gl.TexRect(self.x + efp * self.width,self.y, self.x + (efp + b._drawPercent) * self.width, self.y + self.height, 0,0,bs,b.t)
				gl.Texture(false)  	
			end 
		end 
	end 

  else 
	for _,b in ipairs(self.bars) do 
		if b._drawPercent > 0 then 
			if (self.reverse) then 
				efp = 1 - percentDone - b._drawPercent
			else 
				efp = percentDone
			end 
			
			if (b.color1 ~= nil) then glBeginEnd(GL.QUADS, drawBarV, self.x,self.y + efp * self.height, self.width, self.height * b._drawPercent, b.color1, b.color2) end 
			
			percentDone = percentDone + b._drawPercent
		
			if b.texture then 
				TextureHandler.LoadTexture(b.texture,self)
				glColor(1,1,1,1)
			
				local bt = b.t
				if (b.tileSize) then 
					bt = self.height * b._drawPercent / b.tileSize
				end 
				gl.TexRect(self.x, self.y + efp * self.height, self.x + self.width, self.y + (efp + b._drawPercent) * self.height, 0,0,b.s,bt)
				gl.Texture(false)
			end 
		end
	end 
  end 

  
  
  if self.drawBorder then
	glColor(self.borderColor)
	glBeginEnd(GL.LINE_LOOP, drawBox, self.x, self.y, self.width, self.height)
  end 

end

--//=============================================================================

function Multiprogressbar:HitTest()
  return self
end


--//=============================================================================

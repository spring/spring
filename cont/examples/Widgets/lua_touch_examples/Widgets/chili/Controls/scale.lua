
Scale = Control:Inherit{
  classname = "scale",
  min = -50,
  max = 50,
  step = 10,
  logBase = 1.5,
  
  defaultWidth     = 90,
  defaultHeight    = 12,
  fontsize = 8,
  scaleFunction = nil,--- function that can be used to rescale graph - takes 0-1 and must return 0-1 
  color = {0,0,0,1},
}

local this = Scale

--//=============================================================================

--//=============================================================================

local glVertex = gl.Vertex

local function defaultTransform(x) 
	return (math.log(1+x*140 ) / math.log(141))
end

local function drawScaleLines(self) 
  local hline = self.y + self.height
  local h1 = self.y + self.fontsize
  local h2 = self.y + self.height
    
  glVertex(self.x, hline)
  glVertex(self.x + self.width,hline)
   
  if (self.scaleFunction == nil) then 
		local scale = self.width / (self.max-self.min)
  
		for v = self.min, self.max, self.step do 
			local xp = self.x +  scale * (v - self.min)
			glVertex(xp,  h1)
			glVertex(xp,  h2)
		end 
  else
		local center = self.x + self.width*0.5
		local halfWidth = 0.5 * self.width
		local lastXp = -1
		for v = 0, self.max, self.step do 
			local xp = self.scaleFunction(v/self.max) * halfWidth  + center 
			glVertex(xp,  h1)
			glVertex(xp,  h2)
			if (xp - lastXp < 2) then 
			  glVertex(xp,h1)
				glVertex(self.x + self.width, h1)
			  glVertex(xp,h2)
				glVertex(self.x + self.width, h2)
				break
			end 
			lastXp = xp
		end 
		
		local lastXp = 99999
		for v = 0, self.min, -self.step do 
			local xp = center - self.scaleFunction(v/self.min) * halfWidth
			glVertex(xp,  h1)
			glVertex(xp,  h2)
			if (lastXp - xp <= 2) then 
			  glVertex(xp,h1)
				glVertex(self.x, h1)
			  glVertex(xp,h2)
				glVertex(self.x, h2)
				break
			end 
			lastXp = xp
		end 
	
  end
end 



function Scale:DrawControl()
  gl.Color(self.color)
  gl.BeginEnd(GL.LINES, drawScaleLines, self)

  local font = self.font
     
  if (self.min <=0 and self.max >= 0) then 
    local scale = self.width / (self.max-self.min)
    font:Print(0, self.x +  scale * (0 - self.min), self.y, "center", "ascender")
  end 

  font:Print(self.min, self.x, self.y, "left", "ascender")
  font:Print("+"..self.max, self.x+self.width, self.y, "right", "ascender")
end

--//=============================================================================

function Scale:HitTest()
  return false
end


--//=============================================================================

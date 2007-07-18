--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_xray_shader.lua
--  brief:   xray shader
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "XrayShader",
    desc      = "XrayShader",
    author    = "trepan",
    date      = "Jul 15, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not gl.CreateShader) then
  Spring.Echo("Hardware is incompatible with Xray shader requirements")
  return false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  simple configuration parameters
--

local edgeExponent = 2.5

local doFeatures = false

local featureColor = { 1, 0, 1 }


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local shader


function widget:Shutdown()
  gl.DeleteShader(shader)
end


function widget:Initialize()

  shader = gl.CreateShader({

    uniform = {
      edgeExponent = edgeExponent,
    },

    vertex = [[
      // Application to vertex shader
      varying vec3 normal;
      varying vec3 eyeVec;
      varying vec3 color;
      uniform mat4 camera;
      uniform mat4 caminv;

      void main()
      {
              vec4 P = gl_ModelViewMatrix * gl_Vertex;
              
              eyeVec = P.xyz;
              
              normal  = gl_NormalMatrix * gl_Normal;
              
              color = gl_Color.rgb;
              
              gl_Position = gl_ProjectionMatrix * P;
      }
    ]],  
 
    fragment = [[
      varying vec3 normal;
      varying vec3 eyeVec;
      varying vec3 color;

      uniform float edgeExponent;

      void main()
      {
          float opac = dot(normalize(normal), normalize(eyeVec));
          opac = 1.0 - abs(opac);
          opac = pow(opac, edgeExponent);
          
          gl_FragColor.rgb = color;
          gl_FragColor.a = opac;
      }
    ]],
  })

  if (shader == nil) then
    Spring.Echo(gl.GetShaderLog())
    Spring.Echo("Xray shader compilation failed.")
    widgetHandler:RemoveWidget()
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  speed ups
--

local GetTeamList    = Spring.GetTeamList
local GetTeamUnits   = Spring.GetTeamUnits
local GetAllFeatures = Spring.GetAllFeatures
local IsUnitVisible  = Spring.IsUnitVisible

local glColor   = gl.Color
local glUnit    = gl.Unit
local glFeature = gl.Feature


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  utility routine
--

local teamColors = {}

local function SetTeamColor(teamID)
  local color = teamColors[teamID]
  if (color) then
    glColor(color)
    return
  end
  local _,_,_,_,_,_,r,g,b = Spring.GetTeamInfo(teamID)
  if (r and g and b) then
    color = { r, g, b }
    teamColors[teamID] = color
    glColor(color)
    return
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawWorld()
  gl.Color(1, 1, 1, 1)

  gl.UseShader(shader)

  gl.DepthTest(true)

  gl.Blending(GL.SRC_ALPHA, GL.ONE)

  gl.PolygonOffset(-2, -2)

  for _, teamID in ipairs(GetTeamList()) do
    SetTeamColor(teamID)
    for _, unitID in ipairs(GetTeamUnits(teamID)) do
      if (IsUnitVisible(unitID)) then
        glUnit(unitID, true)
      end
    end
  end

  if (doFeatures) then
    gl.Color(featureColor)
    for _, featureID in ipairs(GetAllFeatures()) do
      glFeature(featureID, true)
    end
  end

  gl.PolygonOffset(false)

  gl.Blending(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)

  gl.DepthTest(false)

  gl.UseShader(0)

  gl.Color(1, 1, 1, 1)
end
              

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

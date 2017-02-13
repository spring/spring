--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    mapDefaults.lua
--  brief:   engine map defaults values
--  author:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--  NOTE: most keys could be omitted, setupopts.lua only reads a limited subset
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local mapDefaults = {

  hardness        = 100.0,
  gravity         = 130.0,
  tidalStrength   = 0.0,
  maxMetal        = 0.02,
  extractorRadius = 500.0,

  notDeformable = false,
  voidWater     = false,

  autoShowMetal = true,

  atmosphere = {
    minWind  = 5.0,
    maxWind  = 25.0,

    fogStart = 0.1,
    fogColor   = { 0.7, 0.7,  0.8 },
    skyColor   = { 0.1, 0.15, 0.7 },
    cloudColor = { 1.0, 1.0,  1.0 },
    cloudDensity = 0.5,

    skyBox = '',
  },

  water = {
    damage = 0.0,

    repeatX = 0.0,
    repeatY = 0.0,

    absorb    = { 0.0, 0.0, 0.0 },
    baseColor = { 0.0, 0.0, 0.0 },
    minColor  = { 0.0, 0.0, 0.0 },

    surfaceColor = { 0.75, 0.8, 0.85 },
    surfaceAlpha = 0.55,

    planeColor = { 0.0, 0.4, 0.0 },

    specularColor = { 0.5, 0.5, 0.5 }, -- tracks groundDiffuseColor
    specularFactor = 20,

    fresnelMin   = 0.2,
    fresnelMax   = 0.3,
    fresnelPower = 4.0,

    texture = '',
    foamTexture = '',
    normalTexture = '',

    caustics = nil, -- a nil value results in using the defaults
  },

  lighting = {
    sunDir = { 0.0, 1.0, 2.0 },

    groundAmbientColor  = { 0.5, 0.5, 0.5 },
    groundDiffuseColor  = { 0.5, 0.5, 0.5 },
    groundSpecularColor = { 0.1, 0.1, 0.1 },
    groundShadowDensity = 0.8,

    unitAmbientColor  = { 0.4, 0.4, 0.4 },
    unitDiffuseColor  = { 0.7, 0.7, 0.7 },
    unitSpecularColor = { 0.7, 0.7, 0.7 }, -- tracks unitDiffuseColor
    unitShadowDensity = 0.8,
  },

  grass = {
    grassBladeWaveScale = 1.0,
    grassBladeWidth     = 0.32,
    grassBladeHeight    = 4.0,
    grassBladeAngle     = 1.57, -- (PI / 2) radians
  },

  splats = {
    texScales = {0.02, 0.02, 0.02, 0.02},
    texMults  = {1.00, 1.00, 1.00, 1.00},
  },

  resources = {
    detailTex            = '',
    specularTex          = '',
    splatDetailTex       = '',
    splatDistrTex        = '',
    grassBladeTex        = '',
    grassShadingTex      = '',
    skyReflectModTex     = '',
    detailNormalTex      = '',
    lightEmissionTex     = '',
    parallaxHeightTex    = '',
    splatDetailNormalTex = {
      '', -- 1
      '', -- 2
      '', -- 3
      '', -- 4
      alpha = false,
    },
  },

  defaultTerrainType = {
    name = '',
    hardness = 1.0,
    moveSpeeds = {
      tank  = 1.0,
      kbot  = 1.0,
      hover = 1.0,
      ship  = 1.0,
    },
  },  
}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return mapDefaults

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

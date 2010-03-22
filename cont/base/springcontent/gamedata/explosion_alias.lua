--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    explosion_alias.lua
--  brief:   explosion_alias.tdf lua parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--  Specifies a list of alias names for spring C++ classes, allows the aliases
--  to have easier names and the C++ names to be changed independent of mods.
--  Projectile class names can change in the future, so mods should not change
--  this file. It is distributed with spring.

--    Syntax:  <alias> = <real name>


local TDF = VFS.Include('gamedata/parse_tdf.lua')

local aliases, err = TDF.Parse('gamedata/explosion_alias.tdf')

--------------------------------------------------------------------------------


if (aliases == nil) then
  -- load the defaults
  aliases = {
    generators = {
      std    = 'CStdExplosionGenerator',
      custom = 'CCustomExplosionGenerator',
    },
    projectiles = {
      beamlaser             = 'CBeamLaserProjectile',
      bitmapmuzzleflame     = 'CBitmapMuzzleFlame',
      bubble                = 'CBubbleProjectile',
      delayspawner          = 'CExpGenSpawner',
      dirt                  = 'CDirtProjectile',
      emg                   = 'CEmgProjectile',
      expl                  = 'CExplosiveProjectile',
      explsphere            = 'CSpherePartSpawner',
      explspike             = 'CExploSpikeProjectile',
      fireball              = 'CFireBallProjectile',
      fire                  = 'CFireProjectile',
      flame                 = 'CFlameProjectile',
      flare                 = 'CFlareProjectile',
      geosquare             = 'CGeoSquareProjectile',
      gfx                   = 'CGfxProjectile',
      heatcloud             = 'CHeatCloudProjectile',
      lighting              = 'CLightingProjectile',
      missile               = 'CMissileProjectile',
      muzzleflame           = 'CMuzzleFlame',
      piece                 = 'CPieceProjectile',
      shieldpart            = 'CShieldPartProjectile',
      simplegroundflash     = 'CSimpleGroundFlash',
      simpleparticlespawner = 'CSphereParticleSpawner',
      simpleparticlesystem  = 'CSimpleParticleSystem',
      smoke                 = 'CSmokeProjectile',
      smoke2                = 'CSmokeProjectile2',
      smoketrail            = 'CSmokeTrailProjectile',
      spherepart            = 'CSpherePartProjectile',
      starburst             = 'CStarburstProjectile',
      torpedo               = 'CTorpedoProjectile',
      tracer                = 'CTracerProjectile',
      wake                  = 'CWakeProjectile',
    },
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return aliases

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


# This list was created using this *nix shell command:
# > find . -name "*.cpp"" | sort
add_library(engineSim STATIC EXCLUDE_FROM_ALL
	Features/Feature.cpp
	Features/FeatureDef.cpp
	Features/FeatureDefHandler.cpp
	Features/FeatureHandler.cpp
	Misc/AllyTeam.cpp
	Misc/BuildingMaskMap.cpp
	Misc/CategoryHandler.cpp
	Misc/CollisionHandler.cpp
	Misc/CollisionVolume.cpp
	Misc/CommonDefHandler.cpp
	Misc/DamageArray.cpp
	Misc/DamageArrayHandler.cpp
	Misc/DefinitionTag.cpp
	Misc/GeometricObjects.cpp
	Misc/GlobalSynced.cpp
	Misc/GroundBlockingObjectMap.cpp
	Misc/InterceptHandler.cpp
	Misc/LosHandler.cpp
	Misc/LosMap.cpp
	Misc/ModInfo.cpp
	Misc/NanoPieceCache.cpp
	Misc/QuadField.cpp
	Misc/Resource.cpp
	Misc/ResourceHandler.cpp
	Misc/ResourceMapAnalyzer.cpp
	Misc/SideParser.cpp
	Misc/SimObjectIDPool.cpp
	Misc/SmoothHeightMesh.cpp
	Misc/Team.cpp
	Misc/TeamBase.cpp
	Misc/TeamHandler.cpp
	Misc/TeamStatistics.cpp
	Misc/Wind.cpp
	MoveTypes/AAirMoveType.cpp
	MoveTypes/StrafeAirMoveType.cpp
	MoveTypes/GroundMoveType.cpp
	MoveTypes/MoveDefHandler.cpp
	MoveTypes/MoveMath/GroundMoveMath.cpp
	MoveTypes/MoveMath/HoverMoveMath.cpp
	MoveTypes/MoveMath/MoveMath.cpp
	MoveTypes/MoveMath/ShipMoveMath.cpp
	MoveTypes/MoveType.cpp
	MoveTypes/MoveTypeFactory.cpp
	MoveTypes/ScriptMoveType.cpp
	MoveTypes/StaticMoveType.cpp
	MoveTypes/HoverAirMoveType.cpp
	Objects/SolidObject.cpp
	Objects/SolidObjectDef.cpp
	Objects/WorldObject.cpp
	Path/Default/IPathFinder.cpp
	Path/Default/PathCache.cpp
	Path/Default/PathEstimator.cpp
	Path/Default/PathFinder.cpp
	Path/Default/PathFinderDef.cpp
	Path/Default/PathFlowMap.cpp
	Path/Default/PathHeatMap.cpp
	Path/Default/PathManager.cpp
	Path/QTPFS/Node.cpp
	Path/QTPFS/NodeLayer.cpp
	Path/QTPFS/PathCache.cpp
	Path/QTPFS/PathSearch.cpp
	Path/QTPFS/PathManager.cpp
	Path/IPathController.cpp
	Path/IPathManager.cpp
	Projectiles/ExpGenSpawnable.cpp
	Projectiles/ExpGenSpawner.cpp
	Projectiles/ExplosionListener.cpp
	Projectiles/ExplosionGenerator.cpp
	Projectiles/FireProjectile.cpp
	Projectiles/FlareProjectile.cpp
	Projectiles/PieceProjectile.cpp
	Projectiles/Projectile.cpp
	Projectiles/ProjectileHandler.cpp
	Projectiles/ProjectileFunctors.cpp
	Projectiles/WeaponProjectiles/BeamLaserProjectile.cpp
	Projectiles/WeaponProjectiles/EmgProjectile.cpp
	Projectiles/WeaponProjectiles/ExplosiveProjectile.cpp
	Projectiles/WeaponProjectiles/FireBallProjectile.cpp
	Projectiles/WeaponProjectiles/FlameProjectile.cpp
	Projectiles/WeaponProjectiles/LargeBeamLaserProjectile.cpp
	Projectiles/WeaponProjectiles/LaserProjectile.cpp
	Projectiles/WeaponProjectiles/LightningProjectile.cpp
	Projectiles/WeaponProjectiles/MissileProjectile.cpp
	Projectiles/WeaponProjectiles/StarburstProjectile.cpp
	Projectiles/WeaponProjectiles/TorpedoProjectile.cpp
	Projectiles/WeaponProjectiles/WeaponProjectile.cpp
	Projectiles/WeaponProjectiles/WeaponProjectileFactory.cpp
	Units/BuildInfo.cpp
	Units/CommandAI/AirCAI.cpp
	Units/CommandAI/BuilderCAI.cpp
	Units/CommandAI/Command.cpp
	Units/CommandAI/CommandAI.cpp
	Units/CommandAI/CommandDescription.cpp
	Units/CommandAI/FactoryCAI.cpp
	Units/CommandAI/MobileCAI.cpp
	Units/Scripts/CobEngine.cpp
	Units/Scripts/CobFile.cpp
	Units/Scripts/CobFileHandler.cpp
	Units/Scripts/CobInstance.cpp
	Units/Scripts/CobScriptNames.cpp
	Units/Scripts/CobThread.cpp
	Units/Scripts/LuaScriptNames.cpp
	Units/Scripts/LuaUnitScript.cpp
	Units/Scripts/NullUnitScript.cpp
	Units/Scripts/UnitScript.cpp
	Units/Scripts/UnitScriptEngine.cpp
	Units/Scripts/UnitScriptFactory.cpp
	Units/Unit.cpp
	Units/UnitDef.cpp
	Units/UnitDefHandler.cpp
	Units/UnitHandler.cpp
	Units/UnitLoader.cpp
	Units/UnitToolTipMap.cpp
	Units/UnitTypes/Builder.cpp
	Units/UnitTypes/Building.cpp
	Units/UnitTypes/ExtractorBuilding.cpp
	Units/UnitTypes/Factory.cpp
	Weapons/BeamLaser.cpp
	Weapons/BombDropper.cpp
	Weapons/Cannon.cpp
	Weapons/DGunWeapon.cpp
	Weapons/EmgCannon.cpp
	Weapons/FlameThrower.cpp
	Weapons/LaserCannon.cpp
	Weapons/LightningCannon.cpp
	Weapons/MeleeWeapon.cpp
	Weapons/MissileLauncher.cpp
	Weapons/NoWeapon.cpp
	Weapons/PlasmaRepulser.cpp
	Weapons/Rifle.cpp
	Weapons/StarburstLauncher.cpp
	Weapons/TorpedoLauncher.cpp
	Weapons/Weapon.cpp
	Weapons/WeaponDef.cpp
	Weapons/WeaponDefHandler.cpp
	Weapons/WeaponLoader.cpp
	Weapons/WeaponTarget.cpp
)

target_include_directories(engineSim
PRIVATE
	${Spring_SOURCE_DIR}/rts
	${Spring_SOURCE_DIR}/rts/lib/lua/include
	${SDL2_INCLUDE_DIR}
	${SPRING_MINIZIP_INCLUDE_DIR}
)

if( CMAKE_COMPILER_IS_GNUCXX)
	# FIXME: hack to avoid linkers to remove not referenced symbols. required because of
	# https://springrts.com/mantis/view.php?id=4511
	if(APPLE)
		make_global_var(SPRING_SIM_LIBRARIES "-Wl,-force_load" engineSim)
	else ()
		make_global_var(SPRING_SIM_LIBRARIES "-Wl,-whole-archive" engineSim "-Wl,-no-whole-archive")
	endif ()
else ()
	make_global_var(SPRING_SIM_LIBRARIES engineSim)
endif ()

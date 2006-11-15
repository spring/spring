// Copyright Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURVector3E. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//
// ======================================================================================

#ifndef _UNITDEFFORCSHARP_H
#define _UNITDEFFORCSHARP_H

#using <mscorlib.dll>
#include <vcclr.h>
using namespace System;
using namespace System::Collections;
using namespace System::Runtime::InteropServices;    

#include "Sim/Units/UnitDef.h"

#include "CSAIProxyMoveData.h"

#include "AbicUnitDefWrapper.h"
#include "AbicMoveDataWrapper.h"

// This is a managed class that we can use to pass unitdefs to C#
// Since unitdefs are very large, and often passed in arrays, we dont copy by value
// but simply wrap a pointer to the original C++ unitdef,
// then we copy out the data on demand, via the get accessor functions
//
// This class derives from a C# class IUnitDef, so you'll need to update the IUnitDef class as well (in the C# code)
// for any additional accessors to work
//
// The function names are systematically get_<property name>  This is automatically defined by C# for any property
//
// This file is partially-generated using BuildTools\GenerateUnitDefClasses.exe
//
__gc class UnitDefForCSharp : public CSharpAI::IUnitDef
{    
public:
    AbicUnitDefWrapper *self;

    UnitDefForCSharp( const AbicUnitDefWrapper *actualunitdef )
    {
        this->self = ( AbicUnitDefWrapper * )actualunitdef;
    }
    
    // please use GetBuildOption( const UnitDef *self, int index ); // assumes map is really a vector where the int is a contiguous index starting from 1
    //CSharpAI::BuildOption *get_buildOptions()[]
    //{
      //  ArrayList *buildoptionarraylist = new ArrayList();
        //for(map<int, string>::const_iterator j = actualunitdef->buildOptions.begin(); j != actualunitdef->buildOptions.end(); j++)
        //{
          //  CSharpAI::BuildOption *buildoption = new CSharpAI::BuildOption( j->first, new System::String( j->second.c_str() ) );
            //buildoptionarraylist->Add( buildoption );
		//}
       // 
        //return dynamic_cast< CSharpAI::BuildOption*[] >( buildoptionarraylist->ToArray( __typeof( CSharpAI::BuildOption ) ) );
    //}
    
    CSharpAI::IMoveData *get_movedata()
    {
        if( self->get_movedata() != 0 )
        {
            return new MoveDataProxy( self->get_movedata() ); 
        }
        return 0;
    }
    
    #include "CSAIProxyIUnitDef_generated.h"
    
    /*
    // Following is auto-generated:
   System::String *get_name(){ return new System::String( actualunitdef->name.c_str() ); }
   System::String *get_humanName(){ return new System::String( actualunitdef->humanName.c_str() ); }
   System::String *get_filename(){ return new System::String( actualunitdef->filename.c_str() ); }
   bool get_loaded(){ return actualunitdef->loaded; }
   int get_id(){ return actualunitdef->id; }
   System::String *get_buildpicname(){ return new System::String( actualunitdef->buildpicname.c_str() ); }
   int get_aihint(){ return actualunitdef->aihint; }
   int get_techLevel(){ return actualunitdef->techLevel; }
   double get_metalUpkeep(){ return actualunitdef->metalUpkeep; }
   double get_energyUpkeep(){ return actualunitdef->energyUpkeep; }
   double get_metalMake(){ return actualunitdef->metalMake; }
   double get_makesMetal(){ return actualunitdef->makesMetal; }
   double get_energyMake(){ return actualunitdef->energyMake; }
   double get_metalCost(){ return actualunitdef->metalCost; }
   double get_energyCost(){ return actualunitdef->energyCost; }
   double get_buildTime(){ return actualunitdef->buildTime; }
   double get_extractsMetal(){ return actualunitdef->extractsMetal; }
   double get_extractRange(){ return actualunitdef->extractRange; }
   double get_windGenerator(){ return actualunitdef->windGenerator; }
   double get_tidalGenerator(){ return actualunitdef->tidalGenerator; }
   double get_metalStorage(){ return actualunitdef->metalStorage; }
   double get_energyStorage(){ return actualunitdef->energyStorage; }
   double get_autoHeal(){ return actualunitdef->autoHeal; }
   double get_idleAutoHeal(){ return actualunitdef->idleAutoHeal; }
   int get_idleTime(){ return actualunitdef->idleTime; }
   double get_power(){ return actualunitdef->power; }
   double get_health(){ return actualunitdef->health; }
   double get_speed(){ return actualunitdef->speed; }
   double get_turnRate(){ return actualunitdef->turnRate; }
   int get_moveType(){ return actualunitdef->moveType; }
   bool get_upright(){ return actualunitdef->upright; }
   double get_controlRadius(){ return actualunitdef->controlRadius; }
   double get_losRadius(){ return actualunitdef->losRadius; }
   double get_airLosRadius(){ return actualunitdef->airLosRadius; }
   double get_losHeight(){ return actualunitdef->losHeight; }
   int get_radarRadius(){ return actualunitdef->radarRadius; }
   int get_sonarRadius(){ return actualunitdef->sonarRadius; }
   int get_jammerRadius(){ return actualunitdef->jammerRadius; }
   int get_sonarJamRadius(){ return actualunitdef->sonarJamRadius; }
   int get_seismicRadius(){ return actualunitdef->seismicRadius; }
   double get_seismicSignature(){ return actualunitdef->seismicSignature; }
   bool get_stealth(){ return actualunitdef->stealth; }
   double get_buildSpeed(){ return actualunitdef->buildSpeed; }
   double get_buildDistance(){ return actualunitdef->buildDistance; }
   double get_mass(){ return actualunitdef->mass; }
   double get_maxSlope(){ return actualunitdef->maxSlope; }
   double get_maxHeightDif(){ return actualunitdef->maxHeightDif; }
   double get_minWaterDepth(){ return actualunitdef->minWaterDepth; }
   double get_waterline(){ return actualunitdef->waterline; }
   double get_maxWaterDepth(){ return actualunitdef->maxWaterDepth; }
   double get_armoredMultiple(){ return actualunitdef->armoredMultiple; }
   int get_armorType(){ return actualunitdef->armorType; }
   System::String *get_type(){ return new System::String( actualunitdef->type.c_str() ); }
   System::String *get_tooltip(){ return new System::String( actualunitdef->tooltip.c_str() ); }
   System::String *get_wreckName(){ return new System::String( actualunitdef->wreckName.c_str() ); }
   System::String *get_deathExplosion(){ return new System::String( actualunitdef->deathExplosion.c_str() ); }
   System::String *get_selfDExplosion(){ return new System::String( actualunitdef->selfDExplosion.c_str() ); }
   System::String *get_TEDClassString(){ return new System::String( actualunitdef->TEDClassString.c_str() ); }
   System::String *get_categoryString(){ return new System::String( actualunitdef->categoryString.c_str() ); }
   System::String *get_iconType(){ return new System::String( actualunitdef->iconType.c_str() ); }
   int get_selfDCountdown(){ return actualunitdef->selfDCountdown; }
   bool get_canfly(){ return actualunitdef->canfly; }
   bool get_canmove(){ return actualunitdef->canmove; }
   bool get_canhover(){ return actualunitdef->canhover; }
   bool get_floater(){ return actualunitdef->floater; }
   bool get_builder(){ return actualunitdef->builder; }
   bool get_activateWhenBuilt(){ return actualunitdef->activateWhenBuilt; }
   bool get_onoffable(){ return actualunitdef->onoffable; }
   bool get_reclaimable(){ return actualunitdef->reclaimable; }
   bool get_canRestore(){ return actualunitdef->canRestore; }
   bool get_canRepair(){ return actualunitdef->canRepair; }
   bool get_canReclaim(){ return actualunitdef->canReclaim; }
   bool get_noAutoFire(){ return actualunitdef->noAutoFire; }
   bool get_canAttack(){ return actualunitdef->canAttack; }
   bool get_canPatrol(){ return actualunitdef->canPatrol; }
   bool get_canFight(){ return actualunitdef->canFight; }
   bool get_canGuard(){ return actualunitdef->canGuard; }
   bool get_canBuild(){ return actualunitdef->canBuild; }
   bool get_canAssist(){ return actualunitdef->canAssist; }
   bool get_canRepeat(){ return actualunitdef->canRepeat; }
   double get_wingDrag(){ return actualunitdef->wingDrag; }
   double get_wingAngle(){ return actualunitdef->wingAngle; }
   double get_drag(){ return actualunitdef->drag; }
   double get_frontToSpeed(){ return actualunitdef->frontToSpeed; }
   double get_speedToFront(){ return actualunitdef->speedToFront; }
   double get_myGravity(){ return actualunitdef->myGravity; }
   double get_maxBank(){ return actualunitdef->maxBank; }
   double get_maxPitch(){ return actualunitdef->maxPitch; }
   double get_turnRadius(){ return actualunitdef->turnRadius; }
   double get_wantedHeight(){ return actualunitdef->wantedHeight; }
   bool get_hoverAttack(){ return actualunitdef->hoverAttack; }
   double get_dlHoverFactor(){ return actualunitdef->dlHoverFactor; }
   double get_maxAcc(){ return actualunitdef->maxAcc; }
   double get_maxDec(){ return actualunitdef->maxDec; }
   double get_maxAileron(){ return actualunitdef->maxAileron; }
   double get_maxElevator(){ return actualunitdef->maxElevator; }
   double get_maxRudder(){ return actualunitdef->maxRudder; }
   int get_xsize(){ return actualunitdef->xsize; }
   int get_ysize(){ return actualunitdef->ysize; }
   int get_buildangle(){ return actualunitdef->buildangle; }
   double get_loadingRadius(){ return actualunitdef->loadingRadius; }
   int get_transportCapacity(){ return actualunitdef->transportCapacity; }
   int get_transportSize(){ return actualunitdef->transportSize; }
   bool get_isAirBase(){ return actualunitdef->isAirBase; }
   double get_transportMass(){ return actualunitdef->transportMass; }
   bool get_canCloak(){ return actualunitdef->canCloak; }
   bool get_startCloaked(){ return actualunitdef->startCloaked; }
   double get_cloakCost(){ return actualunitdef->cloakCost; }
   double get_cloakCostMoving(){ return actualunitdef->cloakCostMoving; }
   double get_decloakDistance(){ return actualunitdef->decloakDistance; }
   bool get_canKamikaze(){ return actualunitdef->canKamikaze; }
   double get_kamikazeDist(){ return actualunitdef->kamikazeDist; }
   bool get_targfac(){ return actualunitdef->targfac; }
   bool get_canDGun(){ return actualunitdef->canDGun; }
   bool get_needGeo(){ return actualunitdef->needGeo; }
   bool get_isFeature(){ return actualunitdef->isFeature; }
   bool get_hideDamage(){ return actualunitdef->hideDamage; }
   bool get_isCommander(){ return actualunitdef->isCommander; }
   bool get_showPlayerName(){ return actualunitdef->showPlayerName; }
   bool get_canResurrect(){ return actualunitdef->canResurrect; }
   bool get_canCapture(){ return actualunitdef->canCapture; }
   int get_highTrajectoryType(){ return actualunitdef->highTrajectoryType; }
   bool get_leaveTracks(){ return actualunitdef->leaveTracks; }
   double get_trackWidth(){ return actualunitdef->trackWidth; }
   double get_trackOffset(){ return actualunitdef->trackOffset; }
   double get_trackStrength(){ return actualunitdef->trackStrength; }
   double get_trackStretch(){ return actualunitdef->trackStretch; }
   int get_trackType(){ return actualunitdef->trackType; }
   bool get_canDropFlare(){ return actualunitdef->canDropFlare; }
   double get_flareReloadTime(){ return actualunitdef->flareReloadTime; }
   double get_flareEfficieny(){ return actualunitdef->flareEfficieny; }
   double get_flareDelay(){ return actualunitdef->flareDelay; }
   int get_flareTime(){ return actualunitdef->flareTime; }
   int get_flareSalvoSize(){ return actualunitdef->flareSalvoSize; }
   int get_flareSalvoDelay(){ return actualunitdef->flareSalvoDelay; }
   bool get_smoothAnim(){ return actualunitdef->smoothAnim; }
   bool get_isMetalMaker(){ return actualunitdef->isMetalMaker; }
   bool get_canLoopbackAttack(){ return actualunitdef->canLoopbackAttack; }
   bool get_levelGround(){ return actualunitdef->levelGround; }
   bool get_useBuildingGroundDecal(){ return actualunitdef->useBuildingGroundDecal; }
   int get_buildingDecalType(){ return actualunitdef->buildingDecalType; }
   int get_buildingDecalSizeX(){ return actualunitdef->buildingDecalSizeX; }
   int get_buildingDecalSizeY(){ return actualunitdef->buildingDecalSizeY; }
   double get_buildingDecalDecaySpeed(){ return actualunitdef->buildingDecalDecaySpeed; }
   bool get_isfireplatform(){ return actualunitdef->isfireplatform; }
   bool get_showNanoFrame(){ return actualunitdef->showNanoFrame; }
   bool get_showNanoSpray(){ return actualunitdef->showNanoSpray; }
   double get_maxFuel(){ return actualunitdef->maxFuel; }
   double get_refuelTime(){ return actualunitdef->refuelTime; }
   double get_minAirBasePower(){ return actualunitdef->minAirBasePower; }
   */
};

#endif

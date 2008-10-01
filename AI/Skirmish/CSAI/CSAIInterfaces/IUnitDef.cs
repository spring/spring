// Copyright Spring project, Hugh Perkins 2006
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
//

namespace CSharpAI
{
    public class BuildOption
    {
        public int TablePosition;
        public string UnitTypeName;
        public BuildOption(){}
        public BuildOption( int TablePosition, string UnitTypeName )
        {
            this.TablePosition = TablePosition;
            this.UnitTypeName = UnitTypeName;
        }
    }
    
    // definition of a unit
    // data is copied out of the C++ object on demand, for speed, by the accessor functions
    // the C#/C++ interface derives from this, and simply stores a pointer to the original C++ UnitDef inside
    //
    
    // This file is partially-generated using BuildTools\GenerateUnitDefClasses.exe
    public interface IUnitDef
    {
        // Hashtable( int tableposition, string unittypename )
        //BuildOption[] buildOptions{ get; }
        int GetNumBuildOptions();
        string GetBuildOption( int index );
        
        IMoveData movedata{ get; }
        
        // you can copy output from buildtools\generateunitdefclasses to here:
        string name{ get; }
        string humanName{ get; }
        string filename{ get; }
        int id{ get; }
       // string buildpicname{ get; }
        int aihint{ get; }
        int techLevel{ get; }
        double metalUpkeep{ get; }
        double energyUpkeep{ get; }
        double metalMake{ get; }
        double makesMetal{ get; }
        double energyMake{ get; }
        double metalCost{ get; }
        double energyCost{ get; }
        double buildTime{ get; }
        double extractsMetal{ get; }
        double extractRange{ get; }
        double windGenerator{ get; }
        double tidalGenerator{ get; }
        double metalStorage{ get; }
        double energyStorage{ get; }
        double autoHeal{ get; }
        double idleAutoHeal{ get; }
        int idleTime{ get; }
        double power{ get; }
        double health{ get; }
        double speed{ get; }
        double turnRate{ get; }
        int moveType{ get; }
        bool upright{ get; }
        double controlRadius{ get; }
        double losRadius{ get; }
        double airLosRadius{ get; }
        double losHeight{ get; }
        int radarRadius{ get; }
        int sonarRadius{ get; }
        int jammerRadius{ get; }
        int sonarJamRadius{ get; }
        int seismicRadius{ get; }
        double seismicSignature{ get; }
        bool stealth{ get; }
        double buildSpeed{ get; }
        double buildDistance{ get; }
        double mass{ get; }
        double maxSlope{ get; }
        double maxHeightDif{ get; }
        double minWaterDepth{ get; }
        double waterline{ get; }
        double maxWaterDepth{ get; }
        double armoredMultiple{ get; }
        int armorType{ get; }
        string type{ get; }
        string tooltip{ get; }
        string wreckName{ get; }
        string deathExplosion{ get; }
        string selfDExplosion{ get; }
        string TEDClassString{ get; }
        string categoryString{ get; }
        //string iconType{ get; }
        int selfDCountdown{ get; }
        bool canfly{ get; }
        bool canmove{ get; }
        bool canhover{ get; }
        bool floater{ get; }
        bool builder{ get; }
        bool activateWhenBuilt{ get; }
        bool onoffable{ get; }
        bool reclaimable{ get; }
        bool canRestore{ get; }
        bool canRepair{ get; }
        bool canReclaim{ get; }
      //  bool noAutoFire{ get; }
        bool canAttack{ get; }
        bool canPatrol{ get; }
        bool canFight{ get; }
        bool canGuard{ get; }
        bool canBuild{ get; }
        bool canAssist{ get; }
        bool canRepeat{ get; }
        double wingDrag{ get; }
        double wingAngle{ get; }
        double drag{ get; }
        double frontToSpeed{ get; }
        double speedToFront{ get; }
        double myGravity{ get; }
        double maxBank{ get; }
        double maxPitch{ get; }
        double turnRadius{ get; }
        double wantedHeight{ get; }
        bool hoverAttack{ get; }
        double dlHoverFactor{ get; }
        double maxAcc{ get; }
        double maxDec{ get; }
        double maxAileron{ get; }
        double maxElevator{ get; }
        double maxRudder{ get; }
        int xsize{ get; }
        int ysize{ get; }
        int buildangle{ get; }
        double loadingRadius{ get; }
        int transportCapacity{ get; }
        int transportSize{ get; }
        bool isAirBase{ get; }
        double transportMass{ get; }
        bool canCloak{ get; }
        bool startCloaked{ get; }
        double cloakCost{ get; }
        double cloakCostMoving{ get; }
        double decloakDistance{ get; }
        bool canKamikaze{ get; }
        double kamikazeDist{ get; }
        bool targfac{ get; }
        bool canDGun{ get; }
        bool needGeo{ get; }
        bool isFeature{ get; }
        bool hideDamage{ get; }
        bool isCommander{ get; }
        bool showPlayerName{ get; }
        bool canResurrect{ get; }
        bool canCapture{ get; }
        int highTrajectoryType{ get; }
        bool leaveTracks{ get; }
        double trackWidth{ get; }
        double trackOffset{ get; }
        double trackStrength{ get; }
        double trackStretch{ get; }
        int trackType{ get; }
        bool canDropFlare{ get; }
        double flareReloadTime{ get; }
       // double flareEfficieny{ get; }
        double flareDelay{ get; }
        int flareTime{ get; }
        int flareSalvoSize{ get; }
        int flareSalvoDelay{ get; }
        bool smoothAnim{ get; }
        bool isMetalMaker{ get; }
        bool canLoopbackAttack{ get; }
        bool levelGround{ get; }
        bool useBuildingGroundDecal{ get; }
        int buildingDecalType{ get; }
        int buildingDecalSizeX{ get; }
        int buildingDecalSizeY{ get; }
        double buildingDecalDecaySpeed{ get; }
        bool isfireplatform{ get; }
        bool showNanoFrame{ get; }
        bool showNanoSpray{ get; }
        double maxFuel{ get; }
        double refuelTime{ get; }
        double minAirBasePower{ get; }
    }
}

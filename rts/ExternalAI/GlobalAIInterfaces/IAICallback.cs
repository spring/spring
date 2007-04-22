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
//

using System;

namespace CSharpAI
{
    
    public class MapPoint
    {
        public Float3 Pos;
       // public int Color;
        public string Label;
    }
    
    public class AIOrderFailedException : Exception
    {
        public AIOrderFailedException() : base()
        {
        }
    }    

    // This is the C# equivalent of AICallback.h
    // This is how we drive the game
    public interface IAICallback
    {
        void SendTextMsg( string text, int priority);        
        
        //IUnitDef[] GetUnitDefList(); // use GetUnitDefByTypeId instead
        IUnitDef GetUnitDefByTypeId( int unittypeid );
        IUnitDef GetUnitDef( int unitid );
        
        //void AddMapPoint( Float3 pos, string label );
        
        Float3 GetUnitPos( int unitid );
        Float3 ClosestBuildSite( IUnitDef unittobuilddef, Float3 targetpos, double searchRadius, int minDistance );    
        bool CanBuildAt( IUnitDef unitDef, Float3 pos );
        int GiveOrder( int unit, Command command );
        void DrawUnit( string name, Float3 pos, double rotation, int lifetime, int team, bool transparent, bool drawBorder );
        
        int CreateGroup(string dll, int aiNumber);							//creates a group and return the id it was given, return -1 on failure (dll didnt exist etc)
        void EraseGroup(int groupid);											//erases a specified group
        //const std::vector<CommandDescription>* GetGroupCommands(int unitid);	//the commands that this unit can understand, other commands will be ignored
        //int GiveGroupOrder(int unitid, Command c);
        
        string GetModName ();
        string GetMapName ();
        
        byte [] GetMetalMap(); // mapwidth / 2 by mapheight / 2
        bool[] GetLosMap(); // mapwidth /2  by mapheight/2
        bool[] GetRadarMap(); // mapwidth /8  by mapheight/8
        //double[] GetSlopeMap();
        double[] GetCentreHeightMap(); // gets the centre height of each square; mapwidth by mapheight ( calls aicallback->GetHeightMap, which ,despite the name, doesnt actually return the heightmap :-/ )
        
        int[] GetFriendlyUnits();	
        int[] GetEnemyUnitsInRadarAndLos();	
        
        //MapPoint[] GetMapPoints();
        
        int CreateLineFigure(Float3 pos1,Float3 pos2,double width,bool arrow,int lifetime,int group);
        void SetFigureColor(int group,double red,double green,double blue,double alpha);
        void DeleteFigureGroup(int group);
        
        // Command[] GetCurrentUnitCommands(int unitid);  ditch this for now, add in:
        bool UnitIsBusy( int unitid );
        
        // From GetProperty etc:
        bool IsGamePaused();
        
        int[] GetFeatures ();
        int[] GetFeatures ( Float3 pos, double radius);
        IFeatureDef GetFeatureDef( int feature );
        Float3 GetFeaturePos (int feature);
    
        // **********************************************************************************************************************
        // all beyond this point was auto-generated by GenerateIACallbackClasses, then copy and pasted in:
        int GetCurrentFrame(  );
        int GetMyTeam(  );
        int GetMyAllyTeam(  );
        int GetPlayerTeam( int player );
        bool AddUnitToGroup( int unitid, int groupid );
        bool RemoveUnitFromGroup( int unitid );
        int GetUnitGroup( int unitid );
        int GetUnitAiHint( int unitid );
        int GetUnitTeam( int unitid );
        int GetUnitAllyTeam( int unitid );
        double GetUnitHealth( int unitid );
        double GetUnitMaxHealth( int unitid );
        double GetUnitSpeed( int unitid );
        double GetUnitPower( int unitid );
        double GetUnitExperience( int unitid );
        double GetUnitMaxRange( int unitid );
        bool IsUnitActivated( int unitid );
        bool UnitBeingBuilt( int unitid );
        int GetBuildingFacing( int unitid );
        bool IsUnitCloaked( int unitid );
        bool IsUnitParalyzed( int unitid );
        int GetMapWidth(  );
        int GetMapHeight(  );
        double GetMaxMetal(  );
        double GetExtractorRadius(  );
        double GetMinWind(  );
        double GetMaxWind(  );
        double GetTidalStrength(  );
        double GetGravity(  );
        double GetElevation( double x, double z );
        double GetMetal(  );
        double GetMetalIncome(  );
        double GetMetalUsage(  );
        double GetMetalStorage(  );
        double GetEnergy(  );
        double GetEnergyIncome(  );
        double GetEnergyUsage(  );
        double GetEnergyStorage(  );
        double GetFeatureHealth( int feature );
        double GetFeatureReclaimLeft( int feature );
        int GetNumUnitDefs(  );
        double GetUnitDefRadius( int def );
        double GetUnitDefHeight( int def );
    }
}

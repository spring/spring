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
using System.IO;
using System.Collections;
using System.Xml;

namespace CSharpAI
{
    // wow, a class that doesnt use Singleton pattern!
    // this class works out appropriate enemies to attack, based on profile we define for it
    // since different units have a different preference for attacking (air vs ground vs speed etc), we need a different object for each
    public class EnemySelector
    {
        public double maxenemyspeed;
        public bool WaterOk;
        public bool BadTerrainOk;
            
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        EnemyController enemycontroller;
        UnitDefHelp unitdefhelp;
        
        public EnemySelector( double maxenemyspeed, bool WaterOk, bool BadTerrainOk )
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
           // csai.EnemyEntersLOSEvent += new CSAI.EnemyEntersLOSHandler( EnemyEntersLOS );
            
            enemycontroller = EnemyController.GetInstance();
            unitdefhelp = new UnitDefHelp( aicallback );
            
            this.maxenemyspeed = maxenemyspeed;
            this.WaterOk = WaterOk;
            this.BadTerrainOk = BadTerrainOk;
        }
        
        // this is going to have to interact with all sorts of stuff in the future
        // for now keep it simple
        // for now we look for nearby buildings, then nearby enemy units with low speed, then anything
        public Float3 ChooseAttackPoint( Float3 ourpos )
        {
            bool gotbuilding = false;
            bool gotknownunit = false;
            double BestSquaredDistance = 100000000000;
            
            int bestid = 0;
            IUnitDef defforbestid = null;
            Float3 posforbestid = null;
         //   logfile.WriteLine( "EnemySelector: checking mobile... " );
            foreach( DictionaryEntry de in enemycontroller.EnemyUnitDefByDeployedId )
            {
                int thisenemyid = (int)de.Key;
                IUnitDef unitdef = de.Value as IUnitDef;
                Float3 enemypos = aicallback.GetUnitPos( thisenemyid );
               // logfile.WriteLine( "Found building " + 
                double thissquareddistance = Float3Helper.GetSquaredDistance( ourpos, enemypos );
             //   logfile.WriteLine( "EnemySelector: Potential enemy at " + enemypos.ToString() + " squareddistance: " + thissquareddistance );
                if( unitdef != null )
                {
                 //   logfile.WriteLine( "unitdef not null " + unitdef.humanName + " ismobile: " + unitdefhelp.IsMobile( unitdef ).ToString() );
                 //   logfile.WriteLine( "gotbuilding = " + gotbuilding.ToString() );
                    if( gotbuilding )
                    {
                        if( !unitdefhelp.IsMobile( unitdef ) )
                        {
                            if( thissquareddistance < BestSquaredDistance )
                            {
                              //  logfile.WriteLine( "best building so far" );
                                bestid = thisenemyid;
                                gotbuilding = true;
                                gotknownunit = true;
                                posforbestid = enemypos;
                                defforbestid = unitdef;
                                BestSquaredDistance = thissquareddistance;
                            }
                        }
                    }
                    else
                    {
                        if( unitdef.speed < maxenemyspeed ) // if we already have building we dont care
                        {
                            if( thissquareddistance < BestSquaredDistance )
                            {
                            //    logfile.WriteLine( "best known so far" );
                                bestid = thisenemyid;
                                gotknownunit = true;
                                posforbestid = enemypos;
                                defforbestid = unitdef;
                                BestSquaredDistance = thissquareddistance;
                            }
                        }
                    }
                }
                else // if unitdef unknown
                {
                  //  logfile.WriteLine( "gotknownunit = " + gotknownunit.ToString() );
                    if( !gotknownunit ) // otherwise just ignore unknown units
                    {
                        if( thissquareddistance < BestSquaredDistance )
                        {
                        //    logfile.WriteLine( "best unknown so far" );
                            bestid = thisenemyid;
                            posforbestid = enemypos;
                            defforbestid = unitdef;
                            BestSquaredDistance = thissquareddistance;
                        }                        
                    }
                }
            }
            
            foreach( DictionaryEntry de in enemycontroller.EnemyStaticPosByDeployedId )
            {
              //  logfile.WriteLine( "EnemySelector: checking static... " );
                int thisenemyid = (int)de.Key;
                Float3 enemypos = de.Value as Float3;
                double thissquareddistance = Float3Helper.GetSquaredDistance( ourpos, enemypos );
              //  logfile.WriteLine( "EnemySelector: Potential enemy at " + enemypos.ToString() + " squareddistance: " + thissquareddistance );
                if( thissquareddistance < BestSquaredDistance )
                {
                 //   logfile.WriteLine( "EnemySelector: best distance so far" );
                    bestid = thisenemyid;
                    gotbuilding = true;
                    gotknownunit = true;
                    posforbestid = enemypos;
                    //defforbestid = unitdef;
                }
            }
            
            //if( enemycontroller.EnemyStaticPosByDeployedId.Contains( bestid ) )
           // {
           //     enemycontroller.EnemyStaticPosByDeployedId.Remove( bestid );
           // }
            
            return posforbestid;
        }
    }
}

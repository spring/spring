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
using System.Collections;

namespace CSharpAI
{
    // 2d index of enemies
    public class EnemyMap
    {
        static EnemyMap instance = new EnemyMap();
        public static EnemyMap GetInstance(){ return instance; }
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        UnitDefHelp unitdefhelp;
        //UnitController unitcontroller;
        EnemyController enemycontroller;
        
        public Hashtable staticenemyposbyid = new Hashtable();
        public int[,] enemymap; // value is 0 or id of enemy;  this is the position of the centre of the enemy; (its only an index, not a build map)
        
        public int mapwidth;
        public int mapheight;
        
        EnemyMap()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();  
            
            enemymap = new int[ mapwidth / 2, mapheight / 2 ];
            
            unitdefhelp = new UnitDefHelp( aicallback );
            //unitcontroller = UnitController.GetInstance();
            enemycontroller = EnemyController.GetInstance();
            
            //csai.UnitCreatedEvent += new CSAI.UnitCreatedHandler( UnitCreated );
            //csai.UnitDestroyedEvent += new CSAI.UnitDestroyedHandler( UnitDestroyed );
            
            enemycontroller.NewStaticEnemyAddedEvent += new EnemyController.NewStaticEnemyAddedHandler( StaticEnemyAdded );
            enemycontroller.EnemyRemovedEvent += new EnemyController.EnemyRemovedHandler( EnemyRemoved );

            Init();
        }

        public delegate void NewEnemyAddedHandler( int enemyid, IUnitDef unitdef );
        public delegate void EnemyRemovedHandler( int enemyid );
            
        public event NewEnemyAddedHandler NewEnemyAddedEvent;
        public event EnemyRemovedHandler EnemyRemovedEvent;
        
        public Hashtable EnemyUnitDefByDeployedId = new Hashtable();
        public Hashtable EnemyStaticPosByDeployedId = new Hashtable();
            
        public void Init()
        {
            mapwidth = aicallback.GetMapWidth();
            mapheight = aicallback.GetMapHeight();
            
            logfile.WriteLine( "EnemyMap.Init finished()" );
        }
        
        public void StaticEnemyAdded( int id, Float3 pos, IUnitDef unitdef )
        {
            if( !staticenemyposbyid.Contains( id ) )
            {
                staticenemyposbyid.Add( id , pos );
                int mapx = (int)( pos.x / 16 );
                int mapy = (int)( pos.y / 16 );
                enemymap[ mapx, mapy ] = id;
                // IN PROGRESS HERE
            }
        }
        
        public void EnemyRemoved( int id )
        {
            if( staticenemyposbyid.Contains( id ) )
            {
                staticenemyposbyid.Remove( id );
            }
        }        
    }
}

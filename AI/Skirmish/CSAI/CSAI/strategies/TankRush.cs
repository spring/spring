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

namespace CSharpAI
{
    // The goal of this strategy is to create lots of level 1 medium tanks in ARM
    // It will only work effectively when run on an AA mod, on a map with no major obstacles, such as mountains or water
    // Note: for anyone who is wondering, no this class doesn't do anything yet ;-)
    public class TankRush : IStrategy
    {
        static TankRush instance = new TankRush();
        public static TankRush GetInstance(){ return instance; }
        
        StrategyController strategycontroller;
        UnitController unitcontroller;
        
        TankRush()
        {
            strategycontroller = StrategyController.GetInstance();
            strategycontroller.RegisterStrategy( this.GetType().ToString(), this );
            unitcontroller = UnitController.GetInstance();
        }
        
        public double GetEffectivenessEstimate()
        {
            return 1;
        }
        
        int commanderid;
        
        public void Tick()
        {
            //commanderid = unitcontroller.GetCommanderId();
            //if( 
        }
    }
}


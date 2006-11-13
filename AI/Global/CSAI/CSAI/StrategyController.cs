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
    // This class manages the different strategies
    // The strategies can register with this class using RegisterStrategy
    // StrategyController asks them how effective they each are, at this time
    // The one that gives the highest estimate is chosen
    // In the future we can run statistics on the strategies and add weighting accordingly
    public class StrategyController
    {
        static StrategyController instance = new StrategyController();
        public static StrategyController GetInstance(){ return instance; }
        
        CSAI csai;
        LogFile logfile;
        Hashtable strategies = new Hashtable();
        IStrategy currentstrategy = null;
        
        StrategyController()
        {
            csai = CSAI.GetInstance();
            logfile = LogFile.GetInstance();
            csai.TickEvent += new CSAI.TickHandler( Tick );
        }
        
        public void Tick()
        {
            if( currentstrategy == null )
            {
                ChooseStrategy();
            }
            if( currentstrategy != null )
            {
                currentstrategy.Tick();
            }
        }
        
        // Note to self: this should randomly choose according to weightings (estimate ~= weighting)rather than systemtically pick highest
        void ChooseStrategy()
        {
            double HighestEstimate = 0;
            IStrategy BestStrategy = null;
            string bestname = "";
            foreach( DictionaryEntry de in strategies )
            {
                IStrategy strategy = de.Value as IStrategy;
                double thisestimate = strategy.GetEffectivenessEstimate();
                if( thisestimate > HighestEstimate )
                {
                    HighestEstimate = thisestimate;
                    BestStrategy = strategy;
                    bestname = (string)de.Key;
                }
            }
            currentstrategy = BestStrategy;
            logfile.WriteLine( "StrategyController chose strategy " + bestname );
        }
        
        public void RegisterStrategy( string name, IStrategy strategy )
        {
            strategies.Add( name, strategy );
        }
        
        // Maybe strategies can decide when they can help again?
        // Maybe call tick on dormant strategies once a second or so???
      //  public void StrategySignalsICanHelp( IStrategy strategy )
       // {
         //   currentstrategy = null;
          //  ChooseStrategy();            
       // }
        
        public void StrategySignalsICantDoAnyMore(IStrategy strategy )
        {
            currentstrategy = null;
            ChooseStrategy();
        }
    }
}

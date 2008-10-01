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
    // manages metal
    // decides whether we need more extractors etc
    public class MetalController
    {
        static MetalController instance = new MetalController();
        public static MetalController GetInstance(){ return instance; }
    
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        MetalController()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
        }
        
        // looks at current power usage, and requirements for unit, and decides if we can build it without stalling
        // assumes nothing else suddenly starting at same time...
        // do something more elegant later
        public bool CanBuild( IUnitDef def )
        {
            return aicallback.GetMetal() > def.metalCost;
            /*
            double excessmetalrequired = ( def.metalCost - aicallback.GetMetal() ) / def.buildTime;
            double OurIncome = aicallback.GetMetalIncome() - aicallback.GetMetalUsage();
            logfile.WriteLine( "Metal income: " + aicallback.GetMetalIncome() + " metal usage: " + aicallback.GetMetalUsage() );
            bool result = excessmetalrequired < OurIncome;
            logfile.WriteLine( "Current metal: " + aicallback.GetMetal() + " itemmetalcost: " + def.metalCost + " buildtime: " + def.buildTime + " excessmetalrequired: " + excessmetalrequired + " our income: " + OurIncome + " overall: " + result );
            return result;
            */
        }
    }
}

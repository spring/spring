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
    // provides info on a factory to an IUnitRequester
    public class Factory : IFactory
    {
        public int factoryid;
        public IUnitDef unitdef;
        
        public Factory(){}
        public Factory( int factoryid, IUnitDef unitdef )
        {
            this.factoryid = factoryid;
            this.unitdef = unitdef;
        }
        
        public string Name{
            get{
                return unitdef.name.ToLower();
            }
        }
        
        // ask if factory can build def
        public bool CanYouBuild( IUnitDef def )
        {
            return true; // lie for now ;-)
        }
            
        // ask for list of all units factory can build (directly build, not build via children...)
        public IUnitDef[] WhatCanYouBuild()
        {
            return null; // placeholder
        }
        
        public Float3 Pos
        {
            get{
                return CSAI.GetInstance().aicallback.GetUnitPos( factoryid );
            }
        }
    }    
}

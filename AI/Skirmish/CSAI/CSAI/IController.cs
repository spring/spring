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
    // IController is implemented by all controllers, ie a unit of control beneath PlayStyle (maybe beneath strategy too???)
    public interface IController
    {
        void Activate();
        void Disactivate();
        
        // planned, controller can control any units at its discretion, for now:
        void AssignUnits( ArrayList unitdefs );  // give units to this controller
        void RevokeUnits( ArrayList unitdefs );  // remove these units from this controller
        
        // planned, not used yet, controller can use energy and metal at its discretion for now:
        void AssignEnergy( int energy ); // give energy to controller; negative to revoke
        void AssignMetal( int metal ); // give metal to this controller; negative to revoke
        void AssignPower( double power ); // assign continuous power flow to this controller; negative for reverse flow
        void AssignMetalStream( double metalstream ); // assign continuous metal flow to this controller; negative for reverse flow
    }
}

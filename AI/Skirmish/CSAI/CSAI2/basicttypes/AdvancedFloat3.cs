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
    // derives from Float3 class, to add vector operations etc
    public class AdvancedFloat3 : Float3
    {
        public AdvancedFloat3(){}
            
        public AdvancedFloat3( double x, double y, double z ) : base( x, y, z )
        {
        }

        public AdvancedFloat3(Float3 float3) : base( float3 )
        {

        }
        
        public void Normalize()
        {
            double magnitude = Math.Sqrt( x * x + y * y + z * z );
            if( magnitude > 0.00001 )
            {
                x /= magnitude;
                y /= magnitude;
                z /= magnitude;
            }
        }
        
        // ported from Spring's float3.h
        public AdvancedFloat3 Cross( AdvancedFloat3 f )
        {
            return new AdvancedFloat3( y*f.z - z*f.y,
                                                     z*f.x - x*f.z,
                                                     x*f.y - y*f.x  );
        }

    }
}

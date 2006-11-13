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
using System.Xml;

namespace CSharpAI
{
    public class Float3Helper
    {
        public static double GetSquaredDistance( Float3 one, Float3 two )
        {
            return ( two.x - one.x ) * ( two.x - one.x ) + ( two.y - one.y ) * ( two.y - one.y ) + ( two.z - one.z ) * ( two.z - one.z );
        }
        public static void WriteFloat3ToXmlElement( Float3 float3, XmlElement element )
        {
            element.SetAttribute( "x", float3.x.ToString() );
            element.SetAttribute( "y", float3.y.ToString() );
            element.SetAttribute( "z", float3.z.ToString() );
        }
        public static void WriteXmlElementToFloat3( XmlElement element, Float3 float3 )
        {
            float3.x = Convert.ToDouble( element.GetAttribute( "x" ) );
            float3.y = Convert.ToDouble( element.GetAttribute( "y" ) );
            float3.z = Convert.ToDouble( element.GetAttribute( "z" ) );
        }
    }
}

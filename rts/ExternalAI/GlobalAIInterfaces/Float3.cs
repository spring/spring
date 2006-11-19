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
    // port of float3 class from TASpring project
    // This should really be an interface in fact probably
    // because adding extra shit here, like operators, is annoying....
    [Serializable]
    public class Float3
    {
        public double x = 0;
        public double y = 0;
        public double z = 0;
            
        public Float3(){}
        public Float3( double x, double y, double z ){ this.x = x; this.y = y; this.z = z; }
        public Float3( Float3 src )
        {
            this.x = src.x;
            this.y = src.y;
            this.z = src.z;
        }
            
        public override string ToString(){ return "Float3( " + x + ", " + y + ", " + z + " )"; }
        public double[] ToDoubleArray(){ return new double[]{ x, y ,z }; }
        
        // serializes to/from string, format "x,y,z"
        public string ToCsv(){ return x + "," + y + "," + z; }
        public void LoadCsv( string csv )
        {
            try{
                string[] splitstring = csv.Split( ",".ToCharArray() );
                x = Convert.ToDouble( splitstring[0] );
                y = Convert.ToDouble( splitstring[1] );
                z = Convert.ToDouble( splitstring[2] );
            }
            catch( Exception ) // silently ignore invalid data; good/bad?
            {
                x = y = z = 0;
            }
        }
        public static Float3 operator+( Float3 a, Float3 b )
        {
            Float3 result = new Float3();
            result.x = a.x + b.x;
            result.y = a.y + b.y;
            result.z = a.z + b.z;
            return result;
        }
            
        public static Float3 operator-( Float3 a, Float3 b )
        {
            Float3 result = new Float3();
            result.x = a.x - b.x;
            result.y = a.y - b.y;
            result.z = a.z - b.z;
            return result;
        }
            
        public static Float3 operator*( Float3 a, double multiplier )
        {
            Float3 result = new Float3();
            result.x = a.x * multiplier;
            result.y = a.y * multiplier;
            result.z = a.z * multiplier;
            return result;
        }

        public static Float3 operator*( double multiplier, Float3 a )
        {
            Float3 result = new Float3();
            result.x = a.x * multiplier;
            result.y = a.y * multiplier;
            result.z = a.z * multiplier;
            return result;
        }
    }
}

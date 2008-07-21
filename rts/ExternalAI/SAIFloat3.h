/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SAIFLOAT3_H
#define	_SAIFLOAT3_H

#ifdef	__cplusplus
extern "C" {
#endif

struct SAIFloat3 {
    float x, y, z;
};

SAIFloat3 newSAIFloat3(float x, float y, float z);

#ifdef	__cplusplus
}
#endif

//#ifdef	__cplusplus
//#include "float3.h"
//
//SAIFloat3 newSAIFloat3_Cpp(float3* f3) {
//    
//    SAIFloat3 sAIFloat3;
//    
//    sAIFloat3.x = f3->x;
//    sAIFloat3.y = f3->y;
//    sAIFloat3.z = f3->z;
//    
//    return  sAIFloat3;
//}
//#endif

#endif	/* _SAIFLOAT3_H */


/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

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
#define _SAIFLOAT3_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C conform version of float3
 *
 * This struct is the C conform version of float3,
 * which is defined in rts/System/float3.h, and
 * is used as base class for float3 from rts/System/float3.h.
 */
struct SAIFloat3 {
    float x, y, z;
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SAIFLOAT3_H

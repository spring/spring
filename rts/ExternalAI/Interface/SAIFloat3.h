/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

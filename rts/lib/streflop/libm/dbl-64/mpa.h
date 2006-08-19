#include "../streflop_libm_bridge.h"
namespace streflop_libm {/* See the import.pl script for potential modifications */

/*
 * IBM Accurate Mathematical Library
 * Written by International Business Machines Corp.
 * Copyright (C) 2001 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/************************************************************************/
/*  MODULE_NAME: mpa.h                                                  */
/*                                                                      */
/*  FUNCTIONS:                                                          */
/*               mcr                                                    */
/*               acr                                                    */
/*               cr                                                     */
/*               cpy                                                    */
/*               cpymn                                                  */
/*               mp_dbl                                                 */
/*               dbl_mp                                                 */
/*               add                                                    */
/*               sub                                                    */
/*               mul                                                    */
/*               inv                                                    */
/*               dvd                                                    */
/*                                                                      */
/* Arithmetic functions for multiple precision numbers.                 */
/* Common types and definition                                          */
/************************************************************************/


typedef struct {/* This structure holds the details of a multi-precision     */
  int e;        /* floating point number, x: d[0] holds its sign (-1,0 or 1) */
  Double mantissa[40];
inline Double& d(int idx) {return mantissa[idx];}
inline const Double& d(int idx) const {return mantissa[idx];}
} mp_no;        /* d[1]...d[p] hold its mantissa digits. The value of x is,  */
                /* x = d[1]*r**(e-1) + d[2]*r**(e-2) + ... + d[p]*r**(e-p).  */
                /* Here   r = 2**24,   0 <= d[i] < r  and  1 <= p <= 32.     */
                /* p is a global variable. A multi-precision number is       */
                /* always normalized. Namely, d[1] > 0. An exception is      */
                /* a zero which is characterized by d[0] = 0. The terms      */
                /* d[p+1], d[p+2], ... of a none zero number have no         */
                /* significance and so are the terms e, d[1],d[2],...        */
                /* of a zero.                                                */

typedef struct {
inline Double& d() {return DOUBLE_FROM_INT_PTR(&i[0]);}
inline Double& x() {return DOUBLE_FROM_INT_PTR(&i[0]);}
inline Double& d(int idx) {return DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
inline Double& x(int idx) {return DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
inline const Double& d() const {return CONST_DOUBLE_FROM_INT_PTR(&i[0]);}
inline const Double& x() const {return CONST_DOUBLE_FROM_INT_PTR(&i[0]);}
inline const Double& d(int idx) const {return CONST_DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
inline const Double& x(int idx) const {return CONST_DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
 int i[2];} number;

#define  X   x->mantissa
#define  Y   y->mantissa
#define  Z   z->mantissa
#define  EX  x->e
#define  EY  y->e
#define  EZ  z->e

#define ABS(x)   ((x) <  0  ? -(x) : (x))

int __acr(const mp_no *, const mp_no *, int);
int  __cr(const mp_no *, const mp_no *, int);
void __cpy(const mp_no *, mp_no *, int);
void __cpymn(const mp_no *, int, mp_no *, int);
void __mp_dbl(const mp_no *, Double *, int);
void __dbl_mp(Double, mp_no *, int);
void __add(const mp_no *, const mp_no *, mp_no *, int);
void __sub(const mp_no *, const mp_no *, mp_no *, int);
void __mul(const mp_no *, const mp_no *, mp_no *, int);
void __inv(const mp_no *, mp_no *, int);
void __dvd(const mp_no *, const mp_no *, mp_no *, int);

extern void __mpatan (mp_no *, mp_no *, int);
extern void __mpatan2 (mp_no *, mp_no *, mp_no *, int);
extern void __mpsqrt (mp_no *, mp_no *, int);
extern void __mpexp (mp_no *, mp_no *__y, int);
extern void __c32 (mp_no *, mp_no *, mp_no *, int);
extern int __mpranred (Double, mp_no *, int);
}

/* See the import.pl script for potential modifications */
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

/******************************************************************/
/*                                                                */
/* MODULE_NAME:mydefs.h                                           */
/*                                                                */
/* common data and definition                                     */
/******************************************************************/

#ifndef MY_H
#define MY_H

typedef int int4;
typedef struct {
inline Double& d() {return DOUBLE_FROM_INT_PTR(&i[0]);}
inline Double& x() {return DOUBLE_FROM_INT_PTR(&i[0]);}
inline Double& d(int idx) {return DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
inline Double& x(int idx) {return DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
inline const Double& d() const {return CONST_DOUBLE_FROM_INT_PTR(&i[0]);}
inline const Double& x() const {return CONST_DOUBLE_FROM_INT_PTR(&i[0]);}
inline const Double& d(int idx) const {return CONST_DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
inline const Double& x(int idx) const {return CONST_DOUBLE_FROM_INT_PTR(&i[idx*(sizeof(double)/sizeof(i))]);}
int4 i[2];} mynumber;

#define ABS(x)   (((x)>0)?(x):-(x))
#define max(x,y)  (((y)>(x))?(y):(x))
#define min(x,y)  (((y)<(x))?(y):(x))

#endif

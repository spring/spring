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
/* MODULE_NAME:ulog.h                                             */
/*                                                                */
/* common data and variables prototype and definition             */
/******************************************************************/

#ifndef ULOG_H
#define ULOG_H

#ifdef BIG_ENDI
  static const number
  /* polynomial I */
/**/ a2             = {{0xbfe00000, 0x0001aa8f} }, /* -0.500... */
/**/ a3             = {{0x3fd55555, 0x55588d2e} }, /*  0.333... */
  /* polynomial II */
/**/ b0             = {{0x3fd55555, 0x55555555} }, /*  0.333... */
/**/ b1             = {{0xbfcfffff, 0xffffffbb} }, /* -0.249... */
/**/ b2             = {{0x3fc99999, 0x9999992f} }, /*  0.199... */
/**/ b3             = {{0xbfc55555, 0x556503fd} }, /* -0.166... */
/**/ b4             = {{0x3fc24924, 0x925b3d62} }, /*  0.142... */
/**/ b5             = {{0xbfbffffe, 0x160472fc} }, /* -0.124... */
/**/ b6             = {{0x3fbc71c5, 0x25db58ac} }, /*  0.111... */
/**/ b7             = {{0xbfb9a4ac, 0x11a2a61c} }, /* -0.100... */
/**/ b8             = {{0x3fb75077, 0x0df2b591} }, /*  0.091... */
  /* polynomial III */
#if 0
/**/ c1             = {{0x3ff00000, 0x00000000} }, /*  1        */
#endif
/**/ c2             = {{0xbfe00000, 0x00000000} }, /* -1/2      */
/**/ c3             = {{0x3fd55555, 0x55555555} }, /*  1/3      */
/**/ c4             = {{0xbfd00000, 0x00000000} }, /* -1/4      */
/**/ c5             = {{0x3fc99999, 0x9999999a} }, /*  1/5      */
  /* polynomial IV */
/**/ d2             = {{0xbfe00000, 0x00000000} }, /* -1/2      */
/**/ dd2            = {{0x00000000, 0x00000000} }, /* -1/2-d2   */
/**/ d3             = {{0x3fd55555, 0x55555555} }, /*  1/3      */
/**/ dd3            = {{0x3c755555, 0x55555555} }, /*  1/3-d3   */
/**/ d4             = {{0xbfd00000, 0x00000000} }, /* -1/4      */
/**/ dd4            = {{0x00000000, 0x00000000} }, /* -1/4-d4   */
/**/ d5             = {{0x3fc99999, 0x9999999a} }, /*  1/5      */
/**/ dd5            = {{0xbc699999, 0x9999999a} }, /*  1/5-d5   */
/**/ d6             = {{0xbfc55555, 0x55555555} }, /* -1/6      */
/**/ dd6            = {{0xbc655555, 0x55555555} }, /* -1/6-d6   */
/**/ d7             = {{0x3fc24924, 0x92492492} }, /*  1/7      */
/**/ dd7            = {{0x3c624924, 0x92492492} }, /*  1/7-d7   */
/**/ d8             = {{0xbfc00000, 0x00000000} }, /* -1/8      */
/**/ dd8            = {{0x00000000, 0x00000000} }, /* -1/8-d8   */
/**/ d9             = {{0x3fbc71c7, 0x1c71c71c} }, /*  1/9      */
/**/ dd9            = {{0x3c5c71c7, 0x1c71c71c} }, /*  1/9-d9   */
/**/ d10            = {{0xbfb99999, 0x9999999a} }, /* -1/10     */
/**/ dd10           = {{0x3c599999, 0x9999999a} }, /* -1/10-d10 */
/**/ d11            = {{0x3fb745d1, 0x745d1746} }, /*  1/11     */
/**/ d12            = {{0xbfb55555, 0x55555555} }, /* -1/12     */
/**/ d13            = {{0x3fb3b13b, 0x13b13b14} }, /*  1/13     */
/**/ d14            = {{0xbfb24924, 0x92492492} }, /* -1/14     */
/**/ d15            = {{0x3fb11111, 0x11111111} }, /*  1/15     */
/**/ d16            = {{0xbfb00000, 0x00000000} }, /* -1/16     */
/**/ d17            = {{0x3fae1e1e, 0x1e1e1e1e} }, /*  1/17     */
/**/ d18            = {{0xbfac71c7, 0x1c71c71c} }, /* -1/18     */
/**/ d19            = {{0x3faaf286, 0xbca1af28} }, /*  1/19     */
/**/ d20            = {{0xbfa99999, 0x9999999a} }, /* -1/20     */
  /* constants    */
/**/ zero           = {{0x00000000, 0x00000000} }, /* 0         */
/**/ one            = {{0x3ff00000, 0x00000000} }, /* 1         */
/**/ half           = {{0x3fe00000, 0x00000000} }, /* 1/2       */
/**/ mhalf          = {{0xbfe00000, 0x00000000} }, /* -1/2      */
/**/ sqrt_2         = {{0x3ff6a09e, 0x667f3bcc} }, /* sqrt(2)   */
/**/ h1             = {{0x3fd2e000, 0x00000000} }, /* 151/2**9  */
/**/ h2             = {{0x3f669000, 0x00000000} }, /* 361/2**17 */
/**/ delu           = {{0x3f700000, 0x00000000} }, /* 1/2**8    */
/**/ delv           = {{0x3ef00000, 0x00000000} }, /* 1/2**16   */
/**/ ln2a           = {{0x3fe62e42, 0xfefa3800} }, /* ln(2) 43 bits */
/**/ ln2b           = {{0x3d2ef357, 0x93c76730} }, /* ln(2)-ln2a    */
/**/ e1             = {{0x3bbcc868, 0x00000000} }, /* 6.095e-21     */
/**/ e2             = {{0x3c1138ce, 0x00000000} }, /* 2.334e-19     */
/**/ e3             = {{0x3aa1565d, 0x00000000} }, /* 2.801e-26     */
/**/ e4             = {{0x39809d88, 0x00000000} }, /* 1.024e-31     */
/**/ e[M]           ={{{0x37da223a, 0x00000000} }, /* 1.2e-39       */
/**/                  {{0x35c851c4, 0x00000000} }, /* 1.3e-49       */
/**/                  {{0x2ab85e51, 0x00000000} }, /* 6.8e-103      */
/**/                  {{0x17383827, 0x00000000} }},/* 8.1e-197      */
/**/ two54          = {{0x43500000, 0x00000000} }, /* 2**54         */
/**/ u03            = {{0x3f9eb851, 0xeb851eb8} }; /* 0.03          */

#else
#ifdef LITTLE_ENDI
  static const number
  /* polynomial I */
/**/ a2             = {{0x0001aa8f, 0xbfe00000} }, /* -0.500... */
/**/ a3             = {{0x55588d2e, 0x3fd55555} }, /*  0.333... */
  /* polynomial II */
/**/ b0             = {{0x55555555, 0x3fd55555} }, /*  0.333... */
/**/ b1             = {{0xffffffbb, 0xbfcfffff} }, /* -0.249... */
/**/ b2             = {{0x9999992f, 0x3fc99999} }, /*  0.199... */
/**/ b3             = {{0x556503fd, 0xbfc55555} }, /* -0.166... */
/**/ b4             = {{0x925b3d62, 0x3fc24924} }, /*  0.142... */
/**/ b5             = {{0x160472fc, 0xbfbffffe} }, /* -0.124... */
/**/ b6             = {{0x25db58ac, 0x3fbc71c5} }, /*  0.111... */
/**/ b7             = {{0x11a2a61c, 0xbfb9a4ac} }, /* -0.100... */
/**/ b8             = {{0x0df2b591, 0x3fb75077} }, /*  0.091... */
  /* polynomial III */
#if 0
/**/ c1             = {{0x00000000, 0x3ff00000} }, /*  1        */
#endif
/**/ c2             = {{0x00000000, 0xbfe00000} }, /* -1/2      */
/**/ c3             = {{0x55555555, 0x3fd55555} }, /*  1/3      */
/**/ c4             = {{0x00000000, 0xbfd00000} }, /* -1/4      */
/**/ c5             = {{0x9999999a, 0x3fc99999} }, /*  1/5      */
  /* polynomial IV */
/**/ d2             = {{0x00000000, 0xbfe00000} }, /* -1/2      */
/**/ dd2            = {{0x00000000, 0x00000000} }, /* -1/2-d2   */
/**/ d3             = {{0x55555555, 0x3fd55555} }, /*  1/3      */
/**/ dd3            = {{0x55555555, 0x3c755555} }, /*  1/3-d3   */
/**/ d4             = {{0x00000000, 0xbfd00000} }, /* -1/4      */
/**/ dd4            = {{0x00000000, 0x00000000} }, /* -1/4-d4   */
/**/ d5             = {{0x9999999a, 0x3fc99999} }, /*  1/5      */
/**/ dd5            = {{0x9999999a, 0xbc699999} }, /*  1/5-d5   */
/**/ d6             = {{0x55555555, 0xbfc55555} }, /* -1/6      */
/**/ dd6            = {{0x55555555, 0xbc655555} }, /* -1/6-d6   */
/**/ d7             = {{0x92492492, 0x3fc24924} }, /*  1/7      */
/**/ dd7            = {{0x92492492, 0x3c624924} }, /*  1/7-d7   */
/**/ d8             = {{0x00000000, 0xbfc00000} }, /* -1/8      */
/**/ dd8            = {{0x00000000, 0x00000000} }, /* -1/8-d8   */
/**/ d9             = {{0x1c71c71c, 0x3fbc71c7} }, /*  1/9      */
/**/ dd9            = {{0x1c71c71c, 0x3c5c71c7} }, /*  1/9-d9   */
/**/ d10            = {{0x9999999a, 0xbfb99999} }, /* -1/10     */
/**/ dd10           = {{0x9999999a, 0x3c599999} }, /* -1/10-d10 */
/**/ d11            = {{0x745d1746, 0x3fb745d1} }, /*  1/11     */
/**/ d12            = {{0x55555555, 0xbfb55555} }, /* -1/12     */
/**/ d13            = {{0x13b13b14, 0x3fb3b13b} }, /*  1/13     */
/**/ d14            = {{0x92492492, 0xbfb24924} }, /* -1/14     */
/**/ d15            = {{0x11111111, 0x3fb11111} }, /*  1/15     */
/**/ d16            = {{0x00000000, 0xbfb00000} }, /* -1/16     */
/**/ d17            = {{0x1e1e1e1e, 0x3fae1e1e} }, /*  1/17     */
/**/ d18            = {{0x1c71c71c, 0xbfac71c7} }, /* -1/18     */
/**/ d19            = {{0xbca1af28, 0x3faaf286} }, /*  1/19     */
/**/ d20            = {{0x9999999a, 0xbfa99999} }, /* -1/20     */
  /* constants    */
/**/ zero           = {{0x00000000, 0x00000000} }, /* 0         */
/**/ one            = {{0x00000000, 0x3ff00000} }, /* 1         */
/**/ half           = {{0x00000000, 0x3fe00000} }, /* 1/2       */
/**/ mhalf          = {{0x00000000, 0xbfe00000} }, /* -1/2      */
/**/ sqrt_2         = {{0x667f3bcc, 0x3ff6a09e} }, /* sqrt(2)   */
/**/ h1             = {{0x00000000, 0x3fd2e000} }, /* 151/2**9  */
/**/ h2             = {{0x00000000, 0x3f669000} }, /* 361/2**17 */
/**/ delu           = {{0x00000000, 0x3f700000} }, /* 1/2**8    */
/**/ delv           = {{0x00000000, 0x3ef00000} }, /* 1/2**16   */
/**/ ln2a           = {{0xfefa3800, 0x3fe62e42} }, /* ln(2) 43 bits */
/**/ ln2b           = {{0x93c76730, 0x3d2ef357} }, /* ln(2)-ln2a    */
/**/ e1             = {{0x00000000, 0x3bbcc868} }, /* 6.095e-21     */
/**/ e2             = {{0x00000000, 0x3c1138ce} }, /* 2.334e-19     */
/**/ e3             = {{0x00000000, 0x3aa1565d} }, /* 2.801e-26     */
/**/ e4             = {{0x00000000, 0x39809d88} }, /* 1.024e-31     */
/**/ e[M]           ={{{0x00000000, 0x37da223a} }, /* 1.2e-39       */
/**/                  {{0x00000000, 0x35c851c4} }, /* 1.3e-49       */
/**/                  {{0x00000000, 0x2ab85e51} }, /* 6.8e-103      */
/**/                  {{0x00000000, 0x17383827} }},/* 8.1e-197      */
/**/ two54          = {{0x00000000, 0x43500000} }, /* 2**54         */
/**/ u03            = {{0xeb851eb8, 0x3f9eb851} }; /* 0.03          */

#endif
#endif

#define  ZERO      zero.d()
#define  ONE       one.d()
#define  HALF      half.d()
#define  MHALF     mhalf.d()
#define  SQRT_2    sqrt_2.d()
#define  DEL_U     delu.d()
#define  DEL_V     delv.d()
#define  LN2A      ln2a.d()
#define  LN2B      ln2b.d()
#define  E1        e1.d()
#define  E2        e2.d()
#define  E3        e3.d()
#define  E4        e4.d()
#define  U03       u03.d()

#endif

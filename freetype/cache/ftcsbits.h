/***************************************************************************/
/*                                                                         */
/*  ftcsbits.h                                                             */
/*                                                                         */
/*    A small-bitmap cache (specification).                                */
/*                                                                         */
/*  Copyright 2000 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTCSBITS_H__
#define __FTCSBITS_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_CACHE_H
#include   FT_CACHE_INTERNAL_CHUNK_H
#include   FT_CACHE_IMAGE_H

FT_BEGIN_HEADER

  /* handle to small bitmap */
  typedef struct FTC_SBitRec_*  FTC_SBit;

  /* handle to small bitmap cache */
  typedef struct FTC_SBit_CacheRec_*  FTC_SBit_Cache;

  /* a compact structure used to hold a single small bitmap */
  typedef struct  FTC_SBitRec_
  {
    FT_Byte   width;
    FT_Byte   height;
    FT_Char   left;
    FT_Char   top;

    FT_Byte   format;
    FT_Char   pitch;
    FT_Char   xadvance;
    FT_Char   yadvance;

    FT_Byte*  buffer;

  } FTC_SBitRec;


  FT_EXPORT( FT_Error )  FTC_SBit_Cache_New( FTC_Manager      manager,
                                             FTC_SBit_Cache  *acache );

  FT_EXPORT( FT_Error )  FTC_SBit_Cache_Lookup( FTC_SBit_Cache   cache,
                                                FTC_Image_Desc*  desc,
                                                FT_UInt          gindex,
                                                FTC_SBit        *sbit );
FT_END_HEADER

#endif /* __FTCSBITS_H__ */


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcimage.c                                                             */
/*                                                                         */
/*    FreeType Image cache (body).                                         */
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


  /*************************************************************************/
  /*                                                                       */
  /* Each image cache really manages FT_Glyph objects.                     */
  /*                                                                       */
  /*************************************************************************/


#ifndef __FTCIMAGE_H__
#define __FTCIMAGE_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_CACHE_H
#include   FT_CACHE_INTERNAL_GLYPH_H

FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       IMAGE CACHE OBJECT                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define  FTC_IMAGE_FORMAT( x )  ( (x) & 7 )


#define  ftc_image_format_bitmap      0
#define  ftc_image_format_outline     1

#define  ftc_image_flag_monochrome   16
#define  ftc_image_flag_unhinted     32
#define  ftc_image_flag_autohinted   64
#define  ftc_image_flag_unscaled    128
#define  ftc_image_flag_no_sbits    256

  /* monochrome bitmap */
#define  ftc_image_mono             ftc_image_format_bitmap | \
                                    ftc_image_flag_monochrome
  /* anti-aliased bitmap */
#define  ftc_image_grays            ftc_image_format_bitmap
  /* scaled outline */
#define  ftc_image_outline          ftc_image_format_outline


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_Image_Desc                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to describe a given glyph image category.  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    size       :: An FTC_SizeRec used to describe the glyph's face &   */
  /*                  size.                                                */
  /*                                                                       */
  /*    image_type :: The glyph image's type.                              */
  /*                                                                       */
  typedef struct  FTC_Image_Desc_
  {
    FTC_FontRec  font;
    FT_UInt      image_type;

  } FTC_Image_Desc;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Image_Cache                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an glyph image cache object.  They are designed to     */
  /*    hold many distinct glyph images, while not exceeding a certain     */
  /*    memory threshold.                                                  */
  /*                                                                       */
  typedef struct FTC_Image_CacheRec_*  FTC_Image_Cache;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Image_Cache_New                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new glyph image cache.                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: The parent manager for the image cache.                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    acache  :: A handle to the new glyph image cache object.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )  FTC_Image_Cache_New( FTC_Manager       manager,
                                              FTC_Image_Cache  *acache );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Image_Cache_Lookup                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves a given glyph image from a glyph image cache.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    cache  :: A handle to the source glyph image cache.                */
  /*                                                                       */
  /*    desc   :: A pointer to a glyph image descriptor.                   */
  /*                                                                       */
  /*    gindex :: The glyph index to retrieve.                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aglyph :: The corresponding FT_Glyph object.  0 in case of         */
  /*              failure.                                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned glyph is owned and managed by the glyph image cache.  */
  /*    Never try to transform or discard it manually!  You can however    */
  /*    create a copy with FT_Glyph_Copy() and modify the new one.         */
  /*                                                                       */
  /*    Because the glyph image cache limits the total amount of memory    */
  /*    taken by the glyphs it holds, the returned glyph might disappear   */
  /*    on a later invocation of this function!  It's a cache after all... */
  /*                                                                       */
  FT_EXPORT( FT_Error )  FTC_Image_Cache_Lookup( FTC_Image_Cache  cache,
                                                 FTC_Image_Desc*  desc,
                                                 FT_UInt          gindex,
                                                 FT_Glyph        *aglyph );


FT_END_HEADER

#endif /* __FTCIMAGE_H__ */


/* END */

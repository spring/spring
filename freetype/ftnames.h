/***************************************************************************/
/*                                                                         */
/*  ftnames.h                                                              */
/*                                                                         */
/*    Simple interface to access SFNT name tables (which are used          */
/*    to hold font names, copyright info, notices, etc.) (specification).  */
/*                                                                         */
/*    This is _not_ used to retrieve glyph names!                          */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTNAMES_H__
#define __FTNAMES_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_FREETYPE_H

FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_SfntName                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model an SFNT `name' table entry.              */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    platform_id :: The platform ID for `string'.                       */
  /*                                                                       */
  /*    encoding_id :: The encoding ID for `string'.                       */
  /*                                                                       */
  /*    language_id :: The language ID for `string'.                       */
  /*                                                                       */
  /*    name_id     :: An identifier for `string'.                         */
  /*                                                                       */
  /*    string      :: The `name' string.  Note that this string is in     */
  /*                   Pascal convention, i.e., the string hasn't a final  */
  /*                   null byte.                                          */
  /*                                                                       */
  /*    string_len  :: The length of `string' in bytes.                    */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Possible values for `platform_id', `encoding_id', `language_id',   */
  /*    and `name_id' are given in the file `ttnameid.h'.  For details     */
  /*    please refer to the TrueType or OpenType specification.            */
  /*                                                                       */
  typedef struct  FT_SfntName_
  {
    FT_UShort  platform_id;
    FT_UShort  encoding_id;
    FT_UShort  language_id;
    FT_UShort  name_id;

    FT_Byte*   string;      /* this string is *not* null-terminated! */
    FT_UInt    string_len;  /* in bytes */

  } FT_SfntName;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Sfnt_Name_Count                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the number of name strings in the SFNT `name' table.     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source face.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The number of strings in the `name' table.                         */
  /*                                                                       */
  FT_EXPORT( FT_UInt )  FT_Get_Sfnt_Name_Count( FT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Sfnt_Name                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves a string of the SFNT `name' table for a given index.     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face  :: A handle to the source face.                              */
  /*                                                                       */
  /*    index :: The index of the `name' string.                           */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aname :: The indexed FT_SfntName structure.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The `string' array returned in the `aname' structure is not        */
  /*    null-terminated.                                                   */
  /*                                                                       */
  /*    Use FT_Get_Sfnt_Name_Count() to get the total number of available  */
  /*    `name' table entries, then do a loop until you get the right       */
  /*    platform, encoding, and name ID.                                   */
  /*                                                                       */
  FT_EXPORT( FT_Error )  FT_Get_Sfnt_Name( FT_Face       face,
                                           FT_UInt       index,
                                           FT_SfntName  *aname );


FT_END_HEADER

#endif /* __FTNAMES_H__ */


/* END */

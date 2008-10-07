/***************************************************************************/
/*                                                                         */
/*  fterrors.h                                                             */
/*                                                                         */
/*    FreeType error codes (specification).                                */
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


  /*************************************************************************/
  /*                                                                       */
  /* This file is used to define the FreeType error enumeration constants. */
  /* It can also be used to create an error message table easily with      */
  /* something like                                                        */
  /*                                                                       */
  /*   {                                                                   */
  /*     #undef FTERRORS_H                                                 */
  /*     #define FT_ERRORDEF( e, v, s )  { e, s },                         */
  /*     #define FT_ERROR_START_LIST  {                                    */
  /*     #define FT_ERROR_END_LIST    { 0, 0 } };                          */
  /*                                                                       */
  /*     const struct                                                      */
  /*     {                                                                 */
  /*       int          err_code;                                          */
  /*       const char*  err_msg                                            */
  /*     } ft_errors[] =                                                   */
  /*                                                                       */
  /*     #include <freetype/fterrors.h>                                    */
  /*   }                                                                   */
  /*                                                                       */
  /* For C++ it might be necessary to use `extern "C" {' and to define     */
  /* FT_NEED_EXTERN_C also.                                                */
  /*                                                                       */
  /*************************************************************************/


#ifndef __FTERRORS_H__
#define __FTERRORS_H__


#undef FT_NEED_EXTERN_C


#ifndef FT_ERRORDEF

#define FT_ERRORDEF( e, v, s )  e = v,
#define FT_ERROR_START_LIST     enum {
#define FT_ERROR_END_LIST       FT_Err_Max };

#ifdef __cplusplus
#define FT_NEED_EXTERN_C
  extern "C" {
#endif

#endif /* !FT_ERRORDEF */


#ifdef FT_ERROR_START_LIST
  FT_ERROR_START_LIST
#endif

  /* generic errors */

  FT_ERRORDEF( FT_Err_Ok,                           0x0000, \
               "no error" )
  FT_ERRORDEF( FT_Err_Cannot_Open_Resource,         0x0001, \
               "cannot open resource" )
  FT_ERRORDEF( FT_Err_Unknown_File_Format,          0x0002, \
               "unknown file format" )
  FT_ERRORDEF( FT_Err_Invalid_File_Format,          0x0003, \
               "broken file" )
  FT_ERRORDEF( FT_Err_Invalid_Version,              0x0004, \
               "invalid FreeType version" )
  FT_ERRORDEF( FT_Err_Lower_Module_Version,         0x0005, \
               "module version is too low" )
  FT_ERRORDEF( FT_Err_Invalid_Argument,             0x0006, \
               "invalid argument" )
  FT_ERRORDEF( FT_Err_Unimplemented_Feature,        0x0007, \
               "unimplemented feature" )

  /* glyph/character errors */

  FT_ERRORDEF( FT_Err_Invalid_Glyph_Index,          0x0010, \
               "invalid glyph index" )
  FT_ERRORDEF( FT_Err_Invalid_Character_Code,       0x0011, \
               "invalid character code" )
  FT_ERRORDEF( FT_Err_Invalid_Glyph_Format,         0x0012, \
               "unsupported glyph image format" )
  FT_ERRORDEF( FT_Err_Cannot_Render_Glyph,          0x0013, \
               "cannot render this glyph format" )
  FT_ERRORDEF( FT_Err_Invalid_Outline,              0x0014, \
               "invalid outline" )
  FT_ERRORDEF( FT_Err_Invalid_Composite,            0x0015, \
               "invalid composite glyph" )
  FT_ERRORDEF( FT_Err_Too_Many_Hints,               0x0016, \
               "too many hints" )
  FT_ERRORDEF( FT_Err_Invalid_Pixel_Size,           0x0017, \
               "invalid pixel size" )

  /* handle errors */

  FT_ERRORDEF( FT_Err_Invalid_Handle,               0x0020, \
               "invalid object handle" )
  FT_ERRORDEF( FT_Err_Invalid_Library_Handle,       0x0021, \
               "invalid library handle" )
  FT_ERRORDEF( FT_Err_Invalid_Driver_Handle,        0x0022, \
               "invalid module handle" )
  FT_ERRORDEF( FT_Err_Invalid_Face_Handle,          0x0023, \
               "invalid face handle" )
  FT_ERRORDEF( FT_Err_Invalid_Size_Handle,          0x0024, \
               "invalid size handle" )
  FT_ERRORDEF( FT_Err_Invalid_Slot_Handle,          0x0025, \
               "invalid glyph slot handle" )
  FT_ERRORDEF( FT_Err_Invalid_CharMap_Handle,       0x0026, \
               "invalid charmap handle" )
  FT_ERRORDEF( FT_Err_Invalid_Cache_Handle,         0x0027, \
               "invalid cache manager handle" )
  FT_ERRORDEF( FT_Err_Invalid_Stream_Handle,        0x0028, \
               "invalid stream handle" )

  /* driver errors */

  FT_ERRORDEF( FT_Err_Too_Many_Drivers,             0x0030, \
               "too many modules" )
  FT_ERRORDEF( FT_Err_Too_Many_Extensions,          0x0031, \
               "too many extensions" )

  /* memory errors */

  FT_ERRORDEF( FT_Err_Out_Of_Memory,                0x0040, \
               "out of memory" )
  FT_ERRORDEF( FT_Err_Unlisted_Object,              0x0041, \
               "unlisted object" )

  /* stream errors */

  FT_ERRORDEF( FT_Err_Cannot_Open_Stream,           0x0051, \
               "cannot open stream" )
  FT_ERRORDEF( FT_Err_Invalid_Stream_Seek,          0x0052, \
               "invalid stream seek" )
  FT_ERRORDEF( FT_Err_Invalid_Stream_Skip,          0x0053, \
               "invalid stream skip" )
  FT_ERRORDEF( FT_Err_Invalid_Stream_Read,          0x0054, \
               "invalid stream read" )
  FT_ERRORDEF( FT_Err_Invalid_Stream_Operation,     0x0055, \
               "invalid stream operation" )
  FT_ERRORDEF( FT_Err_Invalid_Frame_Operation,      0x0056, \
               "invalid frame operation" )
  FT_ERRORDEF( FT_Err_Nested_Frame_Access,          0x0057, \
               "nested frame access" )
  FT_ERRORDEF( FT_Err_Invalid_Frame_Read,           0x0058, \
               "invalid frame read" )

  /* raster errors */

  FT_ERRORDEF( FT_Err_Raster_Uninitialized,         0x0060, \
               "raster uninitialized" )
  FT_ERRORDEF( FT_Err_Raster_Corrupted,             0x0061, \
               "raster corrupted" )
  FT_ERRORDEF( FT_Err_Raster_Overflow,              0x0062, \
               "raster overflow" )
  FT_ERRORDEF( FT_Err_Raster_Negative_Height,       0x0063, \
               "negative height while rastering" )

  /* cache errors */

  FT_ERRORDEF( FT_Err_Too_Many_Caches,              0x0070, \
               "too many registered caches" )

  /* range 0x400 - 0x4FF is reserved for TrueType specific stuff */

  /* range 0x500 - 0x5FF is reserved for CFF specific stuff */

  /* range 0x600 - 0x6FF is reserved for Type1 specific stuff */

#ifdef FT_ERROR_END_LIST
  FT_ERROR_END_LIST
#endif


#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST
#undef FT_ERRORDEF


#ifdef FT_NEED_EXTERN_C
  }
#endif

#endif /* __FTERRORS_H__ */


/* END */

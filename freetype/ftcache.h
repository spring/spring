/***************************************************************************/
/*                                                                         */
/*  ftcache.h                                                              */
/*                                                                         */
/*    FreeType Cache subsystem.                                            */
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
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*********                                                       *********/
  /*********             WARNING, THIS IS BETA CODE.               *********/
  /*********                                                       *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#ifndef __FTCACHE_H__
#define __FTCACHE_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_GLYPH_H

FT_BEGIN_HEADER

#define  FT_CACHE_MANAGER_H               FT_PUBLIC_FILE(cache/ftcmanag.h)
#define  FT_CACHE_IMAGE_H                 FT_PUBLIC_FILE(cache/ftcimage.h)
#define  FT_CACHE_SMALL_BITMAPS_H         FT_PUBLIC_FILE(cache/ftcsbits.h)

#define  FT_CACHE_INTERNAL_LRU_H          FT_PUBLIC_FILE(cache/ftlru.h)
#define  FT_CACHE_INTERNAL_GLYPH_H        FT_PUBLIC_FILE(cache/ftcglyph.h)
#define  FT_CACHE_INTERNAL_CHUNK_H        FT_PUBLIC_FILE(cache/ftcchunk.h)


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    BASIC TYPE DEFINITIONS                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_FaceID                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A generic pointer type that is used to identity face objects.  The */
  /*    contents of such objects is application-dependent.                 */
  /*                                                                       */
  typedef FT_Pointer  FTC_FaceID;


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FTC_Face_Requester                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A callback function provided by client applications.  It is used   */
  /*    to translate a given FTC_FaceID into a new valid FT_Face object.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face_id :: The face ID to resolve.                                 */
  /*                                                                       */
  /*    library :: A handle to a FreeType library object.                  */
  /*                                                                       */
  /*    data    :: Application-provided request data.                      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A new FT_Face handle.                                   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The face requester should not perform funny things on the returned */
  /*    face object, like creating a new FT_Size for it, or setting a      */
  /*    transformation through FT_Set_Transform()!                         */
  /*                                                                       */
  typedef FT_Error  (*FTC_Face_Requester)( FTC_FaceID  face_id,
                                           FT_Library  library,
                                           FT_Pointer  request_data,
                                           FT_Face*    aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_FontRec                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to describe a given `font' to the cache    */
  /*    manager.  Note that a `font' is the combination of a given face    */
  /*    with a given character size.                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face_id    :: The ID of the face to use.                           */
  /*                                                                       */
  /*    pix_width  :: The character width in integer pixels.               */
  /*                                                                       */
  /*    pix_height :: The character height in integer pixels.              */
  /*                                                                       */
  typedef struct  FTC_FontRec_
  {
    FTC_FaceID  face_id;
    FT_UShort   pix_width;
    FT_UShort   pix_height;

  } FTC_FontRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Font                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple handle to a FTC_FontRec structure.                        */
  /*                                                                       */
  typedef FTC_FontRec*  FTC_Font;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      CACHE MANAGER OBJECT                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Manager                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This object is used to cache one or more FT_Face objects, along    */
  /*    with corresponding FT_Size objects.                                */
  /*                                                                       */
  typedef struct FTC_ManagerRec_*  FTC_Manager;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_New                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new cache manager.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library   :: The parent FreeType library handle to use.            */
  /*                                                                       */
  /*    max_faces :: Maximum number of faces to keep alive in manager.     */
  /*                 Use 0 for defaults.                                   */
  /*                                                                       */
  /*    max_sizes :: Maximum number of sizes to keep alive in manager.     */
  /*                 Use 0 for defaults.                                   */
  /*                                                                       */
  /*    max_bytes :: Maximum number of bytes to use for cached data.       */
  /*                 Use 0 for defaults.                                   */
  /*                                                                       */
  /*    requester :: An application-provided callback used to translate    */
  /*                 face IDs into real FT_Face objects.                   */
  /*                                                                       */
  /*    req_data  :: A generic pointer that is passed to the requester     */
  /*                 each time it is called (see FTC_Face_Requester)       */
  /*                                                                       */
  /* <Output>                                                              */
  /*    amanager  :: A handle to a new manager object.  0 in case of       */
  /*                 failure.                                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )  FTC_Manager_New( FT_Library          library,
                                          FT_UInt             max_faces,
                                          FT_UInt             max_sizes,
                                          FT_ULong            max_bytes,
                                          FTC_Face_Requester  requester,
                                          FT_Pointer          req_data,
                                          FTC_Manager        *amanager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Reset                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Empties a given cache manager.  This simply gets rid of all the    */
  /*    currently cached FT_Face & FT_Size objects within the manager.     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    manager :: A handle to the manager.                                */
  /*                                                                       */
  FT_EXPORT( void )  FTC_Manager_Reset( FTC_Manager  manager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Done                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given manager after emptying it.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the target cache manager object.            */
  /*                                                                       */
  FT_EXPORT( void )  FTC_Manager_Done( FTC_Manager  manager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Lookup_Face                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the FT_Face object that corresponds to a given face ID   */
  /*    through a cache manager.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /*    face_id :: The ID of the face object.                              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A handle to the face object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned FT_Face object is always owned by the manager.  You   */
  /*    should never try to discard it yourself.                           */
  /*                                                                       */
  /*    The FT_Face object doesn't necessarily have a current size object  */
  /*    (i.e., face->size can be 0).  If you need a specific `font size',  */
  /*    use FTC_Manager_Lookup_Size() instead.                             */
  /*                                                                       */
  /*    Never change the face's transformation matrix (i.e., never call    */
  /*    the FT_Set_Transform() function) on a returned face!  If you need  */
  /*    to transform glyphs, do it yourself after glyph loading.           */
  /*                                                                       */
  FT_EXPORT( FT_Error )  FTC_Manager_Lookup_Face( FTC_Manager  manager,
                                                  FTC_FaceID   face_id,
                                                  FT_Face     *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Lookup_Size                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the FT_Face & FT_Size objects that correspond to a given */
  /*    FTC_SizeID.                                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /*    size_id :: The ID of the `font size' to use.                       */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A pointer to the handle of the face object.  Set it to  */
  /*               zero if you don't need it.                              */
  /*                                                                       */
  /*    asize   :: A pointer to the handle of the size object.  Set it to  */
  /*               zero if you don't need it.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned FT_Face object is always owned by the manager.  You   */
  /*    should never try to discard it yourself.                           */
  /*                                                                       */
  /*    Never change the face's transformation matrix (i.e., never call    */
  /*    the FT_Set_Transform() function) on a returned face!  If you need  */
  /*    to transform glyphs, do it yourself after glyph loading.           */
  /*                                                                       */
  /*    Similarly, the returned FT_Size object is always owned by the      */
  /*    manager.  You should never try to discard it, and never change its */
  /*    settings with FT_Set_Pixel_Sizes() or FT_Set_Char_Size()!          */
  /*                                                                       */
  /*    The returned size object is the face's current size, which means   */
  /*    that you can call FT_Load_Glyph() with the face if you need to.    */
  /*                                                                       */
  FT_EXPORT( FT_Error )  FTC_Manager_Lookup_Size( FTC_Manager  manager,
                                                  FTC_Font     font,
                                                  FT_Face     *aface,
                                                  FT_Size     *asize );


  /* a cache class is used to describe a unique cache type to the manager */
  typedef struct FTC_Cache_Class_  FTC_Cache_Class;
  typedef struct FTC_CacheRec_*    FTC_Cache;


  /* this must be used internally for the moment */
  FT_EXPORT( FT_Error )  FTC_Manager_Register_Cache(
                           FTC_Manager       manager,
                           FTC_Cache_Class*  clazz,
                           FTC_Cache        *acache );


FT_END_HEADER

#endif /* __FTCACHE_H__ */


/* END */

/***************************************************************************/
/*                                                                         */
/*  ftcglyph.h                                                             */
/*                                                                         */
/*    FreeType abstract glyph cache (specification).                       */
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
  /* Important: The functions defined in this file are only used to        */
  /*            implement an abstract glyph cache class.  You need to      */
  /*            provide additional logic to implement a complete cache.    */
  /*            For example, see `ftcimage.h' and `ftcimage.c' which       */
  /*            implement a FT_Glyph cache based on this code.             */
  /*                                                                       */
  /*  NOTE: For now, each glyph set is implemented as a static hash table. */
  /*        It would be interesting to experiment with dynamic hashes to   */
  /*        see whether this improves performance or not (I don't know why */
  /*        but something tells me it won't).                              */
  /*                                                                       */
  /*        In all cases, this change should not affect any derived glyph  */
  /*        cache class.                                                   */
  /*                                                                       */
  /*************************************************************************/


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


#ifndef __FTCGLYPH_H__
#define __FTCGLYPH_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_CACHE_H
#include   FT_CACHE_MANAGER_H
#include   <stddef.h>

FT_BEGIN_HEADER


  /* maximum number of glyph sets per glyph cache; must be < 256 */
#define FTC_MAX_GLYPH_SETS          16
#define FTC_GSET_HASH_SIZE_DEFAULT  64


  typedef struct FTC_GlyphSetRec_*     FTC_GlyphSet;
  typedef struct FTC_GlyphNodeRec_*    FTC_GlyphNode;
  typedef struct FTC_Glyph_CacheRec_*  FTC_Glyph_Cache;

  typedef struct  FTC_GlyphNodeRec_
  {
    FTC_CacheNodeRec  root;
    FTC_GlyphNode     gset_next;   /* next in glyph set's bucket list */
    FT_UShort         glyph_index;
    FT_UShort         gset_index;

  } FTC_GlyphNodeRec;


#define FTC_GLYPHNODE( x )             ( (FTC_GlyphNode)( x ) )
#define FTC_GLYPHNODE_TO_LRUNODE( n )  ( (FT_ListNode)( n ) )
#define FTC_LRUNODE_TO_GLYPHNODE( n )  ( (FTC_GlyphNode)( n ) )


  /*************************************************************************/
  /*                                                                       */
  /* Glyph set methods.                                                    */
  /*                                                                       */

  typedef FT_Error  (*FTC_GlyphSet_InitFunc)       ( FTC_GlyphSet    gset,
                                                     FT_Pointer      type );

  typedef void      (*FTC_GlyphSet_DoneFunc)       ( FTC_GlyphSet    gset );

  typedef FT_Bool   (*FTC_GlyphSet_CompareFunc)    ( FTC_GlyphSet    gset,
                                                     FT_Pointer      type );


  typedef FT_Error  (*FTC_GlyphSet_NewNodeFunc)    ( FTC_GlyphSet    gset,
                                                     FT_UInt         gindex,
                                                     FTC_GlyphNode*  anode );

  typedef void      (*FTC_GlyphSet_DestroyNodeFunc)( FTC_GlyphNode   node,
                                                     FTC_GlyphSet    gset );

  typedef FT_ULong  (*FTC_GlyphSet_SizeNodeFunc)   ( FTC_GlyphNode   node,
                                                     FTC_GlyphSet    gset );


  typedef struct  FTC_GlyphSet_Class_
  {
    FT_UInt                       gset_byte_size;

    FTC_GlyphSet_InitFunc         init;
    FTC_GlyphSet_DoneFunc         done;
    FTC_GlyphSet_CompareFunc      compare;

    FTC_GlyphSet_NewNodeFunc      new_node;
    FTC_GlyphSet_SizeNodeFunc     size_node;
    FTC_GlyphSet_DestroyNodeFunc  destroy_node;

  } FTC_GlyphSet_Class;


  typedef struct  FTC_GlyphSetRec_
  {
    FTC_Glyph_Cache      cache;
    FTC_Manager          manager;
    FT_Memory            memory;
    FTC_GlyphSet_Class*  clazz;
    FT_UInt              hash_size;
    FTC_GlyphNode*       buckets;
    FT_UInt              gset_index;  /* index in parent cache    */

  } FTC_GlyphSetRec;


  /* the abstract glyph cache class */
  typedef struct  FTC_Glyph_Cache_Class_
  {
    FTC_Cache_Class      root;
    FTC_GlyphSet_Class*  gset_class;

  } FTC_Glyph_Cache_Class;


  /* the abstract glyph cache object */
  typedef struct  FTC_Glyph_CacheRec_
  {
    FTC_CacheRec              root;
    FT_Lru                    gsets_lru;    /* static sets lru list */
    FTC_GlyphSet              last_gset;    /* small cache :-)      */
    FTC_GlyphSet_CompareFunc  compare;      /* useful shortcut      */

  } FTC_Glyph_CacheRec;


  /*************************************************************************/
  /*                                                                       */
  /* These functions are exported so that they can be called from          */
  /* user-provided cache classes; otherwise, they are really part of the   */
  /* cache sub-system internals.                                           */
  /*                                                                       */

  FT_EXPORT( void )  FTC_GlyphNode_Init( FTC_GlyphNode  node,
                                         FTC_GlyphSet   gset,
                                         FT_UInt        gindex );

#define FTC_GlyphNode_Ref( n ) \
          FTC_CACHENODE_TO_DATA_P( &(n)->root )->ref_count++

#define FTC_GlyphNode_Unref( n ) \
          FTC_CACHENODE_TO_DATA_P( &(n)->root )->ref_count--


  FT_EXPORT( void )      FTC_GlyphNode_Destroy( FTC_GlyphNode    node,
                                                FTC_Glyph_Cache  cache );

  FT_EXPORT( FT_Error )  FTC_Glyph_Cache_Init(  FTC_Glyph_Cache  cache );

  FT_EXPORT( void )      FTC_Glyph_Cache_Done(  FTC_Glyph_Cache  cache );


  FT_EXPORT( FT_Error )  FTC_GlyphSet_New( FTC_Glyph_Cache  cache,
                                           FT_Pointer       type,
                                           FTC_GlyphSet    *aset );

  FT_EXPORT( FT_Error )  FTC_GlyphSet_Lookup_Node(
                           FTC_GlyphSet    gset,
                           FT_UInt         glyph_index,
                           FTC_GlyphNode  *anode );

  FT_EXPORT( FT_Error )  FTC_Glyph_Cache_Lookup( FTC_Glyph_Cache  cache,
                                                 FT_Pointer       type,
                                                 FT_UInt          gindex,
                                                 FTC_GlyphNode   *anode );

FT_END_HEADER

#endif /* __FTCGLYPH_H__ */


/* END */

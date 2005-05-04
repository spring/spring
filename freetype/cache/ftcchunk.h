/***************************************************************************/
/*                                                                         */
/*  ftcchunk.h                                                             */
/*                                                                         */
/*    FreeType chunk cache (specification).                                */
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
  /*            implement an abstract chunk cache class.  You need to      */
  /*            provide additional logic to implement a complete cache.    */
  /*            For example, see `ftcmetrx.h' and `ftcmetrx.c' which       */
  /*            implement a glyph metrics cache based on this code.        */
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


#ifndef __FTCCHUNK_H__
#define __FTCCHUNK_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_CACHE_H
#include   FT_CACHE_MANAGER_H

FT_BEGIN_HEADER


  /* maximum number of chunk sets in a given chunk cache */
#define  FTC_MAX_CHUNK_SETS  16


  typedef struct FTC_ChunkNodeRec_*    FTC_ChunkNode;
  typedef struct FTC_ChunkSetRec_*     FTC_ChunkSet;
  typedef struct FTC_Chunk_CacheRec_*  FTC_Chunk_Cache;

  typedef struct  FTC_ChunkNodeRec_
  {
    FTC_CacheNodeRec  root;
    FTC_ChunkSet      cset;
    FT_UShort         cset_index;
    FT_UShort         num_elements;
    FT_Byte*          elements;

  } FTC_ChunkNodeRec;


#define FTC_CHUNKNODE_TO_LRUNODE( x )  ((FT_ListNode)( x ))
#define FTC_LRUNODE_TO_CHUNKNODE( x )  ((FTC_ChunkNode)( x ))


  /*************************************************************************/
  /*                                                                       */
  /*  chunk set methods                                                    */
  /*                                                                       */

  /* used to set "element_max", "element_count" and "element_size" */
  typedef FT_Error  (*FTC_ChunkSet_SizesFunc)  ( FTC_ChunkSet  cset,
                                                 FT_Pointer    type );

  typedef FT_Error  (*FTC_ChunkSet_InitFunc)   ( FTC_ChunkSet  cset,
                                                 FT_Pointer    type );

  typedef void      (*FTC_ChunkSet_DoneFunc)   ( FTC_ChunkSet  cset );

  typedef FT_Bool   (*FTC_ChunkSet_CompareFunc)( FTC_ChunkSet  cset,
                                                 FT_Pointer    type );


  typedef FT_Error  (*FTC_ChunkSet_NewNodeFunc)    ( FTC_ChunkSet    cset,
                                                     FT_UInt         index,
                                                     FTC_ChunkNode*  anode );

  typedef void      (*FTC_ChunkSet_DestroyNodeFunc)( FTC_ChunkNode   node );

  typedef FT_ULong  (*FTC_ChunkSet_SizeNodeFunc)   ( FTC_ChunkNode   node );


  typedef struct  FTC_ChunkSet_Class_
  {
    FT_UInt                       cset_byte_size;

    FTC_ChunkSet_InitFunc         init;
    FTC_ChunkSet_DoneFunc         done;
    FTC_ChunkSet_CompareFunc      compare;
    FTC_ChunkSet_SizesFunc        sizes;

    FTC_ChunkSet_NewNodeFunc      new_node;
    FTC_ChunkSet_SizeNodeFunc     size_node;
    FTC_ChunkSet_DestroyNodeFunc  destroy_node;

  } FTC_ChunkSet_Class;


  typedef struct  FTC_ChunkSetRec_
  {
    FTC_Chunk_Cache      cache;
    FTC_Manager          manager;
    FT_Memory            memory;
    FTC_ChunkSet_Class*  clazz;
    FT_UInt              cset_index;     /* index in parent cache    */

    FT_UInt              element_max;    /* maximum number of elements   */
    FT_UInt              element_size;   /* element size in bytes        */
    FT_UInt              element_count;  /* number of elements per chunk */

    FT_UInt              num_chunks;
    FTC_ChunkNode*       chunks;

  } FTC_ChunkSetRec;


  /* the abstract chunk cache class */
  typedef struct  FTC_Chunk_Cache_Class_
  {
    FTC_Cache_Class      root;
    FTC_ChunkSet_Class*  cset_class;

  } FTC_Chunk_Cache_Class;


  /* the abstract chunk cache object */
  typedef struct  FTC_Chunk_CacheRec_
  {
    FTC_CacheRec              root;
    FT_Lru                    csets_lru;   /* static chunk set lru list */
    FTC_ChunkSet              last_cset;   /* small cache :-)           */
    FTC_ChunkSet_CompareFunc  compare;     /* useful shortcut           */

  } FTC_Chunk_CacheRec;


  /*************************************************************************/
  /*                                                                       */
  /* These functions are exported so that they can be called from          */
  /* user-provided cache classes; otherwise, they are really part of the   */
  /* cache sub-system internals.                                           */
  /*                                                                       */

  FT_EXPORT( FT_Error )  FTC_ChunkNode_Init( FTC_ChunkNode  node,
                                             FTC_ChunkSet   cset,
                                             FT_UInt        index,
                                             FT_Bool        alloc );

#define FTC_ChunkNode_Ref( n ) \
          FTC_CACHENODE_TO_DATA_P( &(n)->root )->ref_count++

#define FTC_ChunkNode_Unref( n ) \
          FTC_CACHENODE_TO_DATA_P( &(n)->root )->ref_count--


  /* chunk set objects */

  FT_EXPORT( void )      FTC_ChunkNode_Destroy( FTC_ChunkNode    node );


  FT_EXPORT( FT_Error )  FTC_ChunkSet_New     ( FTC_Chunk_Cache  cache,
                                                FT_Pointer       type,
                                                FTC_ChunkSet    *aset );


  FT_EXPORT( FT_Error )  FTC_ChunkSet_Lookup_Node(
                           FTC_ChunkSet    cset,
                           FT_UInt         glyph_index,
                           FTC_ChunkNode*  anode,
                           FT_UInt        *anindex );


  /* chunk cache objects */

  FT_EXPORT( FT_Error )  FTC_Chunk_Cache_Init  ( FTC_Chunk_Cache  cache );

  FT_EXPORT( void )      FTC_Chunk_Cache_Done  ( FTC_Chunk_Cache  cache );

  FT_EXPORT( FT_Error )  FTC_Chunk_Cache_Lookup( FTC_Chunk_Cache  cache,
                                                 FT_Pointer       type,
                                                 FT_UInt          gindex,
                                                 FTC_ChunkNode   *anode,
                                                 FT_UInt         *aindex );


FT_END_HEADER

#endif /* __FTCCHUNK_H__ */


/* END */

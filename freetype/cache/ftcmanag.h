/***************************************************************************/
/*                                                                         */
/*  ftcmanag.h                                                             */
/*                                                                         */
/*    FreeType Cache Manager (specification).                              */
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
  /* A cache manager is in charge of the following:                        */
  /*                                                                       */
  /*  - Maintain a mapping between generic FTC_FaceIDs and live FT_Face    */
  /*    objects.  The mapping itself is performed through a user-provided  */
  /*    callback.  However, the manager maintains a small cache of FT_Face */
  /*    & FT_Size objects in order to speed up things considerably.        */
  /*                                                                       */
  /*  - Manage one or more cache objects.  Each cache is in charge of      */
  /*    holding a varying number of `cache nodes'.  Each cache node        */
  /*    represents a minimal amount of individually accessible cached      */
  /*    data.  For example, a cache node can be an FT_Glyph image          */
  /*    containing a vector outline, or some glyph metrics, or anything    */
  /*    else.                                                              */
  /*                                                                       */
  /*    Each cache node has a certain size in bytes that is added to the   */
  /*    total amount of `cache memory' within the manager.                 */
  /*                                                                       */
  /*    All cache nodes are located in a global LRU list, where the oldest */
  /*    node is at the tail of the list.                                   */
  /*                                                                       */
  /*    Each node belongs to a single cache, and includes a reference      */
  /*    count to avoid destroying it (due to caching).                     */
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


#ifndef __FTCMANAG_H__
#define __FTCMANAG_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_CACHE_H
#include   FT_CACHE_INTERNAL_LRU_H

FT_BEGIN_HEADER

#define FTC_MAX_FACES_DEFAULT  2
#define FTC_MAX_SIZES_DEFAULT  4
#define FTC_MAX_BYTES_DEFAULT  200000  /* 200kByte by default! */

  /* maximum number of caches registered in a single manager */
#define FTC_MAX_CACHES         16


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_ManagerRec                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The cache manager structure.                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    library      :: A handle to a FreeType library instance.           */
  /*                                                                       */
  /*    faces_lru    :: The lru list of FT_Face objects in the cache.      */
  /*                                                                       */
  /*    sizes_lru    :: The lru list of FT_Size objects in the cache.      */
  /*                                                                       */
  /*    max_bytes    :: The maximum number of bytes to be allocated in the */
  /*                    cache.  This is only related to the byte size of   */
  /*                    the nodes cached by the manager.                   */
  /*                                                                       */
  /*    num_bytes    :: The current number of bytes allocated in the       */
  /*                    cache.  Only related to the byte size of cached    */
  /*                    nodes.                                             */
  /*                                                                       */
  /*    num_nodes    :: The current number of nodes in the manager.        */
  /*                                                                       */
  /*    global_lru   :: The global lru list of all cache nodes.            */
  /*                                                                       */
  /*    caches       :: A table of installed/registered cache objects.     */
  /*                                                                       */
  /*    request_data :: User-provided data passed to the requester.        */
  /*                                                                       */
  /*    request_face :: User-provided function used to implement a mapping */
  /*                    between abstract FTC_FaceIDs and real FT_Face      */
  /*                    objects.                                           */
  /*                                                                       */
  typedef struct  FTC_ManagerRec_
  {
    FT_Library          library;
    FT_Lru              faces_lru;
    FT_Lru              sizes_lru;

    FT_ULong            max_bytes;
    FT_ULong            num_bytes;
    FT_UInt             num_nodes;
    FT_ListRec          global_lru;
    FTC_Cache           caches[FTC_MAX_CACHES];

    FT_Pointer          request_data;
    FTC_Face_Requester  request_face;

  } FTC_ManagerRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Compress                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to check the state of the cache manager if   */
  /*    its `num_bytes' field is greater than its `max_bytes' field.  It   */
  /*    will flush as many old cache nodes as possible (ignoring cache     */
  /*    nodes with a non-zero reference count).                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Client applications should not call this function directly.  It is */
  /*    normally invoked by specific cache implementations.                */
  /*                                                                       */
  /*    The reason this function is exported is to allow client-specific   */
  /*    cache classes.                                                     */
  /*                                                                       */
  FT_EXPORT( void )  FTC_Manager_Compress( FTC_Manager  manager );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE NODE DEFINITIONS                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* Each cache controls one or more cache nodes.  Each node is part of    */
  /* the global_lru list of the manager.  Its `data' field however is used */
  /* as a reference count for now.                                         */
  /*                                                                       */
  /* A node can be anything, depending on the type of information held by  */
  /* the cache.  It can be an individual glyph image, a set of bitmaps     */
  /* glyphs for a given size, some metrics, etc.                           */
  /*                                                                       */

  typedef FT_ListNodeRec     FTC_CacheNodeRec;
  typedef FTC_CacheNodeRec*  FTC_CacheNode;


  /* the field `cachenode.data' is typecast to this type */
  typedef struct  FTC_CacheNode_Data_
  {
    FT_UShort  cache_index;
    FT_Short   ref_count;

  } FTC_CacheNode_Data;


  /* return a pointer to FTC_CacheNode_Data contained in a */
  /* CacheNode's `data' field                              */
#define FTC_CACHENODE_TO_DATA_P( n ) \
          ( (FTC_CacheNode_Data*)&(n)->data )

#define FTC_LIST_TO_CACHENODE( n )  ( (FTC_CacheNode)(n) )


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FTC_CacheNode_SizeFunc                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to compute the total size in bytes of a given      */
  /*    cache node.  It is used by the cache manager to compute the number */
  /*    of old nodes to flush when the cache is full.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    node       :: A handle to the target cache node.                   */
  /*                                                                       */
  /*    cache_data :: A generic pointer passed to the destructor.          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The size of a given cache node in bytes.                           */
  /*                                                                       */
  typedef FT_ULong  (*FTC_CacheNode_SizeFunc)( FTC_CacheNode  node,
                                               FT_Pointer     cache_data );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FTC_CacheNode_DestroyFunc                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to destroy a given cache node.  It is called by    */
  /*    the manager when the cache is full and old nodes need to be        */
  /*    flushed out.                                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    node       :: A handle to the target cache node.                   */
  /*                                                                       */
  /*    cache_data :: A generic pointer passed to the destructor.          */
  /*                                                                       */
  typedef void  (*FTC_CacheNode_DestroyFunc)( FTC_CacheNode  node,
                                              FT_Pointer     cache_data );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_CacheNode_Class                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple structure used to describe a cache node's class to   */
  /*    the cache manager.                                                 */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    size_node    :: A function used to size the node.                  */
  /*                                                                       */
  /*    destroy_node :: A function used to destroy the node.               */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The cache node class doesn't include a `new_node' function because */
  /*    the cache manager never allocates cache node directly; it          */
  /*    delegates this task to its cache objects.                          */
  /*                                                                       */
  typedef struct  FTC_CacheNode_Class_
  {
    FTC_CacheNode_SizeFunc     size_node;
    FTC_CacheNode_DestroyFunc  destroy_node;

  } FTC_CacheNode_Class;


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       CACHE DEFINITIONS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FTC_Cache_InitFunc                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to initialize a given cache object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    cache :: A handle to the new cache.                                */
  /*                                                                       */
  typedef FT_Error  (*FTC_Cache_InitFunc)( FTC_Cache  cache );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FTC_Cache_DoneFunc                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function to finalize a given cache object.                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    cache :: A handle to the target cache.                             */
  /*                                                                       */
  typedef void  (*FTC_Cache_DoneFunc)( FTC_Cache  cache );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_Cache_Class                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to describe a given cache object class to the     */
  /*    cache manager.                                                     */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    cache_byte_size :: The size of the cache object in bytes.          */
  /*                                                                       */
  /*    init_cache      :: The cache object initializer.                   */
  /*                                                                       */
  /*    done_cache      :: The cache object finalizer.                     */
  /*                                                                       */
  struct  FTC_Cache_Class_
  {
    FT_UInt             cache_byte_size;
    FTC_Cache_InitFunc  init_cache;
    FTC_Cache_DoneFunc  done_cache;
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_CacheRec                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to describe an abstract cache object.             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    manager     :: A handle to the parent cache manager.               */
  /*                                                                       */
  /*    memory      :: A handle to the memory manager.                     */
  /*                                                                       */
  /*    clazz       :: A pointer to the cache class.                       */
  /*                                                                       */
  /*    node_clazz  :: A pointer to the cache's node class.                */
  /*                                                                       */
  /*    cache_index :: An index of the cache in the manager's table.       */
  /*                                                                       */
  /*    cache_data  :: Data passed to the cache node                       */
  /*                   constructor/finalizer.                              */
  /*                                                                       */
  typedef struct  FTC_CacheRec_
  {
    FTC_Manager           manager;
    FT_Memory             memory;
    FTC_Cache_Class*      clazz;
    FTC_CacheNode_Class*  node_clazz;

    FT_UInt               cache_index;  /* in manager's table           */
    FT_Pointer            cache_data;   /* passed to cache node methods */

  } FTC_CacheRec;

  /* */

FT_END_HEADER

#endif /* __FTCMANAG_H__ */


/* END */

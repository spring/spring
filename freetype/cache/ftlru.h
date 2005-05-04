/***************************************************************************/
/*                                                                         */
/*  ftlru.h                                                                */
/*                                                                         */
/*    Simple LRU list-cache (specification).                               */
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
  /* An LRU is a list that cannot hold more than a certain number of       */
  /* elements (`max_elements').  All elements on the list are sorted in    */
  /* least-recently-used order, i.e., the `oldest' element is at the tail  */
  /* of the list.                                                          */
  /*                                                                       */
  /* When doing a lookup (either through `Lookup()' or `Lookup_Node()'),   */
  /* the list is searched for an element with the corresponding key.  If   */
  /* it is found, the element is moved to the head of the list and is      */
  /* returned.                                                             */
  /*                                                                       */
  /* If no corresponding element is found, the lookup routine will try to  */
  /* obtain a new element with the relevant key.  If the list is already   */
  /* full, the oldest element from the list is discarded and replaced by a */
  /* new one; a new element is added to the list otherwise.                */
  /*                                                                       */
  /* Note that it is possible to pre-allocate the element list nodes.      */
  /* This is handy if `max_elements' is sufficiently small, as it saves    */
  /* allocations/releases during the lookup process.                       */
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


#ifndef __FTLRU_H__
#define __FTLRU_H__

#ifndef    FT_BUILD_H
#  define  FT_BUILD_H    <freetype/config/ftbuild.h>
#endif
#include   FT_BUILD_H
#include   FT_FREETYPE_H

FT_BEGIN_HEADER


  /* generic key type */
  typedef FT_Pointer  FT_LruKey;


  /* an lru node -- root.data points to the element */
  typedef struct  FT_LruNodeRec_
  {
    FT_ListNodeRec  root;
    FT_LruKey       key;

  } FT_LruNodeRec, *FT_LruNode;


  /* forward declaration */
  typedef struct FT_LruRec_*  FT_Lru;


  /* LRU class */
  typedef struct  FT_Lru_Class_
  {
    FT_UInt  lru_size;      /* object size in bytes */

    /* this method is used to initialize a new list element node */
    FT_Error  (*init_element)( FT_Lru      lru,
                               FT_LruNode  node );

    /* this method is used to finalize a given list element node */
    void      (*done_element)( FT_Lru      lru,
                               FT_LruNode  node );

    /* If defined, this method is called when the list if full        */
    /* during the lookup process -- it is used to change the contents */
    /* of a list element node, instead of calling `done_element()',   */
    /* then `init_element'.  Set it to 0 for default behaviour.       */
    FT_Error  (*flush_element)( FT_Lru      lru,
                                FT_LruNode  node,
                                FT_LruKey   new_key );

    /* If defined, this method is used to compare a list element node */
    /* with a given key during a lookup.  If set to 0, the `key'      */
    /* fields will be directly compared instead.                      */
    FT_Bool  (*compare_element)( FT_LruNode  node,
                                 FT_LruKey   key );

  } FT_Lru_Class;


  /* A selector is used to indicate whether a given list element node    */
  /* is part of a selection for FT_Lru_Remove_Selection().  The function */
  /* must return true (i.e., non-null) to indicate that the node is part */
  /* of it.                                                              */
  typedef FT_Bool  (*FT_Lru_Selector)( FT_Lru      lru,
                                       FT_LruNode  node,
                                       FT_Pointer  data );


  typedef struct  FT_LruRec_
  {
    FT_Lru_Class*  clazz;
    FT_UInt        max_elements;
    FT_UInt        num_elements;
    FT_ListRec     elements;
    FT_Memory      memory;
    FT_Pointer     user_data;

    /* the following fields are only meaningful for static lru containers */
    FT_ListRec     free_nodes;
    FT_LruNode     nodes;

  } FT_LruRec;


  FT_EXPORT( FT_Error )  FT_Lru_New( const FT_Lru_Class*  clazz,
                                     FT_UInt              max_elements,
                                     FT_Pointer           user_data,
                                     FT_Memory            memory,
                                     FT_Bool              pre_alloc,
                                     FT_Lru              *anlru );

  FT_EXPORT( void )      FT_Lru_Reset( FT_Lru  lru );

  FT_EXPORT( void )      FT_Lru_Done ( FT_Lru  lru );

  FT_EXPORT( FT_Error )  FT_Lru_Lookup_Node( FT_Lru        lru,
                                             FT_LruKey     key,
                                             FT_LruNode   *anode );

  FT_EXPORT( FT_Error )  FT_Lru_Lookup( FT_Lru       lru,
                                        FT_LruKey    key,
                                        FT_Pointer  *anobject );

  FT_EXPORT( void )      FT_Lru_Remove_Node( FT_Lru      lru,
                                             FT_LruNode  node );
		      
  FT_EXPORT( void )      FT_Lru_Remove_Selection( FT_Lru           lru,
                                                  FT_Lru_Selector  selector,
                                                  FT_Pointer       data );

FT_END_HEADER

#endif /* __FTLRU_H__ */


/* END */

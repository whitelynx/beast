/* BEAST - Better Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BST_TREE_STORES_H__
#define __BST_TREE_STORES_H__

#include "bstutils.h"

G_BEGIN_DECLS


/* --- file store --- */
enum {
  BST_FILE_STORE_COL_ID,
  BST_FILE_STORE_COL_FILE,
  BST_FILE_STORE_COL_BASE_NAME,
  BST_FILE_STORE_COL_WAVE_NAME,
  BST_FILE_STORE_COL_SIZE,
  BST_FILE_STORE_COL_TIME_USECS,
  BST_FILE_STORE_COL_TIME_STR,
  BST_FILE_STORE_COL_LOADER,
  BST_FILE_STORE_COL_LOADABLE,
  BST_FILE_STORE_N_COLS
};
GtkTreeModel*   bst_file_store_create           (void);
void            bst_file_store_update_list      (GtkTreeModel *model,
                                                 const gchar  *search_path,
                                                 const gchar  *filter);
void            bst_file_store_forget_list      (GtkTreeModel *model);
void            bst_file_store_destroy          (GtkTreeModel *model);


/* --- proxy stores --- */
enum {
  BST_PROXY_STORE_SEQID,
  BST_PROXY_STORE_NAME,
  BST_PROXY_STORE_BLURB,
  BST_PROXY_STORE_TYPE,
  BST_PROXY_STORE_N_COLS
};
/* store based on a BseItemSeq */
GtkTreeModel*   bst_item_seq_store_new                  (gboolean        sorted);
void            bst_item_seq_store_set                  (GtkTreeModel   *self,
                                                         BseItemSeq     *iseq);
gint            bst_item_seq_store_add                  (GtkTreeModel   *self,
                                                         SfiProxy        proxy);
gint            bst_item_seq_store_remove               (GtkTreeModel   *self,
                                                         SfiProxy        proxy);
gint            bst_item_seq_store_raise                (GtkTreeModel   *self,
                                                         SfiProxy        proxy);
gboolean        bst_item_seq_store_can_raise            (GtkTreeModel   *self,
                                                         SfiProxy        proxy);
gint            bst_item_seq_store_lower                (GtkTreeModel   *self,
                                                         SfiProxy        proxy);
gboolean        bst_item_seq_store_can_lower            (GtkTreeModel   *self,
                                                         SfiProxy        proxy);
BseItemSeq*     bst_item_seq_store_dup                  (GtkTreeModel   *self);
SfiProxy        bst_item_seq_store_get_proxy            (GtkTreeModel   *self,
                                                         gint            row);
SfiProxy        bst_item_seq_store_get_from_iter        (GtkTreeModel   *self,
                                                         GtkTreeIter    *iter);
gboolean        bst_item_seq_store_get_iter             (GtkTreeModel   *self,
                                                         GtkTreeIter    *iter,
                                                         SfiProxy        proxy);
/* store based on the child list of a container */
GxkListWrapper* bst_child_list_wrapper_store_new        (void);


/* --- generic child list wrapper --- */
void     bst_child_list_wrapper_setup           (GxkListWrapper *self,
                                                 SfiProxy        parent,
                                                 const gchar    *child_type);
void     bst_child_list_wrapper_set_listener    (GxkListWrapper *self,
                                                 void          (*listener) (GtkTreeModel *model,
                                                                            SfiProxy      item,
                                                                            gboolean      added));
void     bst_child_list_wrapper_rebuild         (GxkListWrapper *self);
SfiProxy bst_child_list_wrapper_get_proxy       (GxkListWrapper *self,
                                                 gint            row);
SfiProxy bst_child_list_wrapper_get_from_iter   (GxkListWrapper *self,
                                                 GtkTreeIter    *iter);
gboolean bst_child_list_wrapper_get_iter        (GxkListWrapper *self,
                                                 GtkTreeIter    *iter,
                                                 SfiProxy        proxy);
void     bst_child_list_wrapper_proxy_changed   (GxkListWrapper *self,
                                                 SfiProxy        item);


G_END_DECLS

#endif /* __BST_TREE_STORES_H__ */

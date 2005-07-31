/*
 * Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-directory-browser.h"

/* prototypes */
static void xfburn_directory_browser_class_init (XfburnDirectoryBrowserClass *);
static void xfburn_directory_browser_init (XfburnDirectoryBrowser *);

static gint directory_tree_sort_func (GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gpointer);

/* globals */
static GtkTreeViewClass *parent_class = NULL;

/***************************/
/* XfburnDirectoryBrowser class */
/***************************/
GtkType
xfburn_directory_browser_get_type (void)
{
  static GtkType directory_browser_type = 0;

  if (!directory_browser_type) {
    static const GTypeInfo directory_browser_info = {
      sizeof (XfburnDirectoryBrowserClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_directory_browser_class_init,
      NULL,
      NULL,
      sizeof (XfburnDirectoryBrowser),
      0,
      (GInstanceInitFunc) xfburn_directory_browser_init
    };

    directory_browser_type = g_type_register_static (GTK_TYPE_TREE_VIEW, "XfburnDirectoryBrowser", &directory_browser_info, 0);
  }

  return directory_browser_type;
}

static void
xfburn_directory_browser_class_init (XfburnDirectoryBrowserClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_directory_browser_init (XfburnDirectoryBrowser * browser)
{
  GtkTreeStore *model;
  GtkTreeViewColumn *column_file;
  GtkCellRenderer *cell_icon, *cell_file;
  GtkTreeSelection *selection;
     
  model = gtk_tree_store_new (DIRECTORY_BROWSER_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
							  G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), 0, directory_tree_sort_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), 0, GTK_SORT_ASCENDING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (browser), GTK_TREE_MODEL (model));  
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (browser), TRUE);
  
  column_file = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_file, _("File"));
      
  cell_icon = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column_file, cell_icon, FALSE);
  gtk_tree_view_column_set_attributes (column_file, cell_icon, "pixbuf", DIRECTORY_BROWSER_COLUMN_ICON, NULL);
  g_object_set (cell_icon, "xalign", 0.0, "ypad", 0, NULL);
  
  cell_file = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_file, cell_file, TRUE);
  gtk_tree_view_column_set_attributes (column_file, cell_file, "text", DIRECTORY_BROWSER_COLUMN_FILE, NULL);
    
  gtk_tree_view_append_column (GTK_TREE_VIEW (browser), column_file);
  
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (browser), -1, _("Size"), gtk_cell_renderer_text_new (),
											   "text", DIRECTORY_BROWSER_COLUMN_SIZE, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (browser), -1, _("Type"), gtk_cell_renderer_text_new (),
											   "text", DIRECTORY_BROWSER_COLUMN_TYPE, NULL);
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
}

/* internals */
static gint
directory_tree_sort_func (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
  gchar *a_str = NULL;
  gchar *b_str = NULL;
  gint result;

  gtk_tree_model_get (model, a, DIRECTORY_BROWSER_COLUMN_FILE, &a_str, -1);
  gtk_tree_model_get (model, b, DIRECTORY_BROWSER_COLUMN_FILE, &b_str, -1);

  if (a_str == NULL)
    a_str = g_strdup ("");
  if (b_str == NULL)
    b_str = g_strdup ("");

  result = g_utf8_collate (a_str, b_str);

  g_free (a_str);
  g_free (b_str);

  return result;
}

/* public methods */
GtkWidget *
xfburn_directory_browser_new (void)
{
  return g_object_new (xfburn_directory_browser_get_type (), NULL);
}

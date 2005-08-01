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
#include "xfburn-utils.h"

/* prototypes */
static void xfburn_directory_browser_class_init (XfburnDirectoryBrowserClass *);
static void xfburn_directory_browser_init (XfburnDirectoryBrowser *);

static gint directory_tree_sortfunc (GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gpointer);

/* globals */
static GtkTreeViewClass *parent_class = NULL;
static const gchar *DIRECTORY = N_ ("Folder");

/********************************/
/* XfburnDirectoryBrowser class */
/********************************/
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

    directory_browser_type =
      g_type_register_static (GTK_TYPE_TREE_VIEW, "XfburnDirectoryBrowser", &directory_browser_info, 0);
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
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), DIRECTORY_BROWSER_COLUMN_FILE, 
								   directory_tree_sortfunc, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), DIRECTORY_BROWSER_COLUMN_FILE, GTK_SORT_ASCENDING);
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
directory_tree_sortfunc (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
  /* adapted from gnomebaker */
  gchar *aname, *bname, *amime, *bmime;
  gboolean aisdir = FALSE;
  gboolean bisdir = FALSE;
  gint result = 0;
  
  gtk_tree_model_get (model, a, DIRECTORY_BROWSER_COLUMN_FILE, &aname, DIRECTORY_BROWSER_COLUMN_TYPE, &amime, -1);
  gtk_tree_model_get (model, b, DIRECTORY_BROWSER_COLUMN_FILE, &bname, DIRECTORY_BROWSER_COLUMN_TYPE, &bmime, -1);
  
  aisdir = !g_ascii_strcasecmp (amime, DIRECTORY);
  bisdir = !g_ascii_strcasecmp (bmime, DIRECTORY);
  
  if (aisdir && !bisdir)
    result = -1;
  else if (!aisdir && bisdir)
    result = 1;
  else
    result = g_ascii_strcasecmp (aname, bname);

  g_free (aname);
  g_free (amime);
  g_free (bname);
  g_free (bmime);

  return result;
}

/* public methods */
GtkWidget *
xfburn_directory_browser_new (void)
{
  return g_object_new (xfburn_directory_browser_get_type (), NULL);
}

void
xfburn_directory_browser_load_path (XfburnDirectoryBrowser * browser, const gchar * path)
{
  GtkTreeModel *model;
  GDir *dir;
  GError *error = NULL;
  GdkPixbuf *icon_directory, *icon_file;
  const gchar *dir_entry;
  int x,y;
  
  if (GTK_WIDGET (browser)->parent)
	xfburn_busy_cursor (GTK_WIDGET (browser));
  
  dir = g_dir_open (path, 0, &error);
  if (!dir) {
	g_warning ("unable to open the %s directory : %s", path, error->message);
	g_error_free (error);
	return;
  }
  
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (browser));
  gtk_tree_store_clear (GTK_TREE_STORE (model));

  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &x, &y);
  icon_directory = xfce_themed_icon_load ("gnome-fs-directory", x);
  icon_file = xfce_themed_icon_load ("gnome-fs-regular", x);
    
  while ( (dir_entry = g_dir_read_name (dir)) ) {
	gchar *full_path;
	
	full_path = g_build_filename (path, dir_entry, NULL);
	if (dir_entry[0] != '.' && !g_file_test (full_path, G_FILE_TEST_IS_SYMLINK)) {
	  GtkTreeIter iter;
	  	  
	  gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	  
	  if (g_file_test (full_path, G_FILE_TEST_IS_DIR)) {
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							DIRECTORY_BROWSER_COLUMN_ICON, icon_directory,
							DIRECTORY_BROWSER_COLUMN_FILE, dir_entry,
							DIRECTORY_BROWSER_COLUMN_TYPE, DIRECTORY,
							DIRECTORY_BROWSER_COLUMN_PATH, full_path, -1);
	  } else if (g_file_test (full_path, G_FILE_TEST_IS_REGULAR)) {
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							DIRECTORY_BROWSER_COLUMN_ICON, icon_file,
							DIRECTORY_BROWSER_COLUMN_FILE, dir_entry,
							DIRECTORY_BROWSER_COLUMN_TYPE, "file", 
							DIRECTORY_BROWSER_COLUMN_PATH, full_path, -1);
	  }
	}
	g_free (full_path);
  }
    
  if (icon_directory)
	g_object_unref (icon_directory);
  if (icon_file)
	g_object_unref (icon_file);

  g_dir_close (dir);
  
  if (GTK_WIDGET (browser)->parent)
	xfburn_default_cursor (GTK_WIDGET (browser));
}

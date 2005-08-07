/*
 * Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#ifdef HAVE_STRINGS_H
#include <string.h>
#endif

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4util/libxfce4util.h>

#include "xfburn-fs-browser.h"
#include "xfburn-disc-content.h"
#include "xfburn-utils.h"

/* prototypes */
static void xfburn_fs_browser_class_init (XfburnFsBrowserClass * klass);
static void xfburn_fs_browser_init (XfburnFsBrowser * sp);

static void cb_browser_row_expanded (GtkTreeView *, GtkTreeIter *, GtkTreePath *, gpointer);
static void cb_browser_drag_data_get (GtkWidget *, GdkDragContext *, GtkSelectionData *, guint, guint, gpointer);

/* globals */
static GtkTreeViewClass *parent_class = NULL;

GType
xfburn_fs_browser_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnFsBrowserClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_fs_browser_class_init,
      NULL,
      NULL,
      sizeof (XfburnFsBrowser),
      0,
      (GInstanceInitFunc) xfburn_fs_browser_init,
    };

    type = g_type_register_static (GTK_TYPE_TREE_VIEW, "XfburnFsBrowser", &our_info, 0);
  }

  return type;
}

static void
xfburn_fs_browser_class_init (XfburnFsBrowserClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);

}

static void
xfburn_fs_browser_init (XfburnFsBrowser * browser)
{
  GtkTreeStore *model;
  GtkTreeViewColumn *column_directory;
  GtkCellRenderer *cell_icon, *cell_directory;
  GtkTreeSelection *selection;

  GtkTargetEntry gte[] = { {"text/plain", 0, DISC_CONTENT_DND_TARGET_TEXT_PLAIN} };

  gtk_widget_set_size_request (GTK_WIDGET (browser), 200, 300);

  model = gtk_tree_store_new (FS_BROWSER_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), FS_BROWSER_COLUMN_DIRECTORY, GTK_SORT_ASCENDING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (browser), GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (browser), TRUE);

  column_directory = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_directory, _("Filesystem"));
  gtk_tree_view_column_set_expand (column_directory, TRUE);

  cell_icon = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column_directory, cell_icon, FALSE);
  gtk_tree_view_column_set_attributes (column_directory, cell_icon, "pixbuf", FS_BROWSER_COLUMN_ICON, NULL);
  g_object_set (cell_icon, "xalign", 0.0, "ypad", 0, NULL);

  cell_directory = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_directory, cell_directory, TRUE);
  gtk_tree_view_column_set_attributes (column_directory, cell_directory, "text", FS_BROWSER_COLUMN_DIRECTORY, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (browser), column_directory);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  g_signal_connect (G_OBJECT (browser), "row-expanded", G_CALLBACK (cb_browser_row_expanded), browser);

  /* load the directory list */
  xfburn_fs_browser_refresh (browser);

  /* set up DnD */
  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (browser), GDK_BUTTON1_MASK, gte,
                                          DISC_CONTENT_DND_TARGETS, GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (browser), "drag-data-get", G_CALLBACK (cb_browser_drag_data_get), browser);
}

static void
load_directory_in_browser (XfburnFsBrowser * browser, const gchar * path, GtkTreeIter * parent)
{
  GDir *dir;
  GError *error = NULL;
  GdkPixbuf *icon;
  GtkTreeModel *model;
  const gchar *dir_entry;
  int x, y;

  if (GTK_WIDGET (browser)->parent)
    xfburn_busy_cursor (GTK_WIDGET (browser));

  dir = g_dir_open (path, 0, &error);
  if (!dir) {
    g_warning ("unable to open the %s directory : %s", path, error->message);
    g_error_free (error);
    return;
  }

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (browser));

  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &x, &y);
  icon = xfce_themed_icon_load ("gnome-fs-directory", x);

  while ((dir_entry = g_dir_read_name (dir))) {
    gchar *full_path;

    full_path = g_build_filename (path, dir_entry, NULL);
    if (dir_entry[0] != '.' && g_file_test (full_path, G_FILE_TEST_IS_DIR) &&
        !g_file_test (full_path, G_FILE_TEST_IS_SYMLINK)) {
      GtkTreeIter iter;
      GtkTreeIter iter_empty;

      gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent);
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                          FS_BROWSER_COLUMN_ICON, icon,
                          FS_BROWSER_COLUMN_DIRECTORY, dir_entry, FS_BROWSER_COLUMN_PATH, full_path, -1);
      gtk_tree_store_append (GTK_TREE_STORE (model), &iter_empty, &iter);
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter_empty, FS_BROWSER_COLUMN_DIRECTORY, "", -1);
    }
    g_free (full_path);
  }

  if (icon)
    g_object_unref (icon);

  g_dir_close (dir);

  if (GTK_WIDGET (browser)->parent)
    xfburn_default_cursor (GTK_WIDGET (browser));
}

static void
cb_browser_row_expanded (GtkTreeView * treeview, GtkTreeIter * iter, GtkTreePath * path, gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter_empty;
  gchar *value;

  model = gtk_tree_view_get_model (treeview);
  if (!gtk_tree_model_iter_nth_child (model, &iter_empty, iter, 0))
    return;

  gtk_tree_model_get (model, &iter_empty, FS_BROWSER_COLUMN_DIRECTORY, &value, -1);

  if (gtk_tree_path_get_depth (path) > 1 && strlen (value) == 0) {
    gchar *full_path;

    gtk_tree_store_remove (GTK_TREE_STORE (model), &iter_empty);

    gtk_tree_model_get (model, iter, FS_BROWSER_COLUMN_PATH, &full_path, -1);

    load_directory_in_browser (user_data, full_path, iter);
    gtk_tree_view_expand_row (treeview, path, FALSE);

    g_free (full_path);
  }

  g_free (value);
}

static void
cb_browser_drag_data_get (GtkWidget * widget, GdkDragContext * dc,
                          GtkSelectionData * data, guint info, guint time, gpointer user_data)
{
  if (info == DISC_CONTENT_DND_TARGET_TEXT_PLAIN) {
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *path;
    gchar *full_path;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
    gtk_tree_selection_get_selected (selection, &model, &iter);

    gtk_tree_model_get (model, &iter, FS_BROWSER_COLUMN_PATH, &path, -1);

    full_path = g_strdup_printf ("file://%s", path);
    g_free (path);

    gtk_selection_data_set_text (data, full_path, -1);

    g_free (full_path);
  }
}

/* public methods */
GtkWidget *
xfburn_fs_browser_new ()
{
  return g_object_new (XFBURN_TYPE_FS_BROWSER, NULL);
}

void
xfburn_fs_browser_refresh (XfburnFsBrowser * browser)
{
  GtkTreeModel *model;
  GtkTreeIter iter_home, iter_root;
  int x, y;
  gchar *text;
  GdkPixbuf *icon;
  GtkTreeSelection *selection;
  GtkTreePath *path;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (browser));

  gtk_tree_store_clear (GTK_TREE_STORE (model));

  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &x, &y);

  /* load the user's home dir */
  text = g_strdup_printf (_("%s's home"), g_get_user_name ());
  icon = xfce_themed_icon_load ("gnome-fs-home", x);
  gtk_tree_store_append (GTK_TREE_STORE (model), &iter_home, NULL);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter_home,
                      FS_BROWSER_COLUMN_ICON, icon,
                      FS_BROWSER_COLUMN_DIRECTORY, text, FS_BROWSER_COLUMN_PATH, xfce_get_homedir (), -1);
  if (icon)
    g_object_unref (icon);
  g_free (text);

  load_directory_in_browser (browser, xfce_get_homedir (), &iter_home);

  /* load the fs root */
  icon = xfce_themed_icon_load ("gnome-dev-harddisk", x);
  gtk_tree_store_append (GTK_TREE_STORE (model), &iter_root, NULL);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter_root,
                      FS_BROWSER_COLUMN_ICON, icon,
                      FS_BROWSER_COLUMN_DIRECTORY, _("Filesystem"), FS_BROWSER_COLUMN_PATH, "/", -1);
  if (icon)
    g_object_unref (icon);

  load_directory_in_browser (browser, "/", &iter_root);

  /* set cursor on home dir row */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser));
  gtk_tree_selection_select_iter (selection, &iter_home);

  /* expand the home dir row */
  path = gtk_tree_model_get_path (model, &iter_home);
  gtk_tree_view_expand_row (GTK_TREE_VIEW (browser), path, FALSE);
  gtk_tree_path_free (path);
}

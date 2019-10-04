/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "xfburn-fs-browser.h"
#include "xfburn-data-composition.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-main.h"


/* prototypes */
static void xfburn_fs_browser_class_init (XfburnFsBrowserClass * klass, gpointer data);
static void xfburn_fs_browser_init (XfburnFsBrowser * sp, gpointer data);

static void cb_browser_row_expanded (GtkTreeView *, GtkTreeIter *, GtkTreePath *, gpointer);
static void cb_browser_row_activated (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);
static void cb_browser_drag_data_get (GtkWidget *, GdkDragContext *, GtkSelectionData *, guint, guint, XfburnFsBrowser *);

/*************************/
/* XfburnFsBrowser class */
/*************************/
static ExoTreeViewClass *parent_class = NULL;

GType
xfburn_fs_browser_get_type (void)
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
      NULL
    };

    type = g_type_register_static (EXO_TYPE_TREE_VIEW, "XfburnFsBrowser", &our_info, 0);
  }

  return type;
}

static void
xfburn_fs_browser_class_init (XfburnFsBrowserClass * klass, gpointer data)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_fs_browser_init (XfburnFsBrowser * browser, gpointer data)
{
  GtkTreeStore *model;
  GtkTreeViewColumn *column_directory;
  GtkCellRenderer *cell_icon, *cell_directory;
  GtkTreeSelection *selection;

  GtkTargetEntry gte[] = { {"text/plain;charset=utf-8", 0, DATA_COMPOSITION_DND_TARGET_TEXT_PLAIN} };

  model = gtk_tree_store_new (FS_BROWSER_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), FS_BROWSER_COLUMN_DIRECTORY, GTK_SORT_ASCENDING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (browser), GTK_TREE_MODEL (model));
  // gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (browser), TRUE);

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
  g_signal_connect (G_OBJECT (browser), "row-activated", G_CALLBACK (cb_browser_row_activated), browser);

  /* load the directory list */
  xfburn_fs_browser_refresh (browser);

  /* set up DnD */
  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (browser), GDK_BUTTON1_MASK, gte,
                                          G_N_ELEMENTS (gte), GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (browser), "drag-data-get", G_CALLBACK (cb_browser_drag_data_get), browser);
}

static void
load_directory_in_browser (XfburnFsBrowser * browser, const gchar * path, GtkTreeIter * parent)
{
  GDir *dir;
  GError *error = NULL;
  GdkPixbuf *icon;
  GtkTreeModel *model;
  GdkScreen *screen;
  GtkIconTheme *icon_theme;
  const gchar *dir_entry;
  int x, y;
  gboolean show_hidden;

  dir = g_dir_open (path, 0, &error);
  if (!dir) {
    g_warning ("unable to open the %s directory : %s", path, error->message);
    g_error_free (error);
    return;
  }

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (browser));

  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);

  screen = gtk_widget_get_screen (GTK_WIDGET (browser));
  icon_theme = gtk_icon_theme_get_for_screen (screen);
  icon = gtk_icon_theme_load_icon (icon_theme, "folder", x, 0, NULL);

  show_hidden = xfburn_settings_get_boolean ("show-hidden-files", FALSE);

  while ((dir_entry = g_dir_read_name (dir))) {
    gchar *full_path;

    if (dir_entry[0] == '.') {
      /* skip . and .. */
      if (dir_entry[1] == '\0' ||
          (dir_entry[1] == '.' && dir_entry[2] == '\0'))
        continue;

      if (!show_hidden)
        continue;
    }

    full_path = g_build_filename (path, dir_entry, NULL);
    if (g_file_test (full_path, G_FILE_TEST_IS_DIR)) {
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
cb_browser_row_activated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
  gtk_tree_view_expand_row (treeview, path, FALSE);
}

static void
cb_browser_drag_data_get (GtkWidget * widget, GdkDragContext * dc,
                          GtkSelectionData * data, guint info, guint timestamp, XfburnFsBrowser *browser)
{
  if (info == DATA_COMPOSITION_DND_TARGET_TEXT_PLAIN) {
    gchar *full_path = NULL;

    full_path = xfburn_fs_browser_get_selection (browser);
    gtk_selection_data_set_text (data, full_path, -1);
    g_free (full_path);
  }
}

/* public methods */
GtkWidget *
xfburn_fs_browser_new (void)
{
  return g_object_new (XFBURN_TYPE_FS_BROWSER, NULL);
}

void
xfburn_fs_browser_refresh (XfburnFsBrowser * browser)
{
  GtkTreeModel *model;
  GtkTreeIter iter_initial, iter_home, iter_root;
  int x, y;
  gchar *text;
  GdkScreen *screen;
  GtkIconTheme *icon_theme;
  GdkPixbuf *icon;
  GtkTreeSelection *selection;
  GtkTreePath *path;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (browser));

  gtk_tree_store_clear (GTK_TREE_STORE (model));

  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);

  /* load the initial dir, if set */
  if (xfburn_main_has_initial_dir ()) {
    text = g_path_get_basename (xfburn_main_get_initial_dir ());

    screen = gtk_widget_get_screen (GTK_WIDGET (browser));
    icon_theme = gtk_icon_theme_get_for_screen (screen);
    icon = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-directory", x, 0, NULL);

    gtk_tree_store_append (GTK_TREE_STORE (model), &iter_initial, NULL);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter_initial,
                        FS_BROWSER_COLUMN_ICON, icon,
                        FS_BROWSER_COLUMN_DIRECTORY, text, FS_BROWSER_COLUMN_PATH, xfburn_main_get_initial_dir (), -1);
    if (icon)
      g_object_unref (icon);
    g_free (text);

    load_directory_in_browser (browser, xfburn_main_get_initial_dir (), &iter_initial);
  }


  /* load the user's home dir */
  text = g_strdup_printf (_("%s's home"), g_get_user_name ());

  screen = gtk_widget_get_screen (GTK_WIDGET (browser));
  icon_theme = gtk_icon_theme_get_for_screen (screen);
  icon = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-home", x, 0, NULL);

  gtk_tree_store_append (GTK_TREE_STORE (model), &iter_home, NULL);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter_home,
                      FS_BROWSER_COLUMN_ICON, icon,
                      FS_BROWSER_COLUMN_DIRECTORY, text, FS_BROWSER_COLUMN_PATH, xfce_get_homedir (), -1);
  if (icon)
    g_object_unref (icon);
  g_free (text);

  load_directory_in_browser (browser, xfce_get_homedir (), &iter_home);

  /* load the fs root */
  icon = gtk_icon_theme_load_icon (icon_theme, "gnome-dev-harddisk", x, 0, NULL);
  gtk_tree_store_append (GTK_TREE_STORE (model), &iter_root, NULL);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter_root,
                      FS_BROWSER_COLUMN_ICON, icon,
                      FS_BROWSER_COLUMN_DIRECTORY, _("Filesystem"), FS_BROWSER_COLUMN_PATH, "/", -1);
  if (icon)
    g_object_unref (icon);

  load_directory_in_browser (browser, "/", &iter_root);

  /* set cursor on home dir row */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser));
  if (xfburn_main_has_initial_dir ()) {
    /* set cursor on initial dir row */
    gtk_tree_selection_select_iter (selection, &iter_initial);

    /* expand the initial dir row */
    path = gtk_tree_model_get_path (model, &iter_initial);
  } else {
    /* set cursor on home dir row */
    gtk_tree_selection_select_iter (selection, &iter_home);

    /* expand the home dir row */
    path = gtk_tree_model_get_path (model, &iter_home);
  }
  gtk_tree_view_expand_row (GTK_TREE_VIEW (browser), path, FALSE);
  gtk_tree_path_free (path);
}

gchar *
xfburn_fs_browser_get_selection (XfburnFsBrowser *browser)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  gchar *full_path = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gchar *path = NULL;

    gtk_tree_model_get (model, &iter, FS_BROWSER_COLUMN_PATH, &path, -1);

    full_path = g_strdup_printf ("file://%s", path);
    g_free (path);
  }

  return full_path;
}

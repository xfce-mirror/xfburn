/* $Id$ */
/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#ifdef HAVE_THUNAR_VFS
#include <thunar-vfs/thunar-vfs.h>
#endif

#include <exo/exo.h>

#include "xfburn-directory-browser.h"
#include "xfburn-data-composition.h"
#include "xfburn-utils.h"

#define XFBURN_DIRECTORY_BROWSER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_DIRECTORY_BROWSER, XfburnDirectoryBrowserPrivate))

typedef struct
{
  gchar *current_path;
} XfburnDirectoryBrowserPrivate;

/* prototypes */
static void xfburn_directory_browser_class_init (XfburnDirectoryBrowserClass *);
static void xfburn_directory_browser_init (XfburnDirectoryBrowser *);
static void cb_browser_drag_data_get (GtkWidget *, GdkDragContext *, GtkSelectionData *, guint, guint, gpointer);

static gint directory_tree_sortfunc (GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gpointer);

/* globals */
static const gchar *DIRECTORY = N_("Folder");

/********************************/
/* XfburnDirectoryBrowser class */
/********************************/
static ExoTreeViewClass *parent_class = NULL;

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
      g_type_register_static (EXO_TYPE_TREE_VIEW, "XfburnDirectoryBrowser", &directory_browser_info, 0);
  }

  return directory_browser_type;
}

static void
xfburn_directory_browser_finalize (GObject * object)
{
  XfburnDirectoryBrowserPrivate *priv = XFBURN_DIRECTORY_BROWSER_GET_PRIVATE (object);

  g_free (priv->current_path);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_directory_browser_class_init (XfburnDirectoryBrowserClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (XfburnDirectoryBrowserPrivate));
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->finalize = xfburn_directory_browser_finalize;
}

static void
xfburn_directory_browser_init (XfburnDirectoryBrowser * browser)
{
  GtkListStore *model;
  GtkTreeViewColumn *column_file;
  GtkCellRenderer *cell_icon, *cell_file;
  GtkTreeSelection *selection;

  GtkTargetEntry gte[] = { {"text/plain", 0, DATA_COMPOSITION_DND_TARGET_TEXT_PLAIN} };
    
  model = gtk_list_store_new (DIRECTORY_BROWSER_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), DIRECTORY_BROWSER_COLUMN_FILE,
                                   directory_tree_sortfunc, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), DIRECTORY_BROWSER_COLUMN_FILE, GTK_SORT_ASCENDING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (browser), GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (browser), TRUE);

  column_file = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_file, _("File"));
  gtk_tree_view_column_set_resizable (column_file, 1);

  cell_icon = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column_file, cell_icon, FALSE);
  gtk_tree_view_column_set_attributes (column_file, cell_icon, "pixbuf", DIRECTORY_BROWSER_COLUMN_ICON, NULL);
  g_object_set (cell_icon, "xalign", 0.0, "ypad", 0, NULL);

  cell_file = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_file, cell_file, TRUE);
  gtk_tree_view_column_set_attributes (column_file, cell_file, "text", DIRECTORY_BROWSER_COLUMN_FILE, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (browser), column_file);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (browser), -1, _("Size"), gtk_cell_renderer_text_new (),
                                               "text", DIRECTORY_BROWSER_COLUMN_HUMANSIZE, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (browser), -1, _("Type"), gtk_cell_renderer_text_new (),
                                               "text", DIRECTORY_BROWSER_COLUMN_TYPE, NULL);

  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column ( GTK_TREE_VIEW (browser), 1), 1);
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column ( GTK_TREE_VIEW (browser), 2), 1); 

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  /* set up DnD */
  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (browser), GDK_BUTTON1_MASK, gte,
                                          G_N_ELEMENTS (gte), GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (browser), "drag-data-get", G_CALLBACK (cb_browser_drag_data_get), browser);
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

  aisdir = !g_ascii_strcasecmp (amime, _(DIRECTORY));
  bisdir = !g_ascii_strcasecmp (bmime, _(DIRECTORY));

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

static void
cb_browser_drag_data_get (GtkWidget * widget, GdkDragContext * dc,
                          GtkSelectionData * data, guint info, guint time, gpointer user_data)
{
  if (info == DATA_COMPOSITION_DND_TARGET_TEXT_PLAIN) {
    gchar *full_paths = NULL;

    full_paths = xfburn_directory_browser_get_selection (XFBURN_DIRECTORY_BROWSER (widget));
    gtk_selection_data_set_text (data, full_paths, -1);
    g_free (full_paths);
  }
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
  XfburnDirectoryBrowserPrivate *priv = XFBURN_DIRECTORY_BROWSER_GET_PRIVATE (browser);
  
  GtkTreeModel *model;
  GDir *dir;
  GError *error = NULL;
  GdkPixbuf *icon_directory, *icon_file;
  const gchar *dir_entry;
  int x, y;
  gchar *temp;
  GdkScreen *screen;
  GtkIconTheme *icon_theme;
  
  dir = g_dir_open (path, 0, &error);
  if (!dir) {
    g_warning ("unable to open the %s directory : %s", path, error->message);
    g_error_free (error);
    return;
  }
  
  temp = g_strdup (path);
  g_free (priv->current_path);
  priv->current_path = temp;
 
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (browser));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  screen = gtk_widget_get_screen (GTK_WIDGET (browser));
  icon_theme = gtk_icon_theme_get_for_screen (screen);

  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);
  icon_directory = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-directory", x, 0, NULL);
  icon_file = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-regular", x, 0, NULL);

  while ((dir_entry = g_dir_read_name (dir))) {
    gchar *full_path;
    struct stat s;

    full_path = g_build_filename (path, dir_entry, NULL);
    if (g_file_test (full_path, G_FILE_TEST_IS_SYMLINK)) {
      g_free (full_path);
      continue;
    }

    if (dir_entry[0] != '.' && (stat (full_path, &s) == 0)) {
      gchar *humansize;
      
      humansize = xfburn_humanreadable_filesize (s.st_size);
      
      if ((s.st_mode & S_IFDIR)) {
        GtkTreeIter iter;
        
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                            DIRECTORY_BROWSER_COLUMN_ICON, icon_directory,
                            DIRECTORY_BROWSER_COLUMN_FILE, dir_entry,
                            DIRECTORY_BROWSER_COLUMN_HUMANSIZE, humansize,
                            DIRECTORY_BROWSER_COLUMN_SIZE, (guint64) s.st_size,
                            DIRECTORY_BROWSER_COLUMN_TYPE, _(DIRECTORY), DIRECTORY_BROWSER_COLUMN_PATH, full_path, -1);
      }
      else if ((s.st_mode & S_IFREG)) {
        GtkTreeIter iter;
#ifdef HAVE_THUNAR_VFS
		ThunarVfsMimeDatabase *mime_database = NULL;
		ThunarVfsMimeInfo *mime_info = NULL;
		const gchar *mime_icon_name = NULL;
		GdkPixbuf *mime_icon = NULL;
		const gchar *mime_str = NULL;
#endif
		
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		
#ifdef HAVE_THUNAR_VFS
		mime_database = thunar_vfs_mime_database_get_default ();
		mime_info = thunar_vfs_mime_database_get_info_for_file (mime_database, full_path, NULL);
		
		mime_icon_name = thunar_vfs_mime_info_lookup_icon_name (mime_info, icon_theme);
		mime_icon = gtk_icon_theme_load_icon (icon_theme, mime_icon_name, x, 0, NULL);
		
		mime_str = thunar_vfs_mime_info_get_comment (mime_info);
		
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                            DIRECTORY_BROWSER_COLUMN_ICON, (G_IS_OBJECT (mime_icon) ? mime_icon : icon_file),
                            DIRECTORY_BROWSER_COLUMN_FILE, dir_entry,
                            DIRECTORY_BROWSER_COLUMN_HUMANSIZE, humansize,
                            DIRECTORY_BROWSER_COLUMN_SIZE, (guint64) s.st_size,
                            DIRECTORY_BROWSER_COLUMN_TYPE, mime_str, DIRECTORY_BROWSER_COLUMN_PATH, full_path, -1);
		
		if (G_LIKELY (G_IS_OBJECT (mime_icon)))
		  g_object_unref (mime_icon);
		thunar_vfs_mime_info_unref (mime_info);
		g_object_unref (mime_database);
#else
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                            DIRECTORY_BROWSER_COLUMN_ICON, icon_file,
                            DIRECTORY_BROWSER_COLUMN_FILE, dir_entry,
                            DIRECTORY_BROWSER_COLUMN_HUMANSIZE, humansize,
                            DIRECTORY_BROWSER_COLUMN_SIZE, (guint64) s.st_size,
                            DIRECTORY_BROWSER_COLUMN_TYPE, _("File"), DIRECTORY_BROWSER_COLUMN_PATH, full_path, -1);
#endif
      }
      g_free (humansize);
    }
    g_free (full_path);
  }

  if (icon_directory)
    g_object_unref (icon_directory);
  if (icon_file)
    g_object_unref (icon_file);

  g_dir_close (dir);
}

void
xfburn_directory_browser_refresh (XfburnDirectoryBrowser * browser) 
{
  XfburnDirectoryBrowserPrivate *priv = XFBURN_DIRECTORY_BROWSER_GET_PRIVATE (browser);
  gchar *temp;
  
  temp = g_strdup (priv->current_path);
  xfburn_directory_browser_load_path (browser, (const gchar*) temp);
  g_free (temp);
}

gchar *
xfburn_directory_browser_get_selection (XfburnDirectoryBrowser * browser)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *selected_rows;
  gchar *full_paths = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser));

  selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
  selected_rows = g_list_last (selected_rows);
  while (selected_rows) {
    GtkTreeIter iter;
    gchar *current_path = NULL;
    gchar *temp = NULL;
    
    gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) selected_rows->data);
    gtk_tree_model_get (model, &iter, DIRECTORY_BROWSER_COLUMN_PATH, &current_path, -1);

    temp = g_strdup_printf ("file://%s\n%s", current_path, full_paths ? full_paths : "");
    g_free (current_path);
    g_free (full_paths);
    full_paths = temp;

    selected_rows = g_list_previous (selected_rows);
  }

  selected_rows = g_list_first (selected_rows);
  g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (selected_rows);

  return full_paths;
}

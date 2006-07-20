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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

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

#include "xfburn-disc-content.h"

#if 0
#include "xfburn-adding-progress.h"
#endif

#include "xfburn-burn-composition-dialog.h"
#include "xfburn-disc-usage.h"
#include "xfburn-main-window.h"
#include "xfburn-utils.h"

/* prototypes */
static void xfburn_disc_content_class_init (XfburnDiscContentClass *);
static void xfburn_disc_content_init (XfburnDiscContent *);
static void xfburn_disc_content_finalize (GObject * object);

static gint directory_tree_sortfunc (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data);

static void disc_content_action_clear (GtkAction *, XfburnDiscContent *);
static void disc_content_action_remove_selection (GtkAction *, XfburnDiscContent *);
static void disc_content_action_rename_selection (GtkAction * action, XfburnDiscContent * dc);

static gboolean cb_treeview_button_pressed (GtkTreeView * treeview, GdkEventButton * event, XfburnDiscContent * dc);
static void cb_begin_burn (XfburnDiscUsage * du, XfburnDiscContent * dc);
static void cell_file_edited_cb (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnDiscContent * dc);

static void content_drag_data_rcv_cb (GtkWidget *, GdkDragContext *, guint, guint, GtkSelectionData *, guint, guint,
                                      XfburnDiscContent *);
static void content_drag_data_get_cb (GtkWidget * widget, GdkDragContext * dc, GtkSelectionData * data, guint info,
                                      guint time, XfburnDiscContent * content);

static gboolean add_file_to_list_with_name (const gchar *name, XfburnDiscContent * dc, GtkTreeModel * model,
                                            const gchar * path, GtkTreeIter * iter, GtkTreeIter * insertion,
                                            GtkTreeViewDropPosition position);
static gboolean add_file_to_list (XfburnDiscContent * dc, GtkTreeModel * model, const gchar * path, GtkTreeIter * iter,
                                  GtkTreeIter * insertion, GtkTreeViewDropPosition position);
static gboolean generate_file_list (XfburnDiscContent * dc, gchar ** tmpfile);
                                  
enum
{
  DISC_CONTENT_COLUMN_ICON,
  DISC_CONTENT_COLUMN_CONTENT,
  DISC_CONTENT_COLUMN_HUMANSIZE,
  DISC_CONTENT_COLUMN_SIZE,
  DISC_CONTENT_COLUMN_PATH,
  DISC_CONTENT_COLUMN_TYPE,
  DISC_CONTENT_N_COLUMNS
};

typedef enum
{
  DISC_CONTENT_TYPE_FILE,
  DISC_CONTENT_TYPE_DIRECTORY
} DiscContentType;

struct XfburnDiscContentPrivate
{
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  GtkWidget *toolbar;
  GtkWidget *content;
  GtkWidget *disc_usage;
#if 0
  GtkWidget *progress;
#endif
};

/* globals */
static GtkHPanedClass *parent_class = NULL;
static const GtkActionEntry action_entries[] = {
  {"add-file", GTK_STOCK_ADD, N_("Add"), NULL, N_("Add the selected file(s) to the CD"),},
  {"remove-file", GTK_STOCK_REMOVE, N_("Remove"), NULL, N_("Remove the selected file(s) from the CD"),
   G_CALLBACK (disc_content_action_remove_selection),},
  {"clear", GTK_STOCK_CLEAR, N_("Clear"), NULL, N_("Clear the content of the CD"),
   G_CALLBACK (disc_content_action_clear),},
  {"import-session", "xfburn-import-session", N_("Import"), NULL, N_("Import existing session"),},
  {"rename-file", GTK_STOCK_EDIT, N_("Rename"), NULL, N_("Rename the selected file"),
   G_CALLBACK (disc_content_action_rename_selection),},
};

static const gchar *toolbar_actions[] = {
  "add-file",
  "remove-file",
  "clear",
  "import-session",
};

static GdkPixbuf *icon_directory = NULL, *icon_file = NULL;

/***************************/
/* XfburnDiscContent class */
/***************************/
GtkType
xfburn_disc_content_get_type (void)
{
  static GtkType disc_content_type = 0;

  if (!disc_content_type) {
    static const GTypeInfo disc_content_info = {
      sizeof (XfburnDiscContentClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_disc_content_class_init,
      NULL,
      NULL,
      sizeof (XfburnDiscContent),
      0,
      (GInstanceInitFunc) xfburn_disc_content_init
    };

    disc_content_type = g_type_register_static (GTK_TYPE_VBOX, "XfburnDiscContent", &disc_content_info, 0);
  }

  return disc_content_type;
}

static void
xfburn_disc_content_class_init (XfburnDiscContentClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_disc_content_finalize;
}

static void
xfburn_disc_content_init (XfburnDiscContent * disc_content)
{
  gint x, y;
  ExoToolbarsModel *model_toolbar;
  gint toolbar_position;
  GtkWidget *scrolled_window;
  GtkTreeStore *model;
  GtkTreeViewColumn *column_file;
  GtkCellRenderer *cell_icon, *cell_file;
  GtkTreeSelection *selection;

  const gchar ui_string[] = "<ui> <popup name=\"popup-menu\">"
    "<menuitem action=\"rename-file\"/>" "<menuitem action=\"remove-file\"/>" "</popup></ui>";

  GtkTargetEntry gte_src[] = { {"XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, DISC_CONTENT_DND_TARGET_INSIDE} };
  GtkTargetEntry gte_dest[] = { {"XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, DISC_CONTENT_DND_TARGET_INSIDE},
  {"text/plain", 0, DISC_CONTENT_DND_TARGET_TEXT_PLAIN}
  };

  /* initialize static members */
  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);
  if (!icon_directory)
    icon_directory = xfce_themed_icon_load ("gnome-fs-directory", x);
  if (!icon_file)
    icon_file = xfce_themed_icon_load ("gnome-fs-regular", x);

  disc_content->priv = g_new0 (XfburnDiscContentPrivate, 1);

  /* create ui manager */
  disc_content->priv->action_group = gtk_action_group_new ("xfburn-disc-content");
  gtk_action_group_set_translation_domain (disc_content->priv->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (disc_content->priv->action_group, action_entries, G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (disc_content));

  disc_content->priv->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (disc_content->priv->ui_manager, disc_content->priv->action_group, 0);

  gtk_ui_manager_add_ui_from_string (disc_content->priv->ui_manager, ui_string, -1, NULL);

  /* toolbar */
  model_toolbar = exo_toolbars_model_new ();
  exo_toolbars_model_set_actions (model_toolbar, (gchar **) toolbar_actions, G_N_ELEMENTS (toolbar_actions));
  toolbar_position = exo_toolbars_model_add_toolbar (model_toolbar, -1, "content-toolbar");
  exo_toolbars_model_set_style (model_toolbar, GTK_TOOLBAR_BOTH, toolbar_position);

  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "add-file", EXO_TOOLBARS_ITEM_TYPE);
  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "remove-file", EXO_TOOLBARS_ITEM_TYPE);
  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "clear", EXO_TOOLBARS_ITEM_TYPE);
  //exo_toolbars_model_add_separator (model_toolbar, toolbar_position, -1);
  //exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "import-session", EXO_TOOLBARS_ITEM_TYPE);

  disc_content->priv->toolbar = exo_toolbars_view_new_with_model (disc_content->priv->ui_manager, model_toolbar);
  gtk_box_pack_start (GTK_BOX (disc_content), disc_content->priv->toolbar, FALSE, FALSE, 0);
  gtk_widget_show (disc_content->priv->toolbar);

  /* content treeview */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (disc_content), scrolled_window, TRUE, TRUE, 0);

  disc_content->priv->content = gtk_tree_view_new ();
  model = gtk_tree_store_new (DISC_CONTENT_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_UINT);
							  
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), DISC_CONTENT_COLUMN_CONTENT,
                                   directory_tree_sortfunc, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), DISC_CONTENT_COLUMN_CONTENT, GTK_SORT_ASCENDING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (disc_content->priv->content), GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (disc_content->priv->content), TRUE);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (disc_content->priv->content));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  gtk_widget_show (disc_content->priv->content);
  gtk_container_add (GTK_CONTAINER (scrolled_window), disc_content->priv->content);

  column_file = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_file, _("Contents"));

  cell_icon = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column_file, cell_icon, FALSE);
  gtk_tree_view_column_set_attributes (column_file, cell_icon, "pixbuf", DISC_CONTENT_COLUMN_ICON, NULL);
  g_object_set (cell_icon, "xalign", 0.0, "ypad", 0, NULL);

  cell_file = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_file, cell_file, TRUE);
  gtk_tree_view_column_set_attributes (column_file, cell_file, "text", DISC_CONTENT_COLUMN_CONTENT, NULL);
  g_signal_connect (G_OBJECT (cell_file), "edited", G_CALLBACK (cell_file_edited_cb), disc_content);
  g_object_set (G_OBJECT (cell_file), "editable", TRUE, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (disc_content->priv->content), column_file);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (disc_content->priv->content), -1, _("Size"),
                                               gtk_cell_renderer_text_new (), "text", DISC_CONTENT_COLUMN_HUMANSIZE,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (disc_content->priv->content), -1, _("Local Path"),
                                               gtk_cell_renderer_text_new (), "text", DISC_CONTENT_COLUMN_PATH, NULL);

  g_signal_connect (G_OBJECT (disc_content->priv->content), "button-press-event",
                    G_CALLBACK (cb_treeview_button_pressed), disc_content);

#if 0                    
  /* adding progress window */
  disc_content->priv->progress = xfburn_adding_progress_new (); 
#endif
  
  /* disc usage */
  disc_content->priv->disc_usage = xfburn_disc_usage_new ();
  gtk_box_pack_start (GTK_BOX (disc_content), disc_content->priv->disc_usage, FALSE, FALSE, 5);
  gtk_widget_show (disc_content->priv->disc_usage);
  g_signal_connect (G_OBJECT (disc_content->priv->disc_usage), "begin-burn", G_CALLBACK (cb_begin_burn), disc_content);

  /* set up DnD */
  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (disc_content->priv->content), GDK_BUTTON1_MASK, gte_src,
                                          G_N_ELEMENTS (gte_src), GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (disc_content->priv->content), "drag-data-get", G_CALLBACK (content_drag_data_get_cb),
                    disc_content);
  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (disc_content->priv->content), gte_dest, G_N_ELEMENTS (gte_dest),
                                        GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (disc_content->priv->content), "drag-data-received", G_CALLBACK (content_drag_data_rcv_cb),
                    disc_content);
}

static void
xfburn_disc_content_finalize (GObject * object)
{
  XfburnDiscContent *cobj;
  cobj = XFBURN_DISC_CONTENT (object);

  g_free (cobj->priv);

  if (icon_directory) {
    g_object_unref (icon_directory);
    icon_directory = NULL;
  }
  if (icon_file) {
    g_object_unref (icon_file);
    icon_file = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* internals */
static void
cb_begin_burn (XfburnDiscUsage * du, XfburnDiscContent * dc)
{
  XfburnMainWindow *mainwin = xfburn_main_window_get_instance ();
  GtkWidget *dialog;
  gchar *tmpfile = NULL;
  
  generate_file_list (XFBURN_DISC_CONTENT (dc), &tmpfile);
  
  dialog = xfburn_burn_composition_dialog_new (tmpfile);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (mainwin));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  g_free (tmpfile);
}


static gboolean
cb_treeview_button_pressed (GtkTreeView * treeview, GdkEventButton * event, XfburnDiscContent * dc)
{
  if ((event->button == 3) && (event->type == GDK_BUTTON_PRESS)) {
    GtkTreeSelection *selection;
    GtkTreePath *path;
    GtkWidget *menu_popup;
    GtkWidget *menuitem_remove;
    GtkWidget *menuitem_rename;

    selection = gtk_tree_view_get_selection (treeview);

    if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
      gtk_tree_selection_unselect_all (selection);
      gtk_tree_selection_select_path (selection, path);
    }
    gtk_tree_path_free (path);

    menu_popup = gtk_ui_manager_get_widget (dc->priv->ui_manager, "/popup-menu");
    menuitem_remove = gtk_ui_manager_get_widget (dc->priv->ui_manager, "/popup-menu/remove-file");
    menuitem_rename = gtk_ui_manager_get_widget (dc->priv->ui_manager, "/popup-menu/rename-file");

    if (gtk_tree_selection_count_selected_rows (selection) >= 1)
      gtk_widget_set_sensitive (menuitem_remove, TRUE);
    else
      gtk_widget_set_sensitive (menuitem_remove, FALSE);
    if (gtk_tree_selection_count_selected_rows (selection) == 1)
      gtk_widget_set_sensitive (menuitem_rename, TRUE);
    else
      gtk_widget_set_sensitive (menuitem_rename, FALSE);

    gtk_menu_popup (GTK_MENU (menu_popup), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time ());
    return TRUE;
  }

  return FALSE;
}

static gint
directory_tree_sortfunc (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
  /* adapted from gnomebaker */
  gchar *aname, *bname, *apath, *bpath;
  gboolean aisdir = FALSE;
  gboolean bisdir = FALSE;
  gint result = 0;

  gtk_tree_model_get (model, a, DISC_CONTENT_COLUMN_CONTENT, &aname, DISC_CONTENT_COLUMN_PATH, &apath, -1);
  gtk_tree_model_get (model, b, DISC_CONTENT_COLUMN_CONTENT, &bname, DISC_CONTENT_COLUMN_PATH, &bpath, -1);

  aisdir = g_file_test (apath, G_FILE_TEST_IS_DIR);
  bisdir = g_file_test (bpath, G_FILE_TEST_IS_DIR);

  if (aisdir && !bisdir)
    result = -1;
  else if (!aisdir && bisdir)
    result = 1;
  else
    result = g_ascii_strcasecmp (aname, bname);

  g_free (aname);
  g_free (apath);
  g_free (bname);
  g_free (bpath);

  return result;
}

static gboolean
file_exists_on_same_level (GtkTreeModel * model, GtkTreePath * path, gboolean skip_path, const gchar *filename)
{
  GtkTreePath *current_path = NULL;
  GtkTreeIter current_iter;
  
  current_path = gtk_tree_path_copy (path);
  for (;gtk_tree_path_prev (current_path););
   
  if (!gtk_tree_model_get_iter (model, &current_iter, current_path)) {
    return FALSE;
  }
  
  do {
    gchar *current_filename = NULL;
    
    if (skip_path && gtk_tree_path_compare (path, current_path) == 0) {
      gtk_tree_path_next (current_path);
      continue;
    }

    gtk_tree_model_get (model, &current_iter, DISC_CONTENT_COLUMN_CONTENT, &current_filename, -1);
    if (g_ascii_strcasecmp (current_filename, filename) == 0) {
      g_free (current_filename);
      gtk_tree_path_free (current_path);
      return TRUE;
    }
    
    gtk_tree_path_next (current_path);
  } while (gtk_tree_model_iter_next (model, &current_iter));
  
  gtk_tree_path_free (current_path);
  return FALSE;
}

static void
cell_file_edited_cb (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnDiscContent * dc)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *real_path;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dc->priv->content));
  real_path = gtk_tree_path_new_from_string (path);

  if (gtk_tree_model_get_iter (model, &iter, real_path)) {
    if (file_exists_on_same_level (model, real_path, TRUE, newtext)) {
      xfce_err (_("A file with the same name is already present in the compilation"));
    }
    else {
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, DISC_CONTENT_COLUMN_CONTENT, newtext, -1);
    }
  }

  gtk_tree_path_free (real_path);
}

static void
disc_content_action_rename_selection (GtkAction * action, XfburnDiscContent * dc)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dc->priv->content));
  list = gtk_tree_selection_get_selected_rows (selection, &model);

  path = (GtkTreePath *) list->data;
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (dc->priv->content), DISC_CONTENT_COLUMN_CONTENT - 1);
  /* -1 because of COLUMN_ICON */

  gtk_tree_view_set_cursor (GTK_TREE_VIEW (dc->priv->content), path, column, TRUE);

  gtk_tree_path_free (path);
  g_list_free (list);
}

static void
disc_content_action_remove_selection (GtkAction * action, XfburnDiscContent * dc)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list_paths = NULL, *list_iters = NULL, *el;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dc->priv->content));
  list_paths = gtk_tree_selection_get_selected_rows (selection, &model);

  /* recompute new disc usage */
  el = list_paths;
  while (el) {
    GtkTreePath *path = NULL;
    GtkTreeIter *iter = NULL;
    GtkTreeIter parent, iter_temp;
    guint64 size = 0;

    path = (GtkTreePath *) el->data;
    iter = g_new0 (GtkTreeIter, 1);

    gtk_tree_model_get_iter (model, iter, path);
    list_iters = g_list_append (list_iters, iter);

    gtk_tree_model_get (model, iter, DISC_CONTENT_COLUMN_SIZE, &size, -1);
    xfburn_disc_usage_sub_size (XFBURN_DISC_USAGE (dc->priv->disc_usage), size);

    iter_temp = *iter;
    while (gtk_tree_model_iter_parent (model, &parent, &iter_temp)) {
      guint64 old_size;
      gchar *humansize = NULL;

      /* updates parent directories size */
      gtk_tree_model_get (model, &parent, DISC_CONTENT_COLUMN_SIZE, &old_size, -1);

      humansize = xfburn_humanreadable_filesize (old_size - size);
      gtk_tree_store_set (GTK_TREE_STORE (model), &parent, 
			  DISC_CONTENT_COLUMN_HUMANSIZE, humansize,
			  DISC_CONTENT_COLUMN_SIZE, old_size - size, -1);

      iter_temp = parent;

      g_free (humansize);
    }

    gtk_tree_path_free (path);
    el = g_list_next (el);
  }
  g_list_free (list_paths);

  /* remove rows from the list */
  el = list_iters;
  while (el) {
    GtkTreeIter *iter = NULL;

    iter = (GtkTreeIter *) el->data;
    gtk_tree_store_remove (GTK_TREE_STORE (model), iter);

    g_free (iter);
    el = g_list_next (el);
  }
  g_list_free (list_iters);

}

static void
disc_content_action_clear (GtkAction * action, XfburnDiscContent * content)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (content->priv->content));
  gtk_tree_store_clear (GTK_TREE_STORE (model));

  xfburn_disc_usage_set_size (XFBURN_DISC_USAGE (content->priv->disc_usage), 0);
}

static void
content_drag_data_get_cb (GtkWidget * widget, GdkDragContext * dc,
                          GtkSelectionData * data, guint info, guint time, XfburnDiscContent * content)
{
  if (info == DISC_CONTENT_DND_TARGET_INSIDE) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
    GtkTreeModel *model;
    GList *selected_rows;
    gchar *all_paths;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

    all_paths = g_strdup ("");
    selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
    selected_rows = g_list_last (selected_rows);

    while (selected_rows) {
      gchar *current_path;
      gchar *temp;

      current_path = gtk_tree_path_to_string ((GtkTreePath *) selected_rows->data);

      temp = g_strconcat (current_path, "\n", all_paths, NULL);
      g_free (current_path);
      g_free (all_paths);
      all_paths = temp;

      selected_rows = g_list_previous (selected_rows);
    }

    selected_rows = g_list_first (selected_rows);
    g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (selected_rows);

    gtk_selection_data_set (data, gdk_atom_intern ("XFBURN_TREE_PATHS", FALSE), 8, (const guchar *) all_paths,
                            sizeof (all_paths));

    g_free (all_paths);
  }
}

static gboolean
add_file_to_list_with_name (const gchar *name, XfburnDiscContent * dc, GtkTreeModel * model, const gchar * path,
                            GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position)
{
  struct stat s;

  if ((stat (path, &s) == 0)) {
    gchar *humansize = NULL;
    GtkTreeIter *parent = NULL;
    GtkTreePath *tree_path = NULL;

#if 0
    xfburn_adding_progress_pulse (XFBURN_ADDING_PROGRESS (dc->priv->progress));
#endif
    
    /* find parent */
    switch (position){
      case GTK_TREE_VIEW_DROP_BEFORE:
      case GTK_TREE_VIEW_DROP_AFTER:
      if (insertion) {
          GtkTreeIter iter_parent;
          
          if (gtk_tree_model_iter_parent (model, &iter_parent, insertion)) {
            parent = g_new0 (GtkTreeIter, 1);
            memcpy (parent, &iter_parent, sizeof (GtkTreeIter));
          }
        }
        break;
      case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
      case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
        parent = g_new0 (GtkTreeIter, 1);
        memcpy (parent, insertion, sizeof (GtkTreeIter));
        break;
    }
  
    /* check if the filename is valid */
    if (parent) {
      tree_path = gtk_tree_model_get_path (model, parent);
      gtk_tree_path_down (tree_path);
    } else {
      tree_path = gtk_tree_path_new_first ();
    }
    
    if (file_exists_on_same_level (model, tree_path, FALSE, name)) {
      xfce_err (_("A file with the same name is already present in the compilation"));

      gtk_tree_path_free (tree_path);
      g_free (parent);
      return FALSE;
    }
    gtk_tree_path_free (tree_path);
    
    /* new directory */
    if ((s.st_mode & S_IFDIR)) {
      GDir *dir = NULL;
      GError *error = NULL;
      const gchar *filename = NULL;
      guint64 total_size = s.st_size;

      dir = g_dir_open (path, 0, &error);
      if (!dir) {
        g_warning ("unable to open directory : %s", error->message);

        g_error_free (error);
        g_free (parent);
        
        return FALSE;
      }

      gtk_tree_store_append (GTK_TREE_STORE (model), iter, parent);

      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          DISC_CONTENT_COLUMN_ICON, icon_directory,
                          DISC_CONTENT_COLUMN_CONTENT, name,
                          DISC_CONTENT_COLUMN_PATH, path, DISC_CONTENT_COLUMN_TYPE, DISC_CONTENT_TYPE_DIRECTORY, 
			  DISC_CONTENT_COLUMN_SIZE, (guint64) s.st_size, -1);
      xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (dc->priv->disc_usage), s.st_size);

      while ((filename = g_dir_read_name (dir))) {
        GtkTreeIter new_iter;
        gchar *new_path = NULL;

        new_path = g_build_filename (path, filename, NULL);
        if (new_path) {
          guint64 size;

          add_file_to_list (dc, model, new_path, &new_iter, iter, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);

          gtk_tree_model_get (model, &new_iter, DISC_CONTENT_COLUMN_SIZE, &size, -1);
          total_size += size;

          g_free (new_path);
        }
      }

      humansize = xfburn_humanreadable_filesize (total_size);
      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          DISC_CONTENT_COLUMN_HUMANSIZE, humansize, DISC_CONTENT_COLUMN_SIZE, total_size, -1);

      g_dir_close (dir);
    }
    /* new file */
    else if ((s.st_mode & S_IFREG)) {
#ifdef HAVE_THUNAR_VFS
	  GdkScreen *screen;
	  GtkIconTheme *icon_theme;
	  ThunarVfsMimeDatabase *mime_database = NULL;
	  ThunarVfsMimeInfo *mime_info = NULL;
	  const gchar *mime_icon_name = NULL;
	  GdkPixbuf *mime_icon = NULL;
	  gint x,y;
	  
	  screen = gtk_widget_get_screen (GTK_WIDGET (dc));
	  icon_theme = gtk_icon_theme_get_for_screen (screen);
	  
	  mime_database = thunar_vfs_mime_database_get_default ();
	  mime_info = thunar_vfs_mime_database_get_info_for_file (mime_database, path, NULL);
		
	  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);
	  mime_icon_name = thunar_vfs_mime_info_lookup_icon_name (mime_info, icon_theme);
	  mime_icon = gtk_icon_theme_load_icon (icon_theme, mime_icon_name, x, 0, NULL);
#endif
	
      gtk_tree_store_append (GTK_TREE_STORE (model), iter, parent);

      humansize = xfburn_humanreadable_filesize (s.st_size);

#ifdef HAVE_THUNAR_VFS
      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          DISC_CONTENT_COLUMN_ICON, (G_IS_OBJECT (mime_icon) ? mime_icon : icon_file),
                          DISC_CONTENT_COLUMN_CONTENT, name,
                          DISC_CONTENT_COLUMN_HUMANSIZE, humansize,
                          DISC_CONTENT_COLUMN_SIZE, (guint64) s.st_size, DISC_CONTENT_COLUMN_PATH, path,
                          DISC_CONTENT_COLUMN_TYPE, DISC_CONTENT_TYPE_FILE, -1);
#else
      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          DISC_CONTENT_COLUMN_ICON, icon_file,
                          DISC_CONTENT_COLUMN_CONTENT, name,
                          DISC_CONTENT_COLUMN_HUMANSIZE, humansize,
                          DISC_CONTENT_COLUMN_SIZE, (guint64) s.st_size, DISC_CONTENT_COLUMN_PATH, path,
                          DISC_CONTENT_COLUMN_TYPE, DISC_CONTENT_TYPE_FILE, -1);
#endif

      xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (dc->priv->disc_usage), s.st_size);
#ifdef HAVE_THUNAR_VFS
	  if (G_LIKELY (G_IS_OBJECT (mime_icon)))
		g_object_unref (mime_icon);
	  thunar_vfs_mime_info_unref (mime_info);
	  g_object_unref (mime_database);
#endif
    }
    g_free (humansize);
    g_free (parent);

    return TRUE;
  }
  
  return FALSE;
}

static gboolean
add_file_to_list (XfburnDiscContent * dc, GtkTreeModel * model, const gchar * path, GtkTreeIter * iter,
                  GtkTreeIter * insertion, GtkTreeViewDropPosition position)
{
  struct stat s;
  gboolean ret = FALSE;
  
  if ((stat (path, &s) == 0)) {
    gchar *basename = NULL;
    
    basename = g_path_get_basename (path);
    
    ret = add_file_to_list_with_name (basename, dc, model, path, iter, insertion, position);
    
    g_free (basename);
  }

  return ret;
}

static void
content_drag_data_rcv_cb (GtkWidget * widget, GdkDragContext * dc, guint x, guint y, GtkSelectionData * sd,
                          guint info, guint t, XfburnDiscContent * content)
{
  GtkTreeModel *model;
  GtkTreePath *path_where_insert = NULL;
  GtkTreeViewDropPosition position;
  GtkTreeIter iter_where_insert;
  
  g_return_if_fail (sd);
  g_return_if_fail (sd->data);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
  
  gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget), x, y, &path_where_insert, &position);
  
  if (sd->target == gdk_atom_intern ("XFBURN_TREE_PATHS", FALSE)) {
    const gchar *str_path;

    DBG ("DnD");
    str_path = strtok ((gchar *) sd->data, "\n");
    while (str_path) {

      str_path = strtok (NULL, "\n");
    }

    gtk_drag_finish (dc, TRUE, FALSE, t);
  }
  else if (sd->target == gdk_atom_intern ("text/plain", FALSE)) {
    const gchar *file = NULL;

#if 0
    gtk_widget_show (content->priv->progress);
#endif
    
    file = strtok ((gchar *) sd->data, "\n");
    while (file) {
      GtkTreeIter iter;
      gchar *full_path;

      if (g_str_has_prefix (file, "file://"))
        full_path = g_build_filename (&file[7], NULL);
      else if (g_str_has_prefix ((gchar *) sd->data, "file:"))
        full_path = g_build_filename (&file[5], NULL);
      else
        full_path = g_build_filename (file, NULL);

      if (full_path[strlen (full_path) - 1] == '\r')
        full_path[strlen (full_path) - 1] = '\0';

      /* add files to the disc content */
      if (path_where_insert) {
        gtk_tree_model_get_iter (model, &iter_where_insert, path_where_insert);
        add_file_to_list (content, model, full_path, &iter, &iter_where_insert, position);
        
        if (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE 
            || position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
          gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), path_where_insert, FALSE);
        
        gtk_tree_path_free (path_where_insert);
      } else  {
        add_file_to_list (content, model, full_path, &iter, NULL, position);
      }
     
      g_free (full_path);

      file = strtok (NULL, "\n");
    }

#if 0
    gtk_widget_hide (content->priv->progress);
#endif
    gtk_drag_finish (dc, FALSE, (dc->action == GDK_ACTION_COPY), t);
  }
}

static gboolean
foreach_generate_file_list (GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, FILE * file)
{
  gchar *name;
  gchar *src;

  gtk_tree_model_get (model, iter, DISC_CONTENT_COLUMN_CONTENT, &name, DISC_CONTENT_COLUMN_PATH, &src, -1);

  fprintf (file, "%s=%s\n", name, src);

  g_free (name);
  g_free (src);
  return FALSE;
}

static gboolean
generate_file_list (XfburnDiscContent * dc, gchar ** tmpfile)
{
  GError *error = NULL;
  FILE *file_tmp;
  int fd_tmpfile;
  GtkTreeModel *model;

  fd_tmpfile = g_file_open_tmp ("xfburnXXXXXX", tmpfile, &error);
  if (error) {
    g_warning ("Unable to create temporary file: %s", error->message);
    g_error_free (error);
    return FALSE;
  }

  file_tmp = fdopen (fd_tmpfile, "w+");
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dc->priv->content));
  gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) foreach_generate_file_list, file_tmp);
  fclose (file_tmp);

  return TRUE;
}

/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_disc_content_new (void)
{
  return g_object_new (xfburn_disc_content_get_type (), NULL);
}

void
xfburn_disc_content_hide_toolbar (XfburnDiscContent * content)
{
  gtk_widget_hide (content->priv->toolbar);
}

void
xfburn_disc_content_show_toolbar (XfburnDiscContent * content)
{
  gtk_widget_show (content->priv->toolbar);
}

/****************/
/* loading code */
/****************/
typedef struct
{
  gboolean started;
  XfburnDiscContent *dc;
  GQueue *queue_iter;
} LoadParserStruct;

static gint
_find_attribute (const gchar ** attribute_names, const gchar * attr)
{
  gint i;

  for (i = 0; attribute_names[i]; i++) {
    if (!strcmp (attribute_names[i], attr))
      return i;
  }

  return -1;
}

static void
load_composition_start (GMarkupParseContext * context, const gchar * element_name,
                        const gchar ** attribute_names, const gchar ** attribute_values,
                        gpointer data, GError ** error)
{
  LoadParserStruct * parserinfo = (LoadParserStruct *) data;
  
  if (!(parserinfo->started) && !strcmp (element_name, "xfburn-composition"))
    parserinfo->started = TRUE;
  else if (!(parserinfo->started))
    return;

  if (!strcmp (element_name, "file")) {
    int i, j;

    if ((i = _find_attribute (attribute_names, "name")) != -1 &&
        (j = _find_attribute (attribute_names, "source")) != -1) {
      GtkTreeIter iter;
      GtkTreeIter *parent;
      GtkTreeModel *model;

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (parserinfo->dc->priv->content));
      parent = g_queue_peek_head (parserinfo->queue_iter);
          
      add_file_to_list_with_name (attribute_values[i], parserinfo->dc, model, attribute_values[j], &iter, 
                                  parent, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
    }
  } else if (!strcmp (element_name, "directory")) {
    int i, j;

    if ((i = _find_attribute (attribute_names, "name")) != -1 &&
        (j = _find_attribute (attribute_names, "source")) != -1) {
      //GtkTreeIter iter;
      GtkTreeModel *model;

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (parserinfo->dc->priv->content));
      
      //add_directory_to_list (attribute_values[i], parserinfo->dc, model, attribute_values[j], &iter, parent);    
    }
  }
}

static void
load_composition_end (GMarkupParseContext * context, const gchar * element_name, gpointer user_data, GError ** error)
{
  LoadParserStruct *parserinfo = (LoadParserStruct *) user_data;
  
  if (!parserinfo->started)
    return;
  
  if (!strcmp (element_name, "xfburn-composition"))
    parserinfo->started = FALSE;
  
  if (!strcmp (element_name, "directory"))
    parserinfo->queue_iter = g_queue_pop_head (parserinfo->queue_iter);
}

void
xfburn_disc_content_load_from_file (XfburnDiscContent * dc, const gchar * filename)
{
  gchar *file_contents = NULL;
  GMarkupParseContext *gpcontext = NULL;
  struct stat st;
  LoadParserStruct parserinfo;
  GMarkupParser gmparser = {
    load_composition_start, load_composition_end, NULL, NULL, NULL
  };
  GError *err = NULL;
#ifdef HAVE_MMAP
  gint fd = -1;
  void *maddr = NULL;
#endif
  g_return_if_fail (filename != NULL);
  if (stat (filename, &st) < 0) {
    g_warning ("Unable to open %s", filename);
    goto cleanup;
  }

#ifdef HAVE_MMAP
  fd = open (filename, O_RDONLY, 0);
  if (fd < 0)
    goto cleanup;
  maddr = mmap (NULL, st.st_size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  if (maddr)
    file_contents = maddr;
#endif
  if (!file_contents && !g_file_get_contents (filename, &file_contents, NULL, &err)) {
    if (err) {
      g_warning ("Unable to read file '%s' (%d): %s", filename, err->code, err->message);
      g_error_free (err);
    }
    goto cleanup;
  }

  parserinfo.started = FALSE;
  parserinfo.dc = dc;
  parserinfo.queue_iter = g_queue_new ();
  gpcontext = g_markup_parse_context_new (&gmparser, 0, &parserinfo, NULL);
  if (!g_markup_parse_context_parse (gpcontext, file_contents, st.st_size, &err)) {
    g_warning ("Error parsing composition (%d): %s", err->code, err->message);
    g_error_free (err);
    goto cleanup;
  }

  if (g_markup_parse_context_end_parse (gpcontext, NULL)) {
    DBG ("parsed");
  }

  g_queue_free (parserinfo.queue_iter);
  
cleanup:
  if (gpcontext)
    g_markup_parse_context_free (gpcontext);
#ifdef HAVE_MMAP
  if (maddr) {
    munmap (maddr, st.st_size);
    file_contents = NULL;
  }
  if (fd > -1)
    close (fd);
#endif
  if (file_contents)
    g_free (file_contents);
}


/***************/
/* saving code */
/***************/
typedef struct
{
  FILE *file_content;
  gint last_depth;
} CompositionSaveInfo;

static gboolean
foreach_save (GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, CompositionSaveInfo *info)
{
  gchar *space = NULL;
  gint i;
  gchar *name = NULL;
  gchar *source_path = NULL;
  DiscContentType type;

  space = g_strnfill (gtk_tree_path_get_depth (path), '\t');
  
  for (i = info->last_depth; i > gtk_tree_path_get_depth (path); i--) {
    gchar *space2 = NULL;

    space2 = g_strnfill (i - 1, '\t');
    fprintf (info->file_content, "%s</directory>\n", space2);
    
    g_free (space2);
  }
  
  gtk_tree_model_get (model, iter, DISC_CONTENT_COLUMN_CONTENT, &name,
                      DISC_CONTENT_COLUMN_PATH, &source_path,
                      DISC_CONTENT_COLUMN_TYPE, &type, -1);
  
  fprintf (info->file_content, "%s", space);
  switch (type) {
  case DISC_CONTENT_TYPE_FILE:
    fprintf (info->file_content, "<file name=\"%s\" source=\"%s\" />\n", name, source_path);
    break;
  case DISC_CONTENT_TYPE_DIRECTORY:
    fprintf (info->file_content, "<directory name=\"%s\" source=\"%s\"", name, source_path);
  
    if (gtk_tree_model_iter_has_child (model, iter))
      fprintf (info->file_content, ">\n");
    else
      fprintf (info->file_content, "/>\n");

    break;
  }


  info->last_depth = gtk_tree_path_get_depth (path);
  
  g_free (space);
  g_free (name);
  g_free (source_path);

  return FALSE;
}

void
xfburn_disc_content_save_to_file (XfburnDiscContent * dc, const gchar * filename)
{
  FILE *file_content;
  GtkTreeModel *model;
  CompositionSaveInfo info = {};
  gint i;
    
  file_content = fopen (filename, "w+");
  fprintf (file_content, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  fprintf (file_content, "<xfburn-composition version=\"0.1\">\n");

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dc->priv->content));
  info.file_content = file_content;
  info.last_depth = 0;
  gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) foreach_save, &info);

  for (i = info.last_depth; i > 1; i--) {
    gchar *space2 = NULL;

    space2 = g_strnfill (i - 1, '\t');
    fprintf (info.file_content, "%s</directory>\n", space2);
    
    g_free (space2);
  }
    
  fprintf (file_content, "</xfburn-composition>\n");
  fclose (file_content);
}

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
#include <time.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <gio/gio.h>

#include <exo/exo.h>

#include <libisofs.h>
#include <glib/gstdio.h>

#include "xfburn-data-composition.h"
#include "xfburn-global.h"

#include "xfburn-adding-progress.h"
#include "xfburn-composition.h"
#include "xfburn-burn-data-cd-composition-dialog.h"
#include "xfburn-burn-data-dvd-composition-dialog.h"
#include "xfburn-data-disc-usage.h"
#include "xfburn-main-window.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-main.h"

#define XFBURN_DATA_COMPOSITION_GET_PRIVATE(obj) (xfburn_data_composition_get_instance_private (XFBURN_DATA_COMPOSITION (obj)))

enum
{
  DATA_COMPOSITION_COLUMN_ICON,
  DATA_COMPOSITION_COLUMN_CONTENT,
  DATA_COMPOSITION_COLUMN_HUMANSIZE,
  DATA_COMPOSITION_COLUMN_SIZE,
  DATA_COMPOSITION_COLUMN_PATH,
  DATA_COMPOSITION_COLUMN_TYPE,
  DATA_COMPOSITION_N_COLUMNS
};

typedef enum
{
  DATA_COMPOSITION_TYPE_FILE,
  DATA_COMPOSITION_TYPE_DIRECTORY
} DataCompositionEntryType;


/* thread parameters */
typedef struct {
  GSList * filelist;
  XfburnDataComposition *dc;
} ThreadAddFilesCLIParams;

typedef struct {
  XfburnDataComposition *dc;
  GtkTreeModel *model;
  GtkTreeIter iter_where_insert;
  DataCompositionEntryType type;
} ThreadAddFilesActionParams;

typedef struct {
  XfburnDataComposition *composition;
  DataCompositionEntryType type;
  GtkWidget *widget;
  GtkTreeViewDropPosition position;
  GtkTreeIter iter_dummy;
} ThreadAddFilesDragParams;

/* prototypes */
static void composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data);
static void xfburn_data_composition_finalize (GObject * object);

static void show_custom_controls (XfburnComposition *composition);
static void hide_custom_controls (XfburnComposition *composition);
static void load_from_file (XfburnComposition *composition, const gchar *file);
static void save_to_file (XfburnComposition *composition);

static gint directory_tree_sortfunc (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data);

static void set_default_name (XfburnDataComposition * dc);
static gboolean has_default_name (XfburnDataComposition * dc);

static void action_create_directory (GSimpleAction *, GVariant *, XfburnDataComposition *);
static void action_clear (GSimpleAction *, GVariant *, XfburnDataComposition *);
static void action_remove_selection (GSimpleAction *, GVariant *, XfburnDataComposition *);
static void action_rename_selection (GSimpleAction *, GVariant *, XfburnDataComposition *);
static void action_add_or_select (GSimpleAction *, GVariant *, XfburnDataComposition *);
static void add_files (gchar * files, XfburnDataComposition *);
static void add_cb (GtkWidget * widget, gpointer data);

static gboolean cb_treeview_button_pressed (GtkTreeView * treeview, GdkEventButton * event, XfburnDataComposition * dc);
static void cb_treeview_row_activated (GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * column, XfburnDataComposition * composition);
static void cb_selection_changed (GtkTreeSelection *selection, XfburnDataComposition * dc);
static void cb_begin_burn (XfburnDataDiscUsage * du, XfburnDataComposition * dc);
static void cb_cell_file_edited (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnDataComposition * dc);

static void cb_content_drag_data_rcv (GtkWidget * widget, GdkDragContext * dc, guint x, guint y, GtkSelectionData * sd,
                                      guint info, guint t, XfburnDataComposition * composition);
static void cb_content_drag_data_get (GtkWidget * widget, GdkDragContext * dc, GtkSelectionData * data, guint info,
                                      guint timestamp, XfburnDataComposition * content);
static void cb_adding_done (XfburnAddingProgress *progress, XfburnDataComposition *dc);

/* thread entry points */
static gpointer thread_add_files_cli (ThreadAddFilesCLIParams *params);
static gpointer thread_add_files_action (ThreadAddFilesActionParams *params);
static gpointer thread_add_files_drag (ThreadAddFilesDragParams *params);

/* thread helpers */
static gboolean thread_add_file_to_list_with_name (const gchar *name, XfburnDataComposition * dc,
                                                   GtkTreeModel * model, const gchar * path, GtkTreeIter * iter,
                                                   GtkTreeIter * insertion, GtkTreeViewDropPosition position);
static gboolean thread_add_file_to_list (XfburnDataComposition * dc, GtkTreeModel * model, const gchar * path,
                                         GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position);
static void concat_free (char * msg, char ** combined_msg);
static IsoImage * generate_iso_image (XfburnDataComposition * dc);
static gboolean show_add_home_question_dialog (void);

typedef struct
{
  gchar *filename;
  gboolean modified;

  guint n_new_directory;

  GList *full_paths_to_add;
  gchar *selected_files;
  GtkTreePath *path_where_insert;

  GdkDragContext * dc;
  gboolean success;
  gboolean del;
  guint32 time;

  void *thread_params;

  GSimpleActionGroup *action_group;
  GtkBuilder *ui_manager;

  GtkWidget *toolbar;
  GtkWidget *entry_volume_name;
  GtkWidget *content;
  GtkWidget *disc_usage;
  GtkWidget *progress;
  GtkTreeStore *model;
  GtkWidget *add_filechooser;
  gchar * last_directory;
  GtkWidget *add_window;

  gchar *default_vol_name;

  gboolean large_files;
} XfburnDataCompositionPrivate;

/* globals */
static GtkHPanedClass *parent_class = NULL;
static guint instances = 0;

static const GActionEntry action_entries[] = {
  {.name = "add-file", .activate = (gActionCallback)action_add_or_select},
  {.name = "create-dir", .activate = (gActionCallback)action_create_directory},
  {.name = "remove-file", .activate = (gActionCallback)action_remove_selection},
  {.name = "clear", .activate = (gActionCallback)action_clear},
  /*{.name = "import-session", .activate = (gActionCallback)action_remove_selection},*/
  {.name = "rename-file", .activate = (gActionCallback)action_rename_selection},
};
/*
static const GtkActionEntry action_entries[] = {
  {"add-file", "list-add", N_("Add"), NULL, N_("Add the selected file(s) to the composition"),
   G_CALLBACK (action_add_or_select),},
  {"create-dir", "document-new", N_("Create directory"), NULL, N_("Add a new directory to the composition"),
   G_CALLBACK (action_create_directory),},
  {"remove-file", "list-remove", N_("Remove"), NULL, N_("Remove the selected file(s) from the composition"),
   G_CALLBACK (action_remove_selection),},
  {"clear", "edit-clear", N_("Clear"), NULL, N_("Clear the content of the composition"),
   G_CALLBACK (action_clear),},
  /*{"import-session", "xfburn-import-session", N_("Import"), NULL, N_("Import existing session"),}, */
/*  {"rename-file", "gtk-edit", N_("Rename"), NULL, N_("Rename the selected file"),
   G_CALLBACK (action_rename_selection),},
};*/

static const gchar *toolbar_actions[] = {
  "add-file",
  "remove-file",
  "create-dir",
  "clear",
  "import-session",
};

static GdkPixbuf *icon_directory = NULL, *icon_file = NULL;

/***************************/
/* XfburnDataComposition class */
/***************************/
G_DEFINE_TYPE_EXTENDED(
  XfburnDataComposition,
  xfburn_data_composition,
  GTK_TYPE_BOX,
  0,
  G_ADD_PRIVATE (XfburnDataComposition)
  G_IMPLEMENT_INTERFACE (XFBURN_TYPE_COMPOSITION, composition_interface_init)
);


static void
xfburn_data_composition_class_init (XfburnDataCompositionClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_data_composition_finalize;
}

static void
composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data)
{
  composition->show_custom_controls = show_custom_controls;
  composition->hide_custom_controls = hide_custom_controls;
  composition->load = load_from_file;
  composition->save = save_to_file;
}

static void
xfburn_data_composition_init (XfburnDataComposition * composition)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (composition);

  gtk_orientable_set_orientation(GTK_ORIENTABLE (composition), GTK_ORIENTATION_VERTICAL);

  gint x, y;
//  ExoToolbarsModel *model_toolbar;
  gint toolbar_position;
  GtkWidget *hbox_toolbar;
  GtkWidget *hbox, *label;
  GtkWidget *scrolled_window;
  GtkTreeStore *model;
  GtkTreeViewColumn *column_file;
  GtkCellRenderer *cell_icon, *cell_file;
  GtkTreeSelection *selection;
  GAction *action = NULL;
  GdkScreen *screen;
  GtkIconTheme *icon_theme;

  GtkTargetEntry gte_src[] =  { { "XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, DATA_COMPOSITION_DND_TARGET_INSIDE } };
  GtkTargetEntry gte_dest[] = { { "XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, DATA_COMPOSITION_DND_TARGET_INSIDE },
                                { "text/uri-list", 0, DATA_COMPOSITION_DND_TARGET_TEXT_URI_LIST },
                                { "text/plain;charset=utf-8", 0, DATA_COMPOSITION_DND_TARGET_TEXT_PLAIN },
                              };

  priv->full_paths_to_add = NULL;

  instances++;

  /* initialize static members */
  screen = gtk_widget_get_screen (GTK_WIDGET (composition));
  icon_theme = gtk_icon_theme_get_for_screen (screen);

  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);
  if (!icon_directory)
    icon_directory = gtk_icon_theme_load_icon (icon_theme, "folder", x, 0, NULL);
  if (!icon_file)
    icon_file = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-regular", x, 0, NULL);

  /* create ui manager */
  priv->action_group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (priv->action_group), action_entries, G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (composition));

  priv->ui_manager = gtk_builder_new ();
  gtk_builder_set_translation_domain(priv->ui_manager, GETTEXT_PACKAGE);

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  gchar *popup_ui = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfburn/xfburn-popup-menus.ui");
  gtk_builder_add_from_file (priv->ui_manager, popup_ui, NULL);

  hbox_toolbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start (GTK_BOX (composition), hbox_toolbar, FALSE, TRUE, 0);
  gtk_widget_show (hbox_toolbar);

  /* toolbar */
/*  model_toolbar = exo_toolbars_model_new ();
  exo_toolbars_model_set_actions (model_toolbar, (gchar **) toolbar_actions, G_N_ELEMENTS (toolbar_actions));
  toolbar_position = exo_toolbars_model_add_toolbar (model_toolbar, -1, "content-toolbar");
  exo_toolbars_model_set_style (model_toolbar, GTK_TOOLBAR_BOTH, toolbar_position);

  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "add-file", EXO_TOOLBARS_ITEM_TYPE);
  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "create-dir", EXO_TOOLBARS_ITEM_TYPE);
  exo_toolbars_model_add_separator (model_toolbar, toolbar_position, -1);
  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "remove-file", EXO_TOOLBARS_ITEM_TYPE);
  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "clear", EXO_TOOLBARS_ITEM_TYPE);
  //exo_toolbars_model_add_separator (model_toolbar, toolbar_position, -1);
  //exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "import-session", EXO_TOOLBARS_ITEM_TYPE);

  priv->toolbar = exo_toolbars_view_new_with_model (priv->ui_manager, model_toolbar);
*/
  priv->toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_style(GTK_TOOLBAR (priv->toolbar), GTK_TOOLBAR_BOTH);
  gtk_widget_insert_action_group (priv->toolbar, "win", G_ACTION_GROUP (priv->action_group));

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "list-add", _("Add"), "win.add-file", _("Add the selected file(s) to the composition"));
  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "folder-new", _("Create directory"), "win.create-dir", _("Add a new directory to the composition"));

  gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), gtk_separator_tool_item_new(), -1);

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "list-remove", _("Remove"), "win.remove-file", _("Remove the selected file(s) from the composition"));
  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "edit-clear", _("Clear"), "win.clear", _("Clear the content of the composition"));

  gtk_box_pack_start (GTK_BOX (hbox_toolbar), priv->toolbar, TRUE, TRUE, 0);
  gtk_widget_show_all (priv->toolbar);

  /* volume name */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_box_pack_start (GTK_BOX (hbox_toolbar), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Volume name :"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  priv->entry_volume_name = gtk_entry_new ();

  set_default_name (composition);

  gtk_box_pack_start (GTK_BOX (hbox), priv->entry_volume_name, FALSE, FALSE, 0);
  gtk_widget_show (priv->entry_volume_name);

  /* content treeview */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (composition), scrolled_window, TRUE, TRUE, 0);

  priv->content = exo_tree_view_new ();
  model = gtk_tree_store_new (DATA_COMPOSITION_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_UINT);
  priv->model = model;

  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), DATA_COMPOSITION_COLUMN_CONTENT,
                                   directory_tree_sortfunc, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), DATA_COMPOSITION_COLUMN_CONTENT, GTK_SORT_ASCENDING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->content), GTK_TREE_MODEL (model));
  // gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->content), TRUE);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  gtk_widget_show (priv->content);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->content);

  column_file = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_file, _("Contents"));

  cell_icon = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column_file, cell_icon, FALSE);
  gtk_tree_view_column_set_attributes (column_file, cell_icon, "pixbuf", DATA_COMPOSITION_COLUMN_ICON, NULL);
  g_object_set (cell_icon, "xalign", 0.0, "ypad", 0, NULL);

  cell_file = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_file, cell_file, TRUE);
  gtk_tree_view_column_set_attributes (column_file, cell_file, "text", DATA_COMPOSITION_COLUMN_CONTENT, NULL);
  g_signal_connect (G_OBJECT (cell_file), "edited", G_CALLBACK (cb_cell_file_edited), composition);
  g_object_set (G_OBJECT (cell_file), "editable", TRUE, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->content), column_file);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Size"),
                                               gtk_cell_renderer_text_new (), "text", DATA_COMPOSITION_COLUMN_HUMANSIZE,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Local Path"),
                                               gtk_cell_renderer_text_new (), "text", DATA_COMPOSITION_COLUMN_PATH, NULL);

  /* Contents column */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), 0), 1);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), 0), 200);
  /* Size (HUMANSIZE) column */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), 1), 1);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), 1), 60);
  /* Local Path (PATH) column */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), 2), 1);


  g_signal_connect (G_OBJECT (priv->content), "row-activated", G_CALLBACK (cb_treeview_row_activated), composition);
  g_signal_connect (G_OBJECT (priv->content), "button-press-event",
                    G_CALLBACK (cb_treeview_button_pressed), composition);
  g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (cb_selection_changed), composition);

  /* adding progress window */
  priv->progress = GTK_WIDGET (xfburn_adding_progress_new ());
  g_signal_connect (G_OBJECT (priv->progress), "adding-done", G_CALLBACK (cb_adding_done), composition);
  gtk_window_set_transient_for (GTK_WINDOW (priv->progress), GTK_WINDOW (xfburn_main_window_get_instance ()));
  /* FIXME: progress should have a busy cursor */

  /* disc usage */
  priv->disc_usage = xfburn_data_disc_usage_new ();
  gtk_box_pack_start (GTK_BOX (composition), priv->disc_usage, FALSE, FALSE, 5);
  gtk_widget_show (priv->disc_usage);
  g_signal_connect (G_OBJECT (priv->disc_usage), "begin-burn", G_CALLBACK (cb_begin_burn), composition);

  /* set up DnD */
  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (priv->content), GDK_BUTTON1_MASK, gte_src,
                                          G_N_ELEMENTS (gte_src), GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (priv->content), "drag-data-get", G_CALLBACK (cb_content_drag_data_get),
                    composition);
  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (priv->content), gte_dest, G_N_ELEMENTS (gte_dest),
                                        GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (priv->content), "drag-data-received", G_CALLBACK (cb_content_drag_data_rcv),
                    composition);

  action = g_action_map_lookup_action (G_ACTION_MAP (priv->action_group), "remove-file");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
}

static void
xfburn_data_composition_finalize (GObject * object)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (object);

  g_free (priv->filename);

  /* free static members */
  instances--;
  if (instances == 0) {
    if (icon_directory) {
      g_object_unref (icon_directory);
      icon_directory = NULL;
    }
    if (icon_file) {
      g_object_unref (icon_file);
      icon_file = NULL;
    }
  }

  g_object_unref (priv->model);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*************/
/* internals */
/*************/
static void
show_custom_controls (XfburnComposition *composition)
{
  DBG ("show");
}

static void
hide_custom_controls (XfburnComposition *composition)
{
  DBG ("hide");
}

static void
cb_begin_burn (XfburnDataDiscUsage * du, XfburnDataComposition * dc)
{
  XfburnMainWindow *mainwin = xfburn_main_window_get_instance ();
  GtkWidget *dialog = NULL;
  IsoImage *image = NULL;

  if (!iso_init()) {
    g_critical ("Could not initialize libisofs!");
    return;
  }

  image = generate_iso_image (XFBURN_DATA_COMPOSITION (dc));

  switch (xfburn_disc_usage_get_disc_type (XFBURN_DISC_USAGE (du))) {
  case CD_DISC:
    dialog = xfburn_burn_data_cd_composition_dialog_new (image, has_default_name (dc));
    break;
  case DVD_DISC:
    dialog = xfburn_burn_data_dvd_composition_dialog_new (image);
    break;
  }

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (mainwin));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
cb_treeview_row_activated (GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * column,
                         XfburnDataComposition * composition)
{
  gtk_tree_view_expand_row (treeview, path, FALSE);
}

static void
cb_selection_changed (GtkTreeSelection *selection, XfburnDataComposition * dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);
  gint n_selected_rows;
  GAction* action[] = {
    g_action_map_lookup_action (G_ACTION_MAP (priv->action_group), "add-file"),
    g_action_map_lookup_action (G_ACTION_MAP (priv->action_group), "create-dir"),
    g_action_map_lookup_action (G_ACTION_MAP (priv->action_group), "remove-file"),
  };

  n_selected_rows = gtk_tree_selection_count_selected_rows (selection);
  if (n_selected_rows == 0) {
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[0]), TRUE);
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[1]), TRUE);
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[2]), FALSE);
  } else if (n_selected_rows == 1) {
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[0]), TRUE);
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[1]), TRUE);
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[2]), TRUE);
  } else {
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[0]), FALSE);
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[1]), FALSE);
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action[2]), TRUE);
  }
}

static gboolean
cb_treeview_button_pressed (GtkTreeView * treeview, GdkEventButton * event, XfburnDataComposition * dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  if ((event->button == 1) && (event->type == GDK_BUTTON_PRESS)) {
    GtkTreePath *path;

    if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
      gtk_tree_path_free (path);
    } else {
      GtkTreeSelection *selection;

      selection = gtk_tree_view_get_selection (treeview);
      gtk_tree_selection_unselect_all (selection);
    }

    return FALSE;
  }

  if ((event->button == 3) && (event->type == GDK_BUTTON_PRESS)) {
    GtkTreeSelection *selection;
    GtkTreePath *path;
    GMenuModel *model;
    GtkWidget *menu_popup;
    GtkWidget *menuitem_remove;
    GtkWidget *menuitem_rename;

    selection = gtk_tree_view_get_selection (treeview);

    if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
      gtk_tree_selection_unselect_all (selection);
      gtk_tree_selection_select_path (selection, path);
      gtk_tree_path_free (path);
    }

    model = G_MENU_MODEL (gtk_builder_get_object (priv->ui_manager, "data-popup-menu"));
    menu_popup = gtk_menu_new_from_model (model);
    gtk_widget_insert_action_group(GTK_WIDGET(menu_popup), "win", G_ACTION_GROUP(priv->action_group));

    GList *childs = gtk_container_get_children (GTK_CONTAINER (menu_popup));
    menuitem_remove = GTK_WIDGET (childs->next->next->data);
    menuitem_rename = GTK_WIDGET (childs->next->data);

    if (gtk_tree_selection_count_selected_rows (selection) >= 1)
      gtk_widget_set_sensitive (menuitem_remove, TRUE);
    else
      gtk_widget_set_sensitive (menuitem_remove, FALSE);
    if (gtk_tree_selection_count_selected_rows (selection) == 1)
      gtk_widget_set_sensitive (menuitem_rename, TRUE);
    else
      gtk_widget_set_sensitive (menuitem_rename, FALSE);

    GdkRectangle r = {event->x, event->y, 1, 1};
    gtk_menu_popup_at_rect (GTK_MENU (menu_popup),
        gtk_widget_get_parent_window (GTK_WIDGET (treeview)),
        &r, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
    return TRUE;
  }

  return FALSE;
}

static gint
directory_tree_sortfunc (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
  /* adapted from gnomebaker */
  gchar *aname, *bname;
  DataCompositionEntryType atype = -1, btype = -1;
  gint result = 0;

  gtk_tree_model_get (model, a, DATA_COMPOSITION_COLUMN_CONTENT, &aname, DATA_COMPOSITION_COLUMN_TYPE, &atype, -1);
  gtk_tree_model_get (model, b, DATA_COMPOSITION_COLUMN_CONTENT, &bname, DATA_COMPOSITION_COLUMN_TYPE, &btype, -1);

  if ( (atype == DATA_COMPOSITION_TYPE_DIRECTORY) && (btype != DATA_COMPOSITION_TYPE_DIRECTORY) )
    result = -1;
  else if ( (atype != DATA_COMPOSITION_TYPE_DIRECTORY) && (btype == DATA_COMPOSITION_TYPE_DIRECTORY) )
    result = 1;
  else
    result = g_ascii_strcasecmp (aname, bname);

  g_free (aname);
  g_free (bname);

  return result;
}

static gboolean
file_exists_on_same_level (GtkTreeModel * model, GtkTreePath * path, gboolean skip_path, const gchar *filename)
{
  GtkTreePath *current_path = NULL;
  GtkTreeIter current_iter;

  current_path = gtk_tree_path_copy (path);
  for (;gtk_tree_path_prev (current_path););

  if (gtk_tree_model_get_iter (model, &current_iter, current_path)) {
    do {
      gchar *current_filename = NULL;

      if (skip_path && gtk_tree_path_compare (path, current_path) == 0) {
        gtk_tree_path_next (current_path);
        continue;
      }

      gtk_tree_model_get (model, &current_iter, DATA_COMPOSITION_COLUMN_CONTENT, &current_filename, -1);
      if (strcmp (current_filename, filename) == 0) {
        g_free (current_filename);
        gtk_tree_path_free (current_path);
        return TRUE;
      }

      g_free (current_filename);
      gtk_tree_path_next (current_path);
    } while (gtk_tree_model_iter_next (model, &current_iter));
  }

  gtk_tree_path_free (current_path);
  return FALSE;
}

static void
cb_cell_file_edited (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnDataComposition * dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *real_path;

  if (strlen (newtext) == 0) {
    xfce_dialog_show_error (NULL, NULL, _("You must give a name to the file."));
    return;
  }

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  real_path = gtk_tree_path_new_from_string (path);

  if (gtk_tree_model_get_iter (model, &iter, real_path)) {
    if (file_exists_on_same_level (model, real_path, TRUE, newtext)) {
      xfce_dialog_show_error (NULL, NULL, _("A file with the same name is already present in the composition."));
    }
    else {
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, DATA_COMPOSITION_COLUMN_CONTENT, newtext, -1);
    }
  }

  gtk_tree_path_free (real_path);
}

static void
cb_adding_done (XfburnAddingProgress *progress, XfburnDataComposition *dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  gtk_widget_hide (priv->progress);

  if (priv->selected_files) {
    g_free (priv->selected_files);
    priv->selected_files = NULL;
  }

  if (priv->path_where_insert) {
    gtk_tree_path_free (priv->path_where_insert);
    priv->path_where_insert = NULL;
  }

  if (priv->full_paths_to_add) {
    g_list_free_full (priv->full_paths_to_add, (GDestroyNotify) g_free);
    priv->full_paths_to_add = NULL;
  }

  g_free (priv->thread_params);
  xfburn_default_cursor (priv->content);
}

static void
action_rename_selection (GSimpleAction *action, GVariant *param, XfburnDataComposition *dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  list = gtk_tree_selection_get_selected_rows (selection, &model);

  path = (GtkTreePath *) list->data;
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), DATA_COMPOSITION_COLUMN_CONTENT - 1);
  /* -1 because of COLUMN_ICON */

  gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->content), path, column, TRUE);

  gtk_tree_path_free (path);
  g_list_free (list);
}

static void
action_create_directory (GSimpleAction *action, GVariant *param, XfburnDataComposition *dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GList *selected_paths = NULL;
  GtkTreePath *path_where_insert = NULL;
  GtkTreeIter iter_where_insert, iter_directory;
  DataCompositionEntryType type = -1;
  gchar *humansize = NULL;

  GtkTreePath *inserted_path = NULL;
  gchar *directory_text = NULL;

  GtkTreeViewColumn *column;
  GtkTreePath *path = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  selected_paths = gtk_tree_selection_get_selected_rows (selection, NULL);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));

  if (selected_paths) {
    path_where_insert = (GtkTreePath *) (selected_paths->data);

    gtk_tree_model_get_iter (model, &iter_where_insert, path_where_insert);
    gtk_tree_model_get (model, &iter_where_insert, DATA_COMPOSITION_COLUMN_TYPE, &type, -1);
  }

  if (type == DATA_COMPOSITION_TYPE_DIRECTORY) {
    gtk_tree_store_append (GTK_TREE_STORE (model), &iter_directory, &iter_where_insert);
    gtk_tree_view_expand_row (GTK_TREE_VIEW (priv->content), path_where_insert, FALSE);
  } else if (type == DATA_COMPOSITION_TYPE_FILE) {
    GtkTreeIter parent;

    if (gtk_tree_model_iter_parent (model, &parent, &iter_where_insert))
      gtk_tree_store_append (GTK_TREE_STORE (model), &iter_directory, &parent);
    else
      gtk_tree_store_append (GTK_TREE_STORE (model), &iter_directory, NULL);
  } else {
    gtk_tree_store_append (GTK_TREE_STORE (model), &iter_directory, NULL);
  }

  humansize = xfburn_humanreadable_filesize (4);

  inserted_path = gtk_tree_model_get_path (model, &iter_directory);
  if (file_exists_on_same_level (model, inserted_path, TRUE, _("New directory")))
    directory_text = g_strdup_printf ("%s %d", _("New directory"), ++(priv->n_new_directory));
  else
    directory_text = g_strdup (_("New directory"));
  gtk_tree_path_free (inserted_path);

  gtk_tree_store_set (GTK_TREE_STORE (model), &iter_directory,
                      DATA_COMPOSITION_COLUMN_ICON, icon_directory,
                      DATA_COMPOSITION_COLUMN_CONTENT, directory_text,
                      DATA_COMPOSITION_COLUMN_HUMANSIZE, humansize,
                      DATA_COMPOSITION_COLUMN_SIZE, (guint64) 4,
                      DATA_COMPOSITION_COLUMN_TYPE, DATA_COMPOSITION_TYPE_DIRECTORY, -1);
  g_free (directory_text);
  g_free (humansize);

  xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (priv->disc_usage), 4);

  gtk_widget_realize (priv->content);

  /* put the cell renderer in edition mode */
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), DATA_COMPOSITION_COLUMN_CONTENT - 1);
  /* -1 because of COLUMN_ICON */
  path = gtk_tree_model_get_path (model, &iter_directory);

  gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->content), path, column, TRUE);
  gtk_tree_path_free (path);
}

static void
remove_row_reference (GtkTreeRowReference *reference, XfburnDataCompositionPrivate *priv)
{
  GtkTreePath *path = NULL;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));

  path = gtk_tree_row_reference_get_path (reference);
  if (path) {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter (model, &iter, path)) {
      GtkTreeIter parent, iter_temp;
      guint64 size = 0;

      gtk_tree_model_get (model, &iter, DATA_COMPOSITION_COLUMN_SIZE, &size, -1);
      xfburn_disc_usage_sub_size (XFBURN_DISC_USAGE (priv->disc_usage), size);

      iter_temp = iter;
      while (gtk_tree_model_iter_parent (model, &parent, &iter_temp)) {
        guint64 old_size;
        gchar *humansize = NULL;

        /* updates parent directories size */
        gtk_tree_model_get (model, &parent, DATA_COMPOSITION_COLUMN_SIZE, &old_size, -1);

        humansize = xfburn_humanreadable_filesize (old_size - size);
        gtk_tree_store_set (GTK_TREE_STORE (model), &parent,
                            DATA_COMPOSITION_COLUMN_HUMANSIZE, humansize,
                            DATA_COMPOSITION_COLUMN_SIZE, old_size - size, -1);

        iter_temp = parent;

        g_free (humansize);
      }

      gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
    }

    gtk_tree_path_free (path);
  }

  gtk_tree_row_reference_free (reference);
}

static void
action_remove_selection (GSimpleAction *action, GVariant *param, XfburnDataComposition *dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list_paths = NULL, *el;
  GList *references = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  list_paths = gtk_tree_selection_get_selected_rows (selection, &model);

  el = list_paths;
  while (el) {
    GtkTreePath *path = NULL;
    GtkTreeRowReference *reference = NULL;

    path = (GtkTreePath *) el->data;
    reference = gtk_tree_row_reference_new (model, path);
    gtk_tree_path_free (path);

    if (reference)
      references = g_list_prepend (references, reference);

    el = g_list_next (el);
  }
  g_list_free (list_paths);

  g_list_foreach (references, (GFunc) remove_row_reference, priv);
  g_list_free (references);
}

static void
add_cb (GtkWidget * widget, gpointer data)
{
    XfburnDataComposition * dc = XFBURN_DATA_COMPOSITION(data);
    XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);
    gchar *selected_files = NULL;

    GSList *list = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (priv->add_filechooser));
    GString  * str = g_string_new(NULL);
    GSList * curr;

    priv->last_directory = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(priv->add_filechooser));

    for (curr = list; curr!=NULL; curr = curr->next) {
        g_string_append(str, curr->data);
        g_string_append_c(str, '\n');;
    }

    g_slist_free_full (list, g_free);
    selected_files = str->str;
    g_string_free (str, FALSE);
    DBG("selected  files: %s ", selected_files);

    gtk_widget_destroy (priv->add_window);

    add_files (selected_files, dc);
}

static void
select_files (XfburnDataComposition * dc)
{
    XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

    GtkWidget * window;
    GtkWidget * add_button;
    GtkWidget * vbox;
    GtkWidget * bbox;

    priv->add_window = window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title (GTK_WINDOW(window), _("File(s) to add to composition"));
    gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER); // GTK_WINDOW(xfburn_main_window_get_instance()),
    gtk_container_set_border_width (GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    priv->add_filechooser = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(priv->add_filechooser), TRUE);

    if(xfburn_main_has_initial_dir ()) {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(priv->add_filechooser), xfburn_main_get_initial_dir ());
    }

    if (priv->last_directory)
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(priv->add_filechooser), priv->last_directory);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_box_pack_start(GTK_BOX(vbox), priv->add_filechooser, TRUE, TRUE, 3);

    add_button = gtk_button_new_with_label (_("Add"));
    g_signal_connect (add_button, "clicked", G_CALLBACK(add_cb), dc);
    g_signal_connect (priv->add_filechooser, "file-activated", G_CALLBACK(add_cb), dc);

    bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, FALSE, 3);

    gtk_box_pack_end(GTK_BOX(bbox), add_button, FALSE, FALSE, 3);

    g_signal_connect(window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &window);

    gtk_widget_show_all (window);
}

static void
action_add_or_select (GSimpleAction *action, GVariant *param, XfburnDataComposition *dc)
{
  if (xfburn_settings_get_boolean("show-filebrowser", FALSE)) {
    XfburnFileBrowser *browser = xfburn_main_window_get_file_browser (xfburn_main_window_get_instance ());

    add_files (xfburn_file_browser_get_selection (browser), dc);
  } else {
    select_files(dc);
  }
}

static void
add_files(gchar * selected_files, XfburnDataComposition *dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  xfburn_busy_cursor (priv->content);
  if (selected_files) {
    GtkTreeSelection *selection;
    GList *selected_paths = NULL;
    ThreadAddFilesActionParams *params;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));

    params = g_new (ThreadAddFilesActionParams, 1);
    params->dc = dc;
    params->type = -1;
    priv->path_where_insert = NULL;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
    selected_paths = gtk_tree_selection_get_selected_rows (selection, NULL);
    params->model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));

    if (selected_paths) {
      priv->path_where_insert = gtk_tree_path_copy ((GtkTreePath *) (selected_paths->data));

      gtk_tree_model_get_iter (params->model, &params->iter_where_insert, priv->path_where_insert);
      gtk_tree_model_get (params->model, &params->iter_where_insert, DATA_COMPOSITION_COLUMN_TYPE, &params->type, -1);
    }

    priv->selected_files = selected_files;

    priv->thread_params = params;
    g_thread_new ("data_add_files", (GThreadFunc) thread_add_files_action, params);

    g_list_free_full (selected_paths, (GDestroyNotify) gtk_tree_path_free);
  }
}

static void
set_default_name (XfburnDataComposition * dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  char timestr[80];
  struct tm *today;
  time_t tm;
  /* FIXME: put i into the class? */
  static int i = 0;

  tm = time (NULL);
  today = localtime (&tm);

  if (priv->default_vol_name)
    g_free (priv->default_vol_name);

  if (tm && strftime (timestr, 80, "%Y-%m-%d", today))
    /* Note to translators: first %s is the date in "i18n" format (year-month-day), %d is a running number of compositions */
    priv->default_vol_name = g_strdup_printf (_("Data %s~%d"), timestr, ++i);
  else
    priv->default_vol_name = g_strdup_printf ("%s %d", _(DATA_COMPOSITION_DEFAULT_NAME), ++i);

  gtk_entry_set_text (GTK_ENTRY (priv->entry_volume_name), priv->default_vol_name);
}

static gboolean
has_default_name (XfburnDataComposition * dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  const gchar *name;

  name = gtk_entry_get_text (GTK_ENTRY (priv->entry_volume_name));

  return strcmp (name, priv->default_vol_name) == 0;
}

static void
action_clear (GSimpleAction *action, GVariant *param, XfburnDataComposition *dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  gtk_tree_store_clear (GTK_TREE_STORE (model));

  set_default_name (dc);

  xfburn_disc_usage_set_size (XFBURN_DISC_USAGE (priv->disc_usage), 0);
}

static void
cb_content_drag_data_get (GtkWidget * widget, GdkDragContext * dc,
                          GtkSelectionData * data, guint info, guint timestamp, XfburnDataComposition * content)
{
  if (info == DATA_COMPOSITION_DND_TARGET_INSIDE) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
    GtkTreeModel *model;
    GList *selected_rows = NULL, *row = NULL;
    GList *references = NULL;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

    row = selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

    while (row) {
      GtkTreeRowReference *reference = NULL;
      GtkTreePath *temp;

      temp = (GtkTreePath *) row->data;
      reference = gtk_tree_row_reference_new (model, temp);
      gtk_tree_path_free (temp);

      references = g_list_prepend (references, reference);

      row = g_list_next (row);
    }

    g_list_free (selected_rows);

    gtk_selection_data_set (data, gdk_atom_intern ("XFBURN_TREE_PATHS", FALSE), 8, (const guchar *) &references,
                            sizeof (GList **));
  }
}

static void
set_modified (XfburnDataCompositionPrivate *priv)
{
  if (!(priv->modified)) {
    /*
    XfburnMainWindow *mainwin;
    GtkUIManager *ui_manager;
    GtkActionGroup *action_group;

    mainwin = xfburn_main_window_get_instance ();
    ui_manager = xfburn_main_window_get_ui_manager (mainwin);

    action_group = (GtkActionGroup *) gtk_ui_manager_get_action_groups (ui_manager)->data;

    action = gtk_action_group_get_action (action_group, "save-composition");
    gtk_action_set_sensitive (GTK_ACTION (action), TRUE);
    */
    priv->modified = TRUE;
  }
}

static gboolean
thread_add_file_to_list_with_name (const gchar *name, XfburnDataComposition * dc,
                                   GtkTreeModel * model, const gchar * path,
                                   GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  struct stat s;

  if ((g_lstat (path, &s) == 0)) {
    gchar *humansize = NULL;
    GtkTreeIter *parent = NULL;
    GtkTreePath *tree_path = NULL;
    int parent_type = 0;

    if (!(S_ISDIR (s.st_mode) ||S_ISREG (s.st_mode) || S_ISCHR(s.st_mode) || S_ISBLK(s.st_mode) || S_ISLNK (s.st_mode))) {
      return FALSE;
    }

    xfburn_adding_progress_pulse (XFBURN_ADDING_PROGRESS (priv->progress));

    /* ensure that we can only drop on top of folders, not files */
    if (insertion) {
      gdk_threads_enter ();
      gtk_tree_model_get (model, insertion, DATA_COMPOSITION_COLUMN_TYPE, &parent_type, -1);
      gdk_threads_leave ();

      if (parent_type == DATA_COMPOSITION_TYPE_FILE) {
        DBG ("Parent is file, and we're dropping into %d", position);
        if (position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
          position = GTK_TREE_VIEW_DROP_AFTER;
        else if (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
          position = GTK_TREE_VIEW_DROP_BEFORE;
      }
    }

    /* find parent */
    switch (position){
      case GTK_TREE_VIEW_DROP_BEFORE:
      case GTK_TREE_VIEW_DROP_AFTER:
      if (insertion) {
          GtkTreeIter iter_parent;

          gdk_threads_enter ();
          if (gtk_tree_model_iter_parent (model, &iter_parent, insertion)) {
            parent = g_new0 (GtkTreeIter, 1);
            memcpy (parent, &iter_parent, sizeof (GtkTreeIter));
          }
          gdk_threads_leave ();
        }
        break;
      case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
      case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
        parent = g_new0 (GtkTreeIter, 1);
        memcpy (parent, insertion, sizeof (GtkTreeIter));
        break;
    }

    /* check if the filename is valid */
    gdk_threads_enter ();
    if (parent) {
      tree_path = gtk_tree_model_get_path (model, parent);
      gtk_tree_path_down (tree_path);
    } else {
      tree_path = gtk_tree_path_new_first ();
    }

    if (file_exists_on_same_level (model, tree_path, FALSE, name)) {
      xfce_dialog_show_error (NULL, NULL, _("A file with the same name is already present in the composition."));

      gtk_tree_path_free (tree_path);
      gdk_threads_leave ();
      g_free (parent);
      return FALSE;
    }
    gtk_tree_path_free (tree_path);
    gdk_threads_leave ();

    /* new directory */
    if (S_ISDIR (s.st_mode) && !S_ISLNK (s.st_mode)) {
      GDir *dir = NULL;
      GError *error = NULL;
      const gchar *filename = NULL;
      guint64 total_size = 4;

      dir = g_dir_open (path, 0, &error);
      if (!dir) {
        g_warning ("unable to open directory : %s", error->message);

        g_error_free (error);
        g_free (parent);

        return FALSE;
      }

      gdk_threads_enter ();
      gtk_tree_store_append (GTK_TREE_STORE (model), iter, parent);

      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          DATA_COMPOSITION_COLUMN_ICON, icon_directory,
                          DATA_COMPOSITION_COLUMN_CONTENT, name,
                          DATA_COMPOSITION_COLUMN_TYPE, DATA_COMPOSITION_TYPE_DIRECTORY,
                          DATA_COMPOSITION_COLUMN_PATH, path,
                          DATA_COMPOSITION_COLUMN_SIZE, (guint64) 4, -1);
      xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (priv->disc_usage), (guint64) 4);
      gdk_threads_leave ();

      while ((filename = g_dir_read_name (dir))) {
        GtkTreeIter new_iter;
        gchar *new_path = NULL;

        new_path = g_build_filename (path, filename, NULL);
        if (new_path) {
          guint64 size;

          if (thread_add_file_to_list (dc, model, new_path, &new_iter, iter, GTK_TREE_VIEW_DROP_INTO_OR_AFTER)) {
            gdk_threads_enter ();
            gtk_tree_model_get (model, &new_iter, DATA_COMPOSITION_COLUMN_SIZE, &size, -1);
            gdk_threads_leave ();
            total_size += size;
          }

          g_free (new_path);
        }
      }

      humansize = xfburn_humanreadable_filesize (total_size);
      gdk_threads_enter ();
      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          DATA_COMPOSITION_COLUMN_HUMANSIZE, humansize, DATA_COMPOSITION_COLUMN_SIZE, total_size, -1);
      gdk_threads_leave ();

      g_dir_close (dir);
    }
    /* new file */
    else if (S_ISREG (s.st_mode) || S_ISCHR(s.st_mode) || S_ISBLK(s.st_mode) || S_ISLNK (s.st_mode)) {
      GdkScreen *screen;
      GtkIconTheme *icon_theme;
      GdkPixbuf *mime_icon_pixbuf = NULL;
      gint x,y;
      GFile *file = NULL;
      GFileInfo *info = NULL;
      GIcon *mime_icon = NULL;
      GtkIconInfo *icon_info = NULL;

      if (s.st_size > MAXIMUM_ISO_FILE_SIZE) {
        gdk_threads_enter ();
        xfce_dialog_show_error (NULL, NULL, _("%s cannot be added to the composition, because it exceeds the maximum allowed file size for iso9660."), path);
        gdk_threads_leave ();

        return FALSE;
      } else if (s.st_size > MAXIMUM_ISO_LEVEL_2_FILE_SIZE && !priv->large_files) {
        priv->large_files = TRUE;
        gdk_threads_enter ();
        xfce_dialog_show_warning (NULL, NULL, _("%s is larger than what iso9660 level 2 allows. This can be a problem for old systems or software."), path);
        gdk_threads_leave ();
      }

      gdk_threads_enter ();
      screen = gtk_widget_get_screen (GTK_WIDGET (dc));
      icon_theme = gtk_icon_theme_get_for_screen (screen);
      gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);

      file = g_file_new_for_path(path);
      info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
      mime_icon = g_content_type_get_icon (g_file_info_get_content_type (info));
      if (mime_icon != NULL) {
        icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme, mime_icon, x, GTK_ICON_LOOKUP_USE_BUILTIN);
        if (icon_info != NULL) {
          mime_icon_pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
          g_object_unref (G_OBJECT (icon_info));
        }
      }

      gtk_tree_store_append (GTK_TREE_STORE (model), iter, parent);

      humansize = xfburn_humanreadable_filesize (s.st_size);

      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          DATA_COMPOSITION_COLUMN_ICON, (G_IS_OBJECT (mime_icon_pixbuf) ? mime_icon_pixbuf : icon_file),
                          DATA_COMPOSITION_COLUMN_CONTENT, name,
                          DATA_COMPOSITION_COLUMN_HUMANSIZE, humansize,
                          DATA_COMPOSITION_COLUMN_SIZE, (guint64) s.st_size,
                          DATA_COMPOSITION_COLUMN_PATH, path,
                          DATA_COMPOSITION_COLUMN_TYPE, DATA_COMPOSITION_TYPE_FILE, -1);

      xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (priv->disc_usage), s.st_size);

      if (G_LIKELY (G_IS_OBJECT (mime_icon)))
        g_object_unref (mime_icon);
      if (G_LIKELY (G_IS_OBJECT (file)))
        g_object_unref(file);
      gdk_threads_leave ();
    }
    g_free (humansize);
    g_free (parent);

    set_modified (priv);
    return TRUE;
  }

  return FALSE;
}

/* thread entry point */
static gpointer
thread_add_files_cli (ThreadAddFilesCLIParams *params)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (params->dc);
  GtkTreeIter iter;

  GtkTreeModel *model;
  GSList * list_iter;

  gdk_threads_enter ();
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  gdk_threads_leave ();

  for (list_iter = params->filelist; list_iter != NULL; list_iter = g_slist_next (list_iter)) {
    gchar * full_path = (gchar *) list_iter->data;

    g_message ("Adding %s to the data composition... (might take a while)", full_path);
    thread_add_file_to_list (params->dc, model, full_path, &iter, NULL, GTK_TREE_VIEW_DROP_AFTER);

    g_free (full_path);
  }
  g_slist_free (params->filelist);
  xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
  return NULL;
}

static gboolean
show_add_home_question_dialog (void)
{
  gboolean ok = TRUE;

  gdk_threads_enter ();
  DBG ("Adding home directory");
  ok = xfburn_ask_yes_no (GTK_MESSAGE_WARNING, ((const gchar *) _("Adding home directory")),
                          _("You are about to add your home directory to the composition. " \
                            "This is likely to take a very long time, and also to be too big to fit on one disc.\n\n" \
                            "Are you sure you want to proceed?")
                         );

  gdk_threads_leave ();

  return ok;
}

/* thread entry point */
static gpointer
thread_add_files_action (ThreadAddFilesActionParams *params)
{
  XfburnDataComposition *dc = params->dc;
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);
  GtkTreeModel *model = params->model;
  GtkTreeIter iter_where_insert = params->iter_where_insert;
  GtkTreePath *path_where_insert = priv->path_where_insert;
  gchar ** files = NULL;
  int i;


  files = g_strsplit (priv->selected_files, "\n", -1);

  if (files)
    for (i=0; files[i] != NULL && files[i][0] != '\0'; i++) {
      GtkTreeIter iter;
      gchar *full_path = NULL;

      if (g_str_has_prefix (files[i], "file://"))
        full_path = g_build_filename (&files[i][7], NULL);
      else if (g_str_has_prefix (files[i], "file:"))
        full_path = g_build_filename (&files[i][5], NULL);
      else
        full_path = g_build_filename (files[i], NULL);

      if (full_path[strlen (full_path) - 1] == '\r')
        full_path[strlen (full_path) - 1] = '\0';

      if (strcmp (full_path, g_getenv ("HOME")) == 0) {
        if (!show_add_home_question_dialog ()) {
          g_free (full_path);
          break;
        }
      }

      /* add files to the disc content */
      if (params->type == DATA_COMPOSITION_TYPE_DIRECTORY) {
        guint64 old_size, size;
        gchar *humansize = NULL;

        thread_add_file_to_list (dc, model, full_path, &iter, &iter_where_insert, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
        gdk_threads_enter ();
        gtk_tree_view_expand_row (GTK_TREE_VIEW (priv->content), path_where_insert, FALSE);

        /* update parent directory size */
        gtk_tree_model_get (model, &iter_where_insert, DATA_COMPOSITION_COLUMN_SIZE, &old_size, -1);
        gtk_tree_model_get (model, &iter, DATA_COMPOSITION_COLUMN_SIZE, &size, -1);
        gdk_threads_leave ();

        humansize = xfburn_humanreadable_filesize (old_size + size);

        gdk_threads_enter ();
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter_where_insert,
                            DATA_COMPOSITION_COLUMN_HUMANSIZE, humansize,
                            DATA_COMPOSITION_COLUMN_SIZE, old_size + size, -1);
        gdk_threads_leave ();

        g_free (humansize);
      } else if (params->type == DATA_COMPOSITION_TYPE_FILE) {
        GtkTreeIter parent;
        gboolean has_parent;

        gdk_threads_enter ();
        has_parent = gtk_tree_model_iter_parent (model, &parent, &iter_where_insert);
        gdk_threads_leave ();

        if (has_parent)
          thread_add_file_to_list (dc, model, full_path, &iter, &parent, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
        else
          thread_add_file_to_list (dc, model, full_path, &iter, NULL, GTK_TREE_VIEW_DROP_AFTER);
      } else {
        thread_add_file_to_list (dc, model, full_path, &iter, NULL, GTK_TREE_VIEW_DROP_AFTER);
      }

      g_free (full_path);
    }
  /* end if files */

  g_strfreev (files);

  xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
  return NULL;
}

static gboolean
thread_add_file_to_list (XfburnDataComposition * dc, GtkTreeModel * model,
                         const gchar * path, GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);
  struct stat s;
  gboolean ret = FALSE;

  if (xfburn_adding_progress_is_aborted (XFBURN_ADDING_PROGRESS (priv->progress))) {
    DBG ("Adding aborted");
    xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
    /* FIXME: does this properly release the resources allocated in this thread? */
    g_thread_exit (NULL);
  }

  if ((stat (path, &s) == 0)) {
    gchar *basename = NULL;

    basename = g_path_get_basename (path);

    ret = thread_add_file_to_list_with_name (basename, dc, model, path, iter, insertion, position);

    g_free (basename);
  }

  return ret;
}

static gboolean
copy_entry_to (XfburnDataComposition *dc, GtkTreeIter *src, GtkTreeIter *dest, GtkTreeViewDropPosition position)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);

  gboolean ret = FALSE;

  GtkTreeModel *model;
  GtkTreeIter iter_new;

  GdkPixbuf *icon = NULL;
  gchar *name = NULL;
  gchar *humansize = NULL;
  guint64 size = 0;
  gchar *path = NULL;
  DataCompositionEntryType type;

  GtkTreePath *path_level = NULL;

  guint n_children = 0;
  guint i;
  GtkTreePath *path_src = NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));

  gtk_tree_model_get (model, src, DATA_COMPOSITION_COLUMN_ICON, &icon, DATA_COMPOSITION_COLUMN_CONTENT, &name,
                      DATA_COMPOSITION_COLUMN_HUMANSIZE, &humansize, DATA_COMPOSITION_COLUMN_SIZE, &size,
                      DATA_COMPOSITION_COLUMN_PATH, &path, DATA_COMPOSITION_COLUMN_TYPE, &type, -1);

  switch (position) {
    case GTK_TREE_VIEW_DROP_BEFORE:
    case GTK_TREE_VIEW_DROP_AFTER:
      gtk_tree_store_insert_before (GTK_TREE_STORE (model), &iter_new, NULL, dest);
      break;
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
      if (dest) {
        path_level = gtk_tree_model_get_path (model, dest);
        gtk_tree_path_down (path_level);
      } else {
        path_level = gtk_tree_path_new_first ();
      }

      if (file_exists_on_same_level (model, path_level, FALSE, name)) {
	xfce_dialog_show_warning(NULL, NULL, _("A file named \"%s\" already exists in this directory, the file hasn't been added."), name);
        goto cleanup;
      }

      gtk_tree_path_free (path_level);

      gtk_tree_store_append (GTK_TREE_STORE (model), &iter_new, dest);
      break;
  }

  gtk_tree_store_set (GTK_TREE_STORE (model), &iter_new, DATA_COMPOSITION_COLUMN_ICON, icon,
                      DATA_COMPOSITION_COLUMN_CONTENT, name, DATA_COMPOSITION_COLUMN_HUMANSIZE, humansize,
                      DATA_COMPOSITION_COLUMN_SIZE, size, DATA_COMPOSITION_COLUMN_PATH, path,
                      DATA_COMPOSITION_COLUMN_TYPE, type, -1);

  /* copy children */
  n_children = gtk_tree_model_iter_n_children (model, src);

  for (i = 0; i < n_children; i++) {
    GtkTreeIter iter_child;

    if (gtk_tree_model_iter_nth_child (model, &iter_child, src, i))
      copy_entry_to (dc, &iter_child, &iter_new, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
  }

  path_src = gtk_tree_model_get_path (model, src);
  if (n_children > 0 && gtk_tree_view_row_expanded (GTK_TREE_VIEW (priv->content), path_src)) {
    GtkTreePath *path_new = NULL;

    path_new = gtk_tree_model_get_path (model, &iter_new);
    gtk_tree_view_expand_row (GTK_TREE_VIEW (priv->content), path_new, FALSE);

    gtk_tree_path_free (path_new);
  }
  gtk_tree_path_free (path_src);

  ret =  TRUE;

cleanup:
  if (G_LIKELY (G_IS_OBJECT (icon)))
    g_object_unref (icon);
  g_free (name);
  g_free (humansize);
  g_free (path);

  return ret;
}

static void
cb_content_drag_data_rcv (GtkWidget * widget, GdkDragContext * dc, guint x, guint y, GtkSelectionData * sd,
                          guint info, guint t, XfburnDataComposition * composition)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (composition);

  GtkTreeModel *model;
  GtkTreePath *path_where_insert = NULL;
  GtkTreeViewDropPosition position;
  GtkTreeIter iter_where_insert;

  g_return_if_fail (sd);
  g_return_if_fail (gtk_selection_data_get_data (sd));

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

  gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget), x, y, &path_where_insert, &position);

  xfburn_busy_cursor (priv->content);

  /* move a selection inside of the composition window */
  if (gtk_selection_data_get_target(sd) == gdk_atom_intern ("XFBURN_TREE_PATHS", FALSE)) {
    GList *row = NULL, *selected_rows = NULL;
    GtkTreeIter *iter = NULL;
    DataCompositionEntryType type_dest = -1;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));

    row = selected_rows = *((GList **) gtk_selection_data_get_data(sd));

    if (path_where_insert) {      
      gtk_tree_model_get_iter (model, &iter_where_insert, path_where_insert);
      iter = &iter_where_insert;

      gtk_tree_model_get (model, &iter_where_insert, DATA_COMPOSITION_COLUMN_TYPE, &type_dest, -1);

      if (type_dest == DATA_COMPOSITION_TYPE_FILE) {
        if (position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
          position = GTK_TREE_VIEW_DROP_AFTER;
        else if (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
          position = GTK_TREE_VIEW_DROP_BEFORE;
      }
    } else {
      position = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
    }

    /* copy selection */
    while (row) {
      GtkTreePath *path_src = NULL;
      GtkTreeIter iter_src;
      GtkTreeRowReference *reference = NULL;
      DataCompositionEntryType type;
      guint64 size = 0;

      reference = (GtkTreeRowReference *) row->data;

      path_src = gtk_tree_row_reference_get_path (reference);
      if (!path_src) {
        gtk_tree_row_reference_free (reference);

        row = g_list_next (row);
        continue;
      }

      if (path_where_insert && (position == GTK_TREE_VIEW_DROP_AFTER || position == GTK_TREE_VIEW_DROP_BEFORE)
          && (gtk_tree_path_get_depth (path_where_insert) == gtk_tree_path_get_depth (path_src))) {
          gtk_tree_path_free (path_src);
          gtk_tree_row_reference_free (reference);

          row = g_list_next (row);
          continue;
      }

      if (path_where_insert && type == DATA_COMPOSITION_TYPE_DIRECTORY
          && gtk_tree_path_is_descendant (path_where_insert, path_src)) {

        gtk_tree_path_free (path_src);
        gtk_tree_path_free (path_where_insert);
        gtk_tree_row_reference_free (reference);

        gtk_drag_finish (dc, FALSE, FALSE, t);
        return;
      }

      gtk_tree_model_get_iter (model, &iter_src, path_src);
      gtk_tree_model_get (model, &iter_src, DATA_COMPOSITION_COLUMN_TYPE, &type,
                          DATA_COMPOSITION_COLUMN_SIZE, &size, -1);

      /* copy entry */
      if (copy_entry_to (composition, &iter_src, iter, position)) {
        GtkTreePath *path_parent = gtk_tree_path_copy (path_src);

        /* update new parent size */
        if (iter && (position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER
                   || position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)) {
          guint64 old_size = 0;
          gchar *parent_humansize = NULL;

          gtk_tree_model_get (model, iter, DATA_COMPOSITION_COLUMN_SIZE, &old_size, -1);

          parent_humansize = xfburn_humanreadable_filesize (old_size + size);
          gtk_tree_store_set (GTK_TREE_STORE (model), iter, DATA_COMPOSITION_COLUMN_HUMANSIZE, parent_humansize,
                              DATA_COMPOSITION_COLUMN_SIZE, old_size + size, -1);

          g_free (parent_humansize);
        }

        if (gdk_drag_context_get_actions(dc) == GDK_ACTION_MOVE) {       
          /* remove source entry */
          if (gtk_tree_path_up (path_parent) && path_where_insert &&
              !gtk_tree_path_is_descendant (path_where_insert, path_parent)) {
            /* update parent size and humansize */
            GtkTreeIter iter_parent;
            guint64 old_size;
            gchar *parent_humansize = NULL;

            gtk_tree_model_iter_parent (model, &iter_parent, &iter_src);
            gtk_tree_model_get (model, &iter_parent, DATA_COMPOSITION_COLUMN_SIZE, &old_size, -1);

            parent_humansize = xfburn_humanreadable_filesize (old_size - size);
            gtk_tree_store_set (GTK_TREE_STORE (model), &iter_parent,
                                DATA_COMPOSITION_COLUMN_HUMANSIZE, parent_humansize,
                                DATA_COMPOSITION_COLUMN_SIZE, old_size - size, -1);
            g_free (parent_humansize);
          }

          gtk_tree_store_remove (GTK_TREE_STORE (model), &iter_src);
        } else {
          xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (priv->disc_usage), size);
        }

        gtk_tree_path_free (path_parent);
      }

      gtk_tree_path_free (path_src);
      gtk_tree_row_reference_free (reference);

      row = g_list_next (row);
    }

    g_list_free (selected_rows);
    gtk_drag_finish (dc, TRUE, FALSE, t);

    if (path_where_insert)
      gtk_tree_path_free (path_where_insert);
    gtk_widget_hide (priv->progress);
    xfburn_default_cursor (priv->content);
  }
  /* drag from the file selector, or nautilus */
  else if (gtk_selection_data_get_target(sd) == gdk_atom_intern ("text/plain;charset=utf-8", FALSE)) {
    ThreadAddFilesDragParams *params;
    gchar **files = NULL;
    gchar *full_paths;
    int i;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));

    full_paths = (gchar *) gtk_selection_data_get_text (sd);

    files = g_strsplit ((gchar *) full_paths, "\n", -1);

    if (files) {

      for (i=0; files[i] != NULL && files[i][0] != '\0'; i++) {
        gchar *full_path;

        if (g_str_has_prefix (files[i], "file://"))
          full_path = g_build_filename (&files[i][7], NULL);
        else if (g_str_has_prefix (files[i], "file:"))
          full_path = g_build_filename (&files[i][5], NULL);
        else
          full_path = g_build_filename (files[i], NULL);

        if (full_path[strlen (full_path) - 1] == '\r')
          full_path[strlen (full_path) - 1] = '\0';

        DBG ("Adding path '%s'", full_path);

        /* remember path to add it later in another thread */
        priv->full_paths_to_add = g_list_append (priv->full_paths_to_add, full_path);
      }
      g_strfreev (files);
      g_free (full_paths);

      priv->full_paths_to_add = g_list_reverse (priv->full_paths_to_add);
      /* FIXME: path_where_insert is always NULL here */
      priv->path_where_insert = path_where_insert;

      params = g_new (ThreadAddFilesDragParams, 1);
      params->composition = composition;
      params->position = position;
      params->widget = widget;

      /* append a dummy row so that gtk doesn't freak out */
      gtk_tree_store_append (GTK_TREE_STORE (model), &params->iter_dummy, NULL);

      priv->thread_params = params;
      g_thread_new ("data_add_dnd_plain", (GThreadFunc) thread_add_files_drag, params);
    }

    gtk_drag_finish (dc, TRUE, FALSE, t);
  }
  else if (gtk_selection_data_get_target(sd) == gdk_atom_intern ("text/uri-list", FALSE)) {
    GList *vfs_paths = NULL;
    GList *vfs_path;
    GList *lp;
    gchar *full_path;
    gchar **uris;
    gsize   n;

    uris = g_uri_list_extract_uris ((gchar *) gtk_selection_data_get_data(sd));

    for (n = 0; uris != NULL && uris[n] != NULL; ++n)
      vfs_paths = g_list_append (vfs_paths, g_file_new_for_uri (uris[n]));

    g_strfreev (uris);

    if (G_LIKELY (vfs_paths != NULL)) {
      ThreadAddFilesDragParams *params;
      priv->full_paths_to_add = NULL;
      for (vfs_path = vfs_paths; vfs_path != NULL; vfs_path = g_list_next (vfs_path)) {
	GFile *path = vfs_path->data;
	if (path == NULL)
          continue;
	/* unable to handle non-local files */
	if (G_UNLIKELY (!g_file_has_uri_scheme (path, "file"))) {
            g_object_unref (path);
	    continue;
        }
        full_path = g_file_get_path (path);
        /* if there is no local path, use the URI (which always works) */
        if (full_path == NULL)
            full_path = g_file_get_uri (path);
        /* release the location */
        g_debug ("adding uri path: %s", full_path);
        priv->full_paths_to_add = g_list_prepend (priv->full_paths_to_add, full_path);
      }

      for (lp = vfs_paths; lp != NULL; lp = lp->next)
        g_object_unref (lp->data);
      g_list_free (vfs_paths);

      priv->full_paths_to_add = g_list_reverse (priv->full_paths_to_add);
      /* FIXME: path_where_insert is always NULL here */
      priv->path_where_insert = path_where_insert;

      params = g_new (ThreadAddFilesDragParams, 1);
      params->composition = composition;
      params->position = position;
      params->widget = widget;

      /* append a dummy row so that gtk doesn't freak out */
      gtk_tree_store_append (GTK_TREE_STORE (model), &params->iter_dummy, NULL);

      priv->thread_params = params;
      g_thread_new ("data_add_dnd_uri", (GThreadFunc) thread_add_files_drag, params);

      gtk_drag_finish (dc, TRUE, FALSE, t);
    } else {
      g_warning("There were no files in the uri list!");
      gtk_drag_finish (dc, FALSE, FALSE, t);
      xfburn_default_cursor (priv->content);
    }
  }
  else {
    g_warning ("Trying to receive an unsupported drag target, this should not happen.");
    gtk_drag_finish (dc, FALSE, FALSE, t);
    xfburn_default_cursor (priv->content);
  }
}

/* thread entry point */
static gpointer
thread_add_files_drag (ThreadAddFilesDragParams *params)
{
  XfburnDataComposition *composition = params->composition;
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (composition);

  GtkTreeViewDropPosition position = params->position;
  GtkWidget *widget = params->widget;

  GtkTreeModel *model;
  GtkTreeIter iter_where_insert;
  GtkTreeIter *iter_insert = (priv->path_where_insert) ? &iter_where_insert : NULL;
  gboolean expand = (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
  gboolean success = FALSE;
  GList *files = priv->full_paths_to_add;

  gdk_threads_enter ();
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

  /* remove the dummy row again */
  gtk_tree_store_remove (GTK_TREE_STORE (model), &params->iter_dummy);
  gdk_threads_leave ();

  for (; files; files = g_list_next (files)) {
    gchar *full_path = (gchar *) files->data;
    GtkTreeIter iter;

    if (priv->path_where_insert) {
      gdk_threads_enter ();
      gtk_tree_model_get_iter (model, &iter_where_insert, priv->path_where_insert);
      gdk_threads_leave ();
    }

    success = thread_add_file_to_list (composition, model, full_path, &iter, iter_insert, position);

    if (success && expand && priv->path_where_insert) {
      gdk_threads_enter ();
      gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), priv->path_where_insert, FALSE);
      gdk_threads_leave ();
    }

  }
  xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
  return NULL;
}

static void
fill_image_with_composition (GtkTreeModel *model, IsoImage *image, IsoDir * parent, GtkTreeIter *iter, GSList **errors)
{
  do {
      DataCompositionEntryType type;
      gchar *name = NULL;
      gchar *src = NULL;
      IsoNode *node = NULL;
      IsoDir *dir = NULL;
      int r;
      gchar *basename;

      gtk_tree_model_get (model, iter, DATA_COMPOSITION_COLUMN_TYPE, &type,
			  DATA_COMPOSITION_COLUMN_CONTENT, &name, DATA_COMPOSITION_COLUMN_PATH, &src, -1);

      if (type == DATA_COMPOSITION_TYPE_DIRECTORY && src == NULL) {
        /* a directory which the user added for this composition */
        r = iso_tree_add_new_dir (parent, name, &dir);
        node = (IsoNode *) dir;

        /* if the new directory is part of the root of the iso,
         * then its owner will be set to root:root. Not sure what a better
         * default could be, so I'll just leave it like that. */
      } else {
        /* something existing on the filesystem, creating a node
         * will copy its attributes */
        r = iso_tree_add_node (image, parent, src, &node);
      }

      if (r < 0) {
        char * msg;

        if (r == ISO_NULL_POINTER)
          g_error (_("%s: null pointer"), src);
        else if (r == ISO_OUT_OF_MEM)
          g_error (_("%s: out of memory"), src);
        else if (r == ISO_NODE_NAME_NOT_UNIQUE)
          msg = g_strdup_printf (_("%s: node name not unique"), src);
        else
          msg = g_strdup_printf (_("%s: %s (code %X)"), src, iso_error_to_msg(r), r);

        g_warning ("%s", msg);
        *errors = g_slist_prepend (*errors, msg);

        g_free (name);
        g_free (src);
        continue;
      }

      if (src != NULL && *src != '\0') {
        basename = g_path_get_basename (src);

        /* check if the file has been renamed */
        if (strcmp (basename, name) != 0) {
          /* rename the iso_node */
          r = iso_node_set_name (node, name);

          if (r == 0) {
            /* The first string is the renamed name, the second one the original name */
            xfce_dialog_show_warning(NULL, NULL, _("Duplicate filename '%s' for '%s'"), name, src);

            g_free (basename);
            g_free (name);
            g_free (src);

            continue;
          }
        }

        g_free (basename);
      }

      g_free (name);
      g_free (src);

      if (type == DATA_COMPOSITION_TYPE_DIRECTORY && gtk_tree_model_iter_has_child (model, iter)) {
	GtkTreeIter child;

        if (iso_node_get_type(node) != LIBISO_DIR)
            g_error ("Expected %s to be a directory, but it isn't...\n", src);

        dir = (IsoDir *)node;

	gtk_tree_model_iter_children (model, &child, iter);
	fill_image_with_composition (model, image, dir, &child, errors);
      }
  } while (gtk_tree_model_iter_next (model, iter));
}

static
void concat_free (char * msg, char ** combined_msg)
{
  *combined_msg = g_strconcat (*combined_msg, msg, "\n", NULL);
  free (msg);
}

static IsoImage *
generate_iso_image (XfburnDataComposition * dc)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);
  IsoImage *image = NULL;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GSList *errors = NULL;

  iso_image_new (gtk_entry_get_text (GTK_ENTRY (priv->entry_volume_name)), &image);
  iso_image_set_application_id (image, "Xfburn");
  iso_image_set_data_preparer_id (image, "Xfburn");

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  if (gtk_tree_model_get_iter_first (model, &iter)) {
    fill_image_with_composition (model, image, iso_image_get_root (image), &iter, &errors);

    if (errors != NULL) {
      gchar * combined_msg = "";
      GtkWidget * dialog, * label, * textview, * scrolled;
      GtkTextBuffer * buffer;
      XfburnMainWindow *mainwin = xfburn_main_window_get_instance ();
      gchar * title;

      errors = g_slist_reverse (errors);

      g_slist_foreach (errors, (GFunc) concat_free, &combined_msg);

      title = _("Error(s) occured while adding files");
      dialog = gtk_dialog_new_with_buttons (title,
                                            GTK_WINDOW (mainwin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            _("OK"),
                                            GTK_RESPONSE_OK,
                                            NULL);

      label = gtk_label_new (NULL);
      title = g_strdup_printf ("<b>%s</b>", title);
      gtk_label_set_markup(GTK_LABEL (label), title);
      g_free (title);

	  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);

      textview = gtk_text_view_new ();
      buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
      gtk_text_buffer_set_text (buffer, combined_msg, -1);
      gtk_text_view_set_editable (GTK_TEXT_VIEW(textview), FALSE);

      scrolled = gtk_scrolled_window_new (NULL, NULL);
      gtk_container_add (GTK_CONTAINER (scrolled), textview);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolled);

      gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 200);

      gtk_widget_show (label);
      gtk_widget_show (textview);
      gtk_widget_show (scrolled);

      gtk_dialog_run (GTK_DIALOG (dialog));

      gtk_widget_destroy (dialog);
      g_free (combined_msg);
    }
  }

  return image;
}

/****************/
/* loading code */
/****************/
typedef struct
{
  gboolean started;
  XfburnDataComposition *dc;
  GQueue *queue_iter;
} LoadParserStruct;

/*
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
*/

static void
load_composition_start (GMarkupParseContext * context, const gchar * element_name,
                        const gchar ** attribute_names, const gchar ** attribute_values,
                        gpointer data, GError ** error)
{
  g_error ("This method needs to get fixed, and does not work right now!");
/*
  LoadParserStruct * parserinfo = (LoadParserStruct *) data;
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (parserinfo->dc);


  if (!(parserinfo->started) && !strcmp (element_name, "xfburn-composition"))
    parserinfo->started = TRUE;
  else if (!(parserinfo->started))
    return;

  if (!strcmp (element_name, "file")) {
    int i, j;

    if ((i = _find_attribute (attribute_names, "name")) != -1 &&
        (j = _find_attribute (attribute_names, "source")) != -1) {
      //GtkTreeIter iter;
      GtkTreeIter *parent;
      GtkTreeModel *model;

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
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

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));

      //add_directory_to_list (attribute_values[i], parserinfo->dc, model, attribute_values[j], &iter, parent);
    }
  }
  */
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

static void
load_from_file (XfburnComposition * composition, const gchar * filename)
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
  parserinfo.dc = XFBURN_DATA_COMPOSITION (composition);
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
  DataCompositionEntryType type;

  space = g_strnfill (gtk_tree_path_get_depth (path), '\t');

  for (i = info->last_depth; i > gtk_tree_path_get_depth (path); i--) {
    gchar *space2 = NULL;

    space2 = g_strnfill (i - 1, '\t');
    fprintf (info->file_content, "%s</directory>\n", space2);

    g_free (space2);
  }

  gtk_tree_model_get (model, iter, DATA_COMPOSITION_COLUMN_CONTENT, &name,
                      DATA_COMPOSITION_COLUMN_PATH, &source_path,
                      DATA_COMPOSITION_COLUMN_TYPE, &type, -1);

  fprintf (info->file_content, "%s", space);
  switch (type) {
  case DATA_COMPOSITION_TYPE_FILE:
    fprintf (info->file_content, "<file name=\"%s\" source=\"%s\" />\n", name, source_path);
    break;
  case DATA_COMPOSITION_TYPE_DIRECTORY:
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

static void
save_to_file (XfburnComposition * composition)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (composition);
  FILE *file_content;
  GtkTreeModel *model;
  CompositionSaveInfo info;
  gint i;

  if (!(priv->filename)) {
    priv->filename = g_strdup ("/tmp/gna");

    g_signal_emit_by_name (G_OBJECT (composition), "name-changed", priv->filename);
  }

  file_content = fopen (priv->filename, "w+");
  fprintf (file_content, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  fprintf (file_content, "<xfburn-composition version=\"0.1\">\n");

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
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

/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_data_composition_new (void)
{
  return g_object_new (xfburn_data_composition_get_type (), NULL);
}

void
xfburn_data_composition_add_files (XfburnDataComposition *dc, GSList * filelist)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (dc);
  ThreadAddFilesCLIParams *params;

  if (filelist != NULL) {
    params = g_new (ThreadAddFilesCLIParams, 1);

    params->filelist = filelist;
    params->dc = dc;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));
    xfburn_busy_cursor (priv->content);

    priv->thread_params = params;
    g_thread_new ("data_add_files_cli", (GThreadFunc) thread_add_files_cli, params);
  }
}

void
xfburn_data_composition_hide_toolbar (XfburnDataComposition * composition)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (composition);

  gtk_widget_hide (priv->toolbar);
}

void
xfburn_data_composition_show_toolbar (XfburnDataComposition * composition)
{
  XfburnDataCompositionPrivate *priv = XFBURN_DATA_COMPOSITION_GET_PRIVATE (composition);

  gtk_widget_show (priv->toolbar);
}


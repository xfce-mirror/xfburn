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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <errno.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <gio/gio.h>

#if !LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
#include <exo/exo.h>
#endif

#include <libisofs.h>

#include "xfburn-audio-composition.h"
#include "xfburn-global.h"
#include "xfburn-error.h"
#include "xfburn-utils.h"

#include "xfburn-adding-progress.h"
#include "xfburn-composition.h"
#include "xfburn-audio-disc-usage.h"
#include "xfburn-main-window.h"
#include "xfburn-utils.h"
#include "xfburn-burn-audio-cd-composition-dialog.h"
#include "xfburn-transcoder.h"
#include "xfburn-settings.h"
#include "xfburn-main.h"

#define XFBURN_AUDIO_COMPOSITION_GET_PRIVATE(obj) (xfburn_audio_composition_get_instance_private (obj))

enum
{
  AUDIO_COMPOSITION_COLUMN_ICON,
  AUDIO_COMPOSITION_COLUMN_POS,
  AUDIO_COMPOSITION_COLUMN_CONTENT,
  AUDIO_COMPOSITION_COLUMN_LENGTH,
  AUDIO_COMPOSITION_COLUMN_HUMANLENGTH,
  AUDIO_COMPOSITION_COLUMN_SIZE,
  AUDIO_COMPOSITION_COLUMN_ARTIST,
  AUDIO_COMPOSITION_COLUMN_TITLE,
  AUDIO_COMPOSITION_COLUMN_PATH,
  AUDIO_COMPOSITION_COLUMN_TYPE,
  AUDIO_COMPOSITION_COLUMN_TRACK,
  AUDIO_COMPOSITION_N_COLUMNS
};

enum
{
  AUDIO_COMPOSITION_DISPLAY_COLUMN_POS,
  AUDIO_COMPOSITION_DISPLAY_COLUMN_LENGTH,
  AUDIO_COMPOSITION_DISPLAY_COLUMN_PATH,
  AUDIO_COMPOSITION_DISPLAY_N_COLUMNS
};

typedef enum
{
  AUDIO_COMPOSITION_TYPE_RAW,
} AudioCompositionEntryType;


/* thread parameters */
typedef struct {
  GSList * filelist;
  XfburnAudioComposition *dc;
} ThreadAddFilesCLIParams;

typedef struct {
  XfburnAudioComposition *dc;
  GtkTreeModel *model;
  GtkTreeIter iter_where_insert;
  AudioCompositionEntryType type;
} ThreadAddFilesActionParams;

typedef struct {
  XfburnAudioComposition *composition;
  AudioCompositionEntryType type;
  GtkWidget *widget;
  GtkTreeViewDropPosition position;
  GtkTreeIter iter_dummy;
} ThreadAddFilesDragParams;

/* prototypes */
static void composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data);
static void xfburn_audio_composition_finalize (GObject * object);

/* internals */
static void action_clear (GSimpleAction *, GVariant*, XfburnAudioComposition *);
static void action_info (GSimpleAction *, GVariant*, XfburnAudioComposition *);
static void action_remove_selection (GSimpleAction *, GVariant*, XfburnAudioComposition *);
static void action_add_selected_files (GSimpleAction *, GVariant*, XfburnAudioComposition *);

static void tracks_changed (XfburnAudioComposition *ac);
static gboolean cb_treeview_button_pressed (GtkTreeView * treeview, GdkEventButton * event, XfburnAudioComposition * dc);
static void cb_selection_changed (GtkTreeSelection *selection, XfburnAudioComposition * dc);
static GSList * generate_audio_src (XfburnAudioComposition * ac);
static void cb_begin_burn (XfburnDiscUsage * du, XfburnAudioComposition * dc);

static void cb_content_drag_data_rcv (GtkWidget * widget, GdkDragContext * dc, guint x, guint y, GtkSelectionData * sd,
                                      guint info, guint t, XfburnAudioComposition * composition);
static void cb_content_drag_data_get (GtkWidget * widget, GdkDragContext * dc, GtkSelectionData * data, guint info,
                                      guint timestamp, XfburnAudioComposition * content);
static void cb_adding_done (XfburnAddingProgress *progress, XfburnAudioComposition *dc);
static void cb_key_press_event (GtkWidget *widget, GdkEvent *event, XfburnAudioComposition *composition);

/* thread entry points */
static gpointer thread_add_files_cli (ThreadAddFilesCLIParams *params);
static gpointer thread_add_files_action (ThreadAddFilesActionParams *params);
static gpointer thread_add_files_drag (ThreadAddFilesDragParams *params);

/* thread helpers */
static gboolean thread_add_file_to_list_with_name (const gchar *name, XfburnAudioComposition * dc,
                                                   GtkTreeModel * model, const gchar * path, GtkTreeIter * iter,
                                                   GtkTreeIter * insertion, GtkTreeViewDropPosition position);
static gboolean thread_add_file_to_list (XfburnAudioComposition * dc, GtkTreeModel * model, const gchar * path,
                                         GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position);
static gboolean show_add_home_question_dialog (void);

typedef struct
{
  gchar *filename;
  gboolean modified;

  guint n_tracks;

  GList *full_paths_to_add;
  gchar *selected_files;
  GtkTreePath *path_where_insert;

  GdkDragContext * dc;
  gboolean success;
  gboolean del;
  guint32 time;

  void *thread_params;
  GHashTable *warned_about;

  GSimpleActionGroup *action_group;
  GtkBuilder *ui_manager;

  GtkWidget *toolbar;
  GtkWidget *entry_volume_name;
  GtkWidget *content;
  GtkWidget *disc_usage;
  GtkWidget *progress;
  GtkTreeStore *model;

  XfburnTranscoder *trans;
} XfburnAudioCompositionPrivate;

/* globals */
#define MAX_NAME_LENGTH 80
static GtkHPanedClass *parent_class = NULL;
static guint instances = 0;
static gchar *did_warn = "Did warn about this already";
static gchar trans_name[MAX_NAME_LENGTH+1] = {""};

static const GActionEntry actions[] = {
  {.name = "add-file", .activate = (gActionCallback)action_add_selected_files},
  {.name = "remove-file", .activate = (gActionCallback)action_remove_selection},
  {.name = "clear", .activate = (gActionCallback)action_clear},
  {.name = "transcoder-info", .activate = (gActionCallback)action_info},
};

static GdkPixbuf *icon_directory = NULL, *icon_file = NULL;

/********************************/
/* XfburnAudioComposition class */
/********************************/
G_DEFINE_TYPE_EXTENDED(XfburnAudioComposition,
	xfburn_audio_composition,
	GTK_TYPE_BOX,
	0,
	G_ADD_PRIVATE(XfburnAudioComposition)
	G_IMPLEMENT_INTERFACE(XFBURN_TYPE_COMPOSITION, composition_interface_init));

static void
xfburn_audio_composition_class_init (XfburnAudioCompositionClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_audio_composition_finalize;
}

static void
composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data)
{
}

static void
xfburn_audio_composition_init (XfburnAudioComposition * composition)
{
  gint x, y;
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);

  GtkWidget *hbox_toolbar;
  GtkWidget *scrolled_window;
  GtkTreeStore *model;
  GtkTreeSelection *selection;
  GSimpleAction *action = NULL;
  GdkScreen *screen;
  GtkIconTheme *icon_theme;
  gchar *popup_ui;

  GtkTargetEntry gte_src[] =  { { "XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, AUDIO_COMPOSITION_DND_TARGET_INSIDE } };
  GtkTargetEntry gte_dest[] = { { "XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, AUDIO_COMPOSITION_DND_TARGET_INSIDE },
                                { "text/plain;charset=utf-8", 0, AUDIO_COMPOSITION_DND_TARGET_TEXT_PLAIN },
                                { "text/uri-list", 0, AUDIO_COMPOSITION_DND_TARGET_TEXT_URI_LIST },
                              };

  gtk_orientable_set_orientation(GTK_ORIENTABLE (composition), GTK_ORIENTATION_VERTICAL);

  priv->full_paths_to_add = NULL;
  priv->trans = xfburn_transcoder_get_global ();

  if (trans_name[0] == '\0')
    strncpy (trans_name, xfburn_transcoder_get_name (priv->trans), MAX_NAME_LENGTH);

  instances++;

  /* initialize static members */
  screen = gtk_widget_get_screen (GTK_WIDGET (composition));
  icon_theme = gtk_icon_theme_get_for_screen (screen);

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  popup_ui = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfburn/xfburn-popup-menus.ui");

  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);
  if (!icon_directory)
    icon_directory = gtk_icon_theme_load_icon (icon_theme, "folder", x, 0, NULL);
  if (!icon_file)
    icon_file = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-regular", x, 0, NULL);

  /* create ui manager */
  priv->action_group = g_simple_action_group_new();
  g_action_map_add_action_entries (G_ACTION_MAP (priv->action_group), actions, G_N_ELEMENTS (actions),
                                GTK_WIDGET (composition));
  priv->ui_manager = gtk_builder_new ();
  gtk_builder_set_translation_domain (priv->ui_manager, GETTEXT_PACKAGE);
  gtk_builder_add_from_file(priv->ui_manager, popup_ui, NULL);
  gtk_widget_insert_action_group(GTK_WIDGET (composition), "win", G_ACTION_GROUP (priv->action_group));

  hbox_toolbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start (GTK_BOX (composition), hbox_toolbar, FALSE, TRUE, 0);
  gtk_widget_show (hbox_toolbar);

  /* toolbar */
  priv->toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_style(GTK_TOOLBAR (priv->toolbar), GTK_TOOLBAR_BOTH);
  gtk_widget_insert_action_group (priv->toolbar, "win", G_ACTION_GROUP (priv->action_group));

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "list-add", _("Add"), "win.add-file", _("Add the selected file(s) to the composition"));

  gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), gtk_separator_tool_item_new(), -1);

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "list-remove", _("Remove"), "win.remove-file", _("Remove the selected file(s) from the composition"));
  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "edit-clear", _("Clear"), "win.clear", _("Clear the content of the composition"));

  gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), gtk_separator_tool_item_new(), -1);

#ifdef HAVE_GST
  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbar),
    "help-about", _("gstreamer"), "win.transcoder-info", _("What files can get burned to an audio CD?"));
#endif
  gtk_box_pack_start (GTK_BOX (hbox_toolbar), priv->toolbar, TRUE, TRUE, 0);
  gtk_widget_show_all (priv->toolbar);


  /* content treeview */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (composition), scrolled_window, TRUE, TRUE, 0);

#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
  priv->content = xfce_tree_view_new ();
#else
  priv->content = exo_tree_view_new ();
#endif
  model = gtk_tree_store_new (AUDIO_COMPOSITION_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING,
                              G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, XFBURN_TYPE_AUDIO_TRACK);
  priv->model = model;

  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->content), GTK_TREE_MODEL (model));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  gtk_widget_show (priv->content);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->content);


  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Pos"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_POS, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Length"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_HUMANLENGTH, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Filename"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_PATH, NULL);

  /* Position  */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_POS), FALSE);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_POS), 20);
  /* Length */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_LENGTH), FALSE);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_LENGTH), 60);
  /* Local Path (PATH) column */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_PATH), TRUE);


  g_signal_connect (G_OBJECT (priv->content), "button-press-event",
                    G_CALLBACK (cb_treeview_button_pressed), composition);
  g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (cb_selection_changed), composition);

  /* adding progress window */
  priv->progress = GTK_WIDGET (xfburn_adding_progress_new ());
  g_signal_connect (G_OBJECT (priv->progress), "adding-done", G_CALLBACK (cb_adding_done), composition);
  gtk_window_set_transient_for (GTK_WINDOW (priv->progress), GTK_WINDOW (xfburn_main_window_get_instance ()));
  /* FIXME: progress should have a busy cursor */

  /* disc usage */
  priv->disc_usage = xfburn_audio_disc_usage_new ();
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

  action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (priv->action_group), "remove-file"));
  g_simple_action_set_enabled (action, FALSE);

  priv->warned_about = g_hash_table_new (g_direct_hash, g_direct_equal);

  /* map delete key to remove file action */
  g_signal_connect (G_OBJECT (priv->content), "key-press-event", G_CALLBACK (cb_key_press_event),
                    composition);
}

static void
xfburn_audio_composition_finalize (GObject * object)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (XFBURN_AUDIO_COMPOSITION (object));

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

  g_object_unref (priv->trans);
  priv->trans = NULL;

  /* while the content treeview is part of the GUI and is automatically destroyed
     when this tab is closed, it does not take ownership of the model, which then
     must be unref'ed manually */
  g_object_unref (priv->model);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*************/
/* internals */
/*************/
static GSList *
generate_audio_src (XfburnAudioComposition * ac)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (ac);
  GtkTreeModel *model;
  GtkTreeIter iter;
  GSList *list = NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  if (gtk_tree_model_get_iter_first (model, &iter)) {
    do {
      XfburnAudioTrack *atrack;
      gint pos;

      gtk_tree_model_get (model, &iter,
                          AUDIO_COMPOSITION_COLUMN_POS, &pos,
                          AUDIO_COMPOSITION_COLUMN_TRACK, &atrack,
                          -1);

      g_assert (atrack != NULL);

      /* update the track position, which might have changed */
      atrack->pos = pos;

      list = g_slist_prepend (list, atrack);

    } while (gtk_tree_model_iter_next (model, &iter));
  }

  list = g_slist_reverse (list);

  return list;
}

static void
cb_begin_burn (XfburnDiscUsage * du, XfburnAudioComposition * dc)
{
  XfburnMainWindow *mainwin = xfburn_main_window_get_instance ();
  GtkWidget *dialog = NULL;
  GSList *src;

  src = generate_audio_src (dc);

  switch (xfburn_disc_usage_get_disc_type (du)) {
  case CD_DISC:
    dialog = xfburn_burn_audio_cd_composition_dialog_new (src);
    break;
  case DVD_DISC:
    xfce_dialog_show_error (NULL, NULL, _("Cannot burn audio onto a DVD."));
    return;
    break;
  }

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (mainwin));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
cb_selection_changed (GtkTreeSelection *selection, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  gint n_selected_rows;
  GSimpleAction *action = NULL;
  GActionMap *map = G_ACTION_MAP (priv->action_group);

  n_selected_rows = gtk_tree_selection_count_selected_rows (selection);
  if (n_selected_rows == 0) {
    action = G_SIMPLE_ACTION (g_action_map_lookup_action (map, "add-file"));
    g_simple_action_set_enabled (action, TRUE);
    action = G_SIMPLE_ACTION (g_action_map_lookup_action (map, "remove-file"));
    g_simple_action_set_enabled (action, FALSE);
  } else if (n_selected_rows == 1) {
    action = G_SIMPLE_ACTION (g_action_map_lookup_action (map, "add-file"));
    g_simple_action_set_enabled (action, TRUE);
    action = G_SIMPLE_ACTION (g_action_map_lookup_action (map, "remove-file"));
    g_simple_action_set_enabled (action, TRUE);
  } else {
    action = G_SIMPLE_ACTION (g_action_map_lookup_action (map, "add-file"));
    g_simple_action_set_enabled (action, FALSE);
    action = G_SIMPLE_ACTION (g_action_map_lookup_action (map, "remove-file"));
    g_simple_action_set_enabled (action, TRUE);
  }
}

static gboolean
cb_treeview_button_pressed (GtkTreeView * treeview, GdkEventButton * event, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

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
    GtkWidget *menu_popup;
    GMenuModel *model;
    GtkWidget *menuitem_remove;
    GdkRectangle r = {event->x, event->y, 1, 1};

    selection = gtk_tree_view_get_selection (treeview);

    if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
      gtk_tree_selection_unselect_all (selection);
      gtk_tree_selection_select_path (selection, path);
      gtk_tree_path_free (path);
    }

    model = G_MENU_MODEL (gtk_builder_get_object (priv->ui_manager, "audio-popup-menu"));
    menu_popup = gtk_menu_new_from_model (model);
    gtk_widget_insert_action_group(GTK_WIDGET(menu_popup), "win", G_ACTION_GROUP (priv->action_group));
    menuitem_remove = GTK_WIDGET (gtk_container_get_children (GTK_CONTAINER (menu_popup))->data);

    if (gtk_tree_selection_count_selected_rows (selection) >= 1)
      gtk_widget_set_sensitive (menuitem_remove, TRUE);
    else
      gtk_widget_set_sensitive (menuitem_remove, FALSE);

    gtk_menu_popup_at_rect (GTK_MENU (menu_popup),
        gtk_widget_get_parent_window (GTK_WIDGET (treeview)),
        &r, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
    return TRUE;
  }

  return FALSE;
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

      gtk_tree_model_get (model, &current_iter, AUDIO_COMPOSITION_COLUMN_CONTENT, &current_filename, -1);

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
tracks_changed (XfburnAudioComposition *ac)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (ac);
  GtkTreeModel *model;
  GtkTreeIter iter;
  int i=0;

  /* fix up position numbers based on the tree model */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  if (gtk_tree_model_get_iter_first (model, &iter)) {
    do {
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, AUDIO_COMPOSITION_COLUMN_POS, ++i, -1);
    } while (gtk_tree_model_iter_next (model, &iter));
  }
}

static void
cb_adding_done (XfburnAddingProgress *progress, XfburnAudioComposition *dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

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
    g_list_free_full (priv->full_paths_to_add, (GDestroyNotify)g_free);
    priv->full_paths_to_add = NULL;
  }

  if (priv->thread_params) {
    g_free (priv->thread_params);
    priv->thread_params = NULL;
  }

  g_hash_table_remove_all (priv->warned_about);

  tracks_changed (dc);

  xfburn_default_cursor (priv->content);
}

/**
 * Handle the key-press-event on the audio composition interface.
 *
 * If the delete key was pressed, remove the currently-selected item.
 */
static void
cb_key_press_event (GtkWidget *widget, GdkEvent *event, XfburnAudioComposition *composition)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);
  guint keyval;

  gdk_event_get_keyval (event, &keyval);

  if (keyval == GDK_KEY_Delete) {
    g_action_group_activate_action (G_ACTION_GROUP (priv->action_group), "remove-file", NULL);
  }
}

static void
remove_row_reference (GtkTreeRowReference *reference, XfburnAudioCompositionPrivate *priv)
{
  GtkTreePath *path = NULL;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));

  path = gtk_tree_row_reference_get_path (reference);
  if (path) {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter (model, &iter, path)) {
      int secs = 0;

      gtk_tree_model_get (model, &iter, AUDIO_COMPOSITION_COLUMN_LENGTH, &secs, -1);
      xfburn_disc_usage_sub_size (XFBURN_DISC_USAGE (priv->disc_usage), secs);

      gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
      priv->n_tracks--;
    }

    gtk_tree_path_free (path);
  }

  gtk_tree_row_reference_free (reference);
}

static void
action_remove_selection (GSimpleAction * action, GVariant * param, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

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
action_add_selected_files (GSimpleAction * action, GVariant * param, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  XfburnFileBrowser *browser = xfburn_main_window_get_file_browser (xfburn_main_window_get_instance ());

  gchar *selected_files = NULL;

  xfburn_busy_cursor (priv->content);
  if (xfburn_settings_get_boolean("show-filebrowser", FALSE)) {
    selected_files = xfburn_file_browser_get_selection (browser);
  } else {
    selected_files = xfburn_browse_for_files (FALSE);
  }

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
      gtk_tree_model_get (params->model, &params->iter_where_insert, AUDIO_COMPOSITION_COLUMN_TYPE, &params->type, -1);
    }

    priv->selected_files = selected_files;

    priv->thread_params = params;
    g_thread_new ("audio_add_selected", (GThreadFunc) thread_add_files_action, params);

    g_list_free_full (selected_paths, (GDestroyNotify)gtk_tree_path_free);
  }
}

static void
action_clear (GSimpleAction * action, GVariant * param, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  gtk_tree_store_clear (GTK_TREE_STORE (model));

  xfburn_disc_usage_set_size (XFBURN_DISC_USAGE (priv->disc_usage), 0);
  priv->n_tracks = 0;
}

static void
action_info (GSimpleAction * action, GVariant * param, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

  xfce_dialog_show_info(NULL, NULL, "%s", xfburn_transcoder_get_description (priv->trans));
}

static void
cb_content_drag_data_get (GtkWidget * widget, GdkDragContext * dc,
                          GtkSelectionData * data, guint info, guint timestamp, XfburnAudioComposition * content)
{
  if (info == AUDIO_COMPOSITION_DND_TARGET_INSIDE) {
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
set_modified (XfburnAudioCompositionPrivate *priv)
{
  if (!(priv->modified)) {
    priv->modified = TRUE;
  }
}

/* This function ensures that the user is only told about once about each
 * type of error. Useful for dragging folders into the audio composition view. */
static void
notify_not_adding (XfburnAudioComposition * dc, GError *error)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

  g_assert (error != NULL);

  if (error->domain != XFBURN_ERROR) {
    xfce_dialog_show_warning(NULL, NULL, "%s", error->message);
    return;
  }

  if (g_hash_table_lookup (priv->warned_about, GINT_TO_POINTER (error->code)) == NULL) {
    g_hash_table_insert (priv->warned_about, GINT_TO_POINTER (error->code), did_warn);

    xfce_dialog_show_warning(NULL, NULL, "%s", error->message);
  }
}

static gboolean
thread_add_file_to_list_with_name (const gchar *name, XfburnAudioComposition * dc,
                                   GtkTreeModel * model, const gchar * path,
                                   GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

  struct stat s;

  if ((stat (path, &s) == 0)) {
    gchar *basename = NULL;
    gchar *humanlength = NULL;
    int secs = 0;
    GtkTreePath *tree_path = NULL;
    gboolean ret = FALSE;

    if (!S_ISDIR (s.st_mode) && !S_ISREG (s.st_mode)) {
      return FALSE;
    }

    basename = g_path_get_basename (path);
    if ( (strlen (basename) > 1) && (basename[0] == '.') ) {
      /* FIXME: is this really what we want? */
      /* don't add hidden files/directories */

      g_free (basename);
      return FALSE;
    }
    g_free (basename);

    xfburn_adding_progress_pulse (XFBURN_ADDING_PROGRESS (priv->progress));

    /* check if the filename is valid */
    gdk_threads_enter ();
    tree_path = gtk_tree_path_new_first ();
    gdk_threads_leave ();

    gdk_threads_enter ();
    if (file_exists_on_same_level (model, tree_path, FALSE, name)) {
      xfce_dialog_show_error (NULL, NULL, _("A file with the same name is already present in the composition."));

      gtk_tree_path_free (tree_path);
      gdk_threads_leave ();
      return FALSE;
    }
    gtk_tree_path_free (tree_path);
    gdk_threads_leave ();

    /* new directory */
    if (S_ISDIR (s.st_mode)) {
      GDir *dir = NULL;
      GError *error = NULL;
      const gchar *filename = NULL;
      GtkTreeIter *iter_last;

      dir = g_dir_open (path, 0, &error);
      if (!dir) {
        g_warning ("unable to open directory : %s", error->message);

        g_error_free (error);

        return FALSE;
      }

      /* FIXME: if insertion != NULL, we will overwrite its contents, is that OK? */
      iter_last = insertion;

      while ((filename = g_dir_read_name (dir))) {
        GtkTreeIter new_iter;
        gchar *new_path = NULL;

        new_path = g_build_filename (path, filename, NULL);
        if (new_path) {
          guint64 size;

          if (thread_add_file_to_list (dc, model, new_path, &new_iter, iter_last, position)) {
            gdk_threads_enter ();
            gtk_tree_model_get (model, &new_iter, AUDIO_COMPOSITION_COLUMN_SIZE, &size, -1);
            gdk_threads_leave ();
            if (iter_last == NULL)
              iter_last = g_new (GtkTreeIter, 1);
            *iter_last = new_iter;
            position = GTK_TREE_VIEW_DROP_AFTER;
            ret = TRUE;
          }

          g_free (new_path);
        }
      }

      /* since we don't add the folder, the next song needs
       * to get added after the last one from this directory */
      if (iter_last != NULL)
        *iter = *iter_last;

      if (insertion == NULL)
        g_free (iter_last);

      g_dir_close (dir);
    }
    /* new file */
    else if (S_ISREG (s.st_mode)) {
      GError *error = NULL;
      XfburnAudioTrack *atrack = NULL;

      atrack = xfburn_transcoder_get_audio_track (priv->trans, path, &error);
      if (atrack == NULL) {
        gdk_threads_enter ();
        notify_not_adding (dc, error);
        gdk_threads_leave ();

        g_error_free (error);
        return FALSE;
      }

      if (priv->n_tracks == 99) {
        XfburnError err_code = XFBURN_ERROR_TOO_MANY_AUDIO_TRACKS;

        if (g_hash_table_lookup (priv->warned_about, GINT_TO_POINTER (err_code)) == NULL) {
          g_hash_table_insert (priv->warned_about, GINT_TO_POINTER (err_code), did_warn);
          gdk_threads_enter ();
          xfce_dialog_show_error (NULL, NULL, _("You can only have a maximum of 99 tracks."));
          gdk_threads_leave ();
        }

        return FALSE;
      }

      gdk_threads_enter ();
      if (insertion != NULL) {
        if (position == GTK_TREE_VIEW_DROP_AFTER || position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
          gtk_tree_store_insert_after (GTK_TREE_STORE (model), iter, NULL, insertion);
        else if (position == GTK_TREE_VIEW_DROP_BEFORE || position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
          gtk_tree_store_insert_before (GTK_TREE_STORE (model), iter, NULL, insertion);
        else
          g_error ("Invalid position to drop item in!");
      } else
        gtk_tree_store_append (GTK_TREE_STORE (model), iter, NULL);
      gdk_threads_leave ();

      secs = atrack->length;
      humanlength = g_strdup_printf ("%2d:%02d", secs / 60, secs % 60);

      /* pos does not yet get recorded into atrack here, because it might
       * change still and is easier updated inside the model for now */
      gdk_threads_enter ();
      gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                          AUDIO_COMPOSITION_COLUMN_POS, ++priv->n_tracks,
                          AUDIO_COMPOSITION_COLUMN_CONTENT, name,
                          AUDIO_COMPOSITION_COLUMN_SIZE, (guint64) s.st_size,
                          AUDIO_COMPOSITION_COLUMN_PATH, path,
                          AUDIO_COMPOSITION_COLUMN_LENGTH, secs,
                          AUDIO_COMPOSITION_COLUMN_HUMANLENGTH, humanlength,
                          AUDIO_COMPOSITION_COLUMN_ARTIST, "",
                          AUDIO_COMPOSITION_COLUMN_TITLE, "",
                          AUDIO_COMPOSITION_COLUMN_TRACK, atrack,
                          AUDIO_COMPOSITION_COLUMN_TYPE, AUDIO_COMPOSITION_TYPE_RAW, -1);
      gdk_threads_leave ();

      g_free (humanlength);
      /* the tree store makes a copy of the boxed type, so we can free the original */
      g_boxed_free (XFBURN_TYPE_AUDIO_TRACK, atrack);

      gdk_threads_enter ();
      xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (priv->disc_usage), secs);
      gdk_threads_leave ();

      ret = TRUE;
    }

    set_modified (priv);
    return ret;
  }

  return FALSE;
}

/* thread entry point */
static gpointer
thread_add_files_cli (ThreadAddFilesCLIParams *params)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (params->dc);
  GtkTreeIter iter;

  GtkTreeModel *model;
  GSList * list_iter;

  gdk_threads_enter ();
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  gdk_threads_leave ();

  for (list_iter = params->filelist; list_iter != NULL; list_iter = g_slist_next (list_iter)) {
    gchar * full_path = (gchar *) list_iter->data;

    g_message ("Adding %s to the audio composition... (might take a while)", full_path);
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
  XfburnAudioComposition *dc = params->dc;
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  GtkTreeModel *model = params->model;
  GtkTreeIter iter_where_insert = params->iter_where_insert;
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
      if (params->type == AUDIO_COMPOSITION_TYPE_RAW) {
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
thread_add_file_to_list (XfburnAudioComposition * dc, GtkTreeModel * model,
                         const gchar * path, GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
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

static GtkTreeIter *
copy_entry_to (XfburnAudioComposition *dc, GtkTreeIter *src, GtkTreeIter *dest, GtkTreeViewDropPosition position)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

  GtkTreeModel *model;
  GtkTreeIter *iter_new = g_new0 (GtkTreeIter, 1);

  GdkPixbuf *icon = NULL;
  gint pos = 0;
  gchar *name = NULL;
  gint length = 0;
  guint64 size = 0;
  gchar *path = NULL;
  gchar *humanlength;
  AudioCompositionEntryType type;
  XfburnAudioTrack *atrack;

  GtkTreePath *path_level = NULL;

  GtkTreePath *path_src = NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));

  gtk_tree_model_get (model, src,
                      AUDIO_COMPOSITION_COLUMN_POS, &pos,
                      AUDIO_COMPOSITION_COLUMN_CONTENT, &name,
                      AUDIO_COMPOSITION_COLUMN_LENGTH, &length,
                      AUDIO_COMPOSITION_COLUMN_SIZE, &size,
                      AUDIO_COMPOSITION_COLUMN_PATH, &path,
                      AUDIO_COMPOSITION_COLUMN_HUMANLENGTH, &humanlength,
                      AUDIO_COMPOSITION_COLUMN_TRACK, &atrack,
                      AUDIO_COMPOSITION_COLUMN_TYPE, &type,
                      -1);

  if (dest == NULL)
    gtk_tree_store_append (GTK_TREE_STORE (model), iter_new, dest);
  else switch (position) {
    case GTK_TREE_VIEW_DROP_BEFORE:
      gtk_tree_store_insert_before (GTK_TREE_STORE (model), iter_new, NULL, dest);
      break;
    case GTK_TREE_VIEW_DROP_AFTER:
      gtk_tree_store_insert_after (GTK_TREE_STORE (model), iter_new, NULL, dest);
      break;
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
      /* FIXME: do we still need this case? */
      if (dest) {
        path_level = gtk_tree_model_get_path (model, dest);
        gtk_tree_path_down (path_level);
      } else {
        path_level = gtk_tree_path_new_first ();
      }

      gtk_tree_path_free (path_level);

      gtk_tree_store_append (GTK_TREE_STORE (model), iter_new, dest);
      break;
  }

  gtk_tree_store_set (GTK_TREE_STORE (model), iter_new,
                      AUDIO_COMPOSITION_COLUMN_POS, pos,
                      AUDIO_COMPOSITION_COLUMN_CONTENT, name,
                      AUDIO_COMPOSITION_COLUMN_LENGTH, length,
                      AUDIO_COMPOSITION_COLUMN_SIZE, size,
                      AUDIO_COMPOSITION_COLUMN_PATH, path,
                      AUDIO_COMPOSITION_COLUMN_HUMANLENGTH, humanlength,
                      AUDIO_COMPOSITION_COLUMN_TRACK, atrack,
                      AUDIO_COMPOSITION_COLUMN_TYPE, type,
                      -1);

  gtk_tree_path_free (path_src);

//cleanup:
  if (G_LIKELY (G_IS_OBJECT (icon)))
    g_object_unref (icon);
  g_free (name);
  g_free (humanlength);
  g_free (path);

  return iter_new;
}

static void
cb_content_drag_data_rcv (GtkWidget * widget, GdkDragContext * dc, guint x, guint y, GtkSelectionData * sd,
                          guint info, guint t, XfburnAudioComposition * composition)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);

  GtkTreeModel *model;
  GtkTreePath *path_where_insert = NULL;
  GtkTreeViewDropPosition position;
  GtkTreeIter *iter_where_insert;

  g_return_if_fail (sd);
  g_return_if_fail (gtk_selection_data_get_data(sd));

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

  gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget), x, y, &path_where_insert, &position);

  xfburn_busy_cursor (priv->content);

  /* move a selection inside of the composition window */
  if (gtk_selection_data_get_target(sd) == gdk_atom_intern ("XFBURN_TREE_PATHS", FALSE)) {
    GList *row = NULL, *selected_rows = NULL;
    GtkTreeIter *iter = NULL;
    GtkTreeIter *iter_prev = NULL;
    AudioCompositionEntryType type_dest = -1;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));

    row = selected_rows = *((GList **) (gpointer) gtk_selection_data_get_data(sd));

    if (path_where_insert) {
      iter_where_insert = g_new0 (GtkTreeIter, 1);
      gtk_tree_model_get_iter (model, iter_where_insert, path_where_insert);
      iter_prev = iter_where_insert;

      gtk_tree_model_get (model, iter_where_insert, AUDIO_COMPOSITION_COLUMN_TYPE, &type_dest, -1);

      if (type_dest == AUDIO_COMPOSITION_TYPE_RAW) {
        if (position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
          position = GTK_TREE_VIEW_DROP_AFTER;
        else if (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
          position = GTK_TREE_VIEW_DROP_BEFORE;
      }
    } else {
      position = GTK_TREE_VIEW_DROP_AFTER;
    }

    /* copy selection */
    while (row) {
      GtkTreePath *path_src = NULL;
      GtkTreeIter iter_src;
      GtkTreeRowReference *reference = NULL;
      AudioCompositionEntryType type;
      guint64 size = 0;
      int secs = 0;

      reference = (GtkTreeRowReference *) row->data;

      path_src = gtk_tree_row_reference_get_path (reference);
      if (!path_src) {
        gtk_tree_row_reference_free (reference);

        row = g_list_next (row);
        continue;
      }

      gtk_tree_model_get_iter (model, &iter_src, path_src);
      gtk_tree_model_get (model, &iter_src, AUDIO_COMPOSITION_COLUMN_TYPE, &type,
                                            AUDIO_COMPOSITION_COLUMN_LENGTH, &secs,
                                            AUDIO_COMPOSITION_COLUMN_SIZE, &size, -1);

      /* copy entry */
      if ((iter = copy_entry_to (composition, &iter_src, iter_prev, position)) != NULL) {
        GtkTreePath *path_parent = gtk_tree_path_copy (path_src);

        gtk_tree_store_remove (GTK_TREE_STORE (model), &iter_src);

        gtk_tree_path_free (path_parent);
        g_free (iter_prev);
        iter_prev = iter;
        /* selection is in reverse order, so insert next item before */
        position = GTK_TREE_VIEW_DROP_BEFORE;
      }

      gtk_tree_path_free (path_src);
      gtk_tree_row_reference_free (reference);

      row = g_list_next (row);
    }

    g_free (iter);
    g_list_free (selected_rows);
    gtk_drag_finish (dc, TRUE, FALSE, t);

    if (path_where_insert)
      gtk_tree_path_free (path_where_insert);

    tracks_changed (composition);
    gtk_widget_hide (priv->progress);
    xfburn_default_cursor (priv->content);
  }
  /* drag from the file selector */
  else if (gtk_selection_data_get_target(sd) == gdk_atom_intern ("text/plain;charset=utf-8", FALSE)) {
    ThreadAddFilesDragParams *params;
    gchar **files = NULL;
    gboolean ret = FALSE;
    gchar *full_paths;
    int i;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));

    full_paths = (gchar *) gtk_selection_data_get_text (sd);

    files = g_strsplit ((gchar *) full_paths, "\n", -1);

    if (files)
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
        ret = TRUE;
      }
    /* end if files */
    g_strfreev (files);
    g_free (full_paths);

    priv->path_where_insert = path_where_insert;

    if (ret) {
      /* we at least will add one file */
      params = g_new (ThreadAddFilesDragParams, 1);
      params->composition = composition;
      params->position = position;
      params->widget = widget;

      /* append a dummy row so that gtk doesn't freak out */
      gtk_tree_store_append (GTK_TREE_STORE (model), &params->iter_dummy, NULL);

      priv->thread_params = params;
      g_thread_new ("add_files_drag", (GThreadFunc) thread_add_files_drag, params);

      gtk_drag_finish (dc, TRUE, FALSE, t);
    } else {
      gtk_drag_finish (dc, FALSE, FALSE, t);
      cb_adding_done (XFBURN_ADDING_PROGRESS (priv->progress), composition);
    }
  }
  else if (gtk_selection_data_get_target(sd) == gdk_atom_intern ("text/uri-list", FALSE)) {
    GList *vfs_paths = NULL;
    GList *vfs_path;
    GList *lp;
    gchar *full_path;
    gchar **uris;
    gsize   n;
    gboolean ret = FALSE;

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
        ret = TRUE;
      }

      for (lp = vfs_paths; lp != NULL; lp = lp->next)
        g_object_unref (lp->data);
      g_list_free (vfs_paths);

      priv->full_paths_to_add = g_list_reverse (priv->full_paths_to_add);
      priv->path_where_insert = path_where_insert;

      if (ret) {
        params = g_new (ThreadAddFilesDragParams, 1);
        params->composition = composition;
        params->position = position;
        params->widget = widget;

        /* append a dummy row so that gtk doesn't freak out */
        gtk_tree_store_append (GTK_TREE_STORE (model), &params->iter_dummy, NULL);

        priv->thread_params = params;
        g_thread_new ("audio_add_uri_list", (GThreadFunc) thread_add_files_drag, params);

        gtk_drag_finish (dc, TRUE, FALSE, t);
      } else {
        gtk_drag_finish (dc, FALSE, FALSE, t);
        cb_adding_done (XFBURN_ADDING_PROGRESS (priv->progress), composition);
      }
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
  XfburnAudioComposition *composition = params->composition;
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);

  GtkTreeViewDropPosition position = params->position;
  GtkWidget *widget = params->widget;

  GtkTreeModel *model;
  GtkTreeIter iter_where_insert;
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

      if (thread_add_file_to_list (composition, model, full_path, &iter, &iter_where_insert, position)) {
        if (position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE
            || position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
          gdk_threads_enter ();
          gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), priv->path_where_insert, FALSE);
          gdk_threads_leave ();
        }
      }

    } else  {
      thread_add_file_to_list (composition, model, full_path, &iter, NULL, position);
    }
  }
  xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
  return NULL;
}

/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_audio_composition_new (void)
{
  return g_object_new (xfburn_audio_composition_get_type (), NULL);
}

void
xfburn_audio_composition_add_files (XfburnAudioComposition *dc, GSList * filelist)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  ThreadAddFilesCLIParams *params;

  if (filelist != NULL) {
    params = g_new (ThreadAddFilesCLIParams, 1);

    params->filelist = filelist;
    params->dc = dc;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));
    xfburn_busy_cursor (priv->content);

    priv->thread_params = params;
    g_thread_new ("audio_add_files_cli", (GThreadFunc) thread_add_files_cli, params);
  }
}

void
xfburn_audio_composition_hide_toolbar (XfburnAudioComposition * composition)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);

  gtk_widget_hide (priv->toolbar);
}

void
xfburn_audio_composition_show_toolbar (XfburnAudioComposition * composition)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);

  gtk_widget_show (priv->toolbar);
}

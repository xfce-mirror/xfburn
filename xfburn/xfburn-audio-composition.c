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
#include <fcntl.h>

#include <errno.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#ifdef HAVE_THUNAR_VFS
#include <thunar-vfs/thunar-vfs.h>
#endif

#include <exo/exo.h>

#include <libisofs.h>

#include "xfburn-audio-composition.h"
#include "xfburn-global.h"

#include "xfburn-adding-progress.h"
#include "xfburn-composition.h"
#include "xfburn-audio-disc-usage.h"
#include "xfburn-main-window.h"
#include "xfburn-utils.h"
#include "xfburn-burn-audio-cd-composition-dialog.h"

#define XFBURN_AUDIO_COMPOSITION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_AUDIO_COMPOSITION, XfburnAudioCompositionPrivate))

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
  AUDIO_COMPOSITION_N_COLUMNS
};

enum
{
  AUDIO_COMPOSITION_DISPLAY_COLUMN_POS,
  AUDIO_COMPOSITION_DISPLAY_COLUMN_LENGTH,
#if 0 // Needs libburn cd-text suppport
  AUDIO_COMPOSITION_DISPLAY_COLUMN_ARTIST,
  AUDIO_COMPOSITION_DISPLAY_COLUMN_TITLE,
#endif
  AUDIO_COMPOSITION_DISPLAY_COLUMN_PATH,
  AUDIO_COMPOSITION_DISPLAY_N_COLUMNS
};

typedef enum
{
  NOT_ADDING_EXT,
  NOT_ADDING_FMT,
} XfburnNotAddingReason;

typedef enum
{
  AUDIO_COMPOSITION_TYPE_RAW,
} AudioCompositionEntryType;


/* thread parameters */
typedef struct {
  char **filenames;
  int filec;
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
static void xfburn_audio_composition_class_init (XfburnAudioCompositionClass *);
static void composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data);
static void xfburn_audio_composition_init (XfburnAudioComposition *dc);
static void xfburn_audio_composition_finalize (GObject * object);

static void show_custom_controls (XfburnComposition *composition);
static void hide_custom_controls (XfburnComposition *composition);
static void load_from_file (XfburnComposition *composition, const gchar *file);
static void save_to_file (XfburnComposition *composition);

//static gint song_tree_sortfunc (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data);

static void action_clear (GtkAction *, XfburnAudioComposition *);
static void action_remove_selection (GtkAction *, XfburnAudioComposition *);
static void action_rename_selection_artist (GtkAction *, XfburnAudioComposition *);
static void action_rename_selection_title (GtkAction *, XfburnAudioComposition *);
static void action_add_selected_files (GtkAction *, XfburnAudioComposition *);

static void tracks_changed (XfburnAudioComposition *ac);
static gboolean cb_treeview_button_pressed (GtkTreeView * treeview, GdkEventButton * event, XfburnAudioComposition * dc);
static void cb_selection_changed (GtkTreeSelection *selection, XfburnAudioComposition * dc);
static GSList * generate_audio_src (XfburnAudioComposition * ac);
static void cb_begin_burn (XfburnDiscUsage * du, XfburnAudioComposition * dc);
#if 0 // Needs libburn cd-text suppport
static void cb_cell_artist_edited (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnAudioComposition * dc);
static void cb_cell_title_edited (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnAudioComposition * dc);
#endif

static void cb_content_drag_data_rcv (GtkWidget * widget, GdkDragContext * dc, guint x, guint y, GtkSelectionData * sd,
                                      guint info, guint t, XfburnAudioComposition * composition);
static void cb_content_drag_data_get (GtkWidget * widget, GdkDragContext * dc, GtkSelectionData * data, guint info,
                                      guint time, XfburnAudioComposition * content);
static void cb_adding_done (XfburnAddingProgress *progress, XfburnAudioComposition *dc);

/* thread entry points */
static void thread_add_files_cli (ThreadAddFilesCLIParams *params);
static void thread_add_files_action (ThreadAddFilesActionParams *params);
static void thread_add_files_drag (ThreadAddFilesDragParams *params);

/* thread helpers */
static gboolean thread_add_file_to_list_with_name (const gchar *name, XfburnAudioComposition * dc, 
                                                   GtkTreeModel * model, const gchar * path, GtkTreeIter * iter, 
                                                   GtkTreeIter * insertion, GtkTreeViewDropPosition position);
static gboolean thread_add_file_to_list (XfburnAudioComposition * dc, GtkTreeModel * model, const gchar * path, 
                                         GtkTreeIter * iter, GtkTreeIter * insertion, GtkTreeViewDropPosition position);
static gboolean has_audio_ext (const gchar *path);
static gboolean valid_wav_headers (guchar header[44]);
static gboolean is_valid_wav (const gchar *path);
                                  
typedef struct
{
  gchar *filename;
  gboolean modified;
 
  guint n_tracks;

  GList *full_paths_to_add;
  gchar *selected_files;
  GtkTreePath *path_where_insert;

  XfburnNotAddingReason warned_about_not_adding;

  GdkDragContext * dc;
  gboolean success;
  gboolean del;
  guint32 time;

  void *thread_params;
  
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  GtkWidget *toolbar;
  GtkWidget *entry_volume_name;
  GtkWidget *content;
  GtkWidget *disc_usage;
  GtkWidget *progress;

} XfburnAudioCompositionPrivate;

/* globals */
static GtkHPanedClass *parent_class = NULL;
static guint instances = 0;

static const GtkActionEntry action_entries[] = {
  {"add-file", GTK_STOCK_ADD, N_("Add"), NULL, N_("Add the selected file(s) to the composition"),
   G_CALLBACK (action_add_selected_files),},
  {"remove-file", GTK_STOCK_REMOVE, N_("Remove"), NULL, N_("Remove the selected file(s) from the composition"),
   G_CALLBACK (action_remove_selection),},
  {"clear", GTK_STOCK_CLEAR, N_("Clear"), NULL, N_("Clear the content of the composition"),
   G_CALLBACK (action_clear),},
  //{"import-session", "xfburn-import-session", N_("Import"), NULL, N_("Import existing session"),},
  {"rename-artist", GTK_STOCK_EDIT, N_("Rename Artist"), NULL, N_("Rename the artist of the selected file"),
   G_CALLBACK (action_rename_selection_artist),},
  {"rename-title", GTK_STOCK_EDIT, N_("Rename Title"), NULL, N_("Rename the title of the selected file"),
   G_CALLBACK (action_rename_selection_title),},
};

static const gchar *toolbar_actions[] = {
  "add-file",
  "remove-file",
  "clear",
  "import-session",
};

static GdkPixbuf *icon_directory = NULL, *icon_file = NULL;

/********************************/
/* XfburnAudioComposition class */
/********************************/
GtkType
xfburn_audio_composition_get_type (void)
{
  static GtkType audio_composition_type = 0;

  if (!audio_composition_type) {
    static const GTypeInfo audio_composition_info = {
      sizeof (XfburnAudioCompositionClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_audio_composition_class_init,
      NULL,
      NULL,
      sizeof (XfburnAudioComposition),
      0,
      (GInstanceInitFunc) xfburn_audio_composition_init,
      NULL
    };

    static const GInterfaceInfo composition_info = {
      (GInterfaceInitFunc) composition_interface_init,    /* interface_init */
      NULL,                                               /* interface_finalize */
      NULL                                                /* interface_data */
    };
    
    audio_composition_type = g_type_register_static (GTK_TYPE_VBOX, "XfburnAudioComposition", &audio_composition_info, 0);
    
    g_type_add_interface_static (audio_composition_type, XFBURN_TYPE_COMPOSITION, &composition_info);
  }

  return audio_composition_type;
}

static void
xfburn_audio_composition_class_init (XfburnAudioCompositionClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (XfburnAudioCompositionPrivate));
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->finalize = xfburn_audio_composition_finalize;
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
xfburn_audio_composition_init (XfburnAudioComposition * composition)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);
  
  gint x, y;
  ExoToolbarsModel *model_toolbar;
  gint toolbar_position;
  GtkWidget *hbox_toolbar;
  //GtkWidget *hbox, *label;
  GtkWidget *scrolled_window;
  GtkTreeStore *model;
#if 0 // Needs libburn cd-text suppport
  GtkTreeViewColumn *column_artist, *column_title;
  GtkCellRenderer *cell_artist, *cell_title;
#endif
  GtkTreeSelection *selection;
  GtkAction *action = NULL;
  GdkScreen *screen;
  GtkIconTheme *icon_theme;
  
  const gchar ui_string[] = "<ui> <popup name=\"popup-menu\">"
    "<menuitem action=\"rename-artist\"/>" "<menuitem action=\"rename-title\"/>" "<menuitem action=\"remove-file\"/>" "</popup></ui>";

  GtkTargetEntry gte_src[] =  { { "XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, AUDIO_COMPOSITION_DND_TARGET_INSIDE } };
  GtkTargetEntry gte_dest[] = { { "XFBURN_TREE_PATHS", GTK_TARGET_SAME_WIDGET, AUDIO_COMPOSITION_DND_TARGET_INSIDE },
                                { "text/plain", 0, AUDIO_COMPOSITION_DND_TARGET_TEXT_PLAIN },
                                { "text/uri-list", 0, AUDIO_COMPOSITION_DND_TARGET_TEXT_URI_LIST },
                              };

  priv->full_paths_to_add = NULL;

  instances++;
  
  /* initialize static members */
  screen = gtk_widget_get_screen (GTK_WIDGET (composition));
  icon_theme = gtk_icon_theme_get_for_screen (screen);

  gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &x, &y);
  if (!icon_directory)
    icon_directory = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-directory", x, 0, NULL);
  if (!icon_file)
    icon_file = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-regular", x, 0, NULL);

  /* create ui manager */
  priv->action_group = gtk_action_group_new ("xfburn-audio-composition");
  gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (priv->action_group, action_entries, G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (composition));

  priv->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (priv->ui_manager, priv->action_group, 0);

  gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_string, -1, NULL);

  hbox_toolbar = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (composition), hbox_toolbar, FALSE, TRUE, 0);
  gtk_widget_show (hbox_toolbar);
  
  /* toolbar */
  model_toolbar = exo_toolbars_model_new ();
  exo_toolbars_model_set_actions (model_toolbar, (gchar **) toolbar_actions, G_N_ELEMENTS (toolbar_actions));
  toolbar_position = exo_toolbars_model_add_toolbar (model_toolbar, -1, "content-toolbar");
  exo_toolbars_model_set_style (model_toolbar, GTK_TOOLBAR_BOTH, toolbar_position);

  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "add-file", EXO_TOOLBARS_ITEM_TYPE);
  exo_toolbars_model_add_separator (model_toolbar, toolbar_position, -1);
  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "remove-file", EXO_TOOLBARS_ITEM_TYPE);
  exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "clear", EXO_TOOLBARS_ITEM_TYPE);
  //exo_toolbars_model_add_separator (model_toolbar, toolbar_position, -1);
  //exo_toolbars_model_add_item (model_toolbar, toolbar_position, -1, "import-session", EXO_TOOLBARS_ITEM_TYPE);

  priv->toolbar = exo_toolbars_view_new_with_model (priv->ui_manager, model_toolbar);
  gtk_box_pack_start (GTK_BOX (hbox_toolbar), priv->toolbar, TRUE, TRUE, 0);
  gtk_widget_show (priv->toolbar);

    
  /* content treeview */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (composition), scrolled_window, TRUE, TRUE, 0);

  priv->content = exo_tree_view_new ();
  model = gtk_tree_store_new (AUDIO_COMPOSITION_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING,
                              G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);
							  
  /*
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model), AUDIO_COMPOSITION_COLUMN_POS,
                                   song_tree_sortfunc, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), AUDIO_COMPOSITION_COLUMN_CONTENT, GTK_SORT_ASCENDING);
  */
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->content), GTK_TREE_MODEL (model));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->content), TRUE);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  gtk_widget_show (priv->content);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->content);


  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Pos"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_POS, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Length"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_HUMANLENGTH, NULL);
  /*gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Artist"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_ARTIST, NULL);*/
#if 0 // Needs libburn cd-text suppport
  column_artist = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_artist, _("Artist"));

  cell_artist = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_artist, cell_artist, TRUE);
  gtk_tree_view_column_set_attributes (column_artist, cell_artist, "text", AUDIO_COMPOSITION_COLUMN_ARTIST, NULL);
  g_signal_connect (G_OBJECT (cell_artist), "edited", G_CALLBACK (cb_cell_artist_edited), composition);
  g_object_set (G_OBJECT (cell_artist), "editable", TRUE, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->content), column_artist);

  /*gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Song Name"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_TITLE, NULL);*/
  column_title = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_title, _("Title"));

  cell_title = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_title, cell_title, TRUE);
  gtk_tree_view_column_set_attributes (column_title, cell_title, "text", AUDIO_COMPOSITION_COLUMN_TITLE, NULL);
  g_signal_connect (G_OBJECT (cell_title), "edited", G_CALLBACK (cb_cell_title_edited), composition);
  g_object_set (G_OBJECT (cell_title), "editable", TRUE, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->content), column_title);
#endif

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->content), -1, _("Filename"),
                                               gtk_cell_renderer_text_new (), "text", AUDIO_COMPOSITION_COLUMN_PATH, NULL);

  /* Position  */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_POS), FALSE);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_POS), 20);
  /* Length */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_LENGTH), FALSE);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_LENGTH), 60);
#if 0 // Needs libburn cd-text suppport
  /* Artist */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_ARTIST), TRUE);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_ARTIST), 80);
  /* Song Name */
  gtk_tree_view_column_set_resizable (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_TITLE), TRUE);
  gtk_tree_view_column_set_min_width (gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), AUDIO_COMPOSITION_DISPLAY_COLUMN_TITLE), 100);
#endif
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
                    
  action = gtk_action_group_get_action (priv->action_group, "remove-file");
  gtk_action_set_sensitive (GTK_ACTION (action), FALSE);
}

static void
xfburn_audio_composition_finalize (GObject * object)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (object);
  
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
      XfburnAudioTrack *track = g_new0 (XfburnAudioTrack, 1);

      gtk_tree_model_get (model, &iter, 
                          AUDIO_COMPOSITION_COLUMN_PATH, &track->inputfile,
                          AUDIO_COMPOSITION_COLUMN_POS, &track->pos,
                          -1);

      list = g_slist_prepend (list, track);

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
    xfce_err (_("Cannot burn audio onto a DVD"));
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
  GtkAction *action = NULL;
  
  
  n_selected_rows = gtk_tree_selection_count_selected_rows (selection);
  if (n_selected_rows == 0) {
    action = gtk_action_group_get_action (priv->action_group, "add-file");
    gtk_action_set_sensitive (GTK_ACTION (action), TRUE);
    action = gtk_action_group_get_action (priv->action_group, "remove-file");
    gtk_action_set_sensitive (GTK_ACTION (action), FALSE);  
  } else if (n_selected_rows == 1) {
    action = gtk_action_group_get_action (priv->action_group, "add-file");
    gtk_action_set_sensitive (GTK_ACTION (action), TRUE);
    action = gtk_action_group_get_action (priv->action_group, "remove-file");
    gtk_action_set_sensitive (GTK_ACTION (action), TRUE);
  } else {
    action = gtk_action_group_get_action (priv->action_group, "add-file");
    gtk_action_set_sensitive (GTK_ACTION (action), FALSE);
    action = gtk_action_group_get_action (priv->action_group, "remove-file");
    gtk_action_set_sensitive (GTK_ACTION (action), TRUE);     
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
    GtkWidget *menuitem_remove;
    GtkWidget *menuitem_rename_artist;
    GtkWidget *menuitem_rename_title;

    selection = gtk_tree_view_get_selection (treeview);

    if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
      gtk_tree_selection_unselect_all (selection);
      gtk_tree_selection_select_path (selection, path);
      gtk_tree_path_free (path);
    }

    menu_popup = gtk_ui_manager_get_widget (priv->ui_manager, "/popup-menu");
    menuitem_remove = gtk_ui_manager_get_widget (priv->ui_manager, "/popup-menu/remove-file");
    menuitem_rename_artist = gtk_ui_manager_get_widget (priv->ui_manager, "/popup-menu/rename-artist");
    menuitem_rename_title = gtk_ui_manager_get_widget (priv->ui_manager, "/popup-menu/rename-title");

    if (gtk_tree_selection_count_selected_rows (selection) >= 1)
      gtk_widget_set_sensitive (menuitem_remove, TRUE);
    else
      gtk_widget_set_sensitive (menuitem_remove, FALSE);
    if (gtk_tree_selection_count_selected_rows (selection) == 1) {
      gtk_widget_set_sensitive (menuitem_rename_artist, TRUE);
      gtk_widget_set_sensitive (menuitem_rename_title, TRUE);
    } else {
      gtk_widget_set_sensitive (menuitem_rename_artist, FALSE);
      gtk_widget_set_sensitive (menuitem_rename_title, FALSE);
    }

    gtk_menu_popup (GTK_MENU (menu_popup), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time ());
    return TRUE;
  }

  return FALSE;
}

/*
static gint
song_tree_sortfunc (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
  guint *apos, *bpos;
  AudioCompositionEntryType atype = -1, btype = -1;
  gint result = 0;

  gtk_tree_model_get (model, a, AUDIO_COMPOSITION_COLUMN_POS, &apos, AUDIO_COMPOSITION_COLUMN_TYPE, &atype, -1);
  gtk_tree_model_get (model, b, AUDIO_COMPOSITION_COLUMN_POS, &bpos, AUDIO_COMPOSITION_COLUMN_TYPE, &btype, -1);

  result = apos - bpos;

  return result;
}
*/

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

#if 0 // Needs libburn cd-text suppport
static void
cb_cell_artist_edited (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *real_path;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  real_path = gtk_tree_path_new_from_string (path);

  if (gtk_tree_model_get_iter (model, &iter, real_path)) {
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, AUDIO_COMPOSITION_COLUMN_ARTIST, newtext, -1);
  }

  gtk_tree_path_free (real_path);
}

static void
cb_cell_title_edited (GtkCellRenderer * renderer, gchar * path, gchar * newtext, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *real_path;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  real_path = gtk_tree_path_new_from_string (path);

  if (gtk_tree_model_get_iter (model, &iter, real_path)) {
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, AUDIO_COMPOSITION_COLUMN_TITLE, newtext, -1);
  }

  gtk_tree_path_free (real_path);
}
#endif

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
    g_list_foreach (priv->full_paths_to_add, (GFunc) g_free, NULL);
    g_list_free (priv->full_paths_to_add);
    priv->full_paths_to_add = NULL;
  }

  g_free (priv->thread_params);

  tracks_changed (dc);

  xfburn_default_cursor (priv->content);
}

static void
action_rename_selection_artist (GtkAction * action, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  list = gtk_tree_selection_get_selected_rows (selection, &model);

  path = (GtkTreePath *) list->data;
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), 2);

  g_assert (column != NULL);
  
  gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->content), path, column, TRUE);

  gtk_tree_path_free (path);
  g_list_free (list);
}

static void
action_rename_selection_title (GtkAction * action, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content));
  list = gtk_tree_selection_get_selected_rows (selection, &model);

  path = (GtkTreePath *) list->data;
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->content), 3);

  g_assert (column != NULL);
  
  gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->content), path, column, TRUE);

  gtk_tree_path_free (path);
  g_list_free (list);
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
      //GtkTreeIter parent, iter_temp;
      int secs = 0;
      
      gtk_tree_model_get (model, &iter, AUDIO_COMPOSITION_COLUMN_LENGTH, &secs, -1);
      xfburn_disc_usage_sub_size (XFBURN_DISC_USAGE (priv->disc_usage), secs);

      /*
      iter_temp = iter;
      while (gtk_tree_model_iter_parent (model, &parent, &iter_temp)) {
        guint64 old_size;
        gchar *humansize = NULL;

        // updates parent directories size
        gtk_tree_model_get (model, &parent, AUDIO_COMPOSITION_COLUMN_SIZE, &old_size, -1);

        humansize = xfburn_humanreadable_filesize (old_size - size);
        gtk_tree_store_set (GTK_TREE_STORE (model), &parent, 
                            AUDIO_COMPOSITION_COLUMN_SIZE, old_size - size, -1);

        iter_temp = parent;

        g_free (humansize);
      }
      */
      
      gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
    }
    
    gtk_tree_path_free (path);
  }
  
  gtk_tree_row_reference_free (reference);
}

static void
action_remove_selection (GtkAction * action, XfburnAudioComposition * dc)
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
action_add_selected_files (GtkAction *action, XfburnAudioComposition *dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  XfburnFileBrowser *browser = xfburn_main_window_get_file_browser (xfburn_main_window_get_instance ());
  
  gchar *selected_files = NULL;
  
  xfburn_busy_cursor (priv->content);
  selected_files = xfburn_file_browser_get_selection (browser);
  
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
      priv->path_where_insert = (GtkTreePath *) (selected_paths->data);

      gtk_tree_model_get_iter (params->model, &params->iter_where_insert, priv->path_where_insert);
      gtk_tree_model_get (params->model, &params->iter_where_insert, AUDIO_COMPOSITION_COLUMN_TYPE, &params->type, -1);
    }
    
    priv->selected_files = selected_files;

    priv->thread_params = params;
    g_thread_create ((GThreadFunc) thread_add_files_action, params, FALSE, NULL);
    
    g_list_foreach (selected_paths, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (selected_paths);
  }
}

static void
action_clear (GtkAction * action, XfburnAudioComposition * dc)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  gtk_tree_store_clear (GTK_TREE_STORE (model));
  
  xfburn_disc_usage_set_size (XFBURN_DISC_USAGE (priv->disc_usage), 0);
}

static void
cb_content_drag_data_get (GtkWidget * widget, GdkDragContext * dc,
                          GtkSelectionData * data, guint info, guint time, XfburnAudioComposition * content)
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
    XfburnMainWindow *mainwin;
    GtkUIManager *ui_manager;
    GtkActionGroup *action_group;
  
    mainwin = xfburn_main_window_get_instance ();
    ui_manager = xfburn_main_window_get_ui_manager (mainwin);
  
    action_group = (GtkActionGroup *) gtk_ui_manager_get_action_groups (ui_manager)->data;
    
    /*
    action = gtk_action_group_get_action (action_group, "save-composition");
    gtk_action_set_sensitive (GTK_ACTION (action), TRUE);
  */
    priv->modified = TRUE;
  }
}

static gboolean 
has_audio_ext (const gchar *path)
{
  int len = strlen (path);
  const gchar *ext = path + len - 3;

  return (strcmp (ext, "wav") == 0);
}

static gboolean 
is_valid_wav (const gchar *path)
{
  int fd;
  guchar header[44];
  gboolean ret;

  fd = open (path, 0);

  if (fd == -1) {
    xfce_warn (_("Could not open %s!"), path);
    return FALSE;
  }

  read (fd, header, 44);

  ret = valid_wav_headers (header);

  close (fd);

  return ret;
}

/*
 * Simple check of .wav headers, most info from
 * http://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 *
 * This check might very well not be complete, but should catch
 * the most important pieces.
 * FIXME: eventually replace this with a proper check,
 * also this works on x86, and does not consider endianness!
 */
static gboolean
valid_wav_headers (guchar header[44])
{
  /* check if first 4 bytes are RIFF or RIFX */
  if (header[0] == 'R' && header[1] == 'I' && header[2] == 'F') {
    if (!(header[3] == 'X' || header[3] == 'F')) {
      g_warning ("File not in riff format");
      return FALSE;
    }
  }

  /* check if bytes 8-11 are WAVE */
  if (!(header[8] == 'W' && header[9] == 'A' && header[10] == 'V' && header[11] == 'E')) {
    g_warning ("RIFF file not in WAVE format");
    return FALSE;
  }

  /* subchunk starts with 'fmt ' */
  if (!(header[12] == 'f' && header[13] == 'm' && header[14] == 't' && header[15] == ' ')) {
    g_warning ("Could not find format subchunk");
    return FALSE;
  }

  /* check for PCM format */
  if (header[16] != 16 || header[20] != 1) {
    g_warning ("Not in PCM format");
    return FALSE;
  }

  /* check for stereo */
  if (header[22] != 2) {
    g_warning ("Not in stereo");
    return FALSE;
  }

  /* check for 44100 Hz sample rate,
   * being lazy here and just compare the bytes to what I know they should be */
  if (!(header[24] == 0x44 && header[25] == 0xAC && header[26] == 0 && header[27] == 0)) {
    g_warning ("Does not have a sample rate of 44100 Hz");
    return FALSE;
  }

  return TRUE;
}

static void
notify_not_adding (XfburnAudioComposition * dc, XfburnNotAddingReason r, const gchar *path)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);

  if (!(priv->warned_about_not_adding & r)) {
    const gchar *str;

    priv->warned_about_not_adding |= r;

    switch (r) {
      case NOT_ADDING_EXT:
        xfce_warn (_("At the moment only .wav files can get added to the compilation!"));
        break;
      case NOT_ADDING_FMT:
        xfce_warn (_("At the moment only CD quality uncompressed (pcm) .wav files can get added to the compilation!"));
        break;
      default:
        /* This should never happen... */
        str = _("Unsupported files cannot get added to the compilation!");
        xfce_warn (str);
        g_warning (str);
        break;
    }
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
      xfce_err (_("A file with the same name is already present in the composition"));

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
      guint64 total_size = 4;
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
            total_size += size; 
            if (iter_last == NULL)
              iter_last = g_new (GtkTreeIter, 1);
            *iter_last = new_iter;
            position = GTK_TREE_VIEW_DROP_AFTER;
            ret = TRUE;
          }
          
          g_free (new_path);
        }
      }

      if (insertion == NULL)
        g_free (iter_last);

      g_dir_close (dir);
    }
    /* new file */
    else if (S_ISREG (s.st_mode)) {
      if (!has_audio_ext (path)) {
        gdk_threads_enter ();
        notify_not_adding (dc, NOT_ADDING_EXT, path);
        gdk_threads_leave ();
        return FALSE;
      }

      if (!is_valid_wav (path)) {
        gdk_threads_enter ();
        notify_not_adding (dc, NOT_ADDING_FMT, path);
        gdk_threads_leave ();
        return FALSE;
      }

      gdk_threads_enter ();
      if (insertion != NULL) {
        if (position == GTK_TREE_VIEW_DROP_AFTER)
          gtk_tree_store_insert_after (GTK_TREE_STORE (model), iter, NULL, insertion);
        else if (position == GTK_TREE_VIEW_DROP_BEFORE)
          gtk_tree_store_insert_before (GTK_TREE_STORE (model), iter, NULL, insertion);
        else
          g_error ("I wasn't expecting this position!");
      } else
        gtk_tree_store_append (GTK_TREE_STORE (model), iter, NULL);
      gdk_threads_leave ();

      /* (filesize - header_size) / bytes_per_seconds */
      secs = (s.st_size - 44) / PCM_BYTES_PER_SECS;
      humanlength = g_strdup_printf ("%2d:%2d", secs / 60, secs % 60);

      if (priv->n_tracks == 99) {
        gdk_threads_enter ();
        xfce_err (_("You can only have a maximum of 99 tracks!"));
        gdk_threads_leave ();
        return FALSE;
      }

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
                          AUDIO_COMPOSITION_COLUMN_TYPE, AUDIO_COMPOSITION_TYPE_RAW, -1);
      gdk_threads_leave ();

      g_free (humanlength);

      xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (priv->disc_usage), secs);
      ret = TRUE;
    }
    //g_free (humansize);

    set_modified (priv);
    return ret;
  }
  
  return FALSE;
}

/* thread entry point */
static void
thread_add_files_cli (ThreadAddFilesCLIParams *params)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (params->dc);
  GtkTreeIter iter;

  GtkTreeModel *model;
  int i;
  gchar *full_path = NULL;

  gdk_threads_enter ();
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  gdk_threads_leave ();

  for (i=0; i<params->filec; i++) {
    full_path = g_build_filename (params->filenames[i], NULL);
    g_message ("Adding %s to the audio composition... (might take a while)", full_path);
    thread_add_file_to_list (params->dc, model, full_path, &iter, NULL, GTK_TREE_VIEW_DROP_AFTER);  
    g_free (full_path);
  }
  xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
}

static gboolean
show_add_home_question_dialog ()
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
static void
thread_add_files_action (ThreadAddFilesActionParams *params)
{
  XfburnAudioComposition *dc = params->dc;
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  GtkTreeModel *model = params->model;
  GtkTreeIter iter_where_insert = params->iter_where_insert;
  //GtkTreePath *path_where_insert = priv->path_where_insert;
  const gchar * file = NULL;


  file = strtok (priv->selected_files, "\n");
  while (file) {
    GtkTreeIter iter;
    gchar *full_path = NULL;
    
    if (g_str_has_prefix (file, "file://"))
      full_path = g_build_filename (&file[7], NULL);
    else if (g_str_has_prefix (file, "file:"))
      full_path = g_build_filename (&file[5], NULL);
    else
      full_path = g_build_filename (file, NULL);

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

    file = strtok (NULL, "\n");
  }
  xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
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
  guint64 size = 0;
  gchar *path = NULL;
  gchar *humanlength;
  AudioCompositionEntryType type;

  GtkTreePath *path_level = NULL;
  
  //guint n_children = 0;
  //guint i;
  GtkTreePath *path_src = NULL;
  
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->content));
  
  gtk_tree_model_get (model, src, 
                      AUDIO_COMPOSITION_COLUMN_POS, &pos, 
                      AUDIO_COMPOSITION_COLUMN_CONTENT, &name,
                      AUDIO_COMPOSITION_COLUMN_SIZE, &size,
                      AUDIO_COMPOSITION_COLUMN_PATH, &path, 
                      AUDIO_COMPOSITION_COLUMN_HUMANLENGTH, &humanlength, 
                      AUDIO_COMPOSITION_COLUMN_TYPE, &type,
                      -1);
  
  //DBG ("dest = %p", dest);
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
    
      /*
      if (file_exists_on_same_level (model, path_level, FALSE, name)) {
        xfce_warn (_("A file named \"%s\" already exists in this directory, the file hasn't been added"), name);
        goto cleanup;
      }
      */
      
      gtk_tree_path_free (path_level);
      
      gtk_tree_store_append (GTK_TREE_STORE (model), iter_new, dest);                          
      break;
  }
  
  gtk_tree_store_set (GTK_TREE_STORE (model), iter_new,
                      AUDIO_COMPOSITION_COLUMN_POS, pos, 
                      AUDIO_COMPOSITION_COLUMN_CONTENT, name,
                      AUDIO_COMPOSITION_COLUMN_SIZE, size,
                      AUDIO_COMPOSITION_COLUMN_PATH, path, 
                      AUDIO_COMPOSITION_COLUMN_HUMANLENGTH, humanlength, 
                      AUDIO_COMPOSITION_COLUMN_TYPE, type,
                      -1);
    
  /* copy children */
  /*
  n_children = gtk_tree_model_iter_n_children (model, src);

  for (i = 0; i < n_children; i++) {
    GtkTreeIter iter_child;

    if (gtk_tree_model_iter_nth_child (model, &iter_child, src, i))
      copy_entry_to (dc, &iter_child, iter_new, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
  }

  path_src = gtk_tree_model_get_path (model, src);
  if (n_children > 0 && gtk_tree_view_row_expanded (GTK_TREE_VIEW (priv->content), path_src)) {
    GtkTreePath *path_new = NULL;
    
    path_new = gtk_tree_model_get_path (model, iter_new);
    gtk_tree_view_expand_row (GTK_TREE_VIEW (priv->content), path_new, FALSE);
    
    gtk_tree_path_free (path_new);
  }
  */
  gtk_tree_path_free (path_src);
  
//cleanup:
  if (G_LIKELY (G_IS_OBJECT (icon)))
    g_object_unref (icon);
  g_free (name);
  g_free (humanlength);
  g_free (path);
  
  return iter_new;
}

/*
static GtkTreeModel *mymodel;
static GtkTreeIter myiter;

static gboolean
remove_dummy_row ()
{
  gtk_tree_store_remove (GTK_TREE_STORE (mymodel), &myiter);
  return FALSE;
}
*/

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
  g_return_if_fail (sd->data);
  
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

  /*
   * This would be a workaround for the GtkTreeView critical,
   * except that in this case the memory for the iter would never
   * be reclaimed.
   *
  gtk_drag_finish (dc, FALSE, FALSE, t);
  iter_where_insert = g_new0 (GtkTreeIter, 1);
  gtk_tree_store_append (GTK_TREE_STORE (model), iter_where_insert, NULL);
  mymodel = model;
  myiter = *iter_where_insert;
  g_idle_add (remove_dummy_row, NULL);
  return;
  */

  
  gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget), x, y, &path_where_insert, &position);
  
  xfburn_busy_cursor (priv->content);

  /* move a selection inside of the composition window */
  if (sd->target == gdk_atom_intern ("XFBURN_TREE_PATHS", FALSE)) {
    GList *row = NULL, *selected_rows = NULL;
    GtkTreeIter *iter = NULL;
    GtkTreeIter *iter_prev = NULL;
    AudioCompositionEntryType type_dest = -1;
    
    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));

    row = selected_rows = *((GList **) sd->data);
    
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
      //position = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
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
      
      /*
      DBG ("path_where_insert = %p", path_where_insert);
      if (path_where_insert && (position == GTK_TREE_VIEW_DROP_AFTER || position == GTK_TREE_VIEW_DROP_BEFORE) 
          && (gtk_tree_path_get_depth (path_where_insert) == gtk_tree_path_get_depth (path_src))) {
          gtk_tree_path_free (path_src);
          gtk_tree_row_reference_free (reference);
      
          row = g_list_next (row);
          continue;
      }
      */

      gtk_tree_model_get_iter (model, &iter_src, path_src);
      gtk_tree_model_get (model, &iter_src, AUDIO_COMPOSITION_COLUMN_TYPE, &type,
                                            AUDIO_COMPOSITION_COLUMN_LENGTH, &secs,
                                            AUDIO_COMPOSITION_COLUMN_SIZE, &size, -1);
      
      /* copy entry */
      if ((iter = copy_entry_to (composition, &iter_src, iter_prev, position)) != NULL) {
        GtkTreePath *path_parent = gtk_tree_path_copy (path_src);
        
        if (dc->action == GDK_ACTION_MOVE) {
          /* remove source entry */
          /*
           * This shouldn't be able to happen anymore w/o folders
          if (gtk_tree_path_up (path_parent) && path_where_insert && 
              !gtk_tree_path_is_descendant (path_where_insert, path_parent)) {
            // update parent size and humansize
            GtkTreeIter iter_parent;          
            guint64 old_size;
            
            gtk_tree_model_iter_parent (model, &iter_parent, &iter_src);                
            gtk_tree_model_get (model, &iter_parent, AUDIO_COMPOSITION_COLUMN_SIZE, &old_size, -1);
         
            gtk_tree_store_set (GTK_TREE_STORE (model), &iter_parent, 
                                AUDIO_COMPOSITION_COLUMN_SIZE, old_size - size, -1);
          }
          */
          gtk_tree_store_remove (GTK_TREE_STORE (model), &iter_src);

        } else {
          xfburn_disc_usage_add_size (XFBURN_DISC_USAGE (priv->disc_usage), secs);
        }
        
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
  else if (sd->target == gdk_atom_intern ("text/plain", FALSE)) {
    ThreadAddFilesDragParams *params;
    const gchar *file = NULL;
    gboolean ret = FALSE;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));
    
    file = strtok ((gchar *) sd->data, "\n");
    while (file) {
      gchar *full_path;

      if (g_str_has_prefix (file, "file://"))
        full_path = g_build_filename (&file[7], NULL);
      else if (g_str_has_prefix ((gchar *) sd->data, "file:"))
        full_path = g_build_filename (&file[5], NULL);
      else
        full_path = g_build_filename (file, NULL);

      if (full_path[strlen (full_path) - 1] == '\r')
        full_path[strlen (full_path) - 1] = '\0';

      /* remember path to add it later in another thread */
      if (has_audio_ext (full_path)) {
        priv->full_paths_to_add = g_list_append (priv->full_paths_to_add, full_path);
        ret = TRUE;
      } else {
        notify_not_adding (composition, NOT_ADDING_EXT, full_path);
      }

      file = strtok (NULL, "\n");
    }

    priv->full_paths_to_add = g_list_reverse (priv->full_paths_to_add);
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
      g_thread_create ((GThreadFunc) thread_add_files_drag, params, FALSE, NULL);

      gtk_drag_finish (dc, TRUE, FALSE, t);
    } else {
      gtk_drag_finish (dc, FALSE, FALSE, t);
      cb_adding_done (XFBURN_ADDING_PROGRESS (priv->progress), composition);
    }
  } 
  else if (sd->target == gdk_atom_intern ("text/uri-list", FALSE)) {
#ifdef HAVE_THUNAR_VFS
    GList *vfs_paths = NULL;
    GList *vfs_path;
    GError *error = NULL;
    gchar *full_path;
    gboolean ret = FALSE;

    vfs_paths = thunar_vfs_path_list_from_string ((gchar *) sd->data, &error);

    if (G_LIKELY (vfs_paths != NULL)) {
      ThreadAddFilesDragParams *params;
      priv->full_paths_to_add = NULL;
      for (vfs_path = vfs_paths; vfs_path != NULL; vfs_path = g_list_next (vfs_path)) {
        ThunarVfsPath *path = THUNAR_VFS_PATH (vfs_path->data);
        if (thunar_vfs_path_get_scheme (path) != THUNAR_VFS_PATH_SCHEME_FILE)
          continue;
        full_path = thunar_vfs_path_dup_string (path);
        g_debug ("adding uri path: %s", full_path);

        if (has_audio_ext (full_path)) {
          priv->full_paths_to_add = g_list_prepend (priv->full_paths_to_add, full_path);
          ret = TRUE;
        } else {
          notify_not_adding (composition, NOT_ADDING_EXT, full_path);
        }
      }
      thunar_vfs_path_list_free (vfs_paths);

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
        g_thread_create ((GThreadFunc) thread_add_files_drag, params, FALSE, NULL);

        gtk_drag_finish (dc, TRUE, FALSE, t);
      } else {
        gtk_drag_finish (dc, FALSE, FALSE, t);
        cb_adding_done (XFBURN_ADDING_PROGRESS (priv->progress), composition);
      }
    } else {
      if (G_UNLIKELY (error != NULL))
        g_warning ("text/uri-list drag failed because '%s'", error->message);
      else
        g_warning("There were no files in the uri list!");
      gtk_drag_finish (dc, FALSE, FALSE, t);
      xfburn_default_cursor (priv->content);
    }
#else
    g_warning ("Receiving this type of drag and drop requires thunar-vfs support, sorry!");
    gtk_drag_finish (dc, FALSE, FALSE, t);
    xfburn_default_cursor (priv->content);
#endif
  } 
  else {
    g_warning ("Trying to receive an unsupported drag target, this should not happen.");
    gtk_drag_finish (dc, FALSE, FALSE, t);
    xfburn_default_cursor (priv->content);
  }
}

/* thread entry point */
static void
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
            || position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
          gdk_threads_enter ();
          gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), priv->path_where_insert, FALSE);
          gdk_threads_leave ();
      }
      
    } else  {
      thread_add_file_to_list (composition, model, full_path, &iter, NULL, position);
    }
  }
  xfburn_adding_progress_done (XFBURN_ADDING_PROGRESS (priv->progress));
}

/****************/
/* loading code */
/****************/
typedef struct
{
  gboolean started;
  XfburnAudioComposition *dc;
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
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (parserinfo->dc);

  
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
          
      g_error ("This method needs to get fixed, and does not work right now!");
      /*
      add_file_to_list_with_name (attribute_values[i], parserinfo->dc, model, attribute_values[j], &iter, 
                                  parent, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
      */
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
  parserinfo.dc = XFBURN_AUDIO_COMPOSITION (composition);
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
  AudioCompositionEntryType type;

  space = g_strnfill (gtk_tree_path_get_depth (path), '\t');
  
  for (i = info->last_depth; i > gtk_tree_path_get_depth (path); i--) {
    gchar *space2 = NULL;

    space2 = g_strnfill (i - 1, '\t');
    fprintf (info->file_content, "%s</directory>\n", space2);
    
    g_free (space2);
  }
  
  gtk_tree_model_get (model, iter, AUDIO_COMPOSITION_COLUMN_CONTENT, &name,
                      AUDIO_COMPOSITION_COLUMN_PATH, &source_path,
                      AUDIO_COMPOSITION_COLUMN_TYPE, &type, -1);
  
  fprintf (info->file_content, "%s", space);
  switch (type) {
  case AUDIO_COMPOSITION_TYPE_RAW:
    fprintf (info->file_content, "<file name=\"%s\" source=\"%s\" />\n", name, source_path);
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
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (composition);
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
xfburn_audio_composition_new (void)
{
  return g_object_new (xfburn_audio_composition_get_type (), NULL);
}

void 
xfburn_audio_composition_add_files (XfburnAudioComposition *dc, int filec, char **filenames)
{
  XfburnAudioCompositionPrivate *priv = XFBURN_AUDIO_COMPOSITION_GET_PRIVATE (dc);
  ThreadAddFilesCLIParams *params;
  
  if (filec > 0) {
    params = g_new (ThreadAddFilesCLIParams, 1);

    params->filenames = filenames;
    params->filec = filec;
    params->dc = dc;

    xfburn_adding_progress_show (XFBURN_ADDING_PROGRESS (priv->progress));
    xfburn_busy_cursor (priv->content);

    priv->thread_params = params;
    g_thread_create ((GThreadFunc) thread_add_files_cli, params, FALSE, NULL);
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


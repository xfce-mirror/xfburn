$Id$
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

#include <exo/exo.h>

#include "xfburn-main-window.h"
#include "xfburn-preferences-dialog.h"
#include "xfburn-file-browser.h"
#include "xfburn-disc-content.h"
#include "xfburn-blank-cd-dialog.h"
#include "xfburn-copy-cd-dialog.h"
#include "xfburn-burn-composition-dialog.h"
#include "xfburn-burn-image-dialog.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-settings.h"

/* prototypes */
static void xfburn_main_window_class_init (XfburnMainWindowClass *);
static void xfburn_main_window_init (XfburnMainWindow *);

static gboolean cb_delete_main_window (GtkWidget *, GdkEvent *, gpointer);
static void cb_edit_toolbars_view (ExoToolbarsView *, gpointer);
static void cb_burn_composition (XfburnDiscContent *dc, XfburnMainWindow * window);

static void xfburn_window_action_about (GtkAction *, XfburnMainWindow *);
static void xfburn_window_action_preferences (GtkAction *, XfburnMainWindow *);

static void xfburn_window_action_load (GtkAction *, XfburnMainWindow *);
static void xfburn_window_action_save (GtkAction *, XfburnMainWindow *);
static void xfburn_window_action_quit (GtkAction *, XfburnMainWindow *);

static void xfburn_window_action_blank_cd (GtkAction *, XfburnMainWindow *);
static void xfburn_window_action_copy_cd (GtkAction *, XfburnMainWindow *);
static void xfburn_window_action_burn_image (GtkAction *, XfburnMainWindow *);

static void xfburn_window_action_refresh_directorybrowser (GtkAction *, XfburnMainWindow *);
static void xfburn_window_action_show_filebrowser (GtkToggleAction *, XfburnMainWindow *);
static void xfburn_window_action_show_toolbar (GtkToggleAction * action, XfburnMainWindow * window);
static void xfburn_window_action_show_content_toolbar (GtkToggleAction * action, XfburnMainWindow * window);

/* globals */
static GtkWindowClass *parent_class = NULL;
static GtkActionEntry action_entries[] = {
  {"file-menu", NULL, N_("_File"), NULL,},
  {"load-composition", GTK_STOCK_OPEN, N_("Load composition"), NULL, N_("Load composition"),
   G_CALLBACK (xfburn_window_action_load),},
  {"save-composition", GTK_STOCK_SAVE, N_("Save composition"), NULL, N_("Save composition"), 
   G_CALLBACK (xfburn_window_action_save),},
  {"quit", GTK_STOCK_QUIT, N_("_Quit"), NULL, N_("Quit Xfburn"), G_CALLBACK (xfburn_window_action_quit),},
  {"edit-menu", NULL, N_("_Edit"), NULL,},
  {"preferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL, N_("Show preferences dialog"),
   G_CALLBACK (xfburn_window_action_preferences),},
  {"action-menu", NULL, N_("_Actions"), NULL,},
  {"view-menu", NULL, N_("_View"), NULL,},
  {"refresh", GTK_STOCK_REFRESH, N_("Refresh"), NULL, N_("Refresh file list"),
   G_CALLBACK (xfburn_window_action_refresh_directorybrowser),},
  {"help-menu", NULL, N_("_Help"), NULL,},
  {"about", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("Display information about Xfburn"),
   G_CALLBACK (xfburn_window_action_about),},
  {"blank-cd", "xfburn-blank-cdrw", N_("Blank CD-RW"), NULL, N_("Blank CD-RW"),
   G_CALLBACK (xfburn_window_action_blank_cd),},
  {"format-dvd", "xfburn-blank-dvdrw", N_("Format DVD-RW"), NULL, N_("Format DVD-RW"),},
  {"copy-data", "xfburn-data-copy", N_("Copy Data CD"), NULL, N_("Copy Data CD"),
   G_CALLBACK (xfburn_window_action_copy_cd),},
  {"copy-audio", "xfburn-audio-copy", N_("Copy Audio CD"), NULL, N_("Copy Audio CD"),},
  {"burn-cd", "xfburn-burn-cd", N_("Burn CD Image"), NULL, N_("Burn CD Image"),
   G_CALLBACK (xfburn_window_action_burn_image),},
};

static GtkToggleActionEntry toggle_action_entries[] = {
  {"show-filebrowser", NULL, N_("Show file browser"), NULL, N_("Show/hide the file browser"),
   G_CALLBACK (xfburn_window_action_show_filebrowser), TRUE,},
  {"show-toolbar", NULL, N_("Show toolbar"), NULL, N_("Show/hide the toolbar"),
   G_CALLBACK (xfburn_window_action_show_toolbar), TRUE,},
  {"show-content-toolbar", NULL, N_("Show disc content toolbar"), NULL, N_("Show/hide the disc content toolbar"),
   G_CALLBACK (xfburn_window_action_show_content_toolbar), TRUE,},
};

static const gchar *toolbar_actions[] = {
  "blank-cd",
  //"format-dvd",
  "copy-data",
  //"copy-audio",
  "burn-cd",
  "refresh",
  "about",
  "preferences",
};

/**************************/
/* XfburnMainWindow class */
/**************************/
GtkType
xfburn_main_window_get_type (void)
{
  static GtkType main_window_type = 0;

  if (!main_window_type) {
    static const GTypeInfo main_window_info = {
      sizeof (XfburnMainWindowClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_main_window_class_init,
      NULL,
      NULL,
      sizeof (XfburnMainWindow),
      0,
      (GInstanceInitFunc) xfburn_main_window_init
    };

    main_window_type = g_type_register_static (GTK_TYPE_WINDOW, "XfburnMainWindow", &main_window_info, 0);
  }

  return main_window_type;
}

static void
xfburn_main_window_class_init (XfburnMainWindowClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_main_window_init (XfburnMainWindow * mainwin)
{
  GtkAccelGroup *accel_group;
  gchar *file;

  GtkWidget *vbox;
  GtkWidget *vpaned;

  /* the window itself */
  gtk_window_set_position (GTK_WINDOW (mainwin), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_title (GTK_WINDOW (mainwin), "Xfburn");
  gtk_widget_set_size_request (GTK_WIDGET (mainwin), 850, 700);

  g_signal_connect (G_OBJECT (mainwin), "delete-event", G_CALLBACK (cb_delete_main_window), mainwin);

  /* create ui manager */
  mainwin->action_group = gtk_action_group_new ("xfburn-main-window");
  gtk_action_group_set_translation_domain (mainwin->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (mainwin->action_group, action_entries, G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (mainwin));
  gtk_action_group_add_toggle_actions (mainwin->action_group, toggle_action_entries,
                                       G_N_ELEMENTS (toggle_action_entries), GTK_WIDGET (mainwin));

  mainwin->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (mainwin->ui_manager, mainwin->action_group, 0);

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfburn/xfburn.ui");

  if (G_LIKELY (file != NULL)) {
    GError *error = NULL;
    if (gtk_ui_manager_add_ui_from_file (mainwin->ui_manager, file, &error) == 0) {
      g_warning ("Unable to load %s: %s", file, error->message);
      g_error_free (error);
    }
    gtk_ui_manager_ensure_update (mainwin->ui_manager);
    g_free (file);
  }
  else {
    g_warning ("Unable to locate xfburn/xfburn.ui !");
  }

  /* accel group */
  accel_group = gtk_ui_manager_get_accel_group (mainwin->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (mainwin), accel_group);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (mainwin), vbox);
  gtk_widget_show (vbox);

  /* menubar */
  mainwin->menubar = gtk_ui_manager_get_widget (mainwin->ui_manager, "/main-menu");
  if (G_LIKELY (mainwin->menubar != NULL)) {
    gtk_box_pack_start (GTK_BOX (vbox), mainwin->menubar, FALSE, FALSE, 0);
    gtk_widget_show (mainwin->menubar);
  }

  /* toolbar */
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfburn/xfburn-toolbars.ui");

  if (G_LIKELY (file != NULL)) {
    ExoToolbarsModel *model;
    GError *error = NULL;

    model = exo_toolbars_model_new ();
    exo_toolbars_model_set_actions (model, (gchar **) toolbar_actions, G_N_ELEMENTS (toolbar_actions));
    if (exo_toolbars_model_load_from_file (model, file, &error)) {
      mainwin->toolbars = exo_toolbars_view_new_with_model (mainwin->ui_manager, model);
      gtk_box_pack_start (GTK_BOX (vbox), mainwin->toolbars, FALSE, FALSE, 0);
      gtk_widget_show (mainwin->toolbars);

      g_signal_connect (G_OBJECT (mainwin->toolbars), "customize", G_CALLBACK (cb_edit_toolbars_view), mainwin);
    }
    else {
      g_warning ("Unable to load %s: %s", file, error->message);
      g_error_free (error);
    }

    g_free (file);
  }
  else {
    g_warning ("Unable to locate xfburn/xfburn-toolbars.ui !");
  }

  /* vpaned */
  vpaned = gtk_vpaned_new ();
  gtk_container_add (GTK_CONTAINER (vbox), vpaned);
  gtk_widget_show (vpaned);

  /* filebrowser */
  mainwin->file_browser = xfburn_file_browser_new ();
  gtk_paned_add1 (GTK_PANED (vpaned), mainwin->file_browser);
  gtk_widget_show (mainwin->file_browser);

  /* disc content */
  mainwin->disc_content = xfburn_disc_content_new ();
  gtk_paned_add2 (GTK_PANED (vpaned), mainwin->disc_content);
  gtk_widget_show (mainwin->disc_content);
  g_signal_connect (G_OBJECT (mainwin->disc_content), "begin-burn", G_CALLBACK (cb_burn_composition), mainwin);
  
  xfce_resource_pop_path (XFCE_RESOURCE_DATA);
}

/* internals */
static void
cb_edit_toolbars_view_done (ExoToolbarsEditorDialog * dialog, ExoToolbarsView * toolbar)
{
  exo_toolbars_view_set_editing (toolbar, FALSE);

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
cb_edit_toolbars_view (ExoToolbarsView * toolbar, gpointer user_data)
{
  GtkWidget *editor_dialog;
  ExoToolbarsModel *model;
  GtkUIManager *ui_manager;
  GtkWidget *toplevel;

  g_return_if_fail (EXO_IS_TOOLBARS_VIEW (toolbar));

  exo_toolbars_view_set_editing (toolbar, TRUE);

  model = exo_toolbars_view_get_model (EXO_TOOLBARS_VIEW (toolbar));
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (toolbar));
  ui_manager = exo_toolbars_view_get_ui_manager (EXO_TOOLBARS_VIEW (toolbar));

  editor_dialog = exo_toolbars_editor_dialog_new_with_model (ui_manager, model);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (editor_dialog), TRUE);
  gtk_window_set_title (GTK_WINDOW (editor_dialog), _("Toolbar Editor"));
  gtk_window_set_transient_for (GTK_WINDOW (editor_dialog), GTK_WINDOW (toplevel));
  gtk_widget_show (editor_dialog);

  g_signal_connect (G_OBJECT (editor_dialog), "destroy", G_CALLBACK (cb_edit_toolbars_view_done), toolbar);
}

static gboolean
cb_delete_main_window (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gtk_main_quit ();

  return FALSE;
}

/* actions */
static void
xfburn_window_action_blank_cd (GtkAction * action, XfburnMainWindow * window)
{
  GtkWidget *dialog;
  gint ret;
  
  dialog = xfburn_blank_cd_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  ret = gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_hide (dialog);
  
  if (ret == GTK_RESPONSE_OK) {
    gchar *command;
    XfburnDevice *device;
    GtkWidget *dialog_progress;
    
    command = xfburn_blank_cd_dialog_get_command (XFBURN_BLANK_CD_DIALOG (dialog));
    device = xfburn_blank_cd_dialog_get_device (XFBURN_BLANK_CD_DIALOG (dialog));
    
    dialog_progress = xfburn_progress_dialog_new (XFBURN_PROGRESS_DIALOG_BLANK_CD, device, command);
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), GTK_WINDOW (window));
    gtk_widget_show (dialog_progress);
    xfburn_progress_dialog_start (XFBURN_PROGRESS_DIALOG (dialog_progress));
    
    g_free (command);
  }
  
  gtk_widget_destroy (dialog);
}

static void xfburn_window_action_copy_cd (GtkAction *action, XfburnMainWindow *window)
{
  GtkWidget *dialog;
  gint ret;
  
  dialog = xfburn_copy_cd_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  ret = gtk_dialog_run (GTK_DIALOG (dialog));
    
  gtk_widget_hide (dialog);
  
  if (ret == GTK_RESPONSE_OK) {
    gchar *command;
    XfburnDevice *device;
    GtkWidget *dialog_progress;
    
    //command = xfburn_burn_image_dialog_get_command (XFBURN_BURN_IMAGE_DIALOG (dialog));
    //device = xfburn_burn_image_dialog_get_device (XFBURN_BURN_IMAGE_DIALOG (dialog));
    
    //dialog_progress = xfburn_progress_dialog_new (XFBURN_PROGRESS_DIALOG_BURN_ISO, device, command);
    //gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), GTK_WINDOW (window));
    //gtk_widget_show (dialog_progress);
    //xfburn_progress_dialog_start (XFBURN_PROGRESS_DIALOG (dialog_progress));
    
    //g_free (command);
  }
  
  gtk_widget_destroy (dialog);
}

static void
cb_burn_composition (XfburnDiscContent *dc, XfburnMainWindow * window)
{
  GtkWidget *dialog;
  gint ret;
  gchar *tmpfile;
  
  xfburn_disc_content_generate_file_list (XFBURN_DISC_CONTENT (window->disc_content), &tmpfile);
  
  dialog = xfburn_burn_composition_dialog_new (tmpfile);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  ret = gtk_dialog_run (GTK_DIALOG (dialog));
  
  g_free (tmpfile);
  gtk_widget_hide (dialog);
  
  if (ret == GTK_RESPONSE_OK) {
/*     gchar *command;
 *     XfburnDevice *device;
 *     GtkWidget *dialog_progress;
 *     
 *     command = xfburn_burn_image_dialog_get_command (XFBURN_BURN_IMAGE_DIALOG (dialog));
 *     device = xfburn_burn_image_dialog_get_device (XFBURN_BURN_IMAGE_DIALOG (dialog));
 *     
 *     dialog_progress = xfburn_progress_dialog_new (XFBURN_PROGRESS_DIALOG_BURN_ISO, device, command);
 *     gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), GTK_WINDOW (window));
 *     gtk_widget_show (dialog_progress);
 *     xfburn_progress_dialog_start (XFBURN_PROGRESS_DIALOG (dialog_progress));
 *     
 *     g_free (command);
 */
  }
  
  gtk_widget_destroy (dialog);
}

static void
xfburn_window_action_burn_image (GtkAction * action, XfburnMainWindow * window)
{
  GtkWidget *dialog;
  gint ret;
  
  dialog = xfburn_burn_image_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  ret = gtk_dialog_run (GTK_DIALOG (dialog));
    
  gtk_widget_hide (dialog);
  
  if (ret == GTK_RESPONSE_OK) {
    gchar *command;
    XfburnDevice *device;
    GtkWidget *dialog_progress;
    
    command = xfburn_burn_image_dialog_get_command (XFBURN_BURN_IMAGE_DIALOG (dialog));
    device = xfburn_burn_image_dialog_get_device (XFBURN_BURN_IMAGE_DIALOG (dialog));
    
    dialog_progress = xfburn_progress_dialog_new (XFBURN_PROGRESS_DIALOG_BURN_ISO, device, command);
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), GTK_WINDOW (window));
    gtk_widget_show (dialog_progress);
    xfburn_progress_dialog_start (XFBURN_PROGRESS_DIALOG (dialog_progress));
    
    g_free (command);
  }
  
  gtk_widget_destroy (dialog);
}

static void
xfburn_window_action_load (GtkAction *action, XfburnMainWindow * window)
{
  xfburn_disc_content_load_from_file (XFBURN_DISC_CONTENT (window->disc_content), "/tmp/test.xml");
}

static void
xfburn_window_action_save (GtkAction *action, XfburnMainWindow * window)
{
  xfburn_disc_content_save_to_file (XFBURN_DISC_CONTENT (window->disc_content), "/tmp/test.xml");
}

static void
xfburn_window_action_quit (GtkAction * action, XfburnMainWindow * window)
{
  if (xfce_confirm (_("Are sure you want to quit?"), GTK_STOCK_QUIT, _("_Quit")))
    gtk_main_quit ();
}

static void
xfburn_window_action_about (GtkAction * action, XfburnMainWindow * window)
{
  XfceAboutInfo *info;
  GtkWidget *dialog;
  GdkPixbuf *icon;
  guint n;

  static const struct
  {
    gchar *name, *email, *language;
  } translators[] = {};
    //{
  //"Jean-François Wauthy", "pollux@xfce.org", "fr",},};

  icon = xfce_themed_icon_load ("xfburn", 48);
  //if (G_UNLIKELY (icon == NULL))
  //icon = gdk_pixbuf_new_from_file (DATADIR "/icons/hicolor/48x48/apps/Terminal.png", NULL);

  info = xfce_about_info_new ("Xfburn", VERSION "-r" REVISION, _("Another cd burning tool"),
                              XFCE_COPYRIGHT_TEXT ("2005", "Jean-François Wauthy"), XFCE_LICENSE_GPL);
  xfce_about_info_set_homepage (info, "http://www.xfce.org/");
  xfce_about_info_add_credit (info, "Jean-François Wauthy", "pollux@xfce.org", _("Author/Maintainer"));
  //xfce_about_info_add_credit (info, "Francois Le Clainche", "fleclainche@wanadoo.fr", _("Icon Designer"));

  for (n = 0; translators[n].name != NULL; ++n) {
    gchar *s;

    s = g_strdup_printf (_("Translator (%s)"), translators[n].language);
    xfce_about_info_add_credit (info, translators[n].name, translators[n].email, s);
    g_free (s);
  }

  dialog = xfce_about_dialog_new (GTK_WINDOW (window), info, icon);
  gtk_widget_set_size_request (GTK_WIDGET (dialog), 400, 300);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  xfce_about_info_free (info);
  if (G_LIKELY (icon != NULL))
    g_object_unref (G_OBJECT (icon));
}

static void
xfburn_window_action_preferences (GtkAction * action, XfburnMainWindow * window)
{
  GtkWidget *dialog;

  dialog = xfburn_preferences_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
xfburn_window_action_refresh_directorybrowser (GtkAction * action, XfburnMainWindow * window)
{
  xfburn_file_browser_refresh (XFBURN_FILE_BROWSER (window->file_browser));
}

static void
xfburn_window_action_show_filebrowser (GtkToggleAction * action, XfburnMainWindow * window)
{
  xfburn_settings_set_boolean ("show-filebrowser", gtk_toggle_action_get_active (action));
  
  if (gtk_toggle_action_get_active (action)) {
    gtk_widget_show (window->file_browser);  
  }
  else {
    gtk_widget_hide (window->file_browser);
  }
}

static void
xfburn_window_action_show_toolbar (GtkToggleAction * action, XfburnMainWindow * window)
{
  xfburn_settings_set_boolean ("show-toolbar", gtk_toggle_action_get_active (action));
  
  if (gtk_toggle_action_get_active (action)) {
    gtk_widget_show (window->toolbars);
  }
  else {
    gtk_widget_hide (window->toolbars);
  }
}

static void
xfburn_window_action_show_content_toolbar (GtkToggleAction * action, XfburnMainWindow * window)
{
  xfburn_settings_set_boolean ("show-content-toolbar", gtk_toggle_action_get_active (action));
  
  if (gtk_toggle_action_get_active (action)) {
    xfburn_disc_content_show_toolbar (XFBURN_DISC_CONTENT (window->disc_content));
  }
  else {
    xfburn_disc_content_hide_toolbar (XFBURN_DISC_CONTENT (window->disc_content));
  }
}

/* public methods */
GtkWidget *
xfburn_main_window_new (void)
{
  GtkWidget *obj;
  XfburnMainWindow *win;
  GtkAction *action;
  
  obj = g_object_new (xfburn_main_window_get_type (), NULL);
  win = XFBURN_MAIN_WINDOW (obj);
  
  /* load settings */
  action = gtk_action_group_get_action (win->action_group, "show-filebrowser");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), xfburn_settings_get_boolean ("show-filebrowser", TRUE));
  action = gtk_action_group_get_action (win->action_group, "show-toolbar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), xfburn_settings_get_boolean ("show-toolbar", TRUE));
  action = gtk_action_group_get_action (win->action_group, "show-content-toolbar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), xfburn_settings_get_boolean ("show-content-toolbar", TRUE));
  
  return obj;
}

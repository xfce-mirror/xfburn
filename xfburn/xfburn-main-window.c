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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "xfburn-main-window.h"
#include "xfburn-device-list.h"
#include "xfburn-preferences-dialog.h"
#include "xfburn-file-browser.h"
#include "xfburn-compositions-notebook.h"
#include "xfburn-blank-dialog.h"
#include "xfburn-copy-cd-dialog.h"
#include "xfburn-copy-dvd-dialog.h"
#include "xfburn-burn-image-dialog.h"
#include "xfburn-audio-composition.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-settings.h"
#include "xfburn-main.h"
#include "xfburn-utils.h"

#define XFBURN_MAIN_WINDOW_GET_PRIVATE(obj) (xfburn_main_window_get_instance_private(obj))

/* private struct */
typedef struct {
  GSimpleActionGroup *action_map;
  GtkBuilder *ui_manager;

  GtkWidget *menubar;
  GtkWidget *toolbars;

  GtkWidget *vpaned;

  GtkWidget *file_browser;
  GtkWidget *compositions_notebook;
} XfburnMainWindowPrivate;

/* prototypes */
G_DEFINE_TYPE_WITH_PRIVATE (XfburnMainWindow, xfburn_main_window, GTK_TYPE_WINDOW)

static void xfburn_main_window_finalize (GObject *obj);

static gboolean cb_delete_main_window (XfburnMainWindow *, GdkEvent *, XfburnMainWindowPrivate *);
static gboolean cb_key_press_event (GtkWidget *widget, GdkEventKey *event);

static void action_contents (GAction *, GVariant*, XfburnMainWindow *);
static void action_about (GAction *, GVariant*, XfburnMainWindow *);
static void action_preferences (GAction *, GVariant*, XfburnMainWindow *);

static void action_new_data_composition (GAction *, GVariant*, XfburnMainWindow *);
static void action_new_audio_composition (GAction *, GVariant*, XfburnMainWindow *);

/*
static void action_load (GtkAction *, XfburnMainWindow *);
static void action_save (GtkAction *, XfburnMainWindow *);
static void action_save_as (GtkAction *, XfburnMainWindow *);
*/
static void action_close (GAction *, GVariant*, XfburnMainWindow *);
static void action_quit (GAction *, GVariant*, XfburnMainWindow *);

static void action_blank (GAction *, GVariant*, XfburnMainWindow *);
static void action_copy_cd (GAction *, GVariant*, XfburnMainWindow *);
static void action_burn_image (GAction *, GVariant*, XfburnMainWindow *);

//static void action_copy_dvd (GtkAction *, XfburnMainWindow *);
static void action_burn_dvd_image (GAction *, GVariant*, XfburnMainWindow *);

static void action_refresh_directorybrowser (GAction *, GVariant*, XfburnMainWindow *);
static void action_show_filebrowser (GSimpleAction *, GVariant*, XfburnMainWindow *);
static void action_show_toolbar (GSimpleAction * action, GVariant*, XfburnMainWindow * window);

/* globals */
static GtkWindowClass *parent_class = NULL;
// upgrade to
static const GActionEntry action_entries[] = {
  // { "file-menu", NULL},
  { .name = "new-data-composition", .activate = (gActionCallback)action_new_data_composition },
  { .name = "new-audio-composition", .activate = (gActionCallback)action_new_audio_composition},
  { .name = "close-composition", .activate = (gActionCallback)action_close},
  { .name = "quit", .activate = (gActionCallback)action_quit},
  // { .name = "edit-menu", .activate = NULL},
  { .name = "preferences", .activate = (gActionCallback)action_preferences},
  // { "action-menu", NULL},
  { .name = "refresh", .activate = (gActionCallback)action_refresh_directorybrowser},
  // { "help-menu", .activate = NULL},
  { .name = "contents", .activate = (gActionCallback)action_contents },
  { .name = "about", .activate = (gActionCallback)action_about},
  { .name = "blank-disc", .activate = (gActionCallback)action_blank},
  { .name = "copy-data", .activate = (gActionCallback)action_copy_cd},
  { .name = "burn-image", .activate = (gActionCallback)action_burn_image},
  { .name = "burn-dvd", .activate = (gActionCallback)action_burn_dvd_image},
};

static const GActionEntry toggle_action_entries[] = {
  { .name = "show-filebrowser", .state = "false", .change_state = (gActionCallback)action_show_filebrowser },
  { .name = "show-toolbar", .state = "false", .change_state = (gActionCallback)action_show_toolbar },
};

static XfburnMainWindow *instance = NULL;

/**************************/
/* XfburnMainWindow class */
/**************************/
static void
xfburn_main_window_class_init (XfburnMainWindowClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_main_window_finalize;
}

static void
xfburn_main_window_finalize (GObject *obj)
{
  if (instance)
    instance = NULL;
}

static void
xfburn_main_window_init (XfburnMainWindow * mainwin)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (mainwin);

  gchar *file;

  GtkWidget *vbox;
  GMenuModel *menu_model;

  /* the window itself */
  gtk_window_set_position (GTK_WINDOW (mainwin), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_title (GTK_WINDOW (mainwin), "Xfburn");
  gtk_window_set_icon_name (GTK_WINDOW (mainwin), "stock_xfburn");
  gtk_window_resize (GTK_WINDOW (mainwin), xfburn_settings_get_int ("main-window-width", 790),
		     xfburn_settings_get_int ("main-window-height", 500));

  g_signal_connect (G_OBJECT (mainwin), "delete-event", G_CALLBACK (cb_delete_main_window), priv);

  /* create ui manager */
  priv->ui_manager = gtk_builder_new ();
  gtk_builder_set_translation_domain (priv->ui_manager, GETTEXT_PACKAGE);

  priv->action_map = g_simple_action_group_new();

  g_action_map_add_action_entries (G_ACTION_MAP (priv->action_map), action_entries, G_N_ELEMENTS(action_entries), mainwin);
  g_action_map_add_action_entries (G_ACTION_MAP (priv->action_map), toggle_action_entries, G_N_ELEMENTS(toggle_action_entries), mainwin);

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfburn/xfburn.ui");

  if (G_LIKELY (file != NULL)) {
    GError *error = NULL;
    if (gtk_builder_add_from_file (priv->ui_manager, file, &error) == 0) {
      g_warning ("Unable to load %s: %s", file, error->message);
      g_error_free (error);
    }
//    gtk_ui_manager_ensure_update (priv->ui_manager);
    g_free (file);
  }
  else {
    g_warning ("Unable to locate xfburn/xfburn.ui !");
  }

  /* accel group */
 /* accel_group = gtk_ui_manager_get_accel_group (priv->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (mainwin), accel_group);
*/
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (mainwin), vbox);
  gtk_widget_show (vbox);

  /* menubar */
  menu_model = (GMenuModel*)gtk_builder_get_object (priv->ui_manager, "main-menu");

  priv->menubar = gtk_menu_bar_new_from_model (menu_model);
  if (G_LIKELY (priv->menubar != NULL)) {
    gtk_widget_insert_action_group (priv->menubar, "app", G_ACTION_GROUP (priv->action_map));
    gtk_box_pack_start (GTK_BOX (vbox), priv->menubar, FALSE, FALSE, 0);
    gtk_widget_show (priv->menubar);
  }

  /* since Xfburn does not use GtkApplication yet, use this as a workaround to simulate working accelerators */
  g_signal_connect (G_OBJECT (mainwin), "key-press-event", G_CALLBACK (cb_key_press_event), NULL);

  /* toolbar */
  priv->toolbars = gtk_toolbar_new ();
  gtk_toolbar_set_style(GTK_TOOLBAR (priv->toolbars), GTK_TOOLBAR_BOTH);
  gtk_widget_insert_action_group (priv->toolbars, "app", G_ACTION_GROUP (priv->action_map));

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbars),
    "stock_xfburn-new-data-composition", _("New data composition"), "app.new-data-composition", NULL);

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbars),
    "stock_xfburn-audio-cd", _("New audio composition"), "app.new-audio-composition", NULL);

  gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbars), gtk_separator_tool_item_new(), -1);

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbars),
    "stock_xfburn-blank-cdrw", _("Blank CD-RW"), "app.blank-disc", NULL);

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbars),
    "stock_xfburn", _("Burn Image"), "app.burn-image", NULL);

  gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbars), gtk_separator_tool_item_new(), -1);

  xfburn_add_button_to_toolbar (GTK_TOOLBAR (priv->toolbars),
    "view-refresh", _("Refresh"), "app.refresh", _("Refresh file list"));

  gtk_box_pack_start (GTK_BOX (vbox), priv->toolbars, FALSE, FALSE, 5);
  gtk_toolbar_set_icon_size(GTK_TOOLBAR (priv->toolbars), GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_tool_shell_rebuild_menu(GTK_TOOL_SHELL (priv->toolbars));
  gtk_widget_show_all (priv->toolbars);

  /* vpaned */
  priv->vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (vbox), priv->vpaned, TRUE, TRUE, 0);
  gtk_widget_show (priv->vpaned);

  /* filebrowser */
  priv->file_browser = xfburn_file_browser_new ();
  gtk_paned_pack1 (GTK_PANED (priv->vpaned), priv->file_browser, TRUE, FALSE);
  gtk_widget_show (priv->file_browser);

  gtk_paned_set_position (GTK_PANED (priv->file_browser), xfburn_settings_get_int ("hpaned-position", 250));

  /* compositions notebook */
  priv->compositions_notebook = xfburn_compositions_notebook_new ();
  gtk_widget_show (priv->compositions_notebook);
  gtk_paned_pack2 (GTK_PANED (priv->vpaned), priv->compositions_notebook, TRUE, FALSE);

  gtk_paned_set_position (GTK_PANED (priv->vpaned), xfburn_settings_get_int ("vpaned-position", 200));

  xfce_resource_pop_path (XFCE_RESOURCE_DATA);
}

/* internals */
static gboolean
cb_delete_main_window (XfburnMainWindow * mainwin, GdkEvent * event, XfburnMainWindowPrivate *priv)
{
  gint x, y;

  gtk_window_get_size (GTK_WINDOW (mainwin), &x, &y);
  xfburn_settings_set_int ("main-window-width", x);
  xfburn_settings_set_int ("main-window-height", y);

  xfburn_settings_set_int ("vpaned-position", gtk_paned_get_position (GTK_PANED (priv->vpaned)));
  xfburn_settings_set_int ("hpaned-position", gtk_paned_get_position (GTK_PANED (priv->file_browser)));
  gtk_main_quit ();

  return FALSE;
}

static gboolean
cb_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  if (event->keyval == GDK_KEY_F1)
    xfce_dialog_show_help (GTK_WINDOW (widget), "xfburn", "start", "");

  return GDK_EVENT_PROPAGATE;
}

/* actions */
static void
action_blank (GAction * action, GVariant* param, XfburnMainWindow * window)
{
  GtkWidget *dialog;

  dialog = xfburn_blank_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void action_copy_cd (GAction *action, GVariant* param, XfburnMainWindow *window)
{
  /*
  GtkWidget *dialog;
  dialog = xfburn_copy_cd_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  */
}

static void
action_burn_image (GAction * action, GVariant* param, XfburnMainWindow * window)
{
  GtkWidget *dialog;

  dialog = xfburn_burn_image_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/*
static void
action_copy_dvd (GtkAction * action, XfburnMainWindow * window)
{
  GtkWidget *dialog;

  dialog = xfburn_copy_dvd_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
*/

static void
action_burn_dvd_image (GAction * action, GVariant* param, XfburnMainWindow * window)
{
}

/*
static void
action_load (GtkAction *action, XfburnMainWindow * window)
{
  //XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  //xfburn_data_composition_load_from_file (XFBURN_DATA_COMPOSITION (priv->data_composition), "/tmp/test.xml");
}

static void
action_save (GtkAction *action, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  xfburn_compositions_notebook_save_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->compositions_notebook));
}

static void
action_save_as (GtkAction *action, XfburnMainWindow * window)
{
  //XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

}
*/

static void
action_close (GAction *action, GVariant* param, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  xfburn_compositions_notebook_close_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->compositions_notebook));
}

static void
action_new_data_composition (GAction *action, GVariant* param, XfburnMainWindow * window)
{
  xfburn_main_window_add_data_composition_with_files (window, 0, NULL);
}

static void
action_new_audio_composition (GAction *action, GVariant* param, XfburnMainWindow * window)
{
  //XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  xfburn_main_window_add_audio_composition_with_files (window, 0, NULL);
}

static void
action_quit (GAction * action, GVariant* param, XfburnMainWindow * window)
{
  // if (xfce_confirm (_("Are sure you want to quit?"), "application-exit", _("_Quit")))
  gtk_main_quit ();
}

static void
action_contents (GAction *action, GVariant *param, XfburnMainWindow *window)
{
  xfce_dialog_show_help (GTK_WINDOW (window), "xfburn", "start", "");
}

static void
action_about (GAction * action, GVariant* param, XfburnMainWindow * window)
{
  const gchar *auth[] = { "David Mohr david@mcbf.net",
			  "Mario Đanić mario@libburnia-project.org",
			  "Jean-François Wauthy pollux@xfce.org",
			  "Rene Kjellerup rk.katana.steel@gmail.com",
			  "Hunter Turcin huntertur@gmail.com",
			  NULL };
  const gchar *translators =
    "Mohamed Magdy mohamed.m.k@gmail.com ar\n"
    "Pau Rul lan Ferragut paurullan@bulma.net ca\n"
    "Michal Várady miko.vaji@gmail.com cs\n"
    "Enrico Tröger enrico.troeger@uvena.de de\n"
    "Fabian Nowak timstery@arcor.de de\n"
    "Nico Schümann nico@prog.nico22.de de\n"
    "Stavros Giannouris stavrosg2002@freemail.gr el\n"
    "Jeff Bailes thepizzaking@gmail.com en_GB\n"
    "Diego Rodriguez dieymir@yahoo.es es\n"
    "Kristjan Siimson kristjan.siimson@gmail.com et\n"
    "Piarres Beobide pi@beobide.net eu\n"
    "Jari Rahkonen jari.rahkonen@pp1.inet.fi fi\n"
    "Etienne Collet xanaxlnx@gmail.com fr\n"
    "Maximilian Schleiss maximilian@xfce.org fr\n"
    "Attila Szervác sas@321.hu hu\n"
    "Daichi Kawahata daichi@xfce.org ja\n"
    "ByungHyun Choi byunghyun.choi@debianusers.org kr\n"
    "Mantas mantaz@users.sourceforge.net lt\n"
    "Rihards Prieditis RPrieditis@inbox.lv lv\n"
    "Terje Uriansrud ter@operamail.com nb_NO\n"
    "Stephan Arts psybsd@gmail.com nl\n"
    "Szymon Kałasz szymon_maestro@gazeta.pl pl\n"
    "Fábio Nogueira deb-user-ba@ubuntu.com pt_BR\n"
    "Og Maciel omaciel@xfce.org pt_BR\n"
    "Nuno Miguel nunis@netcabo.pt pt_PT\n"
    "Sergey Fedoseev fedoseev.sergey@gmail.com ru\n"
    "Besnik Bleta besnik@programeshqip.org sq\n"
    "Maxim V. Dziumanenko mvd@mylinux.com.ua uk\n"
    "Dmitry Nikitin  uk\n"
    "ﻢﺤﻣﺩ ﻊﻠﻳ ﺎﻠﻤﻜﻳ makki.ma@gmail.com ur\n"
    "正龙 赵 longer.zhao@gmail.com zh_CN\n"
    "Cosmo Chene cosmolax@gmail.com zh_TW\n";

  gtk_show_about_dialog(GTK_WINDOW (window),
		  "logo-icon-name", "media-optical",
		  "program-name", "Xfburn",
		  "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
		  "version", VERSION_FULL,
		  "comments", _("Another cd burning GUI"),
		  "website", "https://docs.xfce.org/apps/xfburn/start",
		  "copyright", "2005-" COPYRIGHT_YEAR " Xfce development team",
		  "authors", auth,
		  "translator-credits", translators,
		  NULL);
}

static void
action_preferences (GAction * action, GVariant* param, XfburnMainWindow * window)
{
  GtkWidget *dialog;

  dialog = xfburn_preferences_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
action_refresh_directorybrowser (GAction * action, GVariant* param, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  xfburn_file_browser_refresh (XFBURN_FILE_BROWSER (priv->file_browser));
}

static void
action_show_filebrowser (GSimpleAction * action, GVariant* value, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  GAction *file;
  gboolean toggle;

  if(g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN))
    g_simple_action_set_state (action, value);

  file = g_action_map_lookup_action(G_ACTION_MAP(priv->action_map), "show-filebrowser");
  toggle = g_variant_get_boolean  (g_action_get_state (file));
  xfburn_settings_set_boolean ("show-filebrowser", toggle);

  if ( toggle ) {
    gtk_widget_show (priv->file_browser);
  }
  else {
    gtk_widget_hide (priv->file_browser);
  }
}

static void
action_show_toolbar (GSimpleAction * action, GVariant* value, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  GAction *tool;
  gboolean toggle;

  if(g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN))
    g_simple_action_set_state (action, value);

  tool = g_action_map_lookup_action(G_ACTION_MAP(priv->action_map), "show-toolbar");
  toggle = g_variant_get_boolean (g_action_get_state (tool));

  xfburn_settings_set_boolean ("show-toolbar", toggle);

  if ( toggle ) {
    gtk_widget_show (priv->toolbars);
  }
  else {
    gtk_widget_hide (priv->toolbars);
  }
}


/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_main_window_new (void)
{
  GtkWidget *obj;

  if (G_UNLIKELY (instance)) {
    g_error ("An instance of XfburnMainWindow has already been created");
    return NULL;
  }

  obj = gtk_widget_new (xfburn_main_window_get_type (), NULL);

  if (obj) {
    XfburnMainWindow *win;
    XfburnMainWindowPrivate *priv;
    GAction *action;
    GActionMap *action_map;

    instance = win = XFBURN_MAIN_WINDOW (obj);
    priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (win);
    action_map = G_ACTION_MAP (priv->action_map);
    /* load settings */
    action = g_action_map_lookup_action (action_map, "show-filebrowser");
    g_action_change_state (action, g_variant_new_boolean (xfburn_settings_get_boolean ("show-filebrowser", FALSE)));
    action = g_action_map_lookup_action (action_map, "show-toolbar");
    g_action_change_state (action, g_variant_new_boolean (xfburn_settings_get_boolean ("show-toolbar", FALSE)));
   /* action = gtk_action_group_get_action (priv->action_group, "save-composition");
    gtk_action_set_sensitive (GTK_ACTION (action), FALSE);*/

    /* show welcome tab */
    xfburn_compositions_notebook_add_welcome_tab (XFBURN_COMPOSITIONS_NOTEBOOK (priv->compositions_notebook), action_map);
  }

  return obj;
}

XfburnMainWindow *
xfburn_main_window_get_instance (void)
{
  if (!instance)
    g_warning ("No existing instance of XfburnMainWindow");

  return instance;
}
/*
GtkUIManager *
xfburn_main_window_get_ui_manager (XfburnMainWindow *window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  return priv->ui_manager;
}
*/
XfburnFileBrowser *
xfburn_main_window_get_file_browser (XfburnMainWindow *window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  return XFBURN_FILE_BROWSER (priv->file_browser);
}

void
xfburn_main_window_add_data_composition_with_files (XfburnMainWindow *window, int filec, char **filenames)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  XfburnComposition *comp;
  GSList * filelist;

  comp = xfburn_compositions_notebook_add_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->compositions_notebook), XFBURN_DATA_COMPOSITION);

  filelist = xfburn_make_abosulet_file_list (filec, filenames);
  xfburn_data_composition_add_files (XFBURN_DATA_COMPOSITION (comp), filelist);
}

void
xfburn_main_window_add_audio_composition_with_files (XfburnMainWindow *window, int filec, char **filenames)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  XfburnComposition *comp;
  GSList * filelist;

  comp = xfburn_compositions_notebook_add_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->compositions_notebook), XFBURN_AUDIO_COMPOSITION);
  filelist = xfburn_make_abosulet_file_list (filec, filenames);
  xfburn_audio_composition_add_files (XFBURN_AUDIO_COMPOSITION (comp), filelist);
}

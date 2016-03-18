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

#include <exo/exo.h>

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
#include "xfburn-stock.h"
#include "xfburn-utils.h"

#define XFBURN_MAIN_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_MAIN_WINDOW, XfburnMainWindowPrivate))

/* private struct */
typedef struct {  
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  GtkWidget *menubar;
  GtkWidget *toolbars;
  
  GtkWidget *vpaned;
  
  GtkWidget *file_browser;
  GtkWidget *compositions_notebook;

  gboolean support_cdr;
  gboolean support_cdrw;
} XfburnMainWindowPrivate;

/* prototypes */
static void xfburn_main_window_class_init (XfburnMainWindowClass *);
static void xfburn_main_window_finalize (GObject *obj);
static void xfburn_main_window_init (XfburnMainWindow *);

static gboolean cb_delete_main_window (XfburnMainWindow *, GdkEvent *, XfburnMainWindowPrivate *);
/*static void cb_edit_toolbars_view (ExoToolbarsView *, gpointer);*/

static void action_about (GtkAction *, XfburnMainWindow *);
static void action_preferences (GtkAction *, XfburnMainWindow *);

static void action_new_data_composition (GtkAction *, XfburnMainWindow *);
static void action_new_audio_composition (GtkAction *, XfburnMainWindow *);

/*
static void action_load (GtkAction *, XfburnMainWindow *);
static void action_save (GtkAction *, XfburnMainWindow *);
static void action_save_as (GtkAction *, XfburnMainWindow *);
*/
static void action_close (GtkAction *, XfburnMainWindow *);
static void action_quit (GtkAction *, XfburnMainWindow *);

static void action_blank (GtkAction *, XfburnMainWindow *);
static void action_copy_cd (GtkAction *, XfburnMainWindow *);
static void action_burn_image (GtkAction *, XfburnMainWindow *);

//static void action_copy_dvd (GtkAction *, XfburnMainWindow *);
static void action_burn_dvd_image (GtkAction *, XfburnMainWindow *);

static void action_refresh_directorybrowser (GtkAction *, XfburnMainWindow *);
static void action_show_filebrowser (GtkToggleAction *, XfburnMainWindow *);
static void action_show_toolbar (GtkToggleAction * action, XfburnMainWindow * window);

/* globals */
static GtkWindowClass *parent_class = NULL;
static const GtkActionEntry action_entries[] = {
  {"file-menu", NULL, N_("_File"), NULL, NULL, NULL},
  /*{"new-composition", GTK_STOCK_NEW, N_("_New composition"), "", N_("Create a new composition"),},*/
  /*{"new-composition", GTK_STOCK_NEW, N_("_New composition"), NULL, N_("Create a new composition"), 
    G_CALLBACK (action_new_data_composition),}, */
  {"new-data-composition", XFBURN_STOCK_NEW_DATA_COMPOSITION, N_("New data composition"), "<Control><Alt>e", N_("New data composition"),
    G_CALLBACK (action_new_data_composition),},
  {"new-audio-composition", XFBURN_STOCK_AUDIO_CD, N_("New audio composition"), "<Control><Alt>A", N_("New audio composition"),
    G_CALLBACK (action_new_audio_composition),},
  /*{"load-composition", GTK_STOCK_OPEN, N_("Load composition"), NULL, N_("Load composition"),
   G_CALLBACK (action_load),},
  {"save-composition", GTK_STOCK_SAVE, N_("Save composition"), NULL, N_("Save composition"), 
   G_CALLBACK (action_save),},
  {"save-composition-as", GTK_STOCK_SAVE_AS, N_("Save composition as..."), NULL, N_("Save composition as"), 
   G_CALLBACK (action_save_as),},*/
  {"close-composition", GTK_STOCK_CLOSE, N_("Close composition"), NULL, N_("Close composition"), 
   G_CALLBACK (action_close),},
  {"quit", GTK_STOCK_QUIT, N_("_Quit"), NULL, N_("Quit Xfburn"), G_CALLBACK (action_quit),},
  {"edit-menu", NULL, N_("_Edit"), NULL, NULL, NULL},
  {"preferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL, N_("Show preferences dialog"),
   G_CALLBACK (action_preferences),},
  {"action-menu", NULL, N_("_Actions"), NULL, NULL, NULL},
  {"view-menu", NULL, N_("_View"), NULL, NULL, NULL},
  {"refresh", GTK_STOCK_REFRESH, N_("Refresh"), NULL, N_("Refresh file list"),
   G_CALLBACK (action_refresh_directorybrowser),},
  {"help-menu", NULL, N_("_Help"), NULL, NULL, NULL},
  {"about", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("Display information about Xfburn"),
   G_CALLBACK (action_about),},
  {"blank-disc", "xfburn-blank-cdrw", N_("Blank CD-RW"), NULL, N_("Blank CD-RW"),
   G_CALLBACK (action_blank),},
  {"copy-data", "xfburn-data-copy", N_("Copy Data CD"), NULL, N_("Copy Data CD"),
   G_CALLBACK (action_copy_cd),},
  /*{"copy-audio", "xfburn-audio-copy", N_("Copy Audio CD"), NULL, N_("Copy Audio CD"),}, */
  {"burn-image", "stock_xfburn", N_("Burn Image"), NULL, N_("Burn Image"),
   G_CALLBACK (action_burn_image),},
/*  {"copy-dvd", "xfburn-data-copy", N_("Copy DVD"), NULL, N_("Copy DVD"),
   G_CALLBACK (action_copy_dvd),}, */
  {"burn-dvd", "xfburn-burn-image", N_("Burn DVD Image"), NULL, N_("Burn DVD Image"),
   G_CALLBACK (action_burn_dvd_image),},
};

static const GtkToggleActionEntry toggle_action_entries[] = {
  {"show-filebrowser", NULL, N_("Show file browser"), NULL, N_("Show/hide the file browser"),
   G_CALLBACK (action_show_filebrowser), TRUE,},
  {"show-toolbar", NULL, N_("Show toolbar"), NULL, N_("Show/hide the toolbar"),
   G_CALLBACK (action_show_toolbar), TRUE,},
};

static const gchar *toolbar_actions[] = {
  "new-data-composition",
  "new-audio-composition",
/*  "load-composition",
  "save-composition",
  "close-composition",*/
  "blank-disc",
  "copy-data",
  //"copy-audio",
  "burn-image",
  "copy-dvd",
  "burn-dvd",
  "refresh",
  "about",
  "preferences",
};

static XfburnMainWindow *instance = NULL;

/**************************/
/* XfburnMainWindow class */
/**************************/
GType
xfburn_main_window_get_type (void)
{
  static GType main_window_type = 0;

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
      (GInstanceInitFunc) xfburn_main_window_init,
      NULL
    };

    main_window_type = g_type_register_static (GTK_TYPE_WINDOW, "XfburnMainWindow", &main_window_info, 0);
  }

  return main_window_type;
}

static void
xfburn_main_window_class_init (XfburnMainWindowClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (XfburnMainWindowPrivate));

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

  GtkAccelGroup *accel_group;
  gchar *file;

  GtkWidget *vbox;

  /* the window itself */
  gtk_window_set_position (GTK_WINDOW (mainwin), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_title (GTK_WINDOW (mainwin), "Xfburn");
  gtk_window_set_icon_name (GTK_WINDOW (mainwin), XFBURN_STOCK);
  gtk_window_resize (GTK_WINDOW (mainwin), xfburn_settings_get_int ("main-window-width", 850),
		     xfburn_settings_get_int ("main-window-height", 700));

  g_signal_connect (G_OBJECT (mainwin), "delete-event", G_CALLBACK (cb_delete_main_window), priv);

  /* create ui manager */
  priv->action_group = gtk_action_group_new ("xfburn-main-window");
  gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (priv->action_group, action_entries, G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (mainwin));
  gtk_action_group_add_toggle_actions (priv->action_group, toggle_action_entries,
                                       G_N_ELEMENTS (toggle_action_entries), GTK_WIDGET (mainwin));

  priv->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (priv->ui_manager, priv->action_group, 0);

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfburn/xfburn.ui");

  if (G_LIKELY (file != NULL)) {
    GError *error = NULL;
    if (gtk_ui_manager_add_ui_from_file (priv->ui_manager, file, &error) == 0) {
      g_warning ("Unable to load %s: %s", file, error->message);
      g_error_free (error);
    }
    gtk_ui_manager_ensure_update (priv->ui_manager);
    g_free (file);
  }
  else {
    g_warning ("Unable to locate xfburn/xfburn.ui !");
  }

  /* accel group */
  accel_group = gtk_ui_manager_get_accel_group (priv->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (mainwin), accel_group);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (mainwin), vbox);
  gtk_widget_show (vbox);

  /* menubar */
  priv->menubar = gtk_ui_manager_get_widget (priv->ui_manager, "/main-menu");
  if (G_LIKELY (priv->menubar != NULL)) {
    gtk_box_pack_start (GTK_BOX (vbox), priv->menubar, FALSE, FALSE, 0);
    gtk_widget_show (priv->menubar);
  }

  /* toolbar */
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfburn/xfburn-toolbars.ui");

  if (G_LIKELY (file != NULL)) {
    ExoToolbarsModel *model;
    GError *error = NULL;

    model = exo_toolbars_model_new ();
    exo_toolbars_model_set_actions (model, (gchar **) toolbar_actions, G_N_ELEMENTS (toolbar_actions));
    if (exo_toolbars_model_load_from_file (model, file, &error)) {
      priv->toolbars = exo_toolbars_view_new_with_model (priv->ui_manager, model);
      gtk_box_pack_start (GTK_BOX (vbox), priv->toolbars, FALSE, FALSE, 0);
      gtk_widget_show (priv->toolbars);

      /*g_signal_connect (G_OBJECT (priv->toolbars), "customize", G_CALLBACK (cb_edit_toolbars_view), mainwin);*/
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
  priv->vpaned = gtk_vpaned_new ();
  gtk_container_add (GTK_CONTAINER (vbox), priv->vpaned);
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
/*static void
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
}*/

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

/* actions */
static void
action_blank (GtkAction * action, XfburnMainWindow * window)
{
  GtkWidget *dialog;
  
  dialog = xfburn_blank_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void action_copy_cd (GtkAction *action, XfburnMainWindow *window)
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
action_burn_image (GtkAction * action, XfburnMainWindow * window)
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
action_burn_dvd_image (GtkAction * action, XfburnMainWindow * window)
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
action_close (GtkAction *action, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  
  xfburn_compositions_notebook_close_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->compositions_notebook));
}

static void
action_new_data_composition (GtkAction *action, XfburnMainWindow * window)
{
  xfburn_main_window_add_data_composition_with_files (window, 0, NULL);
}

static void
action_new_audio_composition (GtkAction *action, XfburnMainWindow * window)
{
  //XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
 
  xfburn_main_window_add_audio_composition_with_files (window, 0, NULL);
}

static void
action_quit (GtkAction * action, XfburnMainWindow * window)
{
  // if (xfce_confirm (_("Are sure you want to quit?"), GTK_STOCK_QUIT, _("_Quit")))
  gtk_main_quit ();
}

static void
action_about (GtkAction * action, XfburnMainWindow * window)
{
  gint x, y;
  GdkPixbuf *icon = NULL;
  const gchar *auth[] = { "David Mohr david@mcbf.net Author/Maintainer",
	  		  "Mario Đanić mario@libburnia-project.org Author/Maintainer",
			  "Jean-François Wauthy pollux@xfce.org Retired author/maintainer",
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

  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &x, &y);
  icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "media-optical", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
  if (!icon)
    icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "media-cdrom", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
  if (!icon)
    icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), GTK_STOCK_CDROM, x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

#if !GTK_CHECK_VERSION (2, 18, 0)
  gtk_about_dialog_set_email_hook (exo_gtk_url_about_dialog_hook, NULL, NULL);
  gtk_about_dialog_set_url_hook (exo_gtk_url_about_dialog_hook, NULL, NULL);
#endif
  gtk_show_about_dialog(GTK_WINDOW (window),
		  "logo", icon,
		  "program-name", "Xfburn",
		  "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
		  "version", VERSION,
		  "comments", _("Another cd burning GUI"),
		  "website", "http://www.xfce.org/projects/xfburn",
		  "copyright", "2005-2008 David Mohr, Mario Đanić, Jean-François Wauthy",
		  "authors", auth, 
		  "translator-credits", translators,
		  NULL);

  if (G_LIKELY (icon != NULL))
    g_object_unref (G_OBJECT (icon));
}

static void
action_preferences (GtkAction * action, XfburnMainWindow * window)
{
  GtkWidget *dialog;

  dialog = xfburn_preferences_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
action_refresh_directorybrowser (GtkAction * action, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  
  xfburn_file_browser_refresh (XFBURN_FILE_BROWSER (priv->file_browser));
}

static void
action_show_filebrowser (GtkToggleAction * action, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  
  xfburn_settings_set_boolean ("show-filebrowser", gtk_toggle_action_get_active (action));
  
  if (gtk_toggle_action_get_active (action)) {
    gtk_widget_show (priv->file_browser);  
  }
  else {
    gtk_widget_hide (priv->file_browser);
  }
}

static void
action_show_toolbar (GtkToggleAction * action, XfburnMainWindow * window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  
  xfburn_settings_set_boolean ("show-toolbar", gtk_toggle_action_get_active (action));
  
  if (gtk_toggle_action_get_active (action)) {
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
  
  obj = g_object_new (xfburn_main_window_get_type (), NULL);
  
  if (obj) {
    XfburnMainWindow *win;
    XfburnMainWindowPrivate *priv;
    GtkAction *action;
    GList *device = NULL;
    XfburnDeviceList *devlist;
    
    instance = win = XFBURN_MAIN_WINDOW (obj);
    priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (win);

    /* load settings */
    action = gtk_action_group_get_action (priv->action_group, "show-filebrowser");
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), xfburn_settings_get_boolean ("show-filebrowser", FALSE));
    action = gtk_action_group_get_action (priv->action_group, "show-toolbar");
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), xfburn_settings_get_boolean ("show-toolbar", FALSE));
   /* action = gtk_action_group_get_action (priv->action_group, "save-composition");
    gtk_action_set_sensitive (GTK_ACTION (action), FALSE);*/

    /* disable action that cannot be used due to device */

    devlist = xfburn_device_list_new ();
    g_object_get (G_OBJECT (devlist), "devices", &device, NULL);
    g_object_unref (devlist);

    /* FIXME: this is really outdated behavior. Needs to get rewritten */
    while (device != NULL) {
      XfburnDevice *device_info = (XfburnDevice *) device->data;
      gboolean cdr, cdrw;

      g_object_get (G_OBJECT (device_info), "cdr", &cdr, "cdrw", &cdrw, NULL);

      if (cdr)
	priv->support_cdr = TRUE;
      if (cdrw)
	priv->support_cdrw = TRUE;

      device = g_list_next (device);
    }

    if (!priv->support_cdr) {
      action = gtk_action_group_get_action (priv->action_group, "copy-data");
      gtk_action_set_sensitive (action, FALSE);
      action = gtk_action_group_get_action (priv->action_group, "copy-audio");
      gtk_action_set_sensitive (action, FALSE);
      action = gtk_action_group_get_action (priv->action_group, "burn-image");
      gtk_action_set_sensitive (action, FALSE);
    }
    if (!priv->support_cdrw) {
      action = gtk_action_group_get_action (priv->action_group, "blank-disc");
      gtk_action_set_sensitive (action, FALSE);
    }

    /* show welcome tab */
    xfburn_compositions_notebook_add_welcome_tab (XFBURN_COMPOSITIONS_NOTEBOOK (priv->compositions_notebook), priv->action_group);
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

GtkUIManager *
xfburn_main_window_get_ui_manager (XfburnMainWindow *window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);
  
  return priv->ui_manager;
}

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

gboolean
xfburn_main_window_support_cdr (XfburnMainWindow *window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  return priv->support_cdr;
}

gboolean
xfburn_main_window_support_cdrw (XfburnMainWindow *window)
{
  XfburnMainWindowPrivate *priv = XFBURN_MAIN_WINDOW_GET_PRIVATE (window);

  return priv->support_cdrw;
}

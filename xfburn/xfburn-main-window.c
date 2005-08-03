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
#include "xfburn-file-browser.h"
#include "xfburn-disc-content.h"

/* prototypes */
static void xfburn_main_window_class_init (XfburnMainWindowClass *);
static void xfburn_main_window_init (XfburnMainWindow *);

static gboolean cb_delete_main_window (GtkWidget *, GdkEvent *, gpointer);
static void cb_edit_toolbars_view (ExoToolbarsView *, gpointer);

static void xfburn_window_action_about (GtkAction *, XfburnMainWindow *);
static void xfburn_window_action_quit (GtkAction *, XfburnMainWindow *);


/* globals */
static GtkWindowClass *parent_class = NULL;
static GtkActionEntry action_entries[] = {
  {"file-menu", NULL, N_("_File"), NULL,},
  {"quit", GTK_STOCK_QUIT, N_("Quit"), NULL, N_("Quit Xfburn"), G_CALLBACK (xfburn_window_action_quit),},
  {"help-menu", NULL, N_("_Help"), NULL,},
  {"about", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("Display information about Xfburn"),
   G_CALLBACK (xfburn_window_action_about),},
  {"blank-cd", "xfburn-blank-cdrw", N_("Blank CD-RW"), NULL, N_("Blank CD-RW"),},
  {"format-dvd", "xfburn-blank-dvdrw", N_("Format DVD-RW"), NULL, N_("Format DVD-RW"),},
  {"copy-data", "xfburn-data-copy", N_("Copy Data CD"), NULL, N_("Copy Data CD"),},
  {"copy-audio", "xfburn-audio-copy", N_("Copy Audio CD"), NULL, N_("Copy Audio CD"),},
  {"burn-cd", "xfburn-burn-cd", N_("Burn CD Image"), NULL, N_("Burn CD Image"),},
  {"refresh", GTK_STOCK_REFRESH, N_("Refresh"), NULL, N_("Refresh file list"),}
};
static const gchar *toolbar_actions[] = {
  "blank-cd",
  "format-dvd",
  "copy-data",
  "copy-audio",
  "burn-cd",
  "refresh",
  "about",
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

  xfce_resource_pop_path (XFCE_RESOURCE_DATA);
}

/* internals */
static void
cb_edit_toolbars_view_done (ExoToolbarsEditorDialog *dialog, ExoToolbarsView *toolbar)
{
  exo_toolbars_view_set_editing (toolbar, FALSE);
  
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
cb_edit_toolbars_view (ExoToolbarsView *toolbar, gpointer user_data)
{
  GtkWidget *editor_dialog;
  ExoToolbarsModel  *model;
  GtkUIManager      *ui_manager;
  GtkWidget         *toplevel;
  
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
xfburn_window_action_quit (GtkAction * action, XfburnMainWindow * window)
{
  if (xfce_confirm (_("Are sure you want to quit?"), GTK_STOCK_QUIT, _("Quit")))
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
  } translators[] = {
    {"Jean-François Wauthy", "pollux@xfce.org", "fr",},
    };

  icon = xfce_themed_icon_load ("xfburn", 48);
  //if (G_UNLIKELY (icon == NULL))
  //icon = gdk_pixbuf_new_from_file (DATADIR "/icons/hicolor/48x48/apps/Terminal.png", NULL);

  info = xfce_about_info_new ("Xfburn", VERSION, _("Another cd burning tool"),
                              XFCE_COPYRIGHT_TEXT ("2005", "Jean-François Wauthy"), XFCE_LICENSE_GPL);
  xfce_about_info_set_homepage (info, "http://www.xfce.org/");
  xfce_about_info_add_credit (info, "Jean-François Wauthy", "pollux@xfce.org", _("Maintainer"));
  //xfce_about_info_add_credit (info, "Francois Le Clainche", "fleclainche@wanadoo.fr", _("Icon Designer"));

  for (n = 0; translators[n].name != NULL; ++n) {
    gchar *s;

    s = g_strdup_printf (_("Translator (%s)"), translators[n].language);
    xfce_about_info_add_credit (info, translators[n].name, translators[n].email, s);
    g_free (s);
  }

  dialog = xfce_about_dialog_new (GTK_WINDOW (window), info, icon);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  xfce_about_info_free (info);
  if (G_LIKELY (icon != NULL))
    g_object_unref (G_OBJECT (icon));
}

/* public methods */
GtkWidget *
xfburn_main_window_new (void)
{
  return g_object_new (xfburn_main_window_get_type (), NULL);
}

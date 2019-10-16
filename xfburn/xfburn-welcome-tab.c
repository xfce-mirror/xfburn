/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *  Copyright (c) 2008      David Mohr (dmohr@mcbf.net)
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

#include "xfburn-global.h"
#include "xfburn-composition.h"

#include "xfburn-welcome-tab.h"

#include "xfburn-main-window.h"
#include "xfburn-compositions-notebook.h"
#include "xfburn-burn-image-dialog.h"
#include "xfburn-blank-dialog.h"

/* prototypes */
static void xfburn_welcome_tab_finalize (GObject * object);

typedef struct {
  XfburnCompositionsNotebook *notebook;

  GtkWidget *button_image;
  GtkWidget *button_data_comp;
  GtkWidget *button_audio_comp;
  GtkWidget *button_blank;
} XfburnWelcomeTabPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (XfburnWelcomeTab, xfburn_welcome_tab, GTK_TYPE_BOX)
#define XFBURN_WELCOME_TAB_GET_PRIVATE(obj) (xfburn_welcome_tab_get_instance_private(obj))

/* internals */
static GtkWidget* create_welcome_button (const gchar *stock, const gchar *text, const gchar *secondary);

static void burn_image (GtkButton *button, XfburnWelcomeTab *tab);
static void new_data_composition (GtkButton *button, XfburnWelcomeTab *tab);
static void new_audio_cd (GtkButton *button, XfburnWelcomeTab *tab);
static void blank_disc (GtkButton *button, XfburnWelcomeTab *tab);


/*********************/
/* class declaration */
/*********************/
static GtkWidget *parent_class = NULL;

static void
xfburn_welcome_tab_class_init (XfburnWelcomeTabClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_welcome_tab_finalize;
}

static void
xfburn_welcome_tab_init (XfburnWelcomeTab * obj)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (obj);

  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *label_welcome;
  GtkWidget *table;

  gtk_box_set_homogeneous (GTK_BOX (obj), TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
  gtk_box_pack_start (GTK_BOX (obj), vbox, FALSE, FALSE, BORDER);
  gtk_widget_show (vbox);

  align = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_widget_show (align);

  label_welcome = gtk_label_new (_("Welcome to Xfburn!"));
  gtk_widget_set_valign (GTK_WIDGET (label_welcome), GTK_ALIGN_END);
  gtk_box_pack_start (GTK_BOX (vbox), label_welcome, FALSE, FALSE, BORDER);
  gtk_widget_show (label_welcome);

  table = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, BORDER);
  gtk_grid_set_row_spacing (GTK_GRID (table), BORDER);
  gtk_grid_set_column_spacing (GTK_GRID (table), BORDER);
  gtk_widget_show (table);

  align = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_widget_show (align);
  align = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_widget_show (align);

  /* buttons */
  priv->button_image = create_welcome_button ("stock_xfburn", _("<big>Burn _Image</big>"), _("Burn a prepared compilation, i.e. an .ISO file"));
  gtk_grid_attach (GTK_GRID (table), priv->button_image, 0, 0, 1, 1);
  gtk_widget_show (priv->button_image);
  g_signal_connect (G_OBJECT(priv->button_image), "clicked", G_CALLBACK(burn_image), obj);

  priv->button_data_comp = create_welcome_button ("stock_xfburn-new-data-composition", _("<big>New _Data Composition</big>"), _("Create a new data disc with the files of your choosing"));
  gtk_grid_attach (GTK_GRID (table), priv->button_data_comp, 1, 0, 1, 1);
  gtk_widget_show (priv->button_data_comp);
  g_signal_connect (G_OBJECT(priv->button_data_comp), "clicked", G_CALLBACK(new_data_composition), obj);

  priv->button_blank = create_welcome_button ("stock_xfburn-blank-cdrw", _("<big>_Blank Disc</big>"), _("Prepare the rewriteable disc for a new burn"));
  gtk_grid_attach (GTK_GRID (table), priv->button_blank, 0, 1, 1, 1);
  gtk_widget_show (priv->button_blank);
  g_signal_connect (G_OBJECT(priv->button_blank), "clicked", G_CALLBACK(blank_disc), obj);

  priv->button_audio_comp = create_welcome_button ("stock_xfburn-audio-cd", _("<big>_Audio CD</big>"), _("Audio CD playable in regular stereos"));
  gtk_grid_attach (GTK_GRID (table), priv->button_audio_comp, 1, 1, 1, 1);
  gtk_widget_show (priv->button_audio_comp);
  g_signal_connect (G_OBJECT(priv->button_audio_comp), "clicked", G_CALLBACK(new_audio_cd), obj);
}

static void
xfburn_welcome_tab_finalize (GObject * object)
{
  //XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*           */
/* internals */
/*           */

/* create_welcome_button was based on xfce_create_mixed_button */
static GtkWidget*
create_welcome_button (const gchar *stock, const gchar *text, const gchar *secondary)
{
  GtkWidget *button, *align, *image, *hbox, *label, *vbox;

  button = gtk_button_new ();
  gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
  label = gtk_label_new (NULL);
  gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), text);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  
  align = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);

  image = gtk_image_new_from_icon_name (stock, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  // adding the lables
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  align = gtk_label_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), align, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  label = gtk_label_new (NULL);
  gtk_label_set_text (GTK_LABEL (label), secondary);
  gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (button), hbox);
  gtk_widget_show_all (button);

  return button;
}

static void
burn_image (GtkButton *button, XfburnWelcomeTab *tab)
{
  GtkWidget *dialog;

  dialog = xfburn_burn_image_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (xfburn_main_window_get_instance ()));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
blank_disc (GtkButton *button, XfburnWelcomeTab *tab)
{
  GtkWidget *dialog;

  dialog = xfburn_blank_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (xfburn_main_window_get_instance ()));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
new_data_composition (GtkButton *button, XfburnWelcomeTab *tab)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (tab);

  xfburn_compositions_notebook_add_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->notebook), XFBURN_DATA_COMPOSITION);
}

static void
new_audio_cd (GtkButton *button, XfburnWelcomeTab *tab)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (tab);

  xfburn_compositions_notebook_add_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->notebook), XFBURN_AUDIO_COMPOSITION);
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_welcome_tab_new (XfburnCompositionsNotebook *notebook, GActionMap *action_group)
{
  GtkWidget *obj;

  obj = g_object_new (XFBURN_TYPE_WELCOME_TAB, NULL);
  if (obj) {
    XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (XFBURN_WELCOME_TAB(obj));
    GAction *action;

    priv->notebook = notebook;

    /* FIXME retrieve action group from UI Manager */
    action = g_action_map_lookup_action (action_group, "burn-image");
    gtk_widget_set_sensitive (priv->button_image, g_action_get_enabled (action));

    action = g_action_map_lookup_action (action_group, "new-data-composition");
    gtk_widget_set_sensitive (priv->button_data_comp, g_action_get_enabled (action));

    action = g_action_map_lookup_action (action_group, "new-audio-composition");
    gtk_widget_set_sensitive (priv->button_audio_comp, g_action_get_enabled (action));

    action = g_action_map_lookup_action (action_group, "blank-disc");
    gtk_widget_set_sensitive (priv->button_blank, g_action_get_enabled (action));
  }

  return obj;
}

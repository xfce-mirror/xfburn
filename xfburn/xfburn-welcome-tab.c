/* $Id: xfburn-welcome-tab.c 4382 2008-04-24 17:08:37Z dmohr $ */
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
#include "xfburn-blank-cd-dialog.h"

/* prototypes */
static void xfburn_welcome_tab_class_init (XfburnWelcomeTabClass * klass);
static void xfburn_welcome_tab_init (XfburnWelcomeTab * sp);
static void xfburn_welcome_tab_finalize (GObject * object);
static void composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data);
static void xfburn_welcome_tab_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfburn_welcome_tab_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void show_custom_controls (XfburnComposition *composition);
static void hide_custom_controls (XfburnComposition *composition);
static void burn_image (GtkButton *button, XfburnWelcomeTab *tab);
static void new_data_composition (GtkButton *button, XfburnWelcomeTab *tab);
static void blank_disc (GtkButton *button, XfburnWelcomeTab *tab);

#define XFBURN_WELCOME_TAB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_WELCOME_TAB, XfburnWelcomeTabPrivate))

enum {
  LAST_SIGNAL,
}; 

enum {
  PROP_0,
  PROP_MAIN_WINDOW,
  PROP_NOTEBOOK,
};

typedef struct {
  XfburnMainWindow *mainwin;
  XfburnCompositionsNotebook *notebook;
} XfburnWelcomeTabPrivate;

/*********************/
/* class declaration */
/*********************/
static GtkWidget *parent_class = NULL;
//static guint signals[LAST_SIGNAL];

GtkType
xfburn_welcome_tab_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnWelcomeTabClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_welcome_tab_class_init,
      NULL,
      NULL,
      sizeof (XfburnWelcomeTab),
      0,
      (GInstanceInitFunc) xfburn_welcome_tab_init,
    };

    static const GInterfaceInfo composition_info = {
      (GInterfaceInitFunc) composition_interface_init,    /* interface_init */
      NULL,                                               /* interface_finalize */
      NULL                                                /* interface_data */
    };
    
    type = g_type_register_static (GTK_TYPE_VBOX, "XfburnWelcomeTab", &our_info, 0);

    g_type_add_interface_static (type, XFBURN_TYPE_COMPOSITION, &composition_info);
  }

  return type;
}

static void
xfburn_welcome_tab_class_init (XfburnWelcomeTabClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnWelcomeTabPrivate));
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_welcome_tab_finalize;
  object_class->set_property = xfburn_welcome_tab_set_property;
  object_class->get_property = xfburn_welcome_tab_get_property;

  /*
  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_WELCOME_TAB, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnWelcomeTabClass, volume_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
  */

  g_object_class_install_property (object_class, PROP_MAIN_WINDOW, 
                                   g_param_spec_object ("main-window", _("The main window"),
                                                        _("The main window"), XFBURN_TYPE_MAIN_WINDOW, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_NOTEBOOK, 
                                   g_param_spec_object ("notebook", _("Notebook"),
                                                        _("NOTEBOOK"), XFBURN_TYPE_COMPOSITIONS_NOTEBOOK, G_PARAM_READWRITE));
}

static void
composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data)
{
  composition->show_custom_controls = show_custom_controls;
  composition->hide_custom_controls = hide_custom_controls;
  composition->load = NULL;
  composition->save = NULL;
}

static void
xfburn_welcome_tab_init (XfburnWelcomeTab * obj)
{
  //XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (obj);

  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *label_welcome;
  GtkWidget *table;
  GtkWidget *button_image;
  GtkWidget *button_data_comp;
  GtkWidget *button_blank;

  gtk_box_set_homogeneous (GTK_BOX (obj), TRUE);

  align = gtk_alignment_new (0.5, 0.5, 0.5, 0.5);
  //gtk_container_add (GTK_CONTAINER (obj), align);
  gtk_box_pack_start (GTK_BOX (obj), align, TRUE, TRUE, BORDER);
  gtk_widget_show (align);

  vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_container_add (GTK_CONTAINER (align), vbox);
  gtk_widget_show (vbox);

  label_welcome = gtk_label_new ("Welcome to xfburn!");
  gtk_box_pack_start (GTK_BOX (vbox), label_welcome, FALSE, FALSE, BORDER);
  gtk_widget_show (label_welcome);

  table = gtk_table_new (2,2,TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, BORDER);
  gtk_table_set_row_spacings (GTK_TABLE (table), BORDER);
  gtk_table_set_col_spacings (GTK_TABLE (table), BORDER);
  gtk_widget_show (table);

  button_image = gtk_button_new_with_mnemonic (_("Burn _Image"));
  gtk_table_attach_defaults (GTK_TABLE (table), button_image, 0, 1, 0, 1);
  gtk_widget_show (button_image);
  g_signal_connect (G_OBJECT(button_image), "clicked", G_CALLBACK(burn_image), obj);

  button_data_comp = gtk_button_new_with_mnemonic (_("New _Data Composition"));
  gtk_table_attach_defaults (GTK_TABLE (table), button_data_comp, 1, 2, 0, 1);
  gtk_widget_show (button_data_comp);
  g_signal_connect (G_OBJECT(button_data_comp), "clicked", G_CALLBACK(new_data_composition), obj);

  button_blank = gtk_button_new_with_mnemonic (_("_Blank Disc"));
  gtk_table_attach_defaults (GTK_TABLE (table), button_blank, 0, 1, 1, 2);
  gtk_widget_show (button_blank);
  g_signal_connect (G_OBJECT(button_blank), "clicked", G_CALLBACK(blank_disc), obj);
}

static void
xfburn_welcome_tab_finalize (GObject * object)
{
  //XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void 
xfburn_welcome_tab_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (object);

  switch (prop_id) {
    case PROP_MAIN_WINDOW:
      g_value_set_object (value, priv->mainwin);
      break;
    case PROP_NOTEBOOK:
      g_value_set_object (value, priv->notebook);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
xfburn_welcome_tab_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (object);
  
  switch (prop_id) {
    case PROP_MAIN_WINDOW:
      priv->mainwin = g_value_get_object (value);
      break;
    case PROP_NOTEBOOK:
      priv->notebook = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*           */
/* internals */
/*           */
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
burn_image (GtkButton *button, XfburnWelcomeTab *tab)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (tab);
  GtkWidget *dialog;

  dialog = xfburn_burn_image_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (priv->mainwin));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
blank_disc (GtkButton *button, XfburnWelcomeTab *tab)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (tab);
  GtkWidget *dialog;

  dialog = xfburn_blank_cd_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (priv->mainwin));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
new_data_composition (GtkButton *button, XfburnWelcomeTab *tab)
{
  XfburnWelcomeTabPrivate *priv = XFBURN_WELCOME_TAB_GET_PRIVATE (tab);
 
  xfburn_compositions_notebook_add_composition (XFBURN_COMPOSITIONS_NOTEBOOK (priv->notebook), XFBURN_DATA_COMPOSITION);
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_welcome_tab_new (XfburnMainWindow *window, XfburnCompositionsNotebook *notebook)
{
  GtkWidget *obj;

  obj = g_object_new (XFBURN_TYPE_WELCOME_TAB, 
                      "main-window", window, 
                      "notebook", notebook,
                      NULL);

  return obj;
}

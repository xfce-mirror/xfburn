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

/* prototypes */
static void xfburn_welcome_tab_class_init (XfburnWelcomeTabClass * klass);
static void xfburn_welcome_tab_init (XfburnWelcomeTab * sp);
static void xfburn_welcome_tab_finalize (GObject * object);
static void composition_interface_init (XfburnCompositionInterface *composition, gpointer iface_data);

static void show_custom_controls (XfburnComposition *composition);
static void hide_custom_controls (XfburnComposition *composition);

#define XFBURN_WELCOME_TAB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_WELCOME_TAB, XfburnWelcomeTabPrivate))

enum {
  LAST_SIGNAL,
}; 

typedef struct {
  gboolean dummy;
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

  /*
  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_WELCOME_TAB, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnWelcomeTabClass, volume_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
  */
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

  GtkWidget *label_welcome;

  label_welcome = gtk_label_new ("Welcome!");

  gtk_box_pack_start (GTK_BOX (obj), label_welcome, TRUE, TRUE, BORDER);

  gtk_widget_show (label_welcome);
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

/*        */
/* public */
/*        */

GtkWidget *
xfburn_welcome_tab_new ()
{
  GtkWidget *obj;

  obj = g_object_new (XFBURN_TYPE_WELCOME_TAB, NULL);

  return obj;
}

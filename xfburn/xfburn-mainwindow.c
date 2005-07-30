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

#include "mainwin.h"

/* prototypes */
static void xfburn_main_window_class_init (XfburnMainWindowClass *);
static void xfburn_main_window_init (XfburnMainWindow *);

static gboolean cb_delete_main_window (GtkWidget *, GdkEvent *, gpointer);

/* globals */
static GtkWindowClass *parent_class = NULL;

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
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_main_window_init (XfburnMainWindow *mainwin)
{
  /* the window itself */
  gtk_window_set_position (GTK_WINDOW (mainwin), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_title (GTK_WINDOW (mainwin), "Xfburn");
  
  g_signal_connect (G_OBJECT (mainwin), "delete-event", G_CALLBACK (cb_delete_main_window), mainwin);
    
  mainwin->vbox = gtk_vbox_new (FALSE, 0);
  
  gtk_container_add (GTK_CONTAINER (mainwin), mainwin->vbox);
  gtk_widget_show (mainwin->vbox);
}

/* private methods */
static gboolean
cb_delete_main_window (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit ();
  
  return FALSE;
}

/* public methods */
GtkWidget *
xfburn_main_window_new (void)
{
  return g_object_new (xfburn_main_window_get_type(), NULL);
}

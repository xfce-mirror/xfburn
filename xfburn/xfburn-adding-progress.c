/* $Id$ */
/*
 * Copyright (c) 2006 Jean-François Wauthy (pollux@xfce.org)
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

#include "xfburn-global.h"
#include "xfburn-adding-progress.h"
#include "xfburn-utils.h"

#define XFBURN_ADDING_PROGRESS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_ADDING_PROGRESS, XfburnAddingProgressPrivate))

enum {
  ADDING_DONE,
  LAST_SIGNAL,
};

/* private struct */
typedef struct
{
  GtkWidget *progress_bar;
  gboolean aborted;
} XfburnAddingProgressPrivate;

/* prototypes */
static void xfburn_adding_progress_class_init (XfburnAddingProgressClass *);
static void xfburn_adding_progress_init (XfburnAddingProgress *);
static void xfburn_adding_progress_finalize (GObject * object);
static gboolean cb_delete (GtkWidget *widget, GdkEvent *event, gpointer data);

/* globals */
static GtkWindowClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

/******************************/
/* XfburnAddingProgress class */
/******************************/

GtkType
xfburn_adding_progress_get_type (void)
{
  static GtkType adding_progress_type = 0;

  if (!adding_progress_type)
    {
      static const GTypeInfo adding_progress_info = {
        sizeof (XfburnAddingProgressClass),
        NULL,
        NULL,
        (GClassInitFunc) xfburn_adding_progress_class_init,
        NULL,
        NULL,
        sizeof (XfburnAddingProgress),
        0,
        (GInstanceInitFunc) xfburn_adding_progress_init,
        NULL
      };

      adding_progress_type = g_type_register_static (GTK_TYPE_WINDOW, "XfburnAddingProgress", &adding_progress_info, 0);
    }

  return adding_progress_type;
}

static void
xfburn_adding_progress_class_init (XfburnAddingProgressClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (XfburnAddingProgressPrivate));

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_adding_progress_finalize;
  
  signals[ADDING_DONE] = g_signal_new ("adding-done", XFBURN_TYPE_ADDING_PROGRESS, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnAddingProgressClass, adding_done),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
}

static void
xfburn_adding_progress_init (XfburnAddingProgress * win)
{
  XfburnAddingProgressPrivate *priv = XFBURN_ADDING_PROGRESS_GET_PRIVATE (win);
  GtkWidget *vbox;
  
  gtk_window_set_resizable (GTK_WINDOW (win), FALSE);

  gtk_window_set_icon_name (GTK_WINDOW (win), GTK_STOCK_ADD);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (win), TRUE);
  gtk_window_set_title (GTK_WINDOW (win), _("Adding files to the composition"));
  
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (win), vbox);
    
  priv->progress_bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.01);
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (GTK_BOX (vbox), priv->progress_bar, TRUE, TRUE, BORDER);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), 0.5);  

  priv->aborted = FALSE;
  g_signal_connect (G_OBJECT (win), "delete-event", G_CALLBACK (cb_delete), NULL);
}

static void
xfburn_adding_progress_finalize (GObject * object)
{
  //XfburnAddingProgressPrivate *priv = XFBURN_ADDING_PROGRESS_GET_PRIVATE (object);

}

/* internals */

static gboolean
cb_delete (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  XfburnAddingProgressPrivate *priv = XFBURN_ADDING_PROGRESS_GET_PRIVATE (widget);
  priv->aborted = TRUE;

  return TRUE;
}

/******************/
/* public methods */
/******************/

/*
 * These functions are expected to be called from the gui thread
 */
XfburnAddingProgress *
xfburn_adding_progress_new (void)
{
  GtkWidget *obj;

  obj = g_object_new (xfburn_adding_progress_get_type (), NULL);

  gtk_widget_realize (obj);

  return XFBURN_ADDING_PROGRESS (obj);
}

/*
 * these functions are expected to be called from the secondary thread
 */
void
xfburn_adding_progress_pulse (XfburnAddingProgress *adding_progress)
{
  XfburnAddingProgressPrivate *priv = XFBURN_ADDING_PROGRESS_GET_PRIVATE (adding_progress);
  
  gdk_threads_enter ();
  //DBG ("pulse");
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));
  gdk_threads_leave ();
}

void
xfburn_adding_progress_done (XfburnAddingProgress *adding_progress)
{
  //XfburnAddingProgressPrivate *priv = XFBURN_ADDING_PROGRESS_GET_PRIVATE (adding_progress);
  
  gdk_threads_enter ();
  g_signal_emit (G_OBJECT (adding_progress), signals[ADDING_DONE], 0);
  gdk_threads_leave ();
}

gboolean
xfburn_adding_progress_is_aborted (XfburnAddingProgress *adding_progress)
{
  XfburnAddingProgressPrivate *priv = XFBURN_ADDING_PROGRESS_GET_PRIVATE (adding_progress);
  
  return priv->aborted;
}

void xfburn_adding_progress_show (XfburnAddingProgress *adding_progress)
{
  XfburnAddingProgressPrivate *priv = XFBURN_ADDING_PROGRESS_GET_PRIVATE (adding_progress);

  priv->aborted = FALSE;
  gtk_widget_show (GTK_WIDGET (adding_progress));
}

/* $Id$ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "xfburn-compositions-notebook.h"

#include "xfburn-composition.h"
#include "xfburn-data-composition.h"

static void xfburn_compositions_notebook_class_init (XfburnCompositionsNotebookClass * klass);
static void xfburn_compositions_notebook_init (XfburnCompositionsNotebook * notebook);
static void xfburn_compositions_notebook_finalize (GObject * object);

typedef struct
{

} XfburnCompositionsNotebookPrivate;

static GtkNotebookClass *parent_class = NULL;

GtkType
xfburn_compositions_notebook_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnCompositionsNotebookClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_compositions_notebook_class_init,
      NULL,
      NULL,
      sizeof (XfburnCompositionsNotebook),
      0,
      (GInstanceInitFunc) xfburn_compositions_notebook_init,
    };

    type = g_type_register_static (GTK_TYPE_NOTEBOOK, "XfburnCompositionsNotebook", &our_info, 0);
  }

  return type;
}

static void
xfburn_compositions_notebook_class_init (XfburnCompositionsNotebookClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (XfburnCompositionsNotebookPrivate));

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_compositions_notebook_finalize;
}

static void
xfburn_compositions_notebook_finalize (GObject * object)
{
}

static void
xfburn_compositions_notebook_init (XfburnCompositionsNotebook * notebook)
{
}

/*************/
/* internals */
/*************/
static void
cb_composition_close (GtkButton *button, gint page_num)
{
  DBG ("%d", page_num);
}

/**********/
/* public */
/**********/
GtkWidget *
xfburn_compositions_notebook_new ()
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_COMPOSITIONS_NOTEBOOK, "scrollable", TRUE, "enable-popup", TRUE, NULL));

  return obj;
}

void
xfburn_compositions_notebook_add_composition (XfburnCompositionsNotebook *notebook, XfburnCompositionType type)
{
  GtkWidget *composition = NULL;
  gchar *label_text = NULL;
  static guint i = 0;
  
  switch (type) {
    case XFBURN_DATA_COMPOSITION:
      composition = xfburn_data_composition_new ();
      label_text = g_strdup_printf ("%s %d", _("Data compilation"), ++i);
      break;
    case XFBURN_AUDIO_COMPOSITION:
      DBG ("later");
      break;
  }
  
  if (composition) {
	GtkWidget *label = NULL;
	GtkWidget *hbox, *button_close, *img;
	gint page_num;

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_widget_show (hbox);
	
	label = gtk_label_new (label_text);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	
	button_close = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button_close), GTK_RELIEF_NONE);
	gtk_widget_show (button_close);
	
	img = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show (img);
	gtk_container_add (GTK_CONTAINER (button_close), img);
	gtk_box_pack_start (GTK_BOX (hbox), button_close, FALSE, FALSE, 0);
	
    gtk_widget_show (composition);
    page_num = gtk_notebook_append_page(GTK_NOTEBOOK (notebook), composition, hbox);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);
	
	g_signal_connect (G_OBJECT (button_close), "clicked", G_CALLBACK (cb_composition_close), GINT_TO_POINTER (page_num));
  }
  
  g_free (label_text);
}

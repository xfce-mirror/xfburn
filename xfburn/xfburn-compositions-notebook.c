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
#include "xfburn-notebook-tab.h"
#include "xfburn-data-composition.h"

#define XFBURN_COMPOSITIONS_NOTEBOOK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_COMPOSITIONS_NOTEBOOK, XfburnCompositionsNotebookPrivate))

/* private members */
typedef struct
{
  gpointer gna;
} XfburnCompositionsNotebookPrivate;


/* prototypes */
static void xfburn_compositions_notebook_class_init (XfburnCompositionsNotebookClass * klass);
static void xfburn_compositions_notebook_init (XfburnCompositionsNotebook * notebook);
static void xfburn_compositions_notebook_finalize (GObject * object);

static void cb_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, 
                            XfburnCompositionsNotebookPrivate *priv);

/* static member */
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
cb_move_focus_out (GtkNotebook *notebook, GtkDirectionType *arg1, XfburnCompositionsNotebookPrivate *priv) 
{
  guint page_num;
  
  page_num = gtk_notebook_get_current_page (notebook);
  DBG ("%d", page_num);
}

static void
xfburn_compositions_notebook_init (XfburnCompositionsNotebook * notebook)
{
  XfburnCompositionsNotebookPrivate *priv = XFBURN_COMPOSITIONS_NOTEBOOK_GET_PRIVATE (notebook);
  
  g_signal_connect (G_OBJECT (notebook), "switch-page", G_CALLBACK (cb_switch_page), priv);
  g_signal_connect (G_OBJECT (notebook), "move-focus-out", G_CALLBACK (cb_move_focus_out), priv);
}

/*************/
/* internals */
/*************/
static void
cb_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, XfburnCompositionsNotebookPrivate *priv)
{

}

static void
cb_composition_close (XfburnNotebookTab *tab, GtkNotebook *notebook)
{
  GtkWidget *composition;
  guint page_num;
  
  composition = g_object_get_data (G_OBJECT (tab), "composition");
  page_num = gtk_notebook_page_num (notebook, composition);
  gtk_notebook_remove_page (notebook, page_num);
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
      label_text = g_strdup_printf ("%s %d", _("Data composition"), ++i);
      break;
    case XFBURN_AUDIO_COMPOSITION:
      DBG ("later");
      break;
  }
  
  if (composition) {
    GtkWidget *tab = NULL;
    guint page_num;
    
    tab = xfburn_notebook_tab_new (label_text);
    gtk_widget_show (tab);
	
    gtk_widget_show (composition);
    page_num = gtk_notebook_append_page(GTK_NOTEBOOK (notebook), composition, tab);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);
	
    g_object_set_data (G_OBJECT (tab), "composition", composition);
    g_signal_connect (G_OBJECT (tab), "button-close-clicked", G_CALLBACK (cb_composition_close), notebook);
  }
  
  g_free (label_text);
}

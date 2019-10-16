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

#include "xfburn-global.h"
#include "xfburn-notebook-tab.h"
#include "xfburn-welcome-tab.h"
#include "xfburn-data-composition.h"
#include "xfburn-audio-composition.h"

#define XFBURN_COMPOSITIONS_NOTEBOOK_GET_PRIVATE(obj) (xfburn_compositions_notebook_get_instance_private (obj))

/* private members */
typedef struct
{
  gpointer dummy;
} XfburnCompositionsNotebookPrivate;


/* prototypes */
static void xfburn_compositions_notebook_finalize (GObject * object);


/* internals */
static void cb_switch_page (GtkNotebook *notebook, GtkWidget *page, guint page_num, 
                            XfburnCompositionsNotebookPrivate *priv);
static void cb_composition_close (XfburnNotebookTab *tab, GtkNotebook *notebook);
static void cb_composition_name_changed (XfburnComposition *composition, const gchar * name, XfburnCompositionsNotebook *notebook);
static XfburnComposition * add_composition_with_data (XfburnCompositionsNotebook *notebook, XfburnCompositionType type, XfburnMainWindow *window);


/* static member */
static GtkNotebookClass *parent_class = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(XfburnCompositionsNotebook, xfburn_compositions_notebook, GTK_TYPE_NOTEBOOK);

static void
xfburn_compositions_notebook_class_init (XfburnCompositionsNotebookClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

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

/***********/
/* actions */
/***********/

/*************/
/* internals */
/*************/
static void
cb_switch_page (GtkNotebook *notebook, GtkWidget *page, guint page_num, XfburnCompositionsNotebookPrivate *priv)
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

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) > 1);
}

static void
cb_composition_name_changed (XfburnComposition *composition, const gchar * name, XfburnCompositionsNotebook *notebook)
{
  //guint page_num;
  GtkWidget *tab, *menu_label;
  
  //page_num = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), GTK_WIDGET (composition));
  
  tab = gtk_notebook_get_tab_label (GTK_NOTEBOOK (notebook), GTK_WIDGET (composition));
  xfburn_notebook_tab_set_label (XFBURN_NOTEBOOK_TAB (tab), name);
  menu_label = gtk_notebook_get_menu_label (GTK_NOTEBOOK (notebook), GTK_WIDGET (composition));
  gtk_label_set_text (GTK_LABEL (menu_label), name);
}

static XfburnComposition *
add_composition_with_data (XfburnCompositionsNotebook *notebook, XfburnCompositionType type, XfburnMainWindow *window)
{
  GtkWidget *composition = NULL;
  gchar *label_text = NULL;
  static guint i = 0;
  
  switch (type) {   
    case XFBURN_DATA_COMPOSITION:
      composition = xfburn_data_composition_new ();
      label_text = g_strdup_printf ("%s %d", _(DATA_COMPOSITION_DEFAULT_NAME), ++i);
      break;
    case XFBURN_AUDIO_COMPOSITION:
      composition = xfburn_audio_composition_new ();
      label_text = g_strdup_printf ("%s %d", _("Audio composition"), ++i);
      break;
  }
  
  if (composition) {
    GtkWidget *tab = NULL;
    GtkWidget *menu_label = NULL;
    
    guint page_num;
    
    tab = xfburn_notebook_tab_new (label_text, TRUE);
    gtk_widget_show (tab);
	
    menu_label = gtk_label_new (label_text);
    gtk_widget_show (menu_label);
    
    gtk_widget_show (composition);
    page_num = gtk_notebook_append_page_menu (GTK_NOTEBOOK (notebook), composition, tab, menu_label);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);

    gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (notebook), composition, 1);
	
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) > 1);

    g_object_set_data (G_OBJECT (tab), "composition", composition);
    g_signal_connect (G_OBJECT (tab), "button-close-clicked", G_CALLBACK (cb_composition_close), notebook);
    
    g_signal_connect (G_OBJECT (composition), "name-changed", G_CALLBACK (cb_composition_name_changed), notebook);
  }
  
  g_free (label_text);

  return XFBURN_COMPOSITION (composition);
}

/**********/
/* public */
/**********/
GtkWidget *
xfburn_compositions_notebook_new (void)
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_COMPOSITIONS_NOTEBOOK, "scrollable", TRUE, "enable-popup", TRUE, NULL));

  return obj;
}

XfburnComposition *
xfburn_compositions_notebook_add_composition (XfburnCompositionsNotebook *notebook, XfburnCompositionType type)
{
  return add_composition_with_data (notebook, type, NULL);
}

void
xfburn_compositions_notebook_add_welcome_tab (XfburnCompositionsNotebook *notebook, GActionMap *action_group)
{
  GtkWidget *welcome_tab = NULL;
  GtkWidget *label;

  welcome_tab = xfburn_welcome_tab_new (notebook, action_group);
  label = gtk_label_new (_("Welcome"));

  gtk_widget_show (welcome_tab);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), welcome_tab, label);

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) > 1);
}

void
xfburn_compositions_notebook_close_composition (XfburnCompositionsNotebook *notebook)
{
  gint page_num;
  
  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
  if (page_num > 0)
    /* don't close the welcome page */
    gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), page_num);
}

void
xfburn_compositions_notebook_save_composition (XfburnCompositionsNotebook *notebook)
{
  XfburnComposition *composition;
  guint page_num;
  
  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
  composition = XFBURN_COMPOSITION (gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num));
  
  xfburn_composition_save (composition);
}

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

#ifndef __XFBURN_NOTEBOOK_TAB_H__
#define __XFBURN_NOTEBOOK_TAB_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_NOTEBOOK_TAB         (xfburn_notebook_tab_get_type ())
#define XFBURN_NOTEBOOK_TAB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_NOTEBOOK_TAB, XfburnNotebookTab))
#define XFBURN_NOTEBOOK_TAB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_NOTEBOOK_TAB, XfburnNotebookTabClass))
#define XFBURN_IS_NOTEBOOK_TAB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_NOTEBOOK_TAB))
#define XFBURN_IS_NOTEBOOK_TAB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_NOTEBOOK_TAB))
#define XFBURN_NOTEBOOK_TAB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_NOTEBOOK_TAB, XfburnNotebookTabClass))

typedef struct
{
  GtkHBox parent;
} XfburnNotebookTab;

typedef struct
{
  GtkHBoxClass parent_class;
  
  void (*button_close_clicked) (XfburnNotebookTab *tab);
} XfburnNotebookTabClass;

GType xfburn_notebook_tab_get_type (void);

GtkWidget *xfburn_notebook_tab_new (const gchar *label, gboolean show_button_close);

void xfburn_notebook_tab_set_label (XfburnNotebookTab *tab, const gchar *label);

void xfburn_notebook_tab_show_button_close (XfburnNotebookTab *tab);
void xfburn_notebook_tab_show_button_hide (XfburnNotebookTab *tab);

G_END_DECLS
#endif /* XFBURN_NOTEBOOK_TAB_H */

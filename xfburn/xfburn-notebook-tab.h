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

#define XFBURN_TYPE_NOTEBOOK_TAB (xfburn_notebook_tab_get_type ())
G_DECLARE_DERIVABLE_TYPE (XfburnNotebookTab, xfburn_notebook_tab, XFBURN, NOTEBOOK_TAB, GtkBox)

struct _XfburnNotebookTabClass
{
  GtkHBoxClass parent_class;

  void (*button_close_clicked) (XfburnNotebookTab *tab);
};

GtkWidget *xfburn_notebook_tab_new (const gchar *label, gboolean show_button_close);

void xfburn_notebook_tab_set_label (XfburnNotebookTab *tab, const gchar *label);

void xfburn_notebook_tab_show_button_close (XfburnNotebookTab *tab);
void xfburn_notebook_tab_show_button_hide (XfburnNotebookTab *tab);

G_END_DECLS
#endif /* XFBURN_NOTEBOOK_TAB_H */

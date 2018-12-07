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

#ifndef __XFBURN_COMPOSITIONS_NOTEBOOK_H__
#define __XFBURN_COMPOSITIONS_NOTEBOOK_H__

#include <gtk/gtk.h>

#include "xfburn-main-window.h"
#include "xfburn-composition.h"

G_BEGIN_DECLS
#define XFBURN_TYPE_COMPOSITIONS_NOTEBOOK         (xfburn_compositions_notebook_get_type ())
#define XFBURN_COMPOSITIONS_NOTEBOOK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_COMPOSITIONS_NOTEBOOK, XfburnCompositionsNotebook))
#define XFBURN_COMPOSITIONS_NOTEBOOK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_COMPOSITIONS_NOTEBOOK, XfburnCompositionsNotebookClass))
#define XFBURN_IS_COMPOSITIONS_NOTEBOOK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_COMPOSITIONS_NOTEBOOK))
#define XFBURN_IS_COMPOSITIONS_NOTEBOOK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_COMPOSITIONS_NOTEBOOK))
#define XFBURN_COMPOSITIONS_NOTEBOOK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_COMPOSITIONS_NOTEBOOK, XfburnCompositionsNotebookClass))

typedef struct
{
  GtkNotebook parent;
} XfburnCompositionsNotebook;

typedef struct
{
  GtkNotebookClass parent_class;
} XfburnCompositionsNotebookClass;

typedef enum
{
  XFBURN_DATA_COMPOSITION,
  XFBURN_AUDIO_COMPOSITION,
} XfburnCompositionType;

GType xfburn_compositions_notebook_get_type (void);

GtkWidget *xfburn_compositions_notebook_new (void);

XfburnComposition *xfburn_compositions_notebook_add_composition (XfburnCompositionsNotebook *notebook, XfburnCompositionType type);
void xfburn_compositions_notebook_add_welcome_tab (XfburnCompositionsNotebook *notebook, GActionMap *action_group);

void xfburn_compositions_notebook_close_composition (XfburnCompositionsNotebook *notebook);

void xfburn_compositions_notebook_load_composition (XfburnCompositionsNotebook *notebook, const gchar *file);
void xfburn_compositions_notebook_save_composition (XfburnCompositionsNotebook *notebook);

#endif /* XFBURN_COMPOSITIONS_NOTEBOOK_H */

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

#define XFBURN_TYPE_COMPOSITIONS_NOTEBOOK (xfburn_compositions_notebook_get_type ())
G_DECLARE_FINAL_TYPE (XfburnCompositionsNotebook, xfburn_compositions_notebook, XFBURN, COMPOSITIONS_NOTEBOOK, GtkNotebook)

typedef enum
{
  XFBURN_COMPOSITION_DATA,
  XFBURN_COMPOSITION_AUDIO,
} XfburnCompositionType;

GtkWidget *xfburn_compositions_notebook_new (void);

XfburnComposition *xfburn_compositions_notebook_add_composition (XfburnCompositionsNotebook *notebook, XfburnCompositionType type);
void xfburn_compositions_notebook_add_welcome_tab (XfburnCompositionsNotebook *notebook, GActionMap *action_group);

void xfburn_compositions_notebook_close_composition (XfburnCompositionsNotebook *notebook);

G_END_DECLS

#endif /* XFBURN_COMPOSITIONS_NOTEBOOK_H */

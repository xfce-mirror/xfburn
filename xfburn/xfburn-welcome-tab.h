/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *  Copyright (c) 2008      David Mohr (david@mcbf.net)
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

#ifndef __XFBURN_WELCOME_TAB_H__
#define __XFBURN_WELCOME_TAB_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "xfburn-compositions-notebook.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_WELCOME_TAB         (xfburn_welcome_tab_get_type ())
#define XFBURN_WELCOME_TAB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_WELCOME_TAB, XfburnWelcomeTab))
#define XFBURN_WELCOME_TAB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_WELCOME_TAB, XfburnWelcomeTabClass))
#define XFBURN_IS_WELCOME_TAB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_WELCOME_TAB))
#define XFBURN_IS_WELCOME_TAB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_WELCOME_TAB))
#define XFBURN_WELCOME_TAB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_WELCOME_TAB, XfburnWelcomeTabClass))

typedef struct
{
  GtkBox parent;
} XfburnWelcomeTab;

typedef struct
{
  GtkBoxClass parent_class;
} XfburnWelcomeTabClass;

GType xfburn_welcome_tab_get_type (void);
GtkWidget *xfburn_welcome_tab_new (XfburnCompositionsNotebook *notebook, GActionMap *action_group);

G_END_DECLS

#endif /* XFBURN_WELCOME_TAB_H */

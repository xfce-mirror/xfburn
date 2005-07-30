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

#include "xfburn-disc-content.h"

/* prototypes */
static void xfburn_disc_content_class_init (XfburnDiscContentClass *);
static void xfburn_disc_content_init (XfburnDiscContent *);

/* globals */
static GtkHPanedClass *parent_class = NULL;

/***************************/
/* XfburnDiscContent class */
/***************************/
GtkType
xfburn_disc_content_get_type (void)
{
  static GtkType disc_content_type = 0;

  if (!disc_content_type) {
    static const GTypeInfo disc_content_info = {
      sizeof (XfburnDiscContentClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_disc_content_class_init,
      NULL,
      NULL,
      sizeof (XfburnDiscContent),
      0,
      (GInstanceInitFunc) xfburn_disc_content_init
    };

    disc_content_type = g_type_register_static (GTK_TYPE_HPANED, "XfburnDiscContent", &disc_content_info, 0);
  }

  return disc_content_type;
}

static void
xfburn_disc_content_class_init (XfburnDiscContentClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_disc_content_init (XfburnDiscContent * disc_content)
{

}

/* public methods */
GtkWidget *
xfburn_disc_content_new (void)
{
  return g_object_new (xfburn_disc_content_get_type (), NULL);
}

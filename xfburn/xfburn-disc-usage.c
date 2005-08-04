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

#include "xfburn-disc-usage.h"
#include "xfburn-utils.h"

/* prototypes */
static void xfburn_disc_usage_class_init (XfburnDiscUsageClass *);
static void xfburn_disc_usage_init (XfburnDiscUsage *);

/* globals */
static GtkHBoxClass *parent_class = NULL;

/***************************/
/* XfburnDiscContent class */
/***************************/
GtkType
xfburn_disc_usage_get_type (void)
{
  static GtkType disc_usage_type = 0;

  if (!disc_usage_type) {
    static const GTypeInfo disc_usage_info = {
      sizeof (XfburnDiscUsageClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_disc_usage_class_init,
      NULL,
      NULL,
      sizeof (XfburnDiscUsage),
      0,
      (GInstanceInitFunc) xfburn_disc_usage_init
    };

    disc_usage_type = g_type_register_static (GTK_TYPE_HBOX, "XfburnDiscUsage", &disc_usage_info, 0);
  }

  return disc_usage_type;
}

static void
xfburn_disc_usage_class_init (XfburnDiscUsageClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_disc_usage_init (XfburnDiscUsage * disc_usage)
{
  disc_usage->progress_bar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (disc_usage), disc_usage->progress_bar, TRUE, TRUE, 5);
  gtk_widget_show (disc_usage->progress_bar);
    
  disc_usage->combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (disc_usage->combo), "640MB");
  gtk_combo_box_append_text (GTK_COMBO_BOX (disc_usage->combo), "700MB");
  gtk_box_pack_start (GTK_BOX (disc_usage), disc_usage->combo, FALSE, FALSE, 5);
  gtk_widget_show (disc_usage->combo);
  
  disc_usage->button = xfce_create_mixed_button (GTK_STOCK_CDROM, _("Burn Data CD"));
  gtk_box_pack_start (GTK_BOX (disc_usage), disc_usage->button, FALSE, FALSE, 5);
  gtk_widget_set_sensitive (disc_usage->button, FALSE);
  gtk_widget_show (disc_usage->button);
}

/* public methods */
GtkWidget *
xfburn_disc_usage_new (void)
{
  return g_object_new (xfburn_disc_usage_get_type (), NULL);
}

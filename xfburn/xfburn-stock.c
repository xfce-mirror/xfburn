/* $Id$ */
/*
 * Copyright (c) 2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>

#include "xfburn-stock.h"

static const gchar *xfburn_stock_icons[] = {
  XFBURN_STOCK_AUDIO_COPY,
  XFBURN_STOCK_BLANK_CDRW,
  XFBURN_STOCK_FORMAT_DVDRW,
  XFBURN_STOCK_BURN_CD,
  XFBURN_STOCK_CD,
  XFBURN_STOCK_DATA_COPY,
  XFBURN_STOCK_IMPORT_SESSION,
};

void
xfburn_stock_init (void)
{
  GtkIconFactory * icon_factory;
  GtkIconSource *icon_source;
  GtkIconSet *icon_set;
  gchar icon_name[128];
  guint n;

  /* allocate a new icon factory for the terminal icons */
  icon_factory = gtk_icon_factory_new ();

  memcpy (icon_name, "stock_", sizeof ("stock_"));

  /* we try to avoid allocating multiple icon sources */
  icon_source = gtk_icon_source_new ();

  /* register our stock icons */
  for (n = 0; n < G_N_ELEMENTS (xfburn_stock_icons); ++n) {
    /* set the new icon name for the icon source */
    strcpy (icon_name + (sizeof ("stock_") - 1), xfburn_stock_icons[n]);
    gtk_icon_source_set_icon_name (icon_source, icon_name);

    /* allocate the icon set */
    icon_set = gtk_icon_set_new ();
    gtk_icon_set_add_source (icon_set, icon_source);
    gtk_icon_factory_add (icon_factory, xfburn_stock_icons[n], icon_set);
    gtk_icon_set_unref (icon_set);
  }

  /* register the icon factory as default */
  gtk_icon_factory_add_default (icon_factory);

  /* cleanup */
  g_object_unref (G_OBJECT (icon_factory));
  gtk_icon_source_free (icon_source);
}

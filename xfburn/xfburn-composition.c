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

#include "xfburn-composition.h"

enum {
  NAME_CHANGED,
  LAST_SIGNAL,
};

/*******************************************/
/* interface definition and initialization */
/*******************************************/
static guint composition_signals[LAST_SIGNAL];

static void
xfburn_composition_base_init (gpointer g_iface)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    composition_signals[NAME_CHANGED] = g_signal_new ("name-changed", XFBURN_TYPE_COMPOSITION, G_SIGNAL_ACTION,
                                                      G_STRUCT_OFFSET (XfburnCompositionInterface, name_changed),
                                                      NULL, NULL, g_cclosure_marshal_VOID__STRING,
                                                      G_TYPE_NONE, 1, G_TYPE_STRING);
    
    initialized = TRUE;
  }
}

GType
xfburn_composition_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (XfburnCompositionInterface),
      xfburn_composition_base_init,  /* base_init */
      NULL,                          /* base_finalize */
      NULL,                          /* class_init */
      NULL,                          /* class_finalize */
      NULL,                          /* class_data */
      0,
      0,                             /* n_preallocs */
      NULL,                          /* instance_init */
      NULL
    };
    
    type = g_type_register_static (G_TYPE_INTERFACE, "XfburnComposition", &info, 0);
  }
  
  return type;
}

/**************/
/* public API */
/**************/

void
xfburn_composition_show_custom_controls (XfburnComposition *composition)
{
  return XFBURN_COMPOSITION_GET_INTERFACE (composition)->show_custom_controls (composition);
}

void
xfburn_composition_hide_custom_controls (XfburnComposition *composition)
{
  return XFBURN_COMPOSITION_GET_INTERFACE (composition)->hide_custom_controls (composition);
}

void
xfburn_composition_load (XfburnComposition *composition, const gchar *file)
{
  return XFBURN_COMPOSITION_GET_INTERFACE (composition)->load (composition, file);
}

void
xfburn_composition_save (XfburnComposition *composition)
{
  return XFBURN_COMPOSITION_GET_INTERFACE (composition)->save (composition);
}

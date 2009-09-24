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
 
#ifndef __XFBURN_COMPOSITION_H__
#define __XFBURN_COMPOSITION_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_COMPOSITION                (xfburn_composition_get_type ())
#define XFBURN_COMPOSITION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_COMPOSITION, XfburnComposition))
#define XFBURN_IS_COMPOSITION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_COMPOSITION))
#define XFBURN_COMPOSITION_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), XFBURN_TYPE_COMPOSITION, XfburnCompositionInterface))

typedef struct _XfburnCompositon XfburnComposition; /* dummy object */
  
typedef struct {
  GTypeInterface parent;
  
  void (*show_custom_controls) (XfburnComposition *composition);
  void (*hide_custom_controls) (XfburnComposition *composition);
  
  void (*load) (XfburnComposition *composition, const gchar *file);
  void (*save) (XfburnComposition *composition);
  
  void (*name_changed) (XfburnComposition *composition, const gchar *name);
} XfburnCompositionInterface;

GType xfburn_composition_get_type (void);

void xfburn_composition_show_custom_controls (XfburnComposition *composition);
void xfburn_composition_hide_custom_controls (XfburnComposition *composition);

void xfburn_composition_load (XfburnComposition *composition, const gchar *file);
void xfburn_composition_save (XfburnComposition *composition);

G_END_DECLS
#endif /* __XFBURN_COMPOSITION_H__ */

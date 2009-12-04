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

#ifndef __XFBURN_DATA_COMPOSITION_H__
#define __XFBURN_DATA_COMPOSITION_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_DATA_COMPOSITION            (xfburn_data_composition_get_type ())
#define XFBURN_DATA_COMPOSITION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_DATA_COMPOSITION, XfburnDataComposition))
#define XFBURN_DATA_COMPOSITION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_DATA_COMPOSITION, XfburnDataCompositionClass))
#define XFBURN_IS_DATA_COMPOSITION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_DATA_COMPOSITION))
#define XFBURN_IS_DATA_COMPOSITION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_DATA_COMPOSITION))
#define XFBURN_DATA_COMPOSITION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_DATA_COMPOSITION, XfburnDataCompositionClass))
  
typedef struct
{
  GtkVBox vbox;
} XfburnDataComposition;

typedef struct
{
  GtkVBoxClass parent_class;
} XfburnDataCompositionClass;

enum
{
  DATA_COMPOSITION_DND_TARGET_INSIDE,
  DATA_COMPOSITION_DND_TARGET_TEXT_PLAIN,
  DATA_COMPOSITION_DND_TARGET_TEXT_URI_LIST,
};

GType xfburn_data_composition_get_type (void);

GtkWidget *xfburn_data_composition_new (void);
void xfburn_data_composition_add_files (XfburnDataComposition *content, GSList * filelist);
void xfburn_data_composition_hide_toolbar (XfburnDataComposition *content);
void xfburn_data_composition_show_toolbar (XfburnDataComposition *content);


G_END_DECLS
#endif

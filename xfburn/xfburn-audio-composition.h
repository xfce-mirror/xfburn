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

#ifndef __XFBURN_AUDIO_COMPOSITION_H__
#define __XFBURN_AUDIO_COMPOSITION_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "xfburn-transcoder.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_AUDIO_COMPOSITION            (xfburn_audio_composition_get_type ())
#define XFBURN_AUDIO_COMPOSITION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_AUDIO_COMPOSITION, XfburnAudioComposition))
#define XFBURN_AUDIO_COMPOSITION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_AUDIO_COMPOSITION, XfburnAudioCompositionClass))
#define XFBURN_IS_AUDIO_COMPOSITION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_AUDIO_COMPOSITION))
#define XFBURN_IS_AUDIO_COMPOSITION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_AUDIO_COMPOSITION))
#define XFBURN_AUDIO_COMPOSITION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_AUDIO_COMPOSITION, XfburnAudioCompositionClass))
  
typedef struct
{
  GtkVBox vbox;
} XfburnAudioComposition;

typedef struct
{
  GtkVBoxClass parent_class;
} XfburnAudioCompositionClass;

enum
{
  AUDIO_COMPOSITION_DND_TARGET_INSIDE,
  AUDIO_COMPOSITION_DND_TARGET_TEXT_PLAIN,
  AUDIO_COMPOSITION_DND_TARGET_TEXT_URI_LIST,
};

GType xfburn_audio_composition_get_type (void);

GtkWidget *xfburn_audio_composition_new (void);
void xfburn_audio_composition_add_files (XfburnAudioComposition *content, GSList * filelist);
void xfburn_audio_composition_hide_toolbar (XfburnAudioComposition *content);
void xfburn_audio_composition_show_toolbar (XfburnAudioComposition *content);


G_END_DECLS
#endif

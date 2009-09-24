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

#ifndef __XFBURN_ADDING_PROGRESS_H__
#define __XFBURN_ADDING_PROGRESS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "xfburn-data-composition.h"

G_BEGIN_DECLS
#define XFBURN_TYPE_ADDING_PROGRESS            (xfburn_adding_progress_get_type ())
#define XFBURN_ADDING_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_ADDING_PROGRESS, XfburnAddingProgress))
#define XFBURN_ADDING_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_ADDING_PROGRESS, XfburnAddingProgressClass))
#define XFBURN_IS_ADDING_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_ADDING_PROGRESS))
#define XFBURN_IS_ADDING_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_ADDING_PROGRESS))
#define XFBURN_ADDING_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_ADDING_PROGRESS, XfburnAddingProgressClass))

typedef struct 
{
  GtkWindow window;
} XfburnAddingProgress;

typedef struct 
{
  GtkWindowClass parent_class;
  void (*adding_done) (XfburnAddingProgress *progress, XfburnDataComposition *dc);
} XfburnAddingProgressClass;

GType xfburn_adding_progress_get_type (void);

XfburnAddingProgress *xfburn_adding_progress_new (void);
void xfburn_adding_progress_pulse (XfburnAddingProgress *adding_progress);
void xfburn_adding_progress_wait_until_done (XfburnAddingProgress *adding_progress);
void xfburn_adding_progress_done (XfburnAddingProgress *adding_progress);
gboolean xfburn_adding_progress_is_aborted (XfburnAddingProgress *adding_progress);
void xfburn_adding_progress_show (XfburnAddingProgress *adding_progress);

G_END_DECLS
#endif

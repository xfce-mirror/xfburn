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

#ifndef __XFBURN_DISC_USAGE_H__
#define __XFBURN_DISC_USAGE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_DISC_USAGE            (xfburn_disc_usage_get_type ())
#define XFBURN_DISC_USAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_DISC_USAGE, XfburnDiscUsage))
#define XFBURN_DISC_USAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_DISC_USAGE, XfburnDiscUsageClass))
#define XFBURN_IS_DISC_USAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_DISC_USAGE))
#define XFBURN_IS_DISC_USAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_DISC_USAGE))
#define XFBURN_DISC_USAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_DISC_USAGE, XfburnDiscUsageClass))

typedef struct
{
  GtkVBox hbox;

  GtkWidget *progress_bar;
  GtkWidget *combo;
  GtkWidget *button;

  gdouble size;
} XfburnDiscUsage;

typedef struct
{
  guint64 size;
  gchar *label;
} XfburnDiscLabels;

typedef struct
{
  GtkHBoxClass parent_class;
  XfburnDiscLabels *labels;
  int num_labels;
  
  /* signals */
  void (*begin_burn) (XfburnDiscUsage *du);

  /* virtual functions */
  void (*update_size) (XfburnDiscUsage *du);
  gboolean (*can_burn) (XfburnDiscUsage *du);
} XfburnDiscUsageClass;

typedef enum
{
  CD_DISC,
  DVD_DISC,
} XfburnDiscType;


GType xfburn_disc_usage_get_type (void);
GtkWidget *xfburn_disc_usage_new (void);

gdouble xfburn_disc_usage_get_size (XfburnDiscUsage *);
void xfburn_disc_usage_set_size (XfburnDiscUsage *, gdouble);
void xfburn_disc_usage_add_size (XfburnDiscUsage *, gdouble);
void xfburn_disc_usage_sub_size (XfburnDiscUsage *, gdouble);

XfburnDiscType xfburn_disc_usage_get_disc_type (XfburnDiscUsage *);

G_END_DECLS

#endif

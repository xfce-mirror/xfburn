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

#ifndef __XFBURN_DISC_CONTENT_H__
#define __XFBURN_DISC_CONTENT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS
#define XFBURN_TYPE_DISC_CONTENT            (xfburn_disc_content_get_type ())
#define XFBURN_DISC_CONTENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_DISC_CONTENT, XfburnDiscContent))
#define XFBURN_DISC_CONTENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_DISC_CONTENT, XfburnDiscContentClass))
#define XFBURN_IS_DISC_CONTENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_DISC_CONTENT))
#define XFBURN_IS_DISC_CONTENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_DISC_CONTENT))
#define XFBURN_DISC_CONTENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_DISC_CONTENT, XfburnDiscContentClass))
typedef struct _XfburnDiscContent XfburnDiscContent;
typedef struct _XfburnDiscContentClass XfburnDiscContentClass;
typedef struct XfburnDiscContentPrivate XfburnDiscContentPrivate;
  
struct _XfburnDiscContent
{
  GtkVBox vbox;

  XfburnDiscContentPrivate *priv;
};

struct _XfburnDiscContentClass
{
  GtkVBoxClass parent_class;
  
  void (*begin_burn) (XfburnDiscContent *dc);
};

enum
{
  DISC_CONTENT_DND_TARGET_INSIDE,
  DISC_CONTENT_DND_TARGET_TEXT_PLAIN,
};

GtkType xfburn_disc_content_get_type (void);

GtkWidget *xfburn_disc_content_new (void);
void xfburn_disc_content_hide_toolbar (XfburnDiscContent *content);
void xfburn_disc_content_show_toolbar (XfburnDiscContent *content);
void xfburn_disc_content_load_from_file (XfburnDiscContent *dc, const gchar *filename);
void xfburn_disc_content_save_to_file (XfburnDiscContent *dc, const gchar *filename);

G_END_DECLS
#endif

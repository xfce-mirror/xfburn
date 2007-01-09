/* $Id$ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFBURN_PROGRESS_DIALOG_H__
#define __XFBURN_PROGRESS_DIALOG_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "xfburn-global.h"

G_BEGIN_DECLS
/* */
#define XFBURN_TYPE_PROGRESS_DIALOG_STATUS (xfburn_progress_dialog_status_get_type ())
typedef enum
{
  XFBURN_PROGRESS_DIALOG_STATUS_RUNNING,
  XFBURN_PROGRESS_DIALOG_STATUS_FAILED,
  XFBURN_PROGRESS_DIALOG_STATUS_CANCELLED,
  XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED
} XfburnProgressDialogStatus;

GType xfburn_progress_dialog_status_get_type (void);


/* */
#define XFBURN_TYPE_PROGRESS_DIALOG         (xfburn_progress_dialog_get_type ())
#define XFBURN_PROGRESS_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_PROGRESS_DIALOG, XfburnProgressDialog))
#define XFBURN_PROGRESS_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_PROGRESS_DIALOG, XfburnProgressDialogClass))
#define XFBURN_IS_PROGRESS_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_PROGRESS_DIALOG))
#define XFBURN_IS_PROGRESS_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_PROGRESS_DIALOG))
#define XFBURN_PROGRESS_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_PROGRESS_DIALOG, XfburnProgressDialogClass))

typedef struct
{
  GtkDialog parent;
} XfburnProgressDialog;

typedef struct
{
  GtkDialogClass parent_class;
  
  void (*output) (XfburnProgressDialog *dialog, const gchar *output);
  void (*finished) (XfburnProgressDialog *dialog);
} XfburnProgressDialogClass;


GtkType xfburn_progress_dialog_get_type ();

void xfburn_progress_dialog_append_output (XfburnProgressDialog * dialog, const gchar * output);
void xfburn_progress_dialog_show_buffers (XfburnProgressDialog * dialog, gboolean show);
void xfburn_progress_dialog_pulse_progress_bar (XfburnProgressDialog * dialog);
void xfburn_progress_dialog_write_input (XfburnProgressDialog * dialog, const gchar * input);

XfburnProgressDialogStatus xfburn_progress_dialog_get_status (XfburnProgressDialog * dialog);
gdouble xfburn_progress_dialog_get_progress_bar_fraction (XfburnProgressDialog * dialog);

void xfburn_progress_dialog_set_progress_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction);
void xfburn_progress_dialog_set_fifo_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction);
void xfburn_progress_dialog_set_buffer_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction);
void xfburn_progress_dialog_set_action_text (XfburnProgressDialog * dialog, const gchar * text);
void xfburn_progress_dialog_set_writing_speed (XfburnProgressDialog * dialog, gfloat speed);
void xfburn_progress_dialog_set_status (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status);

G_END_DECLS

#endif /* XFBURN_PROGRESS_DIALOG_H */

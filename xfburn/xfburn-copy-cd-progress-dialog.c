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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "xfburn-global.h"
#include "xfburn-progress-dialog.h"

#include "xfburn-copy-cd-progress-dialog.h"

static void xfburn_copy_cd_progress_dialog_class_init (XfburnCopyCdProgressDialogClass * klass);
static void xfburn_copy_cd_progress_dialog_init (XfburnCopyCdProgressDialog * sp);

static void cb_new_output (XfburnCopyCdProgressDialog * dialog, const gchar * output, gpointer data);

/*********************/
/* class declaration */
/*********************/
static XfburnProgressDialogClass *parent_class = NULL;

GType
xfburn_copy_cd_progress_dialog_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnCopyCdProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_copy_cd_progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnCopyCdProgressDialog),
      0,
      (GInstanceInitFunc) xfburn_copy_cd_progress_dialog_init,
      NULL
    };

    type = g_type_register_static (XFBURN_TYPE_PROGRESS_DIALOG, "XfburnCopyCdProgressDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_copy_cd_progress_dialog_class_init (XfburnCopyCdProgressDialogClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_copy_cd_progress_dialog_init (XfburnCopyCdProgressDialog * obj)
{
  g_signal_connect_after (G_OBJECT (obj), "output", G_CALLBACK (cb_new_output), NULL);
}

/*           */
/* internals */
/*           */
static void
cb_new_output (XfburnCopyCdProgressDialog * dialog, const gchar * output, gpointer data)
{
  static gint readcd_end = -1;
  
  if (strstr (output, CDRDAO_DONE)) {
    xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED);
  }
  else if (strstr (output, CDRDAO_FLUSHING)) {
    xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Flushing cache..."));
  }
  else if (strstr (output, CDRDAO_LENGTH)) {
    gint min, sec, cent;

    sscanf (output, "%*s %d:%d:%d", &min, &sec, &cent);
    readcd_end = cent + 100 * sec + 60 * 100 * min;
  }
  else if (strstr (output, CDRDAO_INSERT) || strstr (output, CDRDAO_INSERT_AGAIN)) {
    GtkWidget *dialog_confirm;

    dialog_confirm = gtk_message_dialog_new (GTK_WINDOW (dialog), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_INFO, GTK_BUTTONS_OK, _("Please insert a recordable disc."));
    gtk_dialog_run (GTK_DIALOG (dialog_confirm));
    gtk_widget_destroy (dialog_confirm);
    xfburn_progress_dialog_write_input (XFBURN_PROGRESS_DIALOG (dialog), "\n");
  }
  else {
    gint done, total, buffer1, buffer2;
    gint min, sec, cent;
    gdouble fraction;
    
    if (sscanf (output, "Wrote %d %*s %d %*s %*s %d%% %d%%", &done, &total, &buffer1, &buffer2) == 4) {
      gchar *command;
            
      command = g_object_get_data (G_OBJECT (dialog), "command");
      if (strstr (command, "on-the-fly")) {
        fraction = ((gdouble) done) / total;
      }
      else {
        fraction = ((gdouble) done) / total;
        fraction = 0.5 + (fraction / 2);
      }

      xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Writing CD..."));
      xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), fraction * 100.0);
      xfburn_progress_dialog_set_fifo_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), ((gdouble) buffer1));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), ((gdouble) buffer2));
    }
    else if (sscanf (output, "%d:%d:%d", &min, &sec, &cent) == 3) {
      gint readcd_done = -1;

      readcd_done = cent + 100 * sec + 60 * 100 * min;

      fraction = ((gdouble) readcd_done) / readcd_end;
      fraction = fraction / 2;

      xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Reading CD..."));
      xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), fraction);
    }
  }
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_copy_cd_progress_dialog_new (void)
{
  XfburnCopyCdProgressDialog *obj;

  obj = XFBURN_COPY_CD_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_COPY_CD_PROGRESS_DIALOG,
                                                      "title", _("Copy data CD"), NULL));

  return GTK_WIDGET (obj);
}

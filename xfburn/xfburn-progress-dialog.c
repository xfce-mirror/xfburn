/*
 *  Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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

#ifdef HAVE_STRINGS_H
#include <string.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-progress-dialog.h"
#include "xfburn-global.h"
#include "xfburn-utils.h"

static void xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass);
static void xfburn_progress_dialog_init (XfburnProgressDialog * sp);
static void xfburn_progress_dialog_finalize (GObject * object);

static void xfburn_progress_dialog_response_cb (XfburnProgressDialog * dialog, gint response_id,
                                                XfburnProgressDialogPrivate * priv);

struct XfburnProgressDialogPrivate
{
  XfburnDevice *device;
  gchar *command;

  int pid_command;
  int fd_stdout;
  int fd_stderr;

  guint id_refresh_fct;

  GtkWidget *progress_bar;
  GtkWidget *textview_output;

  GtkWidget *button_stop;
  GtkWidget *button_close;
};

static GtkDialogClass *parent_class = NULL;

GtkType
xfburn_progress_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnProgressDialog),
      0,
      (GInstanceInitFunc) xfburn_progress_dialog_init,
    };

    type = g_type_register_static (GTK_TYPE_DIALOG, "XfburnProgressDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_progress_dialog_finalize;

}

static void
xfburn_progress_dialog_init (XfburnProgressDialog * obj)
{
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  XfburnProgressDialogPrivate *priv;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *expander;
  GtkWidget *scrolled_window;
  GtkTextBuffer *textbuffer;

  obj->priv = g_new0 (XfburnProgressDialogPrivate, 1);
  priv = obj->priv;

  frame = xfce_framebox_new (_("Burning disk..."), FALSE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

  /* progress bar */
  priv->progress_bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), "0 %");
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (GTK_BOX (vbox), priv->progress_bar, FALSE, FALSE, BORDER);

  /* output */
  expander = gtk_expander_new_with_mnemonic (_("View _output"));
  gtk_widget_show (expander);
  gtk_box_pack_start (GTK_BOX (vbox), expander, TRUE, TRUE, 0);

#ifdef DEBUG
  gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
#endif

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_container_add (GTK_CONTAINER (expander), scrolled_window);

  priv->textview_output = gtk_text_view_new ();

  textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_output));
  gtk_text_buffer_create_tag (textbuffer, "error", "foreground", "red", NULL);

  gtk_widget_set_size_request (priv->textview_output, 350, 500);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (priv->textview_output), FALSE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->textview_output), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->textview_output), GTK_WRAP_WORD);
  gtk_widget_show (priv->textview_output);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->textview_output);

  /* action buttons */
  priv->button_stop = gtk_button_new_from_stock (GTK_STOCK_STOP);
  gtk_widget_show (priv->button_stop);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), priv->button_stop, GTK_RESPONSE_CANCEL);

  priv->button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (priv->button_close);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), priv->button_close, GTK_RESPONSE_CLOSE);
  GTK_WIDGET_SET_FLAGS (priv->button_close, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (priv->button_close);
  gtk_widget_grab_default (priv->button_close);
  gtk_widget_set_sensitive (priv->button_close, FALSE);

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (xfburn_progress_dialog_response_cb), priv);
}

/* internals */
static void
xfburn_progress_dialog_finalize (GObject * object)
{
  XfburnProgressDialog *cobj;
  cobj = XFBURN_PROGRESS_DIALOG (object);

  g_free (cobj->priv->command);

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_progress_dialog_append_output (XfburnProgressDialog * dialog, const gchar * output)
{
  XfburnProgressDialogPrivate *priv = dialog->priv;
  GtkTextBuffer *buffer;
  GtkTextIter iter;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_output));

  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, output, strlen (output));
  gtk_text_iter_set_line (&iter, gtk_text_buffer_get_line_count (buffer));
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->textview_output), &iter, 0.0, TRUE, 0.0, 0.0);
}

static void
xfburn_progress_dialog_append_error (XfburnProgressDialog * dialog, const gchar * output)
{
  XfburnProgressDialogPrivate *priv = dialog->priv;
  GtkTextBuffer *buffer;
  GtkTextIter iter;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_output));

  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, output, strlen (output), "error", NULL);
  gtk_text_iter_set_line (&iter, gtk_text_buffer_get_line_count (buffer));
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->textview_output), &iter, 0.0, TRUE, 0.0, 0.0);
}

static void
xfburn_progress_dialog_response_cb (XfburnProgressDialog * dialog, gint response_id, XfburnProgressDialogPrivate * priv)
{
  if (priv->id_refresh_fct > 0)
    g_source_remove (priv->id_refresh_fct);

  if (response_id == GTK_RESPONSE_CANCEL) {
    kill (priv->pid_command, SIGTERM);
  }
}
static void
hack_characters (gchar * buf)
{
  /* From GnomeBaker */
  const gint len = strlen (buf);
  gint i = 0;
  for (; i < len; i++) {
    if (!g_ascii_isalnum (buf[i]) && !g_ascii_iscntrl (buf[i])
        && !g_ascii_ispunct (buf[i]) && !g_ascii_isspace (buf[i])) {
      buf[i] = ' ';
    }
  }
}


static gboolean
xfburn_progress_dialog_update (XfburnProgressDialog * dialog)
{
  fd_set set;
  struct timeval tval;
  int ret;

  FD_ZERO (&set);
  FD_SET (dialog->priv->fd_stdout, &set);
  FD_SET (dialog->priv->fd_stderr, &set);
  tval.tv_sec = 2;
  tval.tv_usec = 0;
  
  ret = select (FD_SETSIZE, &set, NULL, NULL, &tval);

  if (ret > 0) {
    gchar buffer[1024];

    if ( (ret = read (dialog->priv->fd_stdout, buffer, sizeof (buffer))) > 0) {
      hack_characters (buffer);
      g_strstrip (buffer);
      xfburn_progress_dialog_append_output (dialog, buffer);
      xfburn_progress_dialog_append_output (dialog, "\n");
    }

    memset (buffer, 0, sizeof (buffer));
    if (read (dialog->priv->fd_stderr, buffer, sizeof (buffer)) > 0) {
      hack_characters (buffer);
      g_strstrip (buffer);
      xfburn_progress_dialog_append_error (dialog, buffer);
      xfburn_progress_dialog_append_output (dialog, "\n");
    }
    return TRUE;
  }
  return FALSE;
}

/* public */
void
xfburn_progress_dialog_start (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = dialog->priv;
  gboolean ready = FALSE;
  gchar *message = NULL;
  GtkWidget *dialog_message;
  gint ret;
  int argc;
  gchar **argvp;
  GError *error = NULL;
  DBG ("command : %s", priv->command);
  while (!ready) {
    switch (xfburn_device_query_cdstatus (priv->device)) {
    case CDS_NO_DISC:
      message = g_strdup (_("No disc in the cdrom drive"));
      break;
    case CDS_DISC_OK:
    default:
      ready = TRUE;
    }

    if (!ready) {
      dialog_message = gtk_message_dialog_new (GTK_WINDOW (dialog), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, message);
      ret = gtk_dialog_run (GTK_DIALOG (dialog_message));
      gtk_widget_destroy (dialog_message);
      g_free (message);
      if (ret == GTK_RESPONSE_CANCEL) {
        gtk_widget_set_sensitive (priv->button_stop, FALSE);
        gtk_widget_set_sensitive (priv->button_close, TRUE);
        return;
      }
    }
  }

  if (!g_shell_parse_argv (priv->command, &argc, &argvp, &error)) {
    g_warning ("Unable to parse command : %s", error->message);
    g_error_free (error);
    return;
  }

  if (!g_spawn_async_with_pipes (NULL, argvp, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &(priv->pid_command),
                                 NULL, &(priv->fd_stdout), &(priv->fd_stderr), &error)) {
    g_warning ("Unable to spawn process : %s", error->message);
    g_error_free (error);
    g_strfreev (argvp);
    return;
  }

  g_strfreev (argvp);

  priv->id_refresh_fct = g_timeout_add (500, (GSourceFunc) xfburn_progress_dialog_update, dialog);
}

GtkWidget *
xfburn_progress_dialog_new (XfburnDevice * device, const gchar * command)
{
  XfburnProgressDialog *obj;
  obj = XFBURN_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_PROGRESS_DIALOG, NULL));
  obj->priv->command = g_strdup (command);
  obj->priv->device = device;
  return GTK_WIDGET (obj);
}

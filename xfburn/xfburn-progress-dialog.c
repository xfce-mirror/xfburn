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

#define CDRECORD_OPC "Performing OPC..."

#define CDRECORD_COPY "Track"
#define CDRECORD_FIXATING "Fixating..."
#define CDRECORD_FIXATING_TIME "Fixating time:"

#define CDRECORD_BLANKING "Blanking "
#define CDRECORD_BLANKING_TIME "Blanking time:"

#define CDRECORD_CANNOT_BLANK "Cannot blank disk, aborting"
#define CDRECORD_INCOMPATIBLE_MEDIUM "cannot format medium - incmedium"


/* globals */
static void xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass);
static void xfburn_progress_dialog_init (XfburnProgressDialog * sp);
static void xfburn_progress_dialog_finalize (GObject * object);

static void xfburn_progress_dialog_label_set_text (XfburnProgressDialog * dialog, const gchar * text);

static void xfburn_progress_dialog_expander_activate_cb (GtkExpander * expander, XfburnProgressDialog * dialog);
static gboolean xfburn_progress_dialog_delete_cb (XfburnProgressDialog * dialog, GdkEvent * event,
                                                  XfburnProgressDialogPrivate * priv);
static void xfburn_progress_dialog_response_cb (XfburnProgressDialog * dialog, gint response_id,
                                                XfburnProgressDialogPrivate * priv);

/* structs */
typedef enum
{
  PROGRESS_STATUS_FAILED,
  PROGRESS_STATUS_CANCELLED,
  PROGRESS_STATUS_COMPLETED
} XfburnProgressDialogStatus;

struct XfburnProgressDialogPrivate
{
  XfburnProgressDialogType dialog_type;
  XfburnDevice *device;
  gchar *command;
  XfburnProgressDialogStatus status;

  int pid_command;
  GIOChannel *channel_stdout;
  GIOChannel *channel_stderr;

  guint id_refresh_stdout;
  guint id_refresh_stderr;
  guint id_pulse;

  GtkWidget *label;
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
  GtkWidget *expander;
  GtkWidget *scrolled_window;
  GtkTextBuffer *textbuffer;

  obj->priv = g_new0 (XfburnProgressDialogPrivate, 1);
  priv = obj->priv;

  gtk_window_set_default_size (GTK_WINDOW (obj), 575, 200);

  /* label */
  priv->label = gtk_label_new ("Initializing ...");
  gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->label), GTK_JUSTIFY_LEFT);
  xfburn_progress_dialog_label_set_text (obj, _("Initializing..."));
  gtk_widget_show (priv->label);
  gtk_box_pack_start (box, priv->label, FALSE, TRUE, BORDER);

  /* progress bar */
  priv->progress_bar = gtk_progress_bar_new ();
  switch (priv->dialog_type) {
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), " ");
    break;
  case XFBURN_PROGRESS_DIALOG_BURN_ISO:
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), "O%");
    break;
  }
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (box, priv->progress_bar, FALSE, FALSE, BORDER);

  /* output */
  expander = gtk_expander_new_with_mnemonic (_("View _output"));
  gtk_widget_show (expander);
  gtk_box_pack_start (box, expander, TRUE, TRUE, 0);
  g_signal_connect_after (G_OBJECT (expander), "activate", G_CALLBACK (xfburn_progress_dialog_expander_activate_cb),
                          obj);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_container_add (GTK_CONTAINER (expander), scrolled_window);

  priv->textview_output = gtk_text_view_new ();

  textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_output));
  gtk_text_buffer_create_tag (textbuffer, "error", "foreground", "red", NULL);

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
  g_signal_connect (G_OBJECT (obj), "delete-event", G_CALLBACK (xfburn_progress_dialog_delete_cb), priv);
}

/* internals */
static void
xfburn_progress_dialog_finalize (GObject * object)
{
  XfburnProgressDialog *cobj;
  XfburnProgressDialogPrivate *priv;

  cobj = XFBURN_PROGRESS_DIALOG (object);
  priv = cobj->priv;

  g_free (priv->command);

  if (priv->channel_stdout) {
    g_io_channel_shutdown (priv->channel_stdout, FALSE, NULL);
    g_io_channel_unref (priv->channel_stdout);
  }
  if (priv->channel_stderr) {
    g_io_channel_shutdown (priv->channel_stderr, FALSE, NULL);
    g_io_channel_unref (priv->channel_stderr);
  }

  g_spawn_close_pid (priv->pid_command);

  g_free (priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_progress_dialog_append_output (XfburnProgressDialog * dialog, const gchar * output, gboolean is_error)
{
  XfburnProgressDialogPrivate *priv = dialog->priv;
  GtkTextBuffer *buffer;
  GtkTextIter iter;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_output));

  gtk_text_buffer_get_end_iter (buffer, &iter);
  if (is_error)
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, output, strlen (output), "error", NULL);
  else
    gtk_text_buffer_insert (buffer, &iter, output, strlen (output));
  gtk_text_iter_set_line (&iter, gtk_text_buffer_get_line_count (buffer));
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->textview_output), &iter, 0.0, TRUE, 0.0, 0.0);
}

static void
xfburn_progress_dialog_expander_activate_cb (GtkExpander * expander, XfburnProgressDialog * dialog)
{
  // TODO
}

static gboolean
xfburn_progress_dialog_delete_cb (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv)
{
  DBG ("test");
  if (!GTK_WIDGET_SENSITIVE (priv->button_close)) {
    xfburn_progress_dialog_response_cb (dialog, GTK_RESPONSE_CANCEL, priv);
    return TRUE;
  }

  return FALSE;
}

static void
xfburn_progress_dialog_response_cb (XfburnProgressDialog * dialog, gint response_id, XfburnProgressDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_CANCEL) {
    if (priv->pid_command > 0)
      kill (priv->pid_command, SIGTERM);

    gtk_widget_set_sensitive (priv->button_stop, FALSE);
    priv->status = PROGRESS_STATUS_CANCELLED;
  }
  else
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static gboolean
xfburn_progress_dialog_update_stdout (GIOChannel * source, GIOCondition condition, XfburnProgressDialog * dialog)
{
  gchar buffer[1024] = "";
  gchar *converted_buffer = NULL;
  gsize bytes_read = 0, bytes_written = 0;
  GError *error = NULL;
  GIOStatus status;

  if (condition == G_IO_HUP || condition == G_IO_ERR) {
    gtk_widget_set_sensitive (dialog->priv->button_close, TRUE);
    gtk_widget_set_sensitive (dialog->priv->button_stop, FALSE);

    if (dialog->priv->id_refresh_stdout > 0)
      g_source_remove (dialog->priv->id_refresh_stdout);

    if (dialog->priv->id_refresh_stderr > 0)
      g_source_remove (dialog->priv->id_refresh_stderr);

    if (dialog->priv->id_pulse > 0) {
      g_source_remove (dialog->priv->id_pulse);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), 1.0);
    }

    xfburn_progress_dialog_label_set_text (dialog, _("Operation finished"));

    switch (dialog->priv->status) {
    case PROGRESS_STATUS_CANCELLED:
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->progress_bar), _("Cancelled"));
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), 1.0);
      break;
    case PROGRESS_STATUS_COMPLETED:
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->progress_bar), _("Completed"));
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), 1.0);
    
      switch (dialog->priv->dialog_type) {
      case XFBURN_PROGRESS_DIALOG_BLANK_CD:
        xfce_info (_("Blanking process exited with success"));
        break;
      case XFBURN_PROGRESS_DIALOG_BURN_ISO:
        xfce_info (_("Burning process exited with success"));
        break;
      }
      break;
    case PROGRESS_STATUS_FAILED:
    default:
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->progress_bar), _("Failed"));
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), 1.0);
    
      switch (dialog->priv->dialog_type) {
      case XFBURN_PROGRESS_DIALOG_BLANK_CD:
        xfce_err (_("An error occured while trying to blank the disc (see output for more details)"));
        break;
      case XFBURN_PROGRESS_DIALOG_BURN_ISO:
        xfce_err (_("An error occured while trying to burn the disc (see output for more details)"));
        break;
      }
    }

    return FALSE;
  }

  if (!(status = g_io_channel_read_chars (source, buffer, sizeof (buffer) - 1, &bytes_read, &error))) {
    g_warning ("Error while reading from pipe : %s", error->message);
    g_error_free (error);
  }

  buffer[bytes_read] = '\0';

  if (!(converted_buffer = g_convert (buffer, bytes_read, "UTF-8", "ISO-8859-1", &bytes_read, &bytes_written, &error))) {
    g_warning ("Conversion error : %s", error->message);
    g_error_free (error);
    return TRUE;
  }

  xfburn_progress_dialog_append_output (dialog, converted_buffer, (source == dialog->priv->channel_stderr));

  switch (dialog->priv->dialog_type) {
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
    if (strstr (converted_buffer, CDRECORD_BLANKING_TIME)) {
      dialog->priv->status = PROGRESS_STATUS_COMPLETED;
    }
    else if (strstr (converted_buffer, CDRECORD_BLANKING)) {
      xfburn_progress_dialog_label_set_text (dialog, _("Blanking..."));
    }
    else if (strstr (converted_buffer, CDRECORD_OPC)) {
      xfburn_progress_dialog_label_set_text (dialog, _("Performing OPC..."));
    }
    break;
  case XFBURN_PROGRESS_DIALOG_BURN_ISO:
    if (strstr (converted_buffer, CDRECORD_FIXATING_TIME)) {
      dialog->priv->status = PROGRESS_STATUS_COMPLETED;
    }
    else if (strstr (converted_buffer, CDRECORD_FIXATING)) {
      xfburn_progress_dialog_label_set_text (dialog, _("Fixating..."));
    }
    else if (strstr (converted_buffer, CDRECORD_COPY)) {
      gfloat current = 0, total = 0;

      if (sscanf (converted_buffer, "%*s %*d %*s %f %*s %f", &current, &total) == 2 && total > 0) {
        gdouble fraction;
        gchar *text;

        fraction = (gdouble) (current / total);
        if (fraction < 0.0)
          fraction = 0.0;
        else if (fraction > 1.0)
          fraction = 1.0;

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), fraction);
        text = g_strdup_printf ("%d%%", (int) (fraction * 100));
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->progress_bar), text);
        g_free (text);
        xfburn_progress_dialog_label_set_text (dialog, _("Writing ISO..."));
      }
    }
    break;
  }

  g_free (converted_buffer);
  return TRUE;
}

static void
xfburn_progress_dialog_label_set_text (XfburnProgressDialog * dialog, const gchar * text)
{
  gchar *temp;

  temp = g_strdup_printf ("<b>%s</b>", text);
  gtk_label_set_markup (GTK_LABEL (dialog->priv->label), temp);

  g_free (temp);
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
  int fd_stdout;
  int fd_stderr;
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
      xfburn_progress_dialog_append_output (dialog, message, TRUE);
      xfburn_progress_dialog_append_output (dialog, "\n", FALSE);
      dialog_message = gtk_message_dialog_new (GTK_WINDOW (dialog), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, message);
      ret = gtk_dialog_run (GTK_DIALOG (dialog_message));
      gtk_widget_destroy (dialog_message);
      g_free (message);
      if (ret == GTK_RESPONSE_CANCEL) {
        gtk_widget_set_sensitive (priv->button_stop, FALSE);
        gtk_widget_set_sensitive (priv->button_close, TRUE);
        priv->status = PROGRESS_STATUS_CANCELLED;
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), _("Cancelled"));
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), 1.0);
        xfburn_progress_dialog_label_set_text (dialog, _("Operation finished"));
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
                                 NULL, &fd_stdout, &fd_stderr, &error)) {
    g_warning ("Unable to spawn process : %s", error->message);
    g_error_free (error);
    g_strfreev (argvp);
    return;
  }
  g_strfreev (argvp);

  priv->channel_stdout = g_io_channel_unix_new (fd_stdout);
  g_io_channel_set_encoding (priv->channel_stdout, NULL, NULL);
  g_io_channel_set_buffered (priv->channel_stdout, FALSE);
  g_io_channel_set_flags (priv->channel_stdout, G_IO_FLAG_NONBLOCK, NULL);

  priv->channel_stderr = g_io_channel_unix_new (fd_stderr);
  g_io_channel_set_encoding (priv->channel_stderr, NULL, NULL);
  g_io_channel_set_buffered (priv->channel_stderr, FALSE);
  g_io_channel_set_flags (priv->channel_stderr, G_IO_FLAG_NONBLOCK, NULL);

  priv->id_refresh_stdout = g_io_add_watch (priv->channel_stdout, G_IO_IN | G_IO_HUP | G_IO_ERR,
                                            (GIOFunc) xfburn_progress_dialog_update_stdout, dialog);
  priv->id_refresh_stderr = g_io_add_watch (priv->channel_stderr, G_IO_IN | G_IO_HUP | G_IO_ERR,
                                            (GIOFunc) xfburn_progress_dialog_update_stdout, dialog);

  switch (priv->dialog_type) {
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
    gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.05);
    priv->id_pulse = g_timeout_add (250, (GSourceFunc) gtk_progress_bar_pulse, GTK_PROGRESS_BAR (priv->progress_bar));
    break;
  case XFBURN_PROGRESS_DIALOG_BURN_ISO:
    break;
  }

}

GtkWidget *
xfburn_progress_dialog_new (XfburnProgressDialogType type, XfburnDevice * device, const gchar * command)
{
  XfburnProgressDialog *obj;
  XfburnProgressDialogPrivate *priv;

  obj = XFBURN_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_PROGRESS_DIALOG, NULL));
  priv = obj->priv;
  priv->command = g_strdup (command);
  priv->device = device;
  priv->dialog_type = type;

  return GTK_WIDGET (obj);
}

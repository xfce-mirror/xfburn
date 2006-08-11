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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-progress-dialog.h"
#include "xfburn-global.h"

#define XFBURN_PROGRESS_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_PROGRESS_DIALOG, XfburnProgressDialogPrivate))

/* struct */
typedef struct
{
  XfburnProgressDialogStatus status;
  int fd_stdin;
  
  GtkWidget *label_action;
  GtkWidget *progress_bar;
  GtkWidget *hbox_buffers;
  GtkWidget *label_speed;
  GtkWidget *fifo_bar;
  GtkWidget *buffer_bar;
  GtkWidget *textview_output;

  GtkWidget *button_stop;
  GtkWidget *button_close;
} XfburnProgressDialogPrivate;

/* globals */
static void xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass);
static void xfburn_progress_dialog_init (XfburnProgressDialog * sp);

static void xfburn_progress_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void xfburn_progress_dialog_set_property (GObject * object, guint prop_id, const GValue * value,
                                                 GParamSpec * pspec);
static void xfburn_progress_dialog_finalize (GObject * object);

static void cb_expander_activate (GtkExpander * expander, XfburnProgressDialog * dialog);
static void cb_dialog_show (XfburnProgressDialog * dialog, XfburnProgressDialogPrivate * priv);
static gboolean cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv);
static void cb_dialog_response (XfburnProgressDialog * dialog, gint response_id, XfburnProgressDialogPrivate * priv);

enum
{
  PROP_0,
  PROP_STATUS,
  PROP_SHOW_BUFFERS,
};

enum
{
  OUTPUT,
  FINISHED,
  LAST_SIGNAL,
};
static guint signals[LAST_SIGNAL];

/*                                    */
/* enumeration type for dialog status */
/*                                    */
GType
xfburn_progress_dialog_status_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GEnumValue values[] = {
      {XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, "XFBURN_PROGRESS_DIALOG_STATUS_RUNNING", "running"},
      {XFBURN_PROGRESS_DIALOG_STATUS_FAILED, "XFBURN_PROGRESS_DIALOG_STATUS_FAILED", "failed"},
      {XFBURN_PROGRESS_DIALOG_STATUS_CANCELLED, "XFBURN_PROGRESS_DIALOG_STATUS_CANCELLED", "cancelled"},
      {XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED, "XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED", "completed"},
      {0, NULL, NULL}
    };
    type = g_enum_register_static ("XfburnProgressDialogStatus", values);
  }
  return type;
}

/*                       */
/* progress dialog class */
/*                       */
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

  g_type_class_add_private (klass, sizeof (XfburnProgressDialogPrivate));
  
  object_class->finalize = xfburn_progress_dialog_finalize;
  object_class->get_property = xfburn_progress_dialog_get_property;
  object_class->set_property = xfburn_progress_dialog_set_property;

  /* properties */
  g_object_class_install_property (object_class, PROP_STATUS,
                                   g_param_spec_enum ("status", "Status", "Status", XFBURN_TYPE_PROGRESS_DIALOG_STATUS,
                                                      XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SHOW_BUFFERS,
                                   g_param_spec_boolean ("show-buffers", "Show buffers", "Show buffers",
                                                         TRUE, G_PARAM_READWRITE));
  /* signals */
  signals[OUTPUT] = g_signal_new ("output", G_TYPE_FROM_CLASS (object_class), G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (XfburnProgressDialogClass, output),
                                  NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
  signals[FINISHED] = g_signal_new ("finished", G_TYPE_FROM_CLASS (object_class), G_SIGNAL_ACTION,
                                    G_STRUCT_OFFSET (XfburnProgressDialogClass, finished),
                                    NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
xfburn_progress_dialog_init (XfburnProgressDialog * obj)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (obj);
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  GtkWidget *expander;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scrolled_window;

  gtk_window_set_default_size (GTK_WINDOW (obj), 575, 200);

  /* label */
  priv->label_action = gtk_label_new ("Initializing ...");
  gtk_misc_set_alignment (GTK_MISC (priv->label_action), 0.0, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->label_action), GTK_JUSTIFY_LEFT);
  xfburn_progress_dialog_set_action_text (obj, _("Initializing..."));
  gtk_widget_show (priv->label_action);
  gtk_box_pack_start (box, priv->label_action, FALSE, TRUE, BORDER);

  /* progress bar */
  priv->progress_bar = gtk_progress_bar_new ();
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (box, priv->progress_bar, FALSE, FALSE, BORDER);
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.05);

  /* buffers */
  priv->hbox_buffers = gtk_hbox_new (FALSE, BORDER);
  gtk_widget_show (priv->hbox_buffers);
  gtk_box_pack_start (box, priv->hbox_buffers, FALSE, TRUE, BORDER);

  frame = gtk_frame_new (_("Estimated writing speed:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (priv->hbox_buffers), frame, FALSE, FALSE, 0);
  priv->label_speed = gtk_label_new (_("no info"));
  xfburn_progress_dialog_set_writing_speed (obj, -1);
  gtk_widget_show (priv->label_speed);
  gtk_container_add (GTK_CONTAINER (frame), priv->label_speed);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), BORDER);
  gtk_table_set_col_spacings (GTK_TABLE (table), BORDER * 2);
  gtk_box_pack_start (GTK_BOX (priv->hbox_buffers), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  label = gtk_label_new (_("FIFO buffer:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  priv->fifo_bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->fifo_bar), _("no info"));
  gtk_table_attach (GTK_TABLE (table), priv->fifo_bar, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (priv->fifo_bar);

  label = gtk_label_new (_("Device buffer:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  priv->buffer_bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->buffer_bar), _("no info"));
  gtk_table_attach (GTK_TABLE (table), priv->buffer_bar, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (priv->buffer_bar);

  /* output */
  expander = gtk_expander_new_with_mnemonic (_("View _output"));
  gtk_widget_show (expander);
  gtk_box_pack_start (box, expander, TRUE, TRUE, 0);
  g_signal_connect_after (G_OBJECT (expander), "activate", G_CALLBACK (cb_expander_activate), obj);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_container_add (GTK_CONTAINER (expander), scrolled_window);

  priv->textview_output = gtk_text_view_new ();
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

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), priv);
  g_signal_connect (G_OBJECT (obj), "delete-event", G_CALLBACK (cb_dialog_delete), priv);
  g_signal_connect_after (G_OBJECT (obj), "show", G_CALLBACK (cb_dialog_show), priv);
}

static void
xfburn_progress_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (object);
  gboolean show_buffers = TRUE;

  switch (prop_id) {
  case PROP_STATUS:
    g_value_set_enum (value, priv->status);
    break;
  case PROP_SHOW_BUFFERS:
    g_object_get (G_OBJECT (priv->hbox_buffers), "visible", &show_buffers, NULL);
    g_value_set_boolean (value, show_buffers);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
xfburn_progress_dialog_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  XfburnProgressDialog *dialog = XFBURN_PROGRESS_DIALOG (object);

  switch (prop_id) {
  case PROP_STATUS:
    xfburn_progress_dialog_set_status (dialog, g_value_get_enum (value));
    break;
  case PROP_SHOW_BUFFERS:
    xfburn_progress_dialog_show_buffers (dialog, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
xfburn_progress_dialog_finalize (GObject * object)
{
  GIOChannel *channel_stdout;
  GIOChannel *channel_stderr;
  GPid pid_command;

  if ((channel_stdout = g_object_get_data (object, "channel-stdout"))) {
    g_io_channel_shutdown (channel_stdout, FALSE, NULL);
    g_io_channel_unref (channel_stdout);
  }
  if ((channel_stderr = g_object_get_data (object, "channel-stderr"))) {
    g_io_channel_shutdown (channel_stderr, FALSE, NULL);
    g_io_channel_unref (channel_stderr);
  }
  if ((pid_command = (GPid) g_object_get_data (object, "pid-command")))
    g_spawn_close_pid (pid_command);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*           */
/* internals */
/*           */
static gboolean
watch_output (GIOChannel * source, GIOCondition condition, XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  gchar buffer[1024] = "";
  gchar *converted_buffer = NULL;
  gsize bytes_read = 0, bytes_written = 0;
  GError *error = NULL;
  GIOStatus status;

  if (condition == G_IO_HUP || condition == G_IO_ERR) {
    guint id_refresh_stdout = -1;
    guint id_refresh_stderr = -1;

    gtk_widget_set_sensitive (priv->button_close, TRUE);
    gtk_widget_set_sensitive (priv->button_stop, FALSE);

    id_refresh_stdout = (guint) g_object_get_data (G_OBJECT (dialog), "id-refresh-stdout");
    id_refresh_stderr = (guint) g_object_get_data (G_OBJECT (dialog), "id-refresh-stderr");

    if (id_refresh_stdout > 0)
      g_source_remove (id_refresh_stdout);

    if (id_refresh_stderr > 0)
      g_source_remove (id_refresh_stderr);

    xfburn_progress_dialog_set_progress_bar_fraction (dialog, 1.0);
    xfburn_progress_dialog_set_action_text (dialog, _("Operation finished"));

    g_signal_emit (G_OBJECT (dialog), signals[FINISHED], 0);
    return FALSE;
  }

  if (!(status = g_io_channel_read_chars (source, buffer, sizeof (buffer) - 1, &bytes_read, &error))) {
    g_warning ("Error while reading from pipe : %s", error->message);
    g_error_free (error);
  }

  buffer[bytes_read] = '\0';

  /* Some distro (at least Gentoo) force the cdrecord output to be in UTF8 if the environment */
  /* is in unicode, we check that to avoid a wrong conversion */
  if (!g_utf8_validate (buffer, -1, NULL)) {
    if (!(converted_buffer =
          g_convert (buffer, bytes_read, "UTF-8", "ISO-8859-1", &bytes_read, &bytes_written, &error))) {
      g_warning ("Conversion error : %s", error->message);
      g_error_free (error);
      return TRUE;
    }
  }
  else
    converted_buffer = g_strdup (buffer);

  xfburn_progress_dialog_append_output (dialog, converted_buffer);

  g_signal_emit (G_OBJECT (dialog), signals[OUTPUT], 0, converted_buffer);
  g_free (converted_buffer);
  return TRUE;
}

/* callbacks */
static void
cb_expander_activate (GtkExpander * expander, XfburnProgressDialog * dialog)
{
  // TODO
}

static void
cb_dialog_show (XfburnProgressDialog * dialog, XfburnProgressDialogPrivate * priv)
{
  gchar *command;
  int argc;
  gchar **argvp;
  int fd_stdout;
  int fd_stderr;
  GError *error = NULL;
  GPid pid_command;
  GIOChannel *channel_stdout;
  GIOChannel *channel_stderr;
  guint id_refresh_stdout = -1;
  guint id_refresh_stderr = -1;

  command = g_object_get_data (G_OBJECT (dialog), "command");

  if (!command)
    return;

  DBG ("command used : %s", command);

  if (!g_shell_parse_argv (command, &argc, &argvp, &error)) {
    g_warning ("Unable to parse command : %s", error->message);
    g_error_free (error);
    return;
  }

  if (!g_spawn_async_with_pipes (NULL, argvp, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid_command,
                                 &(priv->fd_stdin), &fd_stdout, &fd_stderr, &error)) {
    g_warning ("Unable to spawn process : %s", error->message);
    g_error_free (error);
    g_strfreev (argvp);
    return;
  }
  g_strfreev (argvp);

  g_object_set_data (G_OBJECT (dialog), "pid-command", (gpointer) pid_command);
  
  channel_stdout = g_io_channel_unix_new (fd_stdout);
  g_io_channel_set_encoding (channel_stdout, NULL, NULL);
  g_io_channel_set_buffered (channel_stdout, FALSE);
  g_io_channel_set_flags (channel_stdout, G_IO_FLAG_NONBLOCK, NULL);
  g_object_set_data (G_OBJECT (dialog), "channel-stdout", channel_stdout);

  channel_stderr = g_io_channel_unix_new (fd_stderr);
  g_io_channel_set_encoding (channel_stderr, NULL, NULL);
  g_io_channel_set_buffered (channel_stderr, FALSE);
  g_io_channel_set_flags (channel_stderr, G_IO_FLAG_NONBLOCK, NULL);
  g_object_set_data (G_OBJECT (dialog), "channel-stderr", channel_stderr);

  id_refresh_stdout = g_io_add_watch (channel_stdout, G_IO_IN | G_IO_HUP | G_IO_ERR, (GIOFunc) watch_output, dialog);
  g_object_set_data (G_OBJECT (dialog), "id-refresh-stdout", (gpointer) id_refresh_stdout);
  id_refresh_stderr = g_io_add_watch (channel_stderr, G_IO_IN | G_IO_HUP | G_IO_ERR, (GIOFunc) watch_output, dialog);
  g_object_set_data (G_OBJECT (dialog), "id-refresh-stderr", (gpointer) id_refresh_stderr);
}

static gboolean
cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv)
{
  if (!GTK_WIDGET_SENSITIVE (priv->button_close)) {
    cb_dialog_response (dialog, GTK_RESPONSE_CANCEL, priv);
    return TRUE;
  } else {
    cb_dialog_response (dialog, GTK_RESPONSE_CLOSE, priv);
    return FALSE;
  }
}

static void
cb_dialog_response (XfburnProgressDialog * dialog, gint response_id, XfburnProgressDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_CANCEL) {
    GPid pid_command;

    pid_command = (GPid) g_object_get_data (G_OBJECT (dialog), "pid-command");
    if (pid_command > 0)
      kill (pid_command, SIGTERM);
    
    gtk_widget_set_sensitive (priv->button_stop, FALSE);
    priv->status = XFBURN_PROGRESS_DIALOG_STATUS_CANCELLED;
  } else if (response_id == GTK_RESPONSE_CLOSE)
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

/*        */
/* public */
/*        */

void
xfburn_progress_dialog_show_buffers (XfburnProgressDialog * dialog, gboolean show)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  if (show)
    gtk_widget_show (priv->hbox_buffers);
  else
    gtk_widget_hide (priv->hbox_buffers);
}

void
xfburn_progress_dialog_append_output (XfburnProgressDialog * dialog, const gchar * output)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  GtkTextBuffer *buffer;
  GtkTextIter iter;

  if (!output)
    return;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_output));

  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert (buffer, &iter, output, strlen (output));
  gtk_text_iter_set_line (&iter, gtk_text_buffer_get_line_count (buffer));
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->textview_output), &iter, 0.0, TRUE, 0.0, 0.0);
}

void
xfburn_progress_dialog_pulse_progress_bar (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));
}

void
xfburn_progress_dialog_write_input (XfburnProgressDialog * dialog, const gchar * input)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  write (priv->fd_stdin, input, strlen (input) * sizeof (gchar));
}

/* getters */
gdouble
xfburn_progress_dialog_get_progress_bar_fraction (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  return gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (priv->progress_bar));
}

XfburnProgressDialogStatus
xfburn_progress_dialog_get_status (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  return priv->status;
}

/* setters */
void
xfburn_progress_dialog_set_action_text (XfburnProgressDialog * dialog, const gchar * text)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *temp;

  temp = g_strdup_printf ("<b>%s</b>", text);
  gtk_label_set_markup (GTK_LABEL (priv->label_action), temp);

  g_free (temp);
}

void
xfburn_progress_dialog_set_writing_speed (XfburnProgressDialog * dialog, gfloat speed)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *temp;

  if (speed < 0)
    temp = g_strdup_printf ("<b><i>%s</i></b>", _("no info"));
  else
    temp = g_strdup_printf ("<b><i>%.1f x</i></b>", speed);

  gtk_label_set_markup (GTK_LABEL (priv->label_speed), temp);

  g_free (temp);
}

void
xfburn_progress_dialog_set_buffer_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *text = NULL;

  if (fraction > 1.0) {
    fraction = 1.0;
    text = g_strdup ("100%");
  } else if (fraction < 0.0) {
    fraction = 0.0;
    text = g_strdup (_("no info"));
  } else {
    text = g_strdup_printf ("%d%%", (int) (fraction * 100));
  }
  
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->buffer_bar), fraction);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->buffer_bar), text);
  g_free (text);
}

void
xfburn_progress_dialog_set_fifo_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *text = NULL;

  if (fraction > 1.0) {
    fraction = 1.0;
    text = g_strdup ("100%");
  } else if (fraction < 0.0) {
    fraction = 0.0;
    text = g_strdup (_("no info"));
  } else {
    text = g_strdup_printf ("%d%%", (int) (fraction * 100));
  }
  
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->fifo_bar), fraction);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->fifo_bar), text);
  g_free (text);
}

void
xfburn_progress_dialog_set_progress_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *text = NULL;

  if (fraction >= 1.0) {
    fraction = 1.0;
    switch (priv->status) {
    case XFBURN_PROGRESS_DIALOG_STATUS_RUNNING:
      text = g_strdup ("100%");
      break;
    case XFBURN_PROGRESS_DIALOG_STATUS_FAILED:
      text = g_strdup (_("Failed"));
      break;
    case XFBURN_PROGRESS_DIALOG_STATUS_CANCELLED:
      text = g_strdup (_("Cancelled"));
      break;
    case XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED:
      text = g_strdup (_("Completed"));
      break;
    }
  }
  else if (fraction < 0.0) {
    fraction = 0.0;
    text = g_strdup ("0%");
  }
  else if (priv->status == XFBURN_PROGRESS_DIALOG_STATUS_RUNNING) {
    text = g_strdup_printf ("%d%%", (int) (fraction * 100));
  }
    
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), fraction);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
  g_free (text);
}

void
xfburn_progress_dialog_set_status (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  priv->status = status;
}

/* constructor */
GtkWidget *
xfburn_progress_dialog_new ()
{
  XfburnProgressDialog *obj;
  obj = XFBURN_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_PROGRESS_DIALOG, NULL));
    
  return GTK_WIDGET (obj);
}

/* $Id$ */
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
#include "xfburn-utils.h"

#define CDRECORD_OPC "Performing OPC..."

#define CDRECORD_COPY "Track"
#define CDRECORD_AVERAGE_WRITE_SPEED "Average write speed"
#define CDRECORD_FIXATING "Fixating..."
#define CDRECORD_FIXATING_TIME "Fixating time:"

#define CDRECORD_BLANKING "Blanking "
#define CDRECORD_BLANKING_TIME "Blanking time:"

#define CDRECORD_CANNOT_BLANK "Cannot blank disk, aborting"
#define CDRECORD_INCOMPATIBLE_MEDIUM "cannot format medium - incmedium"

#define READCD_CAPACITY "end:"
#define READCD_PROGRESS "addr:"
#define READCD_DONE "Time total:"

#define CDRDAO_LENGTH "length"
#define CDRDAO_FLUSHING "Flushing cache..."
#define CDRDAO_INSERT "Please insert a recordable medium and hit enter"
#define CDRDAO_INSERT_AGAIN "Cannot determine disk status - hit enter to try again."
#define CDRDAO_DONE "CD copying finished successfully."

/* globals */
static void xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass);
static void xfburn_progress_dialog_init (XfburnProgressDialog * sp);
static void xfburn_progress_dialog_finalize (GObject * object);

static void xfburn_progress_dialog_label_action_set_text (XfburnProgressDialog * dialog, const gchar * text);
static void xfburn_progress_dialog_label_speed_set_text (XfburnProgressDialog * dialog, const gchar * text);

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
  XfburnDevice *device_burn;
  XfburnDevice *device_read;

  gchar *command;
  gchar *command2;
  XfburnProgressDialogStatus status;

  int pid_command;
  int fd_stdin;
  GIOChannel *channel_stdout;
  GIOChannel *channel_stderr;

  guint id_refresh_stdout;
  guint id_refresh_stderr;
  guint id_pulse;

  GtkWidget *label_action;
  GtkWidget *progress_bar;
  GtkWidget *label_speed;
  GtkWidget *fifo_bar;
  GtkWidget *buffer_bar;
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
  obj->priv = g_new0 (XfburnProgressDialogPrivate, 1);
}

/* internals */
static void
xfburn_progress_dialog_create (XfburnProgressDialog * obj)
{
  XfburnProgressDialogPrivate *priv;
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  GtkWidget *expander;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scrolled_window;
  GtkTextBuffer *textbuffer;

  priv = obj->priv;

  gtk_window_set_default_size (GTK_WINDOW (obj), 575, 200);

  /* label */
  priv->label_action = gtk_label_new ("Initializing ...");
  gtk_misc_set_alignment (GTK_MISC (priv->label_action), 0.0, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->label_action), GTK_JUSTIFY_LEFT);
  xfburn_progress_dialog_label_action_set_text (obj, _("Initializing..."));
  gtk_widget_show (priv->label_action);
  gtk_box_pack_start (box, priv->label_action, FALSE, TRUE, BORDER);

  /* progress bar */
  priv->progress_bar = gtk_progress_bar_new ();
  switch (priv->dialog_type) {
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), " ");
    break;
  case XFBURN_PROGRESS_DIALOG_BURN_ISO:
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), "0%");
    break;
  case XFBURN_PROGRESS_DIALOG_COPY_CD:
    break;
  }
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (box, priv->progress_bar, FALSE, FALSE, BORDER);

  hbox = gtk_hbox_new (FALSE, BORDER);

  frame = gtk_frame_new (_("Estimated writing speed:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  priv->label_speed = gtk_label_new (_("no info"));
  xfburn_progress_dialog_label_speed_set_text (obj, _("no info"));
  gtk_widget_show (priv->label_speed);
  gtk_container_add (GTK_CONTAINER (frame), priv->label_speed);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), BORDER);
  gtk_table_set_col_spacings (GTK_TABLE (table), BORDER * 2);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
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


  if (priv->dialog_type != XFBURN_PROGRESS_DIALOG_BLANK_CD &&
      (priv->dialog_type == XFBURN_PROGRESS_DIALOG_COPY_CD && priv->device_burn)) {
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, TRUE, TRUE, BORDER);
  }

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

static void
xfburn_progress_dialog_finalize (GObject * object)
{
  XfburnProgressDialog *cobj;
  XfburnProgressDialogPrivate *priv;

  cobj = XFBURN_PROGRESS_DIALOG (object);
  priv = cobj->priv;

  g_free (priv->command);
  g_free (priv->command2);

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

  if (!output)
    return;

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
  static gint readcd_end = -1;
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

    xfburn_progress_dialog_label_action_set_text (dialog, _("Operation finished"));

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
      case XFBURN_PROGRESS_DIALOG_COPY_CD:
        xfce_info (_("Copying process exited with success"));
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
      case XFBURN_PROGRESS_DIALOG_COPY_CD:
        xfce_err (_("An error occured while trying to copy the disc (see output for more details)"));
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

  xfburn_progress_dialog_append_output (dialog, converted_buffer, (source == dialog->priv->channel_stderr));

  switch (dialog->priv->dialog_type) {
  case XFBURN_PROGRESS_DIALOG_COPY_CD:
    if (!dialog->priv->device_burn) {
      /* only create iso is set */
      if (strstr (converted_buffer, READCD_DONE)) {
        dialog->priv->status = PROGRESS_STATUS_COMPLETED;
      }
      else if (strstr (converted_buffer, READCD_PROGRESS)) {
        gint readcd_done = -1;
        gdouble fraction;
        gchar *text;

        sscanf (converted_buffer, "%*s %d", &readcd_done);

        fraction = (gdouble) ((gfloat) readcd_done / readcd_end);
        if (fraction < 0.0)
          fraction = 0.0;
        else if (fraction > 1.0)
          fraction = 1.0;

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), fraction);
        text = g_strdup_printf ("%d%%", (int) (fraction * 100));
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->progress_bar), text);
        g_free (text);
      }
      else if (strstr (converted_buffer, READCD_CAPACITY)) {
        xfburn_progress_dialog_label_action_set_text (dialog, _("Reading CD..."));
        sscanf (converted_buffer, "%*s %d", &readcd_end);
      }
    } else {
      if (strstr (converted_buffer, CDRDAO_DONE)) {
        dialog->priv->status = PROGRESS_STATUS_COMPLETED;
      }
      else if (strstr (converted_buffer, CDRDAO_FLUSHING)) {
        xfburn_progress_dialog_label_action_set_text (dialog, _("Flushing cache..."));
      }
      else if (strstr (converted_buffer, CDRDAO_LENGTH)) {
        gint min, sec, cent;
          
        sscanf (converted_buffer, "%*s %d:%d:%d", &min, &sec, &cent);
        readcd_end = cent + 100*sec + 60*100*min;
      }
      else if (strstr (converted_buffer, CDRDAO_INSERT) ||
               strstr (converted_buffer, CDRDAO_INSERT_AGAIN)) {
        GtkWidget *dialog_confirm;
        
        dialog_confirm = gtk_message_dialog_new (GTK_WINDOW (dialog), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
                                                 _("Please insert a recordable medium in %s."), dialog->priv->device_burn->name);
        gtk_dialog_run (GTK_DIALOG (dialog_confirm));
        gtk_widget_destroy (dialog_confirm);
        write (dialog->priv->fd_stdin, "\n", strlen ("\n") * sizeof (char));
      }
      else {
        gint done, total, buffer1, buffer2;
        gint min, sec, cent;
        gdouble fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar));
        gchar *text;
        
        if (sscanf (converted_buffer, "Wrote %d %*s %d %*s %*s %d%% %d%%", &done, &total, &buffer1, &buffer2) == 4) {
          gdouble cur_fraction;
          static gboolean onthefly = FALSE;
                    
          if ( onthefly || (cur_fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar))) <= 0.0) {
            fraction = (gdouble) ((gfloat) done / total);
          } else {
            onthefly = TRUE;
            fraction = (gdouble) ((gfloat) done / total);
            fraction = cur_fraction + (fraction / 2);
          }
          
          xfburn_progress_dialog_label_action_set_text (dialog, _("Writing CD..."));
          
          gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->fifo_bar), (gdouble) ((gfloat) buffer1 / 100));
          text = g_strdup_printf ("%d%%", buffer1);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->fifo_bar), text);
          g_free (text);
          
          gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->buffer_bar), (gdouble) ((gfloat) buffer2 / 100));
          text = g_strdup_printf ("%d%%", buffer2);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->buffer_bar), text);
          g_free (text);
          
        } else if (sscanf (converted_buffer, "%d:%d:%d", &min, &sec, &cent) == 3) {
          gint readcd_done = -1;
          
          readcd_done = cent + 100*sec + 60*100*min;
          
          fraction = (gdouble) ((gfloat) readcd_done / readcd_end);
          fraction = fraction / 2;
          
          xfburn_progress_dialog_label_action_set_text (dialog, _("Reading CD..."));
        }
               
        if (fraction < 0.0)
          fraction = 0.0;
        else if (fraction > 1.0)
          fraction = 1.0;

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), fraction);
        text = g_strdup_printf ("%d%%", (int) (fraction * 100));
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->progress_bar), text);
        g_free (text);
      }
    }
    break;
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
    if (strstr (converted_buffer, CDRECORD_BLANKING_TIME)) {
      dialog->priv->status = PROGRESS_STATUS_COMPLETED;
    }
    else if (strstr (converted_buffer, CDRECORD_BLANKING)) {
      xfburn_progress_dialog_label_action_set_text (dialog, _("Blanking..."));
    }
    else if (strstr (converted_buffer, CDRECORD_OPC)) {
      xfburn_progress_dialog_label_action_set_text (dialog, _("Performing OPC..."));
    }
    break;
  case XFBURN_PROGRESS_DIALOG_BURN_ISO:
    if (strstr (converted_buffer, CDRECORD_FIXATING_TIME)) {
      xfburn_progress_dialog_label_speed_set_text (dialog, _("no info"));
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->fifo_bar), _("no info"));
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->buffer_bar), _("no info"));
      dialog->priv->status = PROGRESS_STATUS_COMPLETED;
    }
    else if (strstr (converted_buffer, CDRECORD_FIXATING)) {
      xfburn_progress_dialog_label_action_set_text (dialog, _("Fixating..."));
    }
    else if (strstr (converted_buffer, CDRECORD_COPY)) {
      gfloat current = 0, total = 0;
      gint fifo = 0, buf = 0, speed = 0, speed_decimal = 0;

      if (sscanf
          (converted_buffer, "%*s %*d %*s %f %*s %f %*s %*s %*s %d %*s %*s %d %*s %d.%d", &current, &total, &fifo, &buf,
           &speed, &speed_decimal) == 6 && total > 0) {
        gdouble fraction;
        gfloat reformated_speed;
        gchar *text;

        xfburn_progress_dialog_label_action_set_text (dialog, _("Writing ISO..."));

        fraction = (gdouble) (current / total);
        if (fraction < 0.0)
          fraction = 0.0;
        else if (fraction > 1.0)
          fraction = 1.0;

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->progress_bar), fraction);
        text = g_strdup_printf ("%d%%", (int) (fraction * 100));
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->progress_bar), text);
        g_free (text);

        reformated_speed = speed + (0.1 * speed_decimal);
        text = g_strdup_printf ("%.1f x", reformated_speed);
        xfburn_progress_dialog_label_speed_set_text (dialog, text);
        g_free (text);

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->fifo_bar), (gdouble) ((gfloat) fifo / 100));
        text = g_strdup_printf ("%d%%", fifo);
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->fifo_bar), text);
        g_free (text);

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->priv->buffer_bar), (gdouble) ((gfloat) buf / 100));
        text = g_strdup_printf ("%d%%", buf);
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->priv->buffer_bar), text);
        g_free (text);
      }
    }
    break;
  }

  g_free (converted_buffer);
  return TRUE;
}

static void
xfburn_progress_dialog_label_action_set_text (XfburnProgressDialog * dialog, const gchar * text)
{
  gchar *temp;

  temp = g_strdup_printf ("<b>%s</b>", text);
  gtk_label_set_markup (GTK_LABEL (dialog->priv->label_action), temp);

  g_free (temp);
}

static void
xfburn_progress_dialog_label_speed_set_text (XfburnProgressDialog * dialog, const gchar * text)
{
  gchar *temp;

  temp = g_strdup_printf ("<b><i>%s</i></b>", text);
  gtk_label_set_markup (GTK_LABEL (dialog->priv->label_speed), temp);

  g_free (temp);
}

/* public */
void
xfburn_progress_dialog_start (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = dialog->priv;
  gboolean ready = FALSE;
  gint status;
  gchar *message = NULL;
  int argc;
  gchar **argvp;
  int fd_stdout;
  int fd_stderr;
  GError *error = NULL;

  DBG ("command : %s", priv->command);

  /* check if ready to start operation */
  while (!ready) {
    switch (priv->dialog_type) {
    case XFBURN_PROGRESS_DIALOG_COPY_CD:
      status = xfburn_device_query_cdstatus (priv->device_read);
      if (status != CDS_DISC_OK) {
        message = xfburn_device_cdstatus_to_string (status);
        break;
      }
      if (priv->device_burn) {
        status = xfburn_device_query_cdstatus (priv->device_burn);
        if (status == CDS_DISC_OK)
          ready = TRUE;
        else
          message = xfburn_device_cdstatus_to_string (status);
      }
      else
        ready = TRUE;
      break;
    case XFBURN_PROGRESS_DIALOG_BLANK_CD:
    case XFBURN_PROGRESS_DIALOG_BURN_ISO:
      status = xfburn_device_query_cdstatus (priv->device_burn);
      if (status == CDS_DISC_OK)
        ready = TRUE;
      else
        message = xfburn_device_cdstatus_to_string (status);
      break;
    default:
      break;
    }

    if (!ready) {
      GtkWidget *dialog_message;
      gint ret;

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
        xfburn_progress_dialog_label_action_set_text (dialog, _("Operation finished"));
        return;
      }
    }
  }

  /* launch command */
  switch (priv->dialog_type) {
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
  case XFBURN_PROGRESS_DIALOG_BURN_ISO:
  case XFBURN_PROGRESS_DIALOG_COPY_CD:
    if (!g_shell_parse_argv (priv->command, &argc, &argvp, &error)) {
      g_warning ("Unable to parse command : %s", error->message);
      g_error_free (error);
      return;
    }

    if (!g_spawn_async_with_pipes (NULL, argvp, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &(priv->pid_command),
                                   &(priv->fd_stdin), &fd_stdout, &fd_stderr, &error)) {
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
    break;
  default:
    break;
  }

  /* finalize operation */
  switch (priv->dialog_type) {
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
    gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.05);
    priv->id_pulse = g_timeout_add (250, (GSourceFunc) gtk_progress_bar_pulse, GTK_PROGRESS_BAR (priv->progress_bar));
    break;
  default:
    break;
  }
}

GtkWidget *
xfburn_progress_dialog_new (XfburnProgressDialogType type, ...)
{
  XfburnProgressDialog *obj;
  XfburnProgressDialogPrivate *priv;
  va_list args;

  obj = XFBURN_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_PROGRESS_DIALOG, NULL));
  priv = obj->priv;
  priv->dialog_type = type;

  va_start (args, type);
  switch (type) {
  case XFBURN_PROGRESS_DIALOG_BLANK_CD:
  case XFBURN_PROGRESS_DIALOG_BURN_ISO:
    /* device, command */
    priv->device_burn = va_arg (args, XfburnDevice *);
    priv->command = g_strdup (va_arg (args, const gchar *));
    break;
  case XFBURN_PROGRESS_DIALOG_COPY_CD:
    /* device_read, device_burn, command */
    priv->device_read = va_arg (args, XfburnDevice *);
    priv->device_burn = va_arg (args, XfburnDevice *);
    priv->command = g_strdup (va_arg (args, const gchar *));
    break;
  }
  va_end (args);

  xfburn_progress_dialog_create (obj);

  return GTK_WIDGET (obj);
}

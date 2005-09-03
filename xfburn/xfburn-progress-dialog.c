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

/* globals */
static void xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass);
static void xfburn_progress_dialog_init (XfburnProgressDialog * sp);
static void xfburn_progress_dialog_finalize (GObject * object);

static void cb_expander_activate (GtkExpander * expander, XfburnProgressDialog * dialog);
static gboolean cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv);
static void cb_dialog_response (XfburnProgressDialog * dialog, gint response_id, XfburnProgressDialogPrivate * priv);

/* structs */
typedef enum
{
  PROGRESS_STATUS_FAILED,
  PROGRESS_STATUS_CANCELLED,
  PROGRESS_STATUS_COMPLETED
} XfburnProgressDialogStatus;

struct XfburnProgressDialogPrivate
{
  XfburnProgressDialogStatus status;
 
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
  XfburnProgressDialogPrivate *priv;
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  GtkWidget *expander;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scrolled_window;
  GtkTextBuffer *textbuffer;

  obj->priv = g_new0 (XfburnProgressDialogPrivate, 1);
  priv = obj->priv;

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

  hbox = gtk_hbox_new (FALSE, BORDER);

  frame = gtk_frame_new (_("Estimated writing speed:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  priv->label_speed = gtk_label_new (_("no info"));
  xfburn_progress_dialog_set_writing_speed (obj, -1);
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
  gtk_widget_show (hbox);
  gtk_box_pack_start (box, hbox, TRUE, TRUE, BORDER);
  
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

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), priv);
  g_signal_connect (G_OBJECT (obj), "delete-event", G_CALLBACK (cb_dialog_delete), priv);
}

static void
xfburn_progress_dialog_finalize (GObject * object)
{
  XfburnProgressDialog *cobj;
  XfburnProgressDialogPrivate *priv;

  cobj = XFBURN_PROGRESS_DIALOG (object);
  priv = cobj->priv;

  g_free (priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cb_expander_activate (GtkExpander * expander, XfburnProgressDialog * dialog)
{
  // TODO
}

static gboolean
cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv)
{
  if (!GTK_WIDGET_SENSITIVE (priv->button_close)) {
    cb_dialog_response (dialog, GTK_RESPONSE_CANCEL, priv);
    return TRUE;
  }

  return FALSE;
}

static void
cb_dialog_response (XfburnProgressDialog * dialog, gint response_id, XfburnProgressDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_CANCEL) {
    gtk_widget_set_sensitive (priv->button_stop, FALSE);
    priv->status = PROGRESS_STATUS_CANCELLED;
  }
  else
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_progress_dialog_new ()
{
  XfburnProgressDialog *obj;
  
  obj = XFBURN_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_PROGRESS_DIALOG, NULL));
    
  return GTK_WIDGET (obj);
}

void
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

void
xfburn_progress_dialog_set_action_text (XfburnProgressDialog * dialog, const gchar * text)
{
  gchar *temp;

  temp = g_strdup_printf ("<b>%s</b>", text);
  gtk_label_set_markup (GTK_LABEL (dialog->priv->label_action), temp);

  g_free (temp);
}

void
xfburn_progress_dialog_set_writing_speed (XfburnProgressDialog * dialog, gfloat speed)
{
  gchar *temp;

  if (speed < 0)
    temp = g_strdup_printf ("<b><i>%s</i></b>", _("no info"));
  else
    temp = g_strdup_printf ("<b><i>%.1f x</i></b>", speed);
  
  gtk_label_set_markup (GTK_LABEL (dialog->priv->label_speed), temp);

  g_free (temp);
}

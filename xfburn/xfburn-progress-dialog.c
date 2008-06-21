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
  gboolean animate;
  int ani_index;
  
  GtkWidget *label_action;
  GtkWidget *progress_bar;
  GtkWidget *hbox_buffers;
  GtkWidget *label_speed;
  GtkWidget *fifo_bar;
  GtkWidget *buffer_bar;

  GtkWidget *button_stop;
  GtkWidget *button_close;
} XfburnProgressDialogPrivate;

/* globals */
static void xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass);
static void xfburn_progress_dialog_init (XfburnProgressDialog * sp);

static void xfburn_progress_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void xfburn_progress_dialog_set_property (GObject * object, guint prop_id, const GValue * value,
                                                 GParamSpec * pspec);

static void set_writing_speed (XfburnProgressDialog * dialog, gfloat speed);
static void set_action_text (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status, const gchar * text);

static void cb_button_close_clicked (GtkWidget *button, XfburnProgressDialog * dialog);
static gboolean cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv);

enum
{
  PROP_0,
  PROP_STATUS,
  PROP_SHOW_BUFFERS,
  PROP_ANIMATE,
};

static gchar animation[] = { '-', '\\', '|', '/' };

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
  
  object_class->get_property = xfburn_progress_dialog_get_property;
  object_class->set_property = xfburn_progress_dialog_set_property;

  /* properties */
  g_object_class_install_property (object_class, PROP_STATUS,
                                   g_param_spec_enum ("status", "Status", "Status", XFBURN_TYPE_PROGRESS_DIALOG_STATUS,
                                                      XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SHOW_BUFFERS,
                                   g_param_spec_boolean ("show-buffers", "Show buffers", "Show buffers",
                                                         TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_ANIMATE,
                                   g_param_spec_boolean ("animate", "Show an animation", "Show an animation",
                                                         FALSE, G_PARAM_READWRITE));
}

static void
xfburn_progress_dialog_init (XfburnProgressDialog * obj)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (obj);
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;

  gtk_window_set_default_size (GTK_WINDOW (obj), 575, 200);

  /* label */
  priv->label_action = gtk_label_new ("Initializing ...");
  gtk_misc_set_alignment (GTK_MISC (priv->label_action), 0.1, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->label_action), GTK_JUSTIFY_LEFT);
  gtk_label_set_selectable (GTK_LABEL (priv->label_action), TRUE);
  set_action_text (obj, XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Initializing..."));
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
  set_writing_speed (obj, -1);
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

  g_signal_connect (G_OBJECT (priv->button_close), "clicked", G_CALLBACK (cb_button_close_clicked), obj);
  g_signal_connect (G_OBJECT (obj), "delete-event", G_CALLBACK (cb_dialog_delete), priv);
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
  case PROP_ANIMATE:
    g_value_set_boolean (value, priv->animate);
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
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  switch (prop_id) {
  case PROP_STATUS:
    xfburn_progress_dialog_set_status (dialog, g_value_get_enum (value));
    break;
  case PROP_SHOW_BUFFERS:
    xfburn_progress_dialog_show_buffers (dialog, g_value_get_boolean (value));
    break;
  case PROP_ANIMATE:
    priv->animate = g_value_get_boolean (value);
    priv->ani_index = 0;
    //DBG ("Set animate to %d", priv->animate);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
set_writing_speed (XfburnProgressDialog * dialog, gfloat speed)
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

static void
set_action_text (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status, const gchar * text)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *temp = NULL;

  if (status == XFBURN_PROGRESS_DIALOG_STATUS_FAILED)
    temp = g_strdup_printf ("<span size=\"larger\" foreground=\"red\">%s</span>", text);
  else
    temp = g_strdup_printf ("<b>%s</b>", text);

  gtk_label_set_markup (GTK_LABEL (priv->label_action), temp);

  g_free (temp);  
}

/* callbacks */
static void
cb_button_close_clicked (GtkWidget *button, XfburnProgressDialog *dialog)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static gboolean
cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv)
{
  if (!GTK_WIDGET_SENSITIVE (priv->button_close)) {
    gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
    return TRUE;
  } else {
    gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
    return FALSE;
  }
}

/*        */
/* public */
/*        */

void
xfburn_progress_dialog_show_buffers (XfburnProgressDialog * dialog, gboolean show)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  gdk_threads_enter ();  
  if (show)
    gtk_widget_show (priv->hbox_buffers);
  else
    gtk_widget_hide (priv->hbox_buffers);
  gdk_threads_leave ();
}

void
xfburn_progress_dialog_pulse_progress_bar (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  gdk_threads_enter ();
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));
  gdk_threads_leave ();
}

/* getters */
gdouble
xfburn_progress_dialog_get_progress_bar_fraction (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gdouble ret = 0;

  gdk_threads_enter ();
  ret = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (priv->progress_bar));
  gdk_threads_leave ();

  return ret;
}

XfburnProgressDialogStatus
xfburn_progress_dialog_get_status (XfburnProgressDialog * dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  
  return priv->status;
}

/* setters */
void
xfburn_progress_dialog_set_writing_speed (XfburnProgressDialog * dialog, gfloat speed)
{
  gdk_threads_enter ();
  set_writing_speed (dialog, speed);
  gdk_threads_leave ();
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

  gdk_threads_enter ();  
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->buffer_bar), fraction);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->buffer_bar), text);
  gdk_threads_leave ();

  g_free (text);
}

void
xfburn_progress_dialog_set_buffer_bar_min_fill (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *text = NULL;

  text = g_strdup_printf (_("Min. fill was %2d%%"), (int) (fraction * 100));

  gdk_threads_enter ();    
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->buffer_bar), fraction);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->buffer_bar), text);
  gdk_threads_leave ();

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
  
  gdk_threads_enter ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->fifo_bar), fraction);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->fifo_bar), text);
  gdk_threads_leave ();

  g_free (text);
}

void
xfburn_progress_dialog_set_fifo_bar_text (XfburnProgressDialog * dialog, const gchar *text)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  gdk_threads_enter ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->fifo_bar), 0.0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->fifo_bar), text);
  gdk_threads_leave ();
}

void
xfburn_progress_dialog_set_progress_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gdouble cur_fraction = 0;
  gchar *text = NULL;

  cur_fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (priv->progress_bar));

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
  else if (priv->status == XFBURN_PROGRESS_DIALOG_STATUS_RUNNING && fraction >= cur_fraction) {
    if (priv->animate) {
      text = g_strdup_printf ("%2d%% %c", (int) (fraction * 100), animation[priv->ani_index]);
      priv->ani_index = (priv->ani_index + 1) % 4;
    } else {
      text = g_strdup_printf ("%d%%  ", (int) (fraction * 100));
    }
  }
  else if (fraction < cur_fraction) {
    return;
  }

  gdk_threads_enter ();    
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), fraction);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
  gdk_threads_leave ();

  g_free (text);
}

void
xfburn_progress_dialog_set_status (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  priv->status = status;

  if (status != XFBURN_PROGRESS_DIALOG_STATUS_RUNNING) {
    gdk_threads_enter ();
    gtk_widget_set_sensitive (priv->button_stop, FALSE);
    gtk_widget_set_sensitive (priv->button_close, TRUE);
    gdk_threads_leave ();

    xfburn_progress_dialog_set_progress_bar_fraction (dialog, 1.0);
  }
}

void
xfburn_progress_dialog_set_status_with_text (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status, const gchar * text)
{
  xfburn_progress_dialog_set_status (dialog, status);
  gdk_threads_enter ();
  set_action_text (dialog, status, text);
  gdk_threads_leave ();
}

/**
 * Thread safe method that sets status to XFBURN_PROGRESS_DIALOG_STATUS_FAILED and shows
 * an error message box.
 **/
void
xfburn_progress_dialog_burning_failed (XfburnProgressDialog * dialog, const gchar * msg_error)
{
  xfburn_progress_dialog_set_status_with_text (dialog, XFBURN_PROGRESS_DIALOG_STATUS_FAILED, _("Failure"));

  gdk_threads_enter ();
  xfce_err (msg_error);
  gdk_threads_leave ();
}

/* constructor */
GtkWidget *
xfburn_progress_dialog_new (GtkWindow *parent)
{
  XfburnProgressDialog *obj;
  obj = XFBURN_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_PROGRESS_DIALOG, "transient-for", parent,
					      "modal", TRUE, NULL));
    
  return GTK_WIDGET (obj);
}

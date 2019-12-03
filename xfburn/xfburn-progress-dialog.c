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

#include <libxfce4ui/libxfce4ui.h>

#include "xfburn-progress-dialog.h"
#include "xfburn-global.h"
#include "xfburn-main.h"
#include "xfburn-utils.h"

#define XFBURN_PROGRESS_DIALOG_GET_PRIVATE(obj) (xfburn_progress_dialog_get_instance_private (XFBURN_PROGRESS_DIALOG (obj)))

enum {
  BURNING_DONE,
  LAST_SIGNAL,
};

/* struct */
typedef struct
{
  XfburnProgressDialogStatus status;
  int fd_stdin;
  gboolean animate;
  int ani_index;
  gboolean stop;
  gboolean quit;
  
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
static guint signals[LAST_SIGNAL];

static void xfburn_progress_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void xfburn_progress_dialog_set_property (GObject * object, guint prop_id, const GValue * value,
                                                 GParamSpec * pspec);

static void set_writing_speed (XfburnProgressDialog * dialog, gfloat speed);
static void set_action_text (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status, const gchar * text);
static void stop (XfburnProgressDialog *dialog);

static void cb_button_stop_clicked (GtkWidget *button, XfburnProgressDialog * dialog);
static void cb_button_close_clicked (GtkWidget *button, XfburnProgressDialog * dialog);
static gboolean cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv);

enum
{
  PROP_0,
  PROP_STATUS,
  PROP_SHOW_BUFFERS,
  PROP_ANIMATE,
  PROP_QUIT,
  PROP_STOP,
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
      {XFBURN_PROGRESS_DIALOG_STATUS_FORMATTING, "XFBURN_PROGRESS_DIALOG_STATUS_FORMATTING", "formatting"},
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

G_DEFINE_TYPE_WITH_PRIVATE(XfburnProgressDialog, xfburn_progress_dialog, GTK_TYPE_DIALOG)

static void
xfburn_progress_dialog_class_init (XfburnProgressDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  
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
  g_object_class_install_property (object_class, PROP_QUIT,
                                   g_param_spec_boolean ("quit", "Quit", "Quit after successful completion",
                                                         FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_STOP,
                                   g_param_spec_boolean ("stop", "Stop the burning process", "Stop the burning process",
                                                         FALSE, G_PARAM_READABLE));
  /* signals */
  signals[BURNING_DONE] = g_signal_new ("burning-done", XFBURN_TYPE_PROGRESS_DIALOG, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnProgressDialogClass, burning_done),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
}

static void
xfburn_progress_dialog_init (XfburnProgressDialog * obj)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (obj);
  GtkBox *box = GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG(obj)));
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;

  gtk_window_set_default_size (GTK_WINDOW (obj), 575, 200);

  /* label */
  priv->label_action = gtk_label_new (_("Initializing..."));
  gtk_label_set_xalign(GTK_LABEL (priv->label_action), 0.1);
  gtk_label_set_justify (GTK_LABEL (priv->label_action), GTK_JUSTIFY_LEFT);
  gtk_label_set_selectable (GTK_LABEL (priv->label_action), TRUE);
  set_action_text (obj, XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Initializing..."));
  gtk_widget_show (priv->label_action);
  gtk_box_pack_start (box, priv->label_action, FALSE, TRUE, BORDER);

  /* progress bar */
  priv->progress_bar = xfburn_create_progress_bar (NULL);
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (box, priv->progress_bar, FALSE, FALSE, BORDER);
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.05);

  /* buffers */
  priv->hbox_buffers = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
  gtk_widget_show (priv->hbox_buffers);
  gtk_box_pack_start (box, priv->hbox_buffers, FALSE, TRUE, BORDER);

  frame = gtk_frame_new (_("Estimated writing speed:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (priv->hbox_buffers), frame, FALSE, FALSE, 0);
  priv->label_speed = gtk_label_new (_("unknown"));
  set_writing_speed (obj, -1);
  gtk_widget_show (priv->label_speed);
  gtk_container_add (GTK_CONTAINER (frame), priv->label_speed);

  table = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (table), BORDER);
  gtk_grid_set_column_spacing (GTK_GRID (table), BORDER * 2);
  gtk_box_pack_start (GTK_BOX (priv->hbox_buffers), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  label = gtk_label_new (_("FIFO buffer:"));
  gtk_label_set_yalign(GTK_LABEL(label), 0.5);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
  gtk_widget_show (label);
  priv->fifo_bar = xfburn_create_progress_bar (_("unknown"));
  gtk_grid_attach (GTK_GRID (table), priv->fifo_bar, 1, 0, 1, 1);
  gtk_widget_show (priv->fifo_bar);

  label = gtk_label_new (_("Device buffer:"));
  gtk_label_set_yalign(GTK_LABEL(label), 0.5);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);
  gtk_widget_show (label);
  priv->buffer_bar = xfburn_create_progress_bar (_("unknown"));
  gtk_grid_attach (GTK_GRID (table), priv->buffer_bar, 1, 1, 1, 1);
  gtk_widget_show (priv->buffer_bar);

  /* action buttons */
  priv->button_stop = gtk_button_new_with_mnemonic (_("_Stop"));
  gtk_widget_show (priv->button_stop);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), priv->button_stop, GTK_RESPONSE_CANCEL);
  gtk_widget_set_sensitive (priv->button_stop, TRUE);
  g_signal_connect (G_OBJECT (priv->button_stop), "clicked", G_CALLBACK (cb_button_stop_clicked), obj);

  priv->button_close = gtk_button_new_from_icon_name ("gtk-close", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_label (GTK_BUTTON (priv->button_close), _("Close"));
  gtk_widget_show (priv->button_close);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), priv->button_close, GTK_RESPONSE_CLOSE);
  gtk_widget_set_can_default (priv->button_close, TRUE);
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
  case PROP_QUIT:
    g_value_set_boolean (value, priv->quit);
    break;
  case PROP_STOP:
    g_value_set_boolean (value, priv->stop);
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
  case PROP_QUIT:
    priv->quit = g_value_get_boolean (value);
    break;
  case PROP_STOP:
    DBG ("this should not be allowed...");
    priv->stop = g_value_get_boolean (value);
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
    temp = g_strdup_printf ("<b><i>%s</i></b>", _("unknown"));
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

static void
stop (XfburnProgressDialog *dialog)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  //DBG ("setting stop");
  priv->stop = TRUE;
}

/* callbacks */
static void
cb_button_stop_clicked (GtkWidget *button, XfburnProgressDialog *dialog)
{
  gint res;
  GtkWidget *popup;

  popup = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                  _("Are you sure you want to abort?"));

  res = gtk_dialog_run (GTK_DIALOG (popup));
  gtk_widget_destroy (popup);

  if (res != GTK_RESPONSE_YES)
    return;

  stop (dialog);
}

static void
cb_button_close_clicked (GtkWidget *button, XfburnProgressDialog *dialog)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
  xfburn_main_leave_window ();
}

static gboolean
cb_dialog_delete (XfburnProgressDialog * dialog, GdkEvent * event, XfburnProgressDialogPrivate * priv)
{
  xfburn_main_leave_window ();
  if (!gtk_widget_get_sensitive (priv->button_close)) {
    /* burn process is still ongoing, we need to stop first */
    stop (dialog);
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

  if (fraction > 100.0) {
    fraction = 100.0;
    text = g_strdup ("100%");
  } else if (fraction < 0.0) {
    fraction = 0.0;
    text = g_strdup (_("unknown"));
  } else {
    text = g_strdup_printf ("%d%%", (int) (fraction));
  }

  gdk_threads_enter ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->buffer_bar), fraction / 100.0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->buffer_bar), text);
  gdk_threads_leave ();

  g_free (text);
}

void
xfburn_progress_dialog_set_buffer_bar_min_fill (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *text = NULL;

  if (fraction > 100.0) {
    fraction = 100.0;
    text = g_strdup ("100%");
  } else if (fraction < 0.0) {
    fraction = 0.0;
    text = g_strdup (_("unknown"));
  } else {
    text = g_strdup_printf (_("Min. fill was %2d%%"), (int) (fraction));
  }

  gdk_threads_enter ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->buffer_bar), fraction / 100.0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->buffer_bar), text);
  gdk_threads_leave ();

  g_free (text);
}

void
xfburn_progress_dialog_set_fifo_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *text = NULL;

  if (fraction > 100.0) {
    fraction = 100.0;
    text = g_strdup ("100%");
  } else if (fraction < 0.0) {
    fraction = 0.0;
    text = g_strdup (_("unknown"));
  } else {
    text = g_strdup_printf ("%d%%", (int) (fraction));
  }

  gdk_threads_enter ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->fifo_bar), fraction / 100.0);
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
xfburn_progress_dialog_reset (XfburnProgressDialog * dialog)
{
  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog),
                                               XFBURN_PROGRESS_DIALOG_STATUS_RUNNING,
                                               _("Initializing..."));
  xfburn_progress_dialog_set_progress_bar_fraction (dialog, -1.0);
}

void
xfburn_progress_dialog_set_progress_bar_fraction (XfburnProgressDialog * dialog, gdouble fraction)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gdouble cur_fraction = 0.0;
  gchar *text = NULL;

  cur_fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (priv->progress_bar)) * 100.0;

  if (fraction >= 100.0) {
    fraction = 100.0;
    switch (priv->status) {
    case XFBURN_PROGRESS_DIALOG_STATUS_STOPPING:
      text = g_strdup (_("Aborted"));
      break;
    case XFBURN_PROGRESS_DIALOG_STATUS_FORMATTING:
      text = g_strdup (_("Formatted."));
      break;
    case XFBURN_PROGRESS_DIALOG_STATUS_RUNNING:
      text = g_strdup ("100%");
      break;
    case XFBURN_PROGRESS_DIALOG_STATUS_META_DONE:
      g_warning ("Invalid progress dialog state");
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
  else if (priv->status < XFBURN_PROGRESS_DIALOG_STATUS_META_DONE &&
           fraction >= cur_fraction) {
    if (priv->animate) {
      text = g_strdup_printf ("%2d%% %c", (int) (fraction), animation[priv->ani_index]);
      priv->ani_index = (priv->ani_index + 1) % 4;
    } else {
      text = g_strdup_printf ("%d%%  ", (int) (fraction));
    }
  }
  else if (fraction < cur_fraction) {
    return;
  }

  gdk_threads_enter ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), fraction / 100.0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
  gdk_threads_leave ();

  g_free (text);
}

void
xfburn_progress_dialog_set_status (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  priv->status = status;

  if (status == XFBURN_PROGRESS_DIALOG_STATUS_STOPPING) {
    gdk_threads_enter ();
    set_action_text (dialog, status, _("Aborting..."));
    gdk_threads_leave ();
  } else if (status > XFBURN_PROGRESS_DIALOG_STATUS_META_DONE) {
    gdk_threads_enter ();
    gtk_widget_set_sensitive (priv->button_stop, FALSE);
    gtk_widget_set_sensitive (priv->button_close, TRUE);
    gdk_threads_leave ();

    xfburn_progress_dialog_set_progress_bar_fraction (dialog, 100.0);
    
  }
}

void
xfburn_progress_dialog_set_status_with_text (XfburnProgressDialog * dialog, XfburnProgressDialogStatus status, const gchar * text)
{
  XfburnProgressDialogPrivate *priv = XFBURN_PROGRESS_DIALOG_GET_PRIVATE (dialog);

  xfburn_progress_dialog_set_status (dialog, status);

  gdk_threads_enter ();
  set_action_text (dialog, status, text);
  if (status > XFBURN_PROGRESS_DIALOG_STATUS_META_DONE) {
    g_signal_emit (G_OBJECT (dialog), signals[BURNING_DONE], 0);
    if (status == XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED && priv->quit)
      g_idle_add ((GSourceFunc) gtk_main_quit, NULL );
  }
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
  xfce_dialog_show_error (NULL, NULL, "%s", msg_error);
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

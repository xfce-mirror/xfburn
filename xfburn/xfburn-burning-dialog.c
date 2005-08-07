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

#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-burning-dialog.h"
#include "xfburn-global.h"

static void xfburn_burning_dialog_class_init (XfburnBurningDialogClass * klass);
static void xfburn_burning_dialog_init (XfburnBurningDialog * sp);
static void xfburn_burning_dialog_finalize (GObject * object);

struct XfburnBurningDialogPrivate
{
  GtkWidget *progress_bar;
  GtkWidget *textview_output;
};

static GtkDialogClass *parent_class = NULL;

GtkType
xfburn_burning_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBurningDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_burning_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBurningDialog),
      0,
      (GInstanceInitFunc) xfburn_burning_dialog_init,
    };

    type = g_type_register_static (GTK_TYPE_DIALOG, "XfburnBurningDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_burning_dialog_class_init (XfburnBurningDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_burning_dialog_finalize;

}

static void
xfburn_burning_dialog_init (XfburnBurningDialog * obj)
{
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  XfburnBurningDialogPrivate *priv;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *expander;
  GtkWidget *scrolled_window;
  GtkWidget *button;
  
  obj->priv = g_new0 (XfburnBurningDialogPrivate, 1);
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
  
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_container_add (GTK_CONTAINER (expander), scrolled_window);
  
  priv->textview_output = gtk_text_view_new ();
  gtk_widget_set_size_request (priv->textview_output, 350, 200);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (priv->textview_output), FALSE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->textview_output), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->textview_output), GTK_WRAP_WORD);
  gtk_widget_show (priv->textview_output);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->textview_output);
  
  /* action buttons */
  button = gtk_button_new_from_stock (GTK_STOCK_STOP);
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);
  
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CLOSE);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);
  gtk_widget_set_sensitive (button, FALSE);
}

/* internals */
static void
xfburn_burning_dialog_finalize (GObject * object)
{
  XfburnBurningDialog *cobj;
  cobj = XFBURN_BURNING_DIALOG (object);

  /* Free private members, etc. */

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* public */

GtkWidget *
xfburn_burning_dialog_new (const gchar *command)
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_BURNING_DIALOG, NULL));

  return obj;
}

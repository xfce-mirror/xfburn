/* $Id$ */
/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-data-disc-usage.h"
#include "xfburn-global.h"
#include "xfburn-settings.h"
#include "xfburn-utils.h"
#include "xfburn-stock.h"
#include "xfburn-main-window.h"

/* prototypes */
static void xfburn_data_disc_usage_class_init (XfburnDataDiscUsageClass *);
static void xfburn_data_disc_usage_init (XfburnDataDiscUsage *);

static void cb_button_clicked (GtkButton *, XfburnDataDiscUsage *);
static void cb_combo_changed (GtkComboBox *, XfburnDataDiscUsage *);

/* globals */
static GtkHBoxClass *parent_class = NULL;

#define DEFAULT_DISK_SIZE_LABEL 2
#define LAST_CD_LABEL 4

struct
{
  guint64 size;
  gchar *label;
} datadisksizes[] = {
  {
  200 *1024 * 1024, "200MB CD"},
  {
  681984000, "650MB CD"},
  {
  737280000, "700MB CD"},
  {
  829440000, "800MB CD"},
  {
  912384000, "900MB CD"},
  {
  G_GINT64_CONSTANT(0x1182a0000), "4.7GB DVD"}, /* 4 700 372 992 */
  {
  G_GINT64_CONSTANT(0x1fd3e0000), "8.5GB DVD"}, /* 8 543 666 176  */
};

enum
{
  BEGIN_BURN,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL];

/*******************************/
/* XfburnDataComposition class */
/*******************************/
GtkType
xfburn_data_disc_usage_get_type (void)
{
  static GtkType disc_usage_type = 0;

  if (!disc_usage_type) {
    static const GTypeInfo disc_usage_info = {
      sizeof (XfburnDataDiscUsageClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_data_disc_usage_class_init,
      NULL,
      NULL,
      sizeof (XfburnDataDiscUsage),
      0,
      (GInstanceInitFunc) xfburn_data_disc_usage_init
    };

    disc_usage_type = g_type_register_static (GTK_TYPE_HBOX, "XfburnDataDiscUsage", &disc_usage_info, 0);
  }

  return disc_usage_type;
}

static void
xfburn_data_disc_usage_class_init (XfburnDataDiscUsageClass * klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);

  signals[BEGIN_BURN] = g_signal_new ("begin-burn", G_TYPE_FROM_CLASS (gobject_class), G_SIGNAL_ACTION,
                                      G_STRUCT_OFFSET (XfburnDataDiscUsageClass, begin_burn),
                                      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
xfburn_data_disc_usage_init (XfburnDataDiscUsage * disc_usage)
{
  int i;

  disc_usage->size = 0;

  disc_usage->progress_bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (disc_usage->progress_bar), "0 B");
  gtk_box_pack_start (GTK_BOX (disc_usage), disc_usage->progress_bar, TRUE, TRUE, BORDER);
  gtk_widget_show (disc_usage->progress_bar);

  disc_usage->combo = gtk_combo_box_new_text ();
  for (i = 0; i < G_N_ELEMENTS (datadisksizes); i++)
    gtk_combo_box_append_text (GTK_COMBO_BOX (disc_usage->combo), datadisksizes[i].label);
  gtk_combo_box_set_active (GTK_COMBO_BOX (disc_usage->combo), DEFAULT_DISK_SIZE_LABEL);
  gtk_box_pack_start (GTK_BOX (disc_usage), disc_usage->combo, FALSE, FALSE, BORDER);
  gtk_widget_show (disc_usage->combo);

  disc_usage->button = xfce_create_mixed_button (XFBURN_STOCK_BURN_CD, _("Burn composition"));
  gtk_box_pack_start (GTK_BOX (disc_usage), disc_usage->button, FALSE, FALSE, BORDER);
  gtk_widget_set_sensitive (disc_usage->button, FALSE);
  gtk_widget_show (disc_usage->button);
  g_signal_connect (G_OBJECT (disc_usage->button), "clicked", G_CALLBACK (cb_button_clicked), disc_usage);
  
  g_signal_connect (G_OBJECT (disc_usage->combo), "changed", G_CALLBACK (cb_combo_changed), disc_usage);
  /* Disabling burn composition doesn't work when this is enabled */
  /*gtk_widget_set_sensitive (disc_usage->button, xfburn_main_window_support_cdr (xfburn_main_window_get_instance ()));*/
}

/* internals */
static void
xfburn_data_disc_usage_update_size (XfburnDataDiscUsage * disc_usage)
{
  gfloat fraction;
  gchar *size;

  fraction = disc_usage->size / datadisksizes[gtk_combo_box_get_active (GTK_COMBO_BOX (disc_usage->combo))].size;
  if (fraction > 1.0)
    fraction = 1.0;
  if (fraction < 0.0)
    fraction = 0.0;

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (disc_usage->progress_bar), fraction > 1.0 ? 1.0 : fraction);

  if (xfburn_settings_get_boolean ("human-readable-units", TRUE))
    size = xfburn_humanreadable_filesize ((guint64) disc_usage->size);
  else
    size = g_strdup_printf ("%.0lf B", disc_usage->size);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (disc_usage->progress_bar), size);

  if (disc_usage->size == 0 || 
      disc_usage->size > datadisksizes[gtk_combo_box_get_active (GTK_COMBO_BOX (disc_usage->combo))].size)
    gtk_widget_set_sensitive (disc_usage->button, FALSE);
  else
    gtk_widget_set_sensitive (disc_usage->button, TRUE);
 
  
  g_free (size);
}

static void 
cb_button_clicked (GtkButton *button, XfburnDataDiscUsage *du)
{
  if (du->size <= datadisksizes[gtk_combo_box_get_active (GTK_COMBO_BOX (du->combo))].size) {
    g_signal_emit (G_OBJECT (du), signals[BEGIN_BURN], 0);
  } else {
    xfce_err (_("You are trying to burn more data than the disk can contain !"));
  }
}

static void
cb_combo_changed (GtkComboBox * combo, XfburnDataDiscUsage * usage)
{
  xfburn_data_disc_usage_update_size (usage);
}

/* public methods */
gdouble
xfburn_data_disc_usage_get_size (XfburnDataDiscUsage * disc_usage)
{
  return disc_usage->size;
}

void
xfburn_data_disc_usage_set_size (XfburnDataDiscUsage * disc_usage, gdouble size)
{
  disc_usage->size = size;
  xfburn_data_disc_usage_update_size (disc_usage);
}

void
xfburn_data_disc_usage_add_size (XfburnDataDiscUsage * disc_usage, gdouble size)
{
  disc_usage->size = disc_usage->size + size;
  xfburn_data_disc_usage_update_size (disc_usage);
}

void
xfburn_data_disc_usage_sub_size (XfburnDataDiscUsage * disc_usage, gdouble size)
{
  disc_usage->size = disc_usage->size - size;
  xfburn_data_disc_usage_update_size (disc_usage);
}

XfburnDataDiscType
xfburn_data_disc_usage_get_disc_type (XfburnDataDiscUsage * disc_usage)
{
  if (gtk_combo_box_get_active (GTK_COMBO_BOX (disc_usage->combo)) > LAST_CD_LABEL)
    return DVD_DISC;
  else
    return CD_DISC;
}

GtkWidget *
xfburn_data_disc_usage_new (void)
{
  return g_object_new (xfburn_data_disc_usage_get_type (), NULL);
}


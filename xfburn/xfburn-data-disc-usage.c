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

#include "xfburn-disc-usage.h"
#include "xfburn-data-disc-usage.h"
#include "xfburn-global.h"
#include "xfburn-settings.h"
#include "xfburn-utils.h"
#include "xfburn-main-window.h"

/* prototypes */
static void xfburn_data_disc_usage_class_init (XfburnDataDiscUsageClass *, gpointer data);

static gboolean can_burn (XfburnDiscUsage *disc_usage);
static void xfburn_data_disc_usage_update_size (XfburnDiscUsage * disc_usage);

/* globals */
static XfburnDiscUsageClass *parent_class = NULL;

#define DEFAULT_DISK_SIZE_LABEL 2
#define LAST_CD_LABEL 4
#define NUM_LABELS 10

XfburnDiscLabels datadiscsizes[] = {
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
  G_GINT64_CONSTANT(0x1182a0000), "4.3GB DVD"}, /* 4 700 372 992 */
  {
  G_GINT64_CONSTANT(0x1fd3e0000), "7.9GB DVD"}, /* 8 543 666 176 */
  {
  G_GINT64_CONSTANT(0x5a3a00000), "22.5GB BD"}, /* 24 220 008 448 */
  {
  G_GINT64_CONSTANT(0xba7400000), "46.5GB BD"}, /* 50 050 629 632 */
  {
  G_GINT64_CONSTANT(0x20000000000), "2TB"}, /* limit of libburn: 2 TiB */
};

/*******************************/
/* XfburnDataComposition class */
/*******************************/
GType
xfburn_data_disc_usage_get_type (void)
{
  static GType disc_usage_type = 0;

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
      NULL,
      NULL
    };

    disc_usage_type = g_type_register_static (XFBURN_TYPE_DISC_USAGE, "XfburnDataDiscUsage", &disc_usage_info, 0);
  }

  return disc_usage_type;
}

static void
xfburn_data_disc_usage_class_init (XfburnDataDiscUsageClass * klass, gpointer data)
{
  XfburnDiscUsageClass *pklass;

  parent_class = g_type_class_peek_parent (klass);

  /* override virtual methods */
  pklass = XFBURN_DISC_USAGE_CLASS(klass);
  
  pklass->labels      = datadiscsizes;
  pklass->num_labels  = G_N_ELEMENTS (datadiscsizes);
  pklass->update_size = xfburn_data_disc_usage_update_size;
  pklass->can_burn    = can_burn;
}

/* internals */
static void
xfburn_data_disc_usage_update_size (XfburnDiscUsage * disc_usage)
{
  gfloat fraction;
  gchar *size;

  fraction = disc_usage->size / datadiscsizes[gtk_combo_box_get_active (GTK_COMBO_BOX (disc_usage->combo))].size;
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

  g_free (size);
}

static gboolean
can_burn (XfburnDiscUsage *disc_usage)
{
  if (disc_usage->size == 0 || 
      disc_usage->size > datadiscsizes[gtk_combo_box_get_active (GTK_COMBO_BOX (disc_usage->combo))].size)
    return FALSE;
  else
    return TRUE;
}

GtkWidget *
xfburn_data_disc_usage_new (void)
{
  return g_object_new (xfburn_data_disc_usage_get_type (), NULL);
}


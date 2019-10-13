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
#include "xfburn-audio-disc-usage.h"
#include "xfburn-global.h"
#include "xfburn-settings.h"
#include "xfburn-utils.h"
#include "xfburn-main-window.h"

/* prototypes */
static void xfburn_audio_disc_usage_class_init (XfburnAudioDiscUsageClass *, gpointer);

static gboolean can_burn (XfburnDiscUsage *disc_usage);
static void xfburn_audio_disc_usage_update_size (XfburnDiscUsage * disc_usage);

/* globals */
static XfburnDiscUsageClass *parent_class = NULL;

#define DEFAULT_DISK_SIZE_LABEL 2
#define LAST_CD_LABEL 4
#define NUM_LABELS 7

XfburnDiscLabels audiodiscsizes[] = {
  {
  21*60, "200MB / 21min CD"},
  {
  74*60, "650MB / 74min CD"},
  {
  80*60, "700MB / 80min CD"},
  {
  90*60, "800MB / 90min CD"},
  {
  99*60, "900MB / 99min CD"},
};

/*******************************/
/* XfburnAudioComposition class */
/*******************************/
GType
xfburn_audio_disc_usage_get_type (void)
{
  static GType disc_usage_type = 0;

  if (!disc_usage_type) {
    static const GTypeInfo disc_usage_info = {
      sizeof (XfburnAudioDiscUsageClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_audio_disc_usage_class_init,
      NULL,
      NULL,
      sizeof (XfburnAudioDiscUsage),
      0,
      NULL,
      NULL
    };

    disc_usage_type = g_type_register_static (XFBURN_TYPE_DISC_USAGE, "XfburnAudioDiscUsage", &disc_usage_info, 0);
  }

  return disc_usage_type;
}

static void
xfburn_audio_disc_usage_class_init (XfburnAudioDiscUsageClass * klass, gpointer data)
{
  XfburnDiscUsageClass *pklass;

  parent_class = g_type_class_peek_parent (klass);

  /* override virtual methods */
  pklass = XFBURN_DISC_USAGE_CLASS(klass);
  
  pklass->labels      = audiodiscsizes;
  pklass->num_labels  = G_N_ELEMENTS (audiodiscsizes);
  pklass->update_size = xfburn_audio_disc_usage_update_size;
  pklass->can_burn    = can_burn;
}

/* internals */
static void
xfburn_audio_disc_usage_update_size (XfburnDiscUsage * disc_usage)
{
  gfloat fraction;
  gchar *len;

  fraction = disc_usage->size / audiodiscsizes[gtk_combo_box_get_active (GTK_COMBO_BOX (disc_usage->combo))].size;
  if (fraction > 1.0)
    fraction = 1.0;
  if (fraction < 0.0)
    fraction = 0.0;

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (disc_usage->progress_bar), fraction > 1.0 ? 1.0 : fraction);

  if (xfburn_settings_get_boolean ("human-readable-units", TRUE))
    len = g_strdup_printf ("%2.0lfm %02.0lfs", disc_usage->size / 60, (gdouble) (((guint64) disc_usage->size) % 60));
  else
    len = g_strdup_printf ("%.0lf secs", disc_usage->size);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (disc_usage->progress_bar), len);

  g_free (len);
}

static gboolean
can_burn (XfburnDiscUsage *disc_usage)
{
  if (disc_usage->size == 0 || 
      disc_usage->size > audiodiscsizes[gtk_combo_box_get_active (GTK_COMBO_BOX (disc_usage->combo))].size)
    return FALSE;
  else
    return TRUE;
}

GtkWidget *
xfburn_audio_disc_usage_new (void)
{
  return g_object_new (xfburn_audio_disc_usage_get_type (), NULL);
}


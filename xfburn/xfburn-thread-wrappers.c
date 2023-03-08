/*
 *  xfburn-thread-wrappers.c - thread-safe wrappers, mainly for GTK
 *  Copyright (c) 2023 Hunter Turcin (huntertur@gmail.com)
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

/*
 * This file is organized not by public/private, but by wrapped function.
 * Each wrapped function has the following pattern:
 *   - a struct definition, if needed
 *   - the callback that is run on the main thread
 *   - the wrapper that is called on a non-main thread
 */

#include "xfburn-thread-wrappers.h"

/* g_signal_emit */

typedef struct
{
  gpointer instance;
  guint signal_id;
  GQuark detail;
} SignalEmitParams;

static gboolean
cb_g_signal_emit (gpointer user_data)
{
  SignalEmitParams *params = (SignalEmitParams *) user_data;
  g_signal_emit (params->instance, params->signal_id, params->detail);
  g_free (params);
  return G_SOURCE_REMOVE;
}

void
safe_g_signal_emit (gpointer instance, guint signal_id, GQuark detail)
{
  SignalEmitParams *params = g_new (SignalEmitParams, 1);
  params->instance = instance;
  params->signal_id = signal_id;
  params->detail = detail;
  gdk_threads_add_idle (cb_g_signal_emit, params);
}

/* gtk_progress_bar_pulse */

static gboolean
cb_gtk_progress_bar_pulse (gpointer pbar)
{
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (pbar));
  return G_SOURCE_REMOVE;
}

void
safe_gtk_progress_bar_pulse (GtkProgressBar *pbar)
{
  gdk_threads_add_idle (cb_gtk_progress_bar_pulse, pbar);
}

/* gtk_progress_bar_set_fraction */

typedef struct
{
  GtkProgressBar *pbar;
  gdouble fraction;
} SetFractionParams;

static gboolean
cb_gtk_progress_bar_set_fraction (gpointer user_data)
{
  SetFractionParams *params = (SetFractionParams *) user_data;
  gtk_progress_bar_set_fraction (params->pbar, params->fraction);
  g_free (params);
  return G_SOURCE_REMOVE;
}

void
safe_gtk_progress_bar_set_fraction (GtkProgressBar *pbar, gdouble fraction)
{
  SetFractionParams *params = g_new (SetFractionParams, 1);
  params->pbar = pbar;
  params->fraction = fraction;
  gdk_threads_add_idle (cb_gtk_progress_bar_set_fraction, params);
}

/* gtk_progress_bar_set_text */

typedef struct
{
  GtkProgressBar *pbar;
  const gchar *text;
} SetTextParams;

static gboolean
cb_gtk_progress_bar_set_text (gpointer user_data)
{
  SetTextParams *params = (SetTextParams *) user_data;
  gtk_progress_bar_set_text (params->pbar, params->text);
  g_free (params);
  return G_SOURCE_REMOVE;
}

void
safe_gtk_progress_bar_set_text (GtkProgressBar *pbar, const gchar *text)
{
  SetTextParams *params = g_new (SetTextParams, 1);
  params->pbar = pbar;
  params->text = text;
  gdk_threads_add_idle (cb_gtk_progress_bar_set_text, params);
}

/* gtk_tree_path_free */

static gboolean
cb_gtk_tree_path_free (gpointer path)
{
  gtk_tree_path_free ((GtkTreePath *) path);
  return G_SOURCE_REMOVE;
}

void
safe_gtk_tree_path_free (GtkTreePath *path)
{
  gdk_threads_add_idle (cb_gtk_tree_path_free, path);
}

/* gtk_widget_hide */

static gboolean
cb_gtk_widget_hide (gpointer widget)
{
  gtk_widget_hide (GTK_WIDGET (widget));
  return G_SOURCE_REMOVE;
}

void
safe_gtk_widget_hide (GtkWidget *widget)
{
  gdk_threads_add_idle (cb_gtk_widget_hide, widget);
}

/* gtk_widget_set_sensitive */

typedef struct
{
  GtkWidget *widget;
  gboolean sensitive;
} SetSensitiveParams;

static gboolean
cb_gtk_widget_set_sensitive (gpointer user_data)
{
  SetSensitiveParams *params = (SetSensitiveParams *) user_data;
  gtk_widget_set_sensitive (params->widget, params->sensitive);
  g_free (params);
  return G_SOURCE_REMOVE;
}

void
safe_gtk_widget_set_sensitive (GtkWidget *widget, gboolean sensitive)
{
  SetSensitiveParams *params = g_new (SetSensitiveParams, 1);
  params->widget = widget;
  params->sensitive = sensitive;
  gdk_threads_add_idle (cb_gtk_widget_set_sensitive, params);
}

/* gtk_widget_show */

static gboolean
cb_gtk_widget_show (gpointer widget)
{
  gtk_widget_show (GTK_WIDGET (widget));
  return G_SOURCE_REMOVE;
}

void
safe_gtk_widget_show (GtkWidget *widget)
{
  gdk_threads_add_idle (cb_gtk_widget_show, widget);
}

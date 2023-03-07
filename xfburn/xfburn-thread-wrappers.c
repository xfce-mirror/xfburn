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

#include "xfburn-thread-wrappers.h"

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

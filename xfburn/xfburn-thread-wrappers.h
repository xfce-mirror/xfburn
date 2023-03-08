/*
 *  xfburn-thread-wrappers.h - thread-safe wrappers, mainly for GTK
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

#ifndef __XFBURN_THREAD_WRAPPERS_H__
#define __XFBURN_THREAD_WRAPPERS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Thread-safe wrapper for g_signal_emit with minimal arguments*/
void safe_g_signal_emit (gpointer instance, guint signal_id, GQuark detail);
/* Thread-safe wrapper for gtk_progress_bar_pulse */
void safe_gtk_progress_bar_pulse (GtkProgressBar *pbar);
/* Thread-safe wrapper for gtk_progress_bar_set_fraction */
void safe_gtk_progress_bar_set_fraction (GtkProgressBar *pbar, gdouble fraction);
/* Thread-safe wrapper for gtk_progress_bar_set_text */
void safe_gtk_progress_bar_set_text (GtkProgressBar *pbar, const gchar *text);
/* Thread-safe wrapper for gtk_tree_path_free */
void safe_gtk_tree_path_free (GtkTreePath *path);
/* Thread-safe wrapper for gtk_widget_hide */
void safe_gtk_widget_hide (GtkWidget *widget);
/* Thread-safe wrapper for gtk_widget_show */
void safe_gtk_widget_show (GtkWidget *widget);

G_END_DECLS

#endif

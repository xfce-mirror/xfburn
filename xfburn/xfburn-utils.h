/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifndef __XFBURN_UTILS_H__
#define __XFBURN_UTILS_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "xfburn-global.h"

/*
#define XFBURN_GDK_ENTER() { DBG("gdk-thread-enter"); gdk_threads_enter(); }
#define XFBURN_GDK_LEAVE() { DBG("gdk-thread-leave"); gdk_threads_leave(); }
*/

void xfburn_busy_cursor (GtkWidget *);
void xfburn_default_cursor (GtkWidget *);

gchar *xfburn_humanreadable_filesize (guint64);
guint64 xfburn_calc_dirsize (const gchar *);
GSList * xfburn_make_abosulet_file_list (gint count, gchar *filenames[]);

void xfburn_browse_for_file (GtkEntry *entry, GtkWindow *parent);

gboolean xfburn_ask_yes_no (GtkMessageType type, const gchar *primary_text, const gchar *secondary_text);
GtkWidget * xfburn_gtk_button_new_mixed (const gchar *stock_id, const gchar *label);

void xfburn_capture_libburn_messages (void);
void xfburn_console_libburn_messages (void);
int xfburn_media_profile_to_kb (int media_no);

void xfburn_add_button_to_toolbar(GtkToolbar *toolbar, const gchar *stock, const gchar *text, const gchar *action, const gchar *tooltip);
GtkWidget * xfburn_create_progress_bar(const gchar *);

#endif

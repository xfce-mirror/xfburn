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

#ifndef __XFBURN_MAIN_WINDOW_H__
#define __XFBURN_MAIN_WINDOW_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "xfburn-data-composition.h"
#include "xfburn-file-browser.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_MAIN_WINDOW            (xfburn_main_window_get_type ())
#define XFBURN_MAIN_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_MAIN_WINDOW, XfburnMainWindow))
#define XFBURN_MAIN_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_MAIN_WINDOW, XfburnMainWindowClass))
#define XFBURN_IS_MAIN_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_MAIN_WINDOW))
#define XFBURN_IS_MAIN_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_MAIN_WINDOW))
#define XFBURN_MAIN_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_MAIN_WINDOW, XfburnMainWindowClass))

typedef void (* gActionCallback) (GSimpleAction *,GVariant *,gpointer);

typedef struct 
{
  GtkWindow window;
} XfburnMainWindow;

typedef struct 
{
  GtkWindowClass parent_class;
} XfburnMainWindowClass;

GType xfburn_main_window_get_type (void);

GtkWidget *xfburn_main_window_new (void);
XfburnMainWindow *xfburn_main_window_get_instance (void);

GtkUIManager *xfburn_main_window_get_ui_manager (XfburnMainWindow *window);
XfburnFileBrowser *xfburn_main_window_get_file_browser (XfburnMainWindow *window);

void xfburn_main_window_add_data_composition_with_files (XfburnMainWindow *window, int filec, char **filenames);
void xfburn_main_window_add_audio_composition_with_files (XfburnMainWindow *window, int filec, char **filenames);

gboolean xfburn_main_window_support_cdr (XfburnMainWindow *window);
gboolean xfburn_main_window_support_cdrw (XfburnMainWindow *window);

G_END_DECLS
#endif

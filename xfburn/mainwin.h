/*
 * Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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
 
#ifndef __XFBURN_MAINWIN_H__
#define __XFBURN_MAINWIN_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_MAIN_WINDOW            (main_window_get_type ())
#define MAIN_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MAIN_WINDOW, MainWindow))
#define MAIN_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MAIN_WINDOW, MainWindowClass))
#define IS_MAIN_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MAIN_WINDOW))
#define IS_MAIN_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MAIN_WINDOW))
#define MAIN_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MAIN_WINDOW, MainWindowClass))

typedef struct _MainWindow		MainWindow;
typedef struct _MainWindowClass	MainWindowClass;

struct _MainWindow {
  GtkWindow window;
  
  GtkWidget *vbox;
};

struct _MainWindowClass {
  GtkWindowClass	parent_class;
};

extern GtkType		main_window_get_type (void) G_GNUC_CONST;
extern GtkWidget	*main_window_new (void);

G_END_DECLS

#endif

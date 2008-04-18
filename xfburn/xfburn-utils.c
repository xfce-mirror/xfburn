/* $Id$ */
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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xfburn-utils.h"
#include "xfburn-settings.h"

/***********/
/* cursors */
/***********/
void
xfburn_busy_cursor (GtkWidget * widget)
{
  GdkCursor *cursor;

  g_return_if_fail (widget != NULL);
  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (gtk_widget_get_parent_window (widget), cursor);
  gdk_cursor_destroy (cursor);
  gdk_flush ();
}

void
xfburn_default_cursor (GtkWidget * widget)
{
  g_return_if_fail (widget != NULL);
  gdk_window_set_cursor (gtk_widget_get_parent_window (widget), NULL);
  gdk_flush ();
}

/*******************/
/* for filebrowser */
/*******************/
gchar *
xfburn_humanreadable_filesize (guint64 size)
{
  if (!xfburn_settings_get_boolean ("human-readable-units", TRUE))
    return g_strdup_printf ("%lu B", (long unsigned int) size);
  
  /* copied from GnomeBaker */

  gchar *ret = NULL;
  const gchar *unit_list[5] = { "B ", "KB", "MB", "GB", "TB" };
  gint unit = 0;
  gdouble human_size = (gdouble) size;

  while (human_size > 1024) {
    human_size = human_size / 1024;
    unit++;
  }

  if ((human_size - (gulong) human_size) > 0.1)
    ret = g_strdup_printf ("%.2f %s", human_size, unit_list[unit]);
  else
    ret = g_strdup_printf ("%.0f %s", human_size, unit_list[unit]);
  return ret;
}

guint64
xfburn_calc_dirsize (const gchar * dirname)
{
  /* copied from GnomeBaker */
  guint64 size = 0;

  GDir *dir = g_dir_open (dirname, 0, NULL);
  if (dir != NULL) {
    const gchar *name = g_dir_read_name (dir);
    while (name != NULL) {
      /* build up the full path to the name */
      gchar *fullname = g_build_filename (dirname, name, NULL);
      struct stat s;

      if (stat (fullname, &s) == 0) {
        /* see if the name is actually a directory or a regular file */
        if (s.st_mode & S_IFDIR)
          size += (guint64) s.st_size + xfburn_calc_dirsize (fullname);
        else if (s.st_mode & S_IFREG)
          size += (guint64) s.st_size;
      }

      g_free (fullname);
      name = g_dir_read_name (dir);
    }

    g_dir_close (dir);
  }

  return size;
}

void
xfburn_browse_for_file (GtkEntry *entry, GtkWindow *parent)
{
  GtkWidget *dialog;
  const gchar *text;
  
  text = gtk_entry_get_text (entry);

  dialog = gtk_file_chooser_dialog_new (_("Select command"), parent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
  if (strlen (text) > 0)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), text);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gchar *filename = NULL;
    
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_entry_set_text (entry, filename);
    g_free (filename);
  } 

  gtk_widget_destroy (dialog);
}


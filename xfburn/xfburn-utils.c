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
#include <libburn.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-device-list.h"
#include "xfburn-main.h"
#include "xfburn-main-window.h"

/***********/
/* cursors */
/***********/
void
xfburn_busy_cursor (GtkWidget * widget)
{
  GdkCursor *cursor;

  g_return_if_fail (widget != NULL);
  cursor = gdk_cursor_new_for_display( gtk_widget_get_display(widget), GDK_WATCH);
  gdk_window_set_cursor (gtk_widget_get_parent_window (widget), cursor);
  g_object_unref (cursor);
  gdk_display_flush (gtk_widget_get_display(widget));
}

void
xfburn_default_cursor (GtkWidget * widget)
{
  g_return_if_fail (widget != NULL);
  gdk_window_set_cursor (gtk_widget_get_parent_window (widget), NULL);
  gdk_display_flush (gtk_widget_get_display(widget));
}

/*******************/
/* for filebrowser */
/*******************/
gchar *
xfburn_humanreadable_filesize (guint64 size)
{
  gchar *ret = NULL;
  const gchar *unit_list[5] = { "B ", "KB", "MB", "GB", "TB" };
  gint unit = 0;
  gdouble human_size = (gdouble) size;

  if (!xfburn_settings_get_boolean ("human-readable-units", TRUE))
    return g_strdup_printf ("%lu B", (long unsigned int) size);

  /* copied from GnomeBaker */

  while (human_size > 1024 && unit < 4) {
    human_size = human_size / 1024;
    unit++;
  }

  if ((human_size - (gulong) human_size) > 0.05)
    ret = g_strdup_printf ("%.2f %s", human_size, unit_list[unit]);
  else
    ret = g_strdup_printf ("%.0f %s", human_size, unit_list[unit]);
  return ret;
}

void
xfburn_browse_for_file (GtkEntry *entry, GtkWindow *parent)
{
  GtkWidget *dialog;
  const gchar *text;

  text = gtk_entry_get_text (entry);

  dialog = gtk_file_chooser_dialog_new (_("Select command"), parent, GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"),
                                        GTK_RESPONSE_CANCEL, "document-save", GTK_RESPONSE_ACCEPT, NULL);

  if(xfburn_main_has_initial_dir ()) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), xfburn_main_get_initial_dir ());
  }

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

/**
 * Open a file picker that will allow the user to select multiple files.
 * 
 * Returns a caller-freed string containing the path to each selected file,
 * separated by newlines, or NULL if nothing was selected.
 * 
 * If allow_directories is TRUE, both files and directories may be selected.
 */
gchar *
xfburn_browse_for_files (gboolean allow_directories)
{
  /* The stock file chooser only allows selecting files *or* folders,
     but this can be gotten around if defining our own button
     instead of using GTK_RESPONSE_ACCEPT. */
  const gint XFBURN_RESPONSE_ADD_ANYTHING = 0;
  gint response;
  GtkWidget *dialog;
  gchar *selected_files = NULL;

  dialog = gtk_file_chooser_dialog_new (_("File(s) to add to composition"),
                                        GTK_WINDOW (xfburn_main_window_get_instance ()),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        NULL,
                                        NULL);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  if (xfburn_main_has_initial_dir ()) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), xfburn_main_get_initial_dir ());
  }

  if (allow_directories) {
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Add"), XFBURN_RESPONSE_ADD_ANYTHING);
  } else {
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Add"), GTK_RESPONSE_ACCEPT);
  }

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_ACCEPT || response == XFBURN_RESPONSE_ADD_ANYTHING) {
    GSList *list = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
    GString *str = g_string_new (NULL);
    GSList *curr;

    for (curr = list; curr != NULL; curr = curr->next) {
      g_string_append (str, curr->data);
      g_string_append_c (str, '\n');
    }

    g_slist_free_full (list, g_free);
    selected_files = str->str;
    g_string_free (str, FALSE);
    DBG ("selected files: %s", selected_files);
  }

  gtk_widget_destroy (dialog);

  return selected_files;
}

/**********************/
/* Simple GUI helpers */
/**********************/

gboolean
xfburn_ask_yes_no (GtkMessageType type, const gchar *primary_text, const gchar *secondary_text)
{
  GtkMessageDialog *dialog;
  gint ret;
  gboolean ok = TRUE;

  dialog = (GtkMessageDialog *) gtk_message_dialog_new (NULL,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  type,
                                  GTK_BUTTONS_YES_NO,
                                  "%s",
                                  primary_text);
  gtk_message_dialog_format_secondary_text (dialog, "%s", secondary_text);
  ret = gtk_dialog_run (GTK_DIALOG (dialog));
  switch (ret) {
    case GTK_RESPONSE_YES:
      break;
    default:
      ok = FALSE;
  }
  xfburn_busy_cursor (gtk_dialog_get_content_area(GTK_DIALOG (dialog)));
  gtk_widget_destroy (GTK_WIDGET (dialog));

  return ok;
}

/* Based on xfce_gtk_Button_new_mixed, but always use a stock icon */
GtkWidget *
xfburn_gtk_button_new_mixed (const gchar *stock_id, const gchar *label)
{
    GtkWidget *button;
    GtkWidget *image;
    g_return_val_if_fail (stock_id != NULL || label != NULL, NULL);
    if (label != NULL) {
        button = gtk_button_new_with_mnemonic (label);
        if (stock_id != NULL) {
            /* create image widget */
            image = gtk_image_new_from_icon_name (stock_id, GTK_ICON_SIZE_BUTTON);
            gtk_button_set_image (GTK_BUTTON (button), image);
        }
    } else {
        /* fall back to a stock button */
        button = gtk_button_new_with_label (stock_id);
    }
    return button;
}


/*******************/
/* libburn helpers */
/*******************/

static char * libburn_msg_prefix = "libburn-";

void
xfburn_capture_libburn_messages (void)
{
  int ret;

#ifdef DEBUG_LIBBURN
  ret = burn_msgs_set_severities ("NEVER", "DEBUG", libburn_msg_prefix);
#else
  ret = burn_msgs_set_severities ("ALL", "NEVER", libburn_msg_prefix);
#endif

  if (ret <= 0)
    g_warning ("Failed to set libburn message severities, burn errors might not get detected");
}

void
xfburn_console_libburn_messages (void)
{
  int ret;

#ifdef DEBUG_LIBBURN
  ret = burn_msgs_set_severities ("NEVER", "DEBUG", libburn_msg_prefix);
#else
  ret = burn_msgs_set_severities ("NEVER", "FATAL", libburn_msg_prefix);
#endif

  if (ret <= 0)
    g_warning ("Failed to set libburn message severities");

}

int
xfburn_media_profile_to_kb (int media_no)
{
  int factor = 1;
  if (media_no <= XFBURN_PROFILE_CDR)
    factor = CDR_1X_SPEED;
  else if (media_no >= XFBURN_PROFILE_DVD_MINUS_R && media_no <= XFBURN_PROFILE_DVD_PLUS_R_DL)
    factor = DVD_1X_SPEED;
  else if (media_no >= XFBURN_PROFILE_BD_R && media_no <= XFBURN_PROFILE_BD_RE)
    factor = BD_1X_SPEED;
  else {
    g_warning ("unknown profile, assuming BD");
    factor = BD_1X_SPEED;
  }
  return factor;
}


GSList *
xfburn_make_abosulet_file_list (gint count, gchar *filenames[])
{
  gint i;
  GSList * list = NULL;
  gchar *file, *abs, *pwd;

  pwd = g_get_current_dir ();

  for (i=0; i<count; i++) {
    file = filenames[i];

    if (!g_path_is_absolute (file))
      abs = g_build_filename (pwd, file, NULL);
    else
      abs = g_build_filename (file, NULL);

    list = g_slist_prepend (list, abs);
  }

  g_free (pwd);

  return list;
}
/* adding items to a toolbar*/
void
xfburn_add_button_to_toolbar(GtkToolbar *toolbar, const gchar *stock, const gchar *text, const gchar *action, const gchar *tooltip)
{
  GtkToolItem *item;
  GtkWidget *icon;
  GtkWidget *label;

  label = gtk_label_new(text);
  icon = gtk_image_new_from_icon_name (stock, 0);
  item = gtk_tool_button_new(icon, text);
  gtk_tool_item_set_tooltip_text(item, (tooltip == NULL) ? text : tooltip);
  gtk_tool_button_set_label_widget(GTK_TOOL_BUTTON (item), label);
  gtk_toolbar_insert(toolbar, item, -1);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action);
}

/* creates a progress bar with the initial label set to text and with the label visible */
GtkWidget *
xfburn_create_progress_bar (const gchar *text)
{
  GtkProgressBar *pbar = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  gtk_progress_bar_set_show_text (pbar, TRUE);
  if (text != NULL && *text != '\0')
    gtk_progress_bar_set_text (pbar, text);
  return GTK_WIDGET (pbar);
}

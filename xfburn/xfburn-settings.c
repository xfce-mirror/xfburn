/* $Id$ */
/*
 *  Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>

#include <libxfce4util/libxfce4util.h>

#include "xfburn-settings.h"

/* global */
static GHashTable *settings = NULL;

/* structs */
typedef enum
{
  SETTING_TYPE_INT,
  SETTING_TYPE_BOOL,
  SETTING_TYPE_STRING
} SettingType;

typedef struct
{
  SettingType type;
  union
  {
    gchar *string;
    gint integer;
    gboolean boolean;
  } value;
} Setting;

/* internals */

/* saving functions */
static void
foreach_save (gpointer key, gpointer value, FILE * file_settings)
{
  Setting *setting = (Setting *) value;
  gchar *str_type = NULL, *str_value = NULL;

  switch (setting->type) {
  case SETTING_TYPE_INT:
    str_type = g_strdup ("int");
    str_value = g_strdup_printf ("%d", setting->value.integer);
    break;
  case SETTING_TYPE_BOOL:
    str_type = g_strdup ("boolean");
    str_value = g_strdup_printf ("%s", setting->value.boolean ? "true" : "false");
    break;
  case SETTING_TYPE_STRING:
    str_type = g_strdup ("string");
    str_value = g_strdup (setting->value.string);
    break;
  }

  fprintf (file_settings, "\t<setting name=\"%s\" type=\"%s\" value=\"%s\"/>\n", (gchar *) key, str_type, str_value);

  g_free (str_type);
  g_free (str_value);
}

static void
save_settings ()
{
  FILE *file_settings;
  gchar *path;
  
  path = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, "xfburn/settings.xml", TRUE);
  if (!path) {
    g_error ("the settings saving location couldn't be found");
    return;
  }
  file_settings = fopen (path, "w+");

  fprintf (file_settings, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  fprintf (file_settings, "<xfburn-settings>\n");

  g_hash_table_foreach (settings, (GHFunc) foreach_save, file_settings);

  fprintf (file_settings, "</xfburn-settings>\n");
  fclose (file_settings);
}

/* loading functions */
static gint
_find_attribute (const gchar ** attribute_names, const gchar * attr)
{
  gint i;

  for (i = 0; attribute_names[i]; i++) {
    if (!strcmp (attribute_names[i], attr))
      return i;
  }

  return -1;
}

static void
load_settings_start (GMarkupParseContext * context, const gchar * element_name,
                     const gchar ** attribute_names, const gchar ** attribute_values,
                     gpointer user_data, GError ** error)
{
  gboolean *started = (gboolean *) user_data;
  
  if (!(*started) && !strcmp (element_name, "xfburn-settings"))
    *started = TRUE;
  else if (!(*started)) 
    return;

  if (!strcmp (element_name, "setting")) {
    int i, j, k;

    if ((i = _find_attribute (attribute_names, "name")) != -1 &&
        (j = _find_attribute (attribute_names, "type")) != -1 &&
        (k = _find_attribute (attribute_names, "value")) != -1) {

      if (!strcmp (attribute_values[j], "int"))
        xfburn_settings_set_int (g_strdup (attribute_values[i]), atoi (attribute_values[k]));
      else if (!strcmp (attribute_values[j], "boolean")) {
        if (!strcmp (attribute_values[k], "true"))
          xfburn_settings_set_boolean (g_strdup (attribute_values[i]), TRUE);
        else
          xfburn_settings_set_boolean (g_strdup (attribute_values[i]), FALSE);
      }
      else if (!strcmp (attribute_values[j], "string"))
        xfburn_settings_set_string (g_strdup (attribute_values[i]), attribute_values[k]);
    }
  }
}

static void
load_settings_end (GMarkupParseContext * context, const gchar * element_name, gpointer user_data, GError ** error)
{
  gboolean *started = (gboolean *) user_data;
  
  if (!strcmp (element_name, "xfburn-settings"))
    *started = FALSE;
}

static void
load_settings (const gchar * filename)
{
  gchar *file_contents = NULL;
  GMarkupParseContext *gpcontext = NULL;
  struct stat st;
  gboolean started = FALSE;
  GMarkupParser gmparser = { load_settings_start, load_settings_end, NULL, NULL, NULL };
  GError *err = NULL;
#ifdef HAVE_MMAP
  gint fd = -1;
  void *maddr = NULL;
#endif

  g_return_val_if_fail (filename != NULL, NULL);

  if (stat (filename, &st) < 0) {
    g_warning ("Unable to open the settings file");
    goto cleanup;
  }

#ifdef HAVE_MMAP
  fd = open (filename, O_RDONLY, 0);
  if (fd < 0)
    goto cleanup;

  maddr = mmap (NULL, st.st_size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  if (maddr)
    file_contents = maddr;
#endif

  if (!file_contents && !g_file_get_contents (filename, &file_contents, NULL, &err)) {
    if (err) {
      g_warning ("Unable to read file '%s' (%d): %s", filename, err->code, err->message);
      g_error_free (err);
    }
    goto cleanup;
  }

  gpcontext = g_markup_parse_context_new (&gmparser, 0, &started, NULL);

  if (!g_markup_parse_context_parse (gpcontext, file_contents, st.st_size, &err)) {
    g_warning ("Error parsing settings (%d): %s", err->code, err->message);
    g_error_free (err);
    goto cleanup;
  }

  if (g_markup_parse_context_end_parse (gpcontext, NULL)) {
    DBG ("parsed");
  }

cleanup:
  if (gpcontext)
    g_markup_parse_context_free (gpcontext);
#ifdef HAVE_MMAP
  if (maddr) {
    munmap (maddr, st.st_size);
    file_contents = NULL;
  }
  if (fd > -1)
    close (fd);
#endif
  if (file_contents)
    g_free (file_contents);
}

static void
value_destroy (Setting * val)
{
  if (val->type == SETTING_TYPE_STRING)
    g_free (val->value.string);
}

/* public */
void
xfburn_settings_init ()
{
  gchar *path;

  settings = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) value_destroy);

  path = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, "xfburn/");

  if (path) {
    gchar *full_path;

    full_path = g_build_filename (path, "settings.xml", NULL);
    load_settings (full_path);
DBG ("%s", full_path);
    g_free (full_path);
    g_free (path);
  }
  else {
    g_message ("no settings file found, using defaults");
  }
}

void
xfburn_settings_flush ()
{
  save_settings ();
}

void
xfburn_settings_free ()
{
  g_hash_table_destroy (settings);
}

gboolean
xfburn_settings_get_boolean (const gchar * key, gboolean fallback)
{
  gpointer orig;
  gpointer value;

  if (g_hash_table_lookup_extended (settings, key, &orig, &value)) {
    Setting *setting = (Setting *) value;

    if (setting->type == SETTING_TYPE_BOOL)
      return setting->value.boolean;
    else
      g_warning ("The %s setting isn't of type boolean", key);
  }

  return fallback;
}

gint
xfburn_settings_get_int (const gchar * key, gint fallback)
{
  gpointer orig;
  gpointer value;

  if (g_hash_table_lookup_extended (settings, key, &orig, &value)) {
    Setting *setting = (Setting *) value;

    if (setting->type == SETTING_TYPE_INT)
      return setting->value.integer;
    else
      g_warning ("The %s setting isn't of type integer", key);
  }

  return fallback;
}

gchar *
xfburn_settings_get_string (const gchar * key, const gchar * fallback)
{
  gpointer orig;
  gpointer value;

  if (g_hash_table_lookup_extended (settings, key, &orig, &value)) {
    Setting *setting = (Setting *) value;

    if (setting->type == SETTING_TYPE_STRING)
      return g_strdup (setting->value.string);
    else
      g_warning ("The %s setting isn't of type string", key);
  }

  return g_strdup (fallback);
}

void
xfburn_settings_set_boolean (const gchar * key, gboolean value)
{
  Setting *setting;

  setting = g_new0 (Setting, 1);
  setting->type = SETTING_TYPE_BOOL;
  setting->value.integer = value;

  g_hash_table_replace (settings, (gpointer) key, (gpointer) setting);
}

void
xfburn_settings_set_int (const gchar * key, gint value)
{
  Setting *setting;

  setting = g_new0 (Setting, 1);
  setting->type = SETTING_TYPE_INT;
  setting->value.integer = value;

  g_hash_table_replace (settings, (gpointer) key, (gpointer) setting);
}

void
xfburn_settings_set_string (const gchar * key, const gchar * value)
{
  Setting *setting;

  setting = g_new0 (Setting, 1);
  setting->type = SETTING_TYPE_STRING;
  setting->value.string = g_strdup (value);

  g_hash_table_replace (settings, (gpointer) key, (gpointer) setting);
}

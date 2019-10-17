/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#define XFBURN_SETTINGS_GET_PRIVATE(obj) (xfburn_settings_internal_get_instance_private (XFBURN_SETTINGS (obj)))

/* global */
typedef struct _Setting Setting;
  
static void xfburn_settings_finalize (GObject * object);

static void value_destroy (Setting * val);
static void load_settings (XfburnSettingsPrivate *priv);

/* private */
struct XfburnSettingsPrivate
{
  GHashTable *settings;
  gchar *full_path;
};

/* structs */
typedef enum
{
  SETTING_TYPE_INT,
  SETTING_TYPE_BOOL,
  SETTING_TYPE_STRING,
} SettingType;

struct _Setting
{
  SettingType type;
  union
  {
    gchar *string;
    gint integer;
    gboolean boolean;
  } value;
};

/**********************/
/* object declaration */
/**********************/
static GObjectClass *parent_class = NULL;
static XfburnSettings *instance = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(XfburnSettings, xfburn_settings_internal, G_TYPE_OBJECT);

GType
xfburn_settings_get_type(void)
{
  return xfburn_settings_internal_get_type();
}

static void
xfburn_settings_internal_class_init (XfburnSettingsClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_settings_finalize;
}

static void
xfburn_settings_finalize (GObject * object)
{
  XfburnSettings *cobj;
  XfburnSettingsPrivate *priv;

  cobj = XFBURN_SETTINGS (object);

  priv = XFBURN_SETTINGS_GET_PRIVATE (cobj);
  
  if (instance) {
    instance = NULL;
  }
  
  if (G_LIKELY (priv->settings))
    g_hash_table_destroy (priv->settings);
  if (G_LIKELY (priv->full_path))
    g_free (priv->full_path);
}

static void
xfburn_settings_internal_init (XfburnSettings *settings)
{
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);

  priv->settings = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) value_destroy);

  priv->full_path = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, "xfburn/settings.xml", TRUE);
}

/*************/
/* internals */
/*************/

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
save_settings (XfburnSettingsPrivate *priv)
{
  FILE *file_settings;
  
  file_settings = fopen (priv->full_path, "w+");

  fprintf (file_settings, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
  fprintf (file_settings, "<xfburn-settings>\n");

  g_hash_table_foreach (priv->settings, (GHFunc) foreach_save, file_settings);

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
        xfburn_settings_set_int (attribute_values[i], atoi (attribute_values[k]));
      else if (!strcmp (attribute_values[j], "boolean")) {
        if (!strcmp (attribute_values[k], "true"))
          xfburn_settings_set_boolean (attribute_values[i], TRUE);
        else
          xfburn_settings_set_boolean (attribute_values[i], FALSE);
      }
      else if (!strcmp (attribute_values[j], "string"))
        xfburn_settings_set_string (attribute_values[i], attribute_values[k]);
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
load_settings (XfburnSettingsPrivate *priv)
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

  if (stat (priv->full_path, &st) < 0) {
    g_message ("No existing settings file, using default settings");
    goto cleanup;
  }

#ifdef HAVE_MMAP
  fd = open (priv->full_path, O_RDONLY, 0);
  if (fd < 0)
    goto cleanup;

  maddr = mmap (NULL, st.st_size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  if (maddr)
    file_contents = maddr;
#endif

  if (!file_contents && !g_file_get_contents (priv->full_path, &file_contents, NULL, &err)) {
    if (err) {
      g_warning ("Unable to read file '%s' (%d): %s", priv->full_path, err->code, err->message);
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

  g_free (val);
}

static XfburnSettings*
get_instance (void)
{
  if (G_LIKELY (instance))
	return instance;
  else
	g_critical ("XfburnSettings hasn't been initialized");
  
  return NULL;
}

/******************/
/* public methods */
/******************/
void
xfburn_settings_init (void)
{
  if (G_LIKELY (instance == NULL)) {
	XfburnSettingsPrivate *priv;
	
	instance = XFBURN_SETTINGS (g_object_new (XFBURN_TYPE_SETTINGS, NULL));
	
	if (instance) {
	  priv = XFBURN_SETTINGS_GET_PRIVATE (instance);
	  load_settings (priv);
	}
  } else {
	g_critical ("XfburnSettings is already initialized");
  }
}

void
xfburn_settings_flush (void)
{
  XfburnSettings *settings = get_instance ();
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);
  
  save_settings (priv);
}

void
xfburn_settings_free (void)
{
  XfburnSettings *settings = get_instance ();
 
  g_object_unref (settings);
}

gboolean
xfburn_settings_get_boolean (const gchar * key, gboolean fallback)
{
  XfburnSettings *settings = get_instance ();
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);
  gpointer orig;
  gpointer value;

  if (g_hash_table_lookup_extended (priv->settings, key, &orig, &value)) {
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
  XfburnSettings *settings = get_instance ();
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);
  gpointer orig;
  gpointer value;

  if (g_hash_table_lookup_extended (priv->settings, key, &orig, &value)) {
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
  XfburnSettings *settings = get_instance ();
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);
  gpointer orig;
  gpointer value;

  if (g_hash_table_lookup_extended (priv->settings, key, &orig, &value)) {
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
  XfburnSettings *settings = get_instance ();
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);
  Setting *setting;

  setting = g_new0 (Setting, 1);
  setting->type = SETTING_TYPE_BOOL;
  setting->value.integer = value;

  g_hash_table_replace (priv->settings, (gpointer) g_strdup (key), (gpointer) setting);
}

void
xfburn_settings_set_int (const gchar * key, gint value)
{
  XfburnSettings *settings = get_instance ();
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);
  Setting *setting;

  setting = g_new0 (Setting, 1);
  setting->type = SETTING_TYPE_INT;
  setting->value.integer = value;

  g_hash_table_replace (priv->settings, (gpointer) g_strdup (key), (gpointer) setting);
}

void
xfburn_settings_set_string (const gchar * key, const gchar * value)
{
  XfburnSettings *settings = get_instance ();
  XfburnSettingsPrivate *priv = XFBURN_SETTINGS_GET_PRIVATE (settings);
  Setting *setting;

  setting = g_new0 (Setting, 1);
  setting->type = SETTING_TYPE_STRING;
  setting->value.string = g_strdup (value);

  g_hash_table_replace (priv->settings, (gpointer) g_strdup (key), (gpointer) setting);
}

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

#ifndef __XFBURN_SETTINGS_H__
#define __XFBURN_SETTINGS_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_SETTINGS         (xfburn_settings_get_type ())
#define XFBURN_SETTINGS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_SETTINGS, XfburnSettings))
#define XFBURN_SETTINGS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_SETTINGS, XfburnSettingsClass))
#define XFBURN_IS_SETTINGS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_SETTINGS))
#define XFBURN_IS_SETTINGS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_SETTINGS))
#define XFBURN_SETTINGS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_SETTINGS, XfburnSettingsClass))
typedef struct XfburnSettingsPrivate XfburnSettingsPrivate;

typedef struct
{
  GObject parent;
} XfburnSettings;

typedef struct
{
  GObjectClass parent_class;
} XfburnSettingsClass;

GType xfburn_settings_get_type (void);

void xfburn_settings_init (void);
void xfburn_settings_free (void);
void xfburn_settings_flush (void);

gint xfburn_settings_get_boolean (const gchar *key, gboolean fallback);
gint xfburn_settings_get_int (const gchar *key, gint fallback);
gchar *xfburn_settings_get_string (const gchar *key, const gchar *fallback);

void xfburn_settings_set_boolean (const gchar *key, gboolean value);
void xfburn_settings_set_int (const gchar *key, gint value);
void xfburn_settings_set_string (const gchar *key, const gchar *value);

G_END_DECLS
#endif

/*
 * Copyright (c) 2008 David Mohr <david@mcbf.net>
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

#ifndef __XFBURN_ERROR_H__
#define __XFBURN_ERROR_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

typedef enum 
{
  XFBURN_ERROR_NONE,
  XFBURN_ERROR_NOT_IMPLEMENTED,
  XFBURN_ERROR_STAT,
  XFBURN_ERROR_NOT_AUDIO_EXT,
  XFBURN_ERROR_NOT_AUDIO_FORMAT,
  XFBURN_ERROR_COULD_NOT_OPEN_FILE,
  XFBURN_ERROR_BURN_SOURCE,
  XFBURN_ERROR_BURN_TRACK,
  XFBURN_ERROR_GST_CREATION,
  XFBURN_ERROR_GST_STATE,
  XFBURN_ERROR_GST_TIMEOUT,
  XFBURN_ERROR_GST_DISCOVERER,
  XFBURN_ERROR_PIPE,
  XFBURN_ERROR_GST_NO_AUDIO,
  XFBURN_ERROR_TOO_MANY_AUDIO_TRACKS,
  XFBURN_ERROR_MISSING_PLUGIN,
} XfburnError;

#define XFBURN_ERROR xfburn_error_quark ()

GQuark xfburn_error_quark (void);

#endif /* __XFBURN_ERROR_H__ */

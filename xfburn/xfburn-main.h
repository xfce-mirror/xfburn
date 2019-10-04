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

#ifndef __XFBURN_MAIN_H__
#define __XFBURN_MAIN_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

G_BEGIN_DECLS

void xfburn_main_leave_window (void);
void xfburn_main_enter_window (void);
const gchar *xfburn_main_get_initial_dir ();
gboolean xfburn_main_has_initial_dir ();

G_END_DECLS
#endif /* __XFBURN_MAIN_H__ */

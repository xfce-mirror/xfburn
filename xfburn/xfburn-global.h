/* $Id$ */
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

#ifndef __XFBURN_GLOBAL_H__
#define __XFBURN_GLOBAL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define BORDER 5

#define READCD_CAPACITY "Capacity:"
#define READCD_PROGRESS "addr:"
#define READCD_DONE "Time total:"

#define CDRDAO_LENGTH "length"
#define CDRDAO_FLUSHING "Flushing cache..."
#define CDRDAO_INSERT "Please insert a recordable medium and hit enter"
#define CDRDAO_INSERT_AGAIN "Cannot determine disk status - hit enter to try again."
#define CDRDAO_DONE "CD copying finished successfully."

/* in reality CDR_1X_SPEED is 176.4 (see libburn.h:1577), but that shouldn't matter */
#define CDR_1X_SPEED 176
#define DVD_1X_SPEED 1385

#define DATA_COMPOSITION_DEFAULT_NAME "Data composition"

#define XFBURN_FIFO_DEFAULT_SIZE 4096 /* in kb/s */

#endif

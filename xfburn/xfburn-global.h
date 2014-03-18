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

#define READCD_CAPACITY N_("Capacity:")
#define READCD_PROGRESS N_("addr:")
#define READCD_DONE N_("Time total:")

#define CDRDAO_LENGTH N_("length")
#define CDRDAO_FLUSHING N_("Flushing cache...")
#define CDRDAO_INSERT N_("Please insert a recordable disc and hit enter")
#define CDRDAO_INSERT_AGAIN N_("Cannot determine disc status - hit enter to try again.")
#define CDRDAO_DONE N_("CD copying finished successfully.")

/* in reality CDR_1X_SPEED is 176.4 (see libburn.h:1577), but that shouldn't matter */
#define CDR_1X_SPEED 176
#define DVD_1X_SPEED 1385
#define BD_1X_SPEED  4496

#define DATA_COMPOSITION_DEFAULT_NAME N_("Data composition")

#define FIFO_DEFAULT_SIZE  4096    /* in kb, as int */

#define AUDIO_BYTES_PER_SECTOR 2352
#define DATA_BYTES_PER_SECTOR 2048

#define PCM_BYTES_PER_SECS 176400

#define MAXIMUM_ISO_LEVEL_2_FILE_SIZE 0xFFFFFFFF
#define MAXIMUM_ISO_FILE_SIZE 0x20000000000

/* DEBUG ONLY */

/* do not use the real device, instead make everything write to /dev/null
 * Note that this special device only supports one track!  */
//#define DEBUG_NULL_DEVICE

//#define DEBUG_LIBBURN 1

#endif

/*
 *  Copyright (c) 2005-2007 Jean-François Wauthy (pollux@xfce.org)
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

#ifndef __XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_H__
#define __XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_H__

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

#include <libisofs.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_BURN_AUDIO_CD_COMPOSITION_DIALOG (xfburn_burn_audio_cd_composition_dialog_get_type ())
G_DECLARE_FINAL_TYPE (XfburnBurnAudioCdCompositionDialog, xfburn_burn_audio_cd_composition_dialog, XFBURN, BURN_AUDIO_CD_COMPOSITION_DIALOG, XfceTitledDialog)

enum {
  XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_CANCEL = -1,
  XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_BURN,
  XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_BLANK,
};

GtkWidget *xfburn_burn_audio_cd_composition_dialog_new (GSList *tracklist);

G_END_DECLS
#endif /* XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_H */

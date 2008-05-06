/* $Id: xfburn-perform-burn.c 4557 2008-04-13 23:21:56Z squisher $ */
/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 * Copyright (c) 2008      David Mohr (squisher@xfce.org)
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

#include <glib.h>
#include <libburn.h>
#include <unistd.h>

#include "xfburn-perform-burn.h"
#include "xfburn-progress-dialog.h"

/*************/
/* internals */
/*************/


/**************/
/* public API */
/**************/
/*
void
xfburn_perform_burn_init (struct burn_disc **disc, struct burn_session **session, struct burn_track **track)
{
}
*/

void
xfburn_perform_burn_write (GtkWidget *dialog_progress, struct burn_drive *drive, XfburnWriteMode write_mode, struct burn_write_opts *burn_options, struct burn_disc *disc)
{
  enum burn_disc_status disc_state;
  enum burn_drive_status status;
  struct burn_progress progress;
  time_t time_start;
  char media_name[80];
  int media_no;
  int factor;

  int ret;
  gboolean error = FALSE;
  int error_code;
  char msg_text[BURN_MSGS_MESSAGE_LEN];
  int os_errno;
  char severity[80];
  const char *final_status_text;
  XfburnProgressDialogStatus final_status;
  const char *final_message;

  while (burn_drive_get_status (drive, NULL) != BURN_DRIVE_IDLE)
    usleep(100001);
 
  /* retrieve media type, so we can convert from 'kb/s' into 'x' rating */
  if (burn_disc_get_profile(drive, &media_no, media_name) == 1) {
    /* this will fail if newer disk types get supported */
    if (media_no <= 0x0a)
      factor = CDR_1X_SPEED;
    else
      /* assume DVD for now */
      factor = DVD_1X_SPEED;
  } else {
    g_warning ("no profile could be retrieved to calculate current burn speed");
    factor = 1;
  }

  /* Evaluate drive and media */
  while ((disc_state = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
    usleep(100001);
  if (disc_state == BURN_DISC_APPENDABLE && write_mode != WRITE_MODE_TAO) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot append data to multisession disc in this write mode (use TAO instead)"));
    return;
  } else if (disc_state != BURN_DISC_BLANK) {
    if (disc_state == BURN_DISC_FULL)
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Closed media with data detected. Need blank or appendable media"));
    else if (disc_state == BURN_DISC_EMPTY) 
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("No media detected in drive"));
    else
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot recognize state of drive and media"));
    return;
  }

  /* set us up to receive fatal errors */
  ret = burn_msgs_set_severities ("ALL", "NEVER", "libburn");

  if (ret <= 0)
    g_warning ("Failed to set libburn message severities, burn errors might not get detected!");
 
  burn_disc_write (burn_options, disc); 

  while (burn_drive_get_status (drive, NULL) == BURN_DRIVE_SPAWNING)
    usleep(1002);
  time_start = time (NULL);
  while ((status = burn_drive_get_status (drive, &progress)) != BURN_DRIVE_IDLE) {
    time_t time_now = time (NULL);

    switch (status) {
    case BURN_DRIVE_WRITING:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Burning composition..."));
      if (progress.sectors > 0 && progress.sector >= 0) {
	gdouble percent = 0.0;
        gdouble cur_speed = 0.0;

	percent = (gdouble) (progress.buffer_capacity - progress.buffer_available) / (gdouble) progress.buffer_capacity;
	xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent);

	percent = 1.0 + ((gdouble) progress.sector+1.0) / ((gdouble) progress.sectors) * 98.0;
	xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent / 100.0);

        cur_speed = ((gdouble) ((gdouble)(glong)progress.sector * 2048L) / (gdouble) (time_now - time_start)) / ((gdouble) (factor * 1000.0));
        //DBG ("(%f / %f) / %f = %f\n", (gdouble) ((gdouble)(glong)progress.sector * 2048L), (gdouble) (time_now - time_start), ((gdouble) (factor * 1000.0)), cur_speed);
	xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						  cur_speed);
      }
      break;
    case BURN_DRIVE_WRITING_LEADIN:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Writing Lead-In..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_WRITING_LEADOUT:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Writing Lead-Out..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_WRITING_PREGAP:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Writing pregap..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_CLOSING_TRACK:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Closing track..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_CLOSING_SESSION:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Closing session..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_FORMATTING:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Formatting..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    default:
      DBG ("Status %d not supported", status);
      break;
    }    
    usleep (500000);
  }

  /* check the libburn message queue for errors */
  while ((ret = burn_msgs_obtain ("FAILURE", &error_code, msg_text, &os_errno, severity)) == 1) {
    g_warning ("[%s] %d: %s (%d)", severity, error_code, msg_text, os_errno);
    error = TRUE;
  }
#ifdef DEBUG
  while ((ret = burn_msgs_obtain ("ALL", &error_code, msg_text, &os_errno, severity)) == 1) {
    g_warning ("[%s] %d: %s (%d)", severity, error_code, msg_text, os_errno);
  }
#endif

  if (ret < 0)
    g_warning ("Fatal error while trying to retrieve libburn message!");

  if (G_LIKELY (!error)) {
    final_message = _("Done");
    final_status = XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED;
  } else {
    final_status_text  = _("Failure");
    final_status = XFBURN_PROGRESS_DIALOG_STATUS_FAILED;
    final_message = g_strdup_printf ("%s: %s", final_status_text, msg_text);
  }

  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), final_status, final_message);
}

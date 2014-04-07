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

#include <libburn.h>

#include "xfburn-perform-burn.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-utils.h"

/*************/
/* internals */
/*************/

/* ts B40225 : Automatically format unformatted BD-RE and DVD-RAM
   Return: <=-1 failure, 0 = aborted, 1 = formatting happened, 2 = no formatting needed
*/
static int
xfburn_auto_format(GtkWidget *dialog_progress, struct burn_drive *drive)
{
  int ret, profile, status, num_formats;
  char profile_name[80];
  off_t size;
  unsigned int dummy;
  struct burn_progress p;
  double percent;
  int format_flag = 64 | (3 << 1); /* Fast formatting with default size */
  gboolean stop;
  gboolean stopping = FALSE;

/* Test mock-up for non-BD burners with DVD+RW
 #de fine XFBURN_DVD_PLUS_RW_FOR_BD_RE 1
*/
/* Test mock-up for already formatted DVD-RAM and BD-RE
 #de fine XFBURN_PRETEND_UNFORMATTED 1
*/
/* Test mock-up for protecting your only unformatted BD-RE from formatting.
   Will keep burning from happening and make xfburn stall, so that it has to
   be killed externally.
 #de fine XFBURN_KEEP_UNFORMATTED 1
*/

 ret = burn_disc_get_profile (drive, &profile, profile_name);

#ifdef XFBURN_DVD_PLUS_RW_FOR_BD_RE
 if (profile == 0x1a) {
    DBG("Pretending DVD+RW is a BD RE");
    profile = 0x43;           /* Pretend (for this function only) to be BD-RE */
    format_flag |= 16;        /* Re-format already formatted medium */
    strcpy(profile_name, "DVD+RW as BD-RE");
  }
#endif

  if (ret <= 0 || (profile != 0x12 &&  profile != 0x43))
    return 2;
  /* The medium is DVD-RAM or BD-RE */
  ret= burn_disc_get_formats (drive, &status, &size, &dummy, &num_formats);

#ifdef XFBURN_DVD_PLUS_RW_FOR_BD_RE
  if (strncmp(profile_name, "DVD+RW", 6) == 0) {
    DBG("Pretending to be unformatted");
    ret = 1;
    status = BURN_FORMAT_IS_UNFORMATTED;
  }
#endif
#ifdef XFBURN_PRETEND_UNFORMATTED
  DBG("Pretending to be unformatted (using re-format)");
  status = BURN_FORMAT_IS_UNFORMATTED;
  format_flag |= 16;        /* Re-format already formatted medium */
#endif

  if(ret <= 0 || status != BURN_FORMAT_IS_UNFORMATTED)
    return 2;

  /* The medium is unformatted */

  //g_object_set (dialog_progress, "animate", TRUE, NULL);
  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress),
                                               XFBURN_PROGRESS_DIALOG_STATUS_FORMATTING,
                                               _("Formatting..."));

#ifdef XFBURN_KEEP_UNFORMATTED
  g_warning ("Will not Format");
  return 0;
#endif

  /* Apply formatting */
  percent = 0.0;
  burn_disc_format (drive, 0, format_flag);
  while (burn_drive_get_status (drive, &p) != BURN_DRIVE_IDLE) {
    if (p.sectors > 0 && p.sector >= 0) {
      /* display 1 to 99 percent */
      percent = 1.0 + ((double) p.sector + 1.0) / ((double) p.sectors) * 98.0;
      xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress),
                                                        percent);
    }

    g_object_get (G_OBJECT (dialog_progress), "stop", &stop, NULL);

    if (stop && !stopping) {
      DBG ("cancelling...");
      stopping = TRUE;
      xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_STOPPING);
    }

 
    //DBG ("Formatting (%.f%%)", percent);
 
    usleep (500000);
  }

  /* Check for success */
  if (burn_drive_wrote_well (drive)) {
    DBG ("Formatting done");
 
    if (stopping) {
      return 0;
    }
    
    percent = 100.0;
 
    xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress),
                                                      percent);
  } else {
    DBG ("Formatting failed");

    return -1;
  }
  return 1;
}


/**************/
/* public API */
/**************/
/*
void
xfburn_perform_burn_init (struct burn_disc **disc, struct burn_session **session, struct burn_track **track)
{
}
*/
gboolean
xfburn_set_write_mode (struct burn_write_opts *opts, XfburnWriteMode write_mode, struct burn_disc *disc, 
                       XfburnWriteMode fallback)
{
  g_assert (fallback != WRITE_MODE_AUTO);

  /* set all other burn_write_opts before calling this!
   * See docs on burn_write_opts_auto_write_type
   */

  /* keep all modes here, just in case we want to change the default sometimes */
  if (write_mode == WRITE_MODE_AUTO) {
    char reason[BURN_REASONS_LEN];
    enum burn_write_types mode;

    mode = burn_write_opts_auto_write_type (opts, disc, reason, 0);

    if (mode != BURN_WRITE_NONE) {
      DBG("Automatically selected burn mode %d", mode);

      // write mode set, we're done
      
      return TRUE;

    } else {
      g_warning ("Could not automatically determine write mode: %s", reason);
      DBG ("This could be bad. Falling back to default %d.", fallback);

      write_mode = fallback;
      // fall through to set opts up
    }
  }

  switch (write_mode) {
  case WRITE_MODE_AUTO:
    g_error ("WRITE_MODE_AUTO not valid at this point.");
    break;
  case WRITE_MODE_TAO:
    burn_write_opts_set_write_type (opts, BURN_WRITE_TAO, BURN_BLOCK_MODE1);
    break;
  case WRITE_MODE_SAO:
    burn_write_opts_set_write_type (opts, BURN_WRITE_SAO, BURN_BLOCK_SAO);
    break;
  case WRITE_MODE_RAW16:
    burn_write_opts_set_write_type (opts, BURN_WRITE_RAW, BURN_BLOCK_RAW16);
    break;
  case WRITE_MODE_RAW96P:
    burn_write_opts_set_write_type (opts, BURN_WRITE_RAW, BURN_BLOCK_RAW96P);
    break;
  case WRITE_MODE_RAW96R:
    burn_write_opts_set_write_type (opts, BURN_WRITE_RAW, BURN_BLOCK_RAW96R);
    break;
  default:
    /*
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("The write mode is not supported currently."));
    */
    return FALSE;
  }

  return TRUE;
}

void
xfburn_perform_burn_write (GtkWidget *dialog_progress, 
                           struct burn_drive *drive, XfburnWriteMode write_mode, struct burn_write_opts *burn_options, int sector_size, 
                           struct burn_disc *disc, struct burn_source **fifos, int *track_sectors)
{
  enum burn_disc_status disc_state;
  enum burn_drive_status status;
  struct burn_progress progress;
  time_t time_start;
  char media_name[80];
  int media_no;
  int factor;

  int ret;
  int error_code;
  char msg_text[BURN_MSGS_MESSAGE_LEN];
  int os_errno;
  char severity[80];
  const char *final_status_text;
  XfburnProgressDialogStatus final_status;
  char *final_message;
  gdouble percent = 0.0;
  int dbg_no;
  int total_sectors, burned_sectors;
  off_t disc_size;
  int disc_sectors;
  gboolean stopping = FALSE;

  while (burn_drive_get_status (drive, NULL) != BURN_DRIVE_IDLE)
    usleep(100001);
 
  /* retrieve media type, so we can convert from 'kb/s' into 'x' rating */
  if (burn_disc_get_profile(drive, &media_no, media_name) == 1) {
    factor = xfburn_media_profile_to_kb (media_no);
  } else {
    g_warning ("no profile could be retrieved to calculate current burn speed");
    factor = 1;
  }

  /* Evaluate drive and media */
  while ((disc_state = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
    usleep(100001);
  if (disc_state == BURN_DISC_APPENDABLE && write_mode != WRITE_MODE_TAO) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot append data to multisession disc in this write mode (use TAO instead)."));
    return;
  } else if (disc_state != BURN_DISC_BLANK) {
    if (disc_state == BURN_DISC_FULL)
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Closed disc with data detected, a blank or appendable disc is needed."));
    else if (disc_state == BURN_DISC_EMPTY) 
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("No disc detected in drive."));
    else {
      g_warning ("Cannot recognize state of drive and disc: disc_state = %d", disc_state);
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot recognize the state of the drive and disc."));
    }
    return;
  }

  ret = xfburn_auto_format (dialog_progress, drive);
  if (ret < 0) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Formatting failed."));
    return;
  } else if (ret == 0) {
    xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_CANCELLED);
    return;
  } else if (ret == 1) {
    /* formatting happened, reset the dialog */
    xfburn_progress_dialog_reset (XFBURN_PROGRESS_DIALOG(dialog_progress));
  }

  total_sectors = burn_disc_get_sectors (disc);

  disc_size = burn_disc_available_space (drive, burn_options);

  /* available_space assumes data tracks, so we'll just compare sectors */
  disc_sectors = disc_size / DATA_BYTES_PER_SECTOR;

  if (disc_sectors < total_sectors) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("There is not enough space available on the inserted disc."));
    return;
  }

  /* set us up to receive libburn errors */
  xfburn_capture_libburn_messages ();

  /* Install the default libburn abort signal handler.
   * Hopefully this means the drive won't be left in a burning state if we catch a signal */
  burn_set_signal_handling ("xfburn", NULL, 0);

  burn_disc_write (burn_options, disc); 

  while (burn_drive_get_status (drive, NULL) == BURN_DRIVE_SPAWNING)
    usleep(1002);

  time_start = time (NULL);
  dbg_no = 0;
  burned_sectors = 0;
  while ((status = burn_drive_get_status (drive, &progress)) != BURN_DRIVE_IDLE) {
    time_t time_now = time (NULL);
    gboolean stop;
    dbg_no++;

    switch (status) {
    case BURN_DRIVE_WRITING:
      /* Is this really thread safe? It would seems so, but it's not verified */
      g_object_get (G_OBJECT (dialog_progress), "stop", &stop, NULL);

      if (stop && !stopping) {
        DBG ("cancelling...");
        burn_drive_cancel (drive);
        stopping = TRUE;
      }

      if (progress.sectors > 0 && progress.sector >= 0) {
        gdouble cur_speed = 0.0;
        int fifo_status, fifo_size, fifo_free;
        char *fifo_text;

        if (stopping) {
          xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_STOPPING);
        } else if (progress.tracks > 1) {
          gchar *str;

          str = g_strdup_printf (_("Burning track %2d/%d..."), progress.track+1, progress.tracks);
          xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, str);
          g_free (str);
        } else {
          xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Burning composition..."));
        }

	percent = (gdouble) (progress.buffer_capacity - progress.buffer_available) / (gdouble) progress.buffer_capacity * 100.0;

        /* assume that we don't have buffer info if the result is outside the sane range */
        if (percent >= 0 && percent <= 100.0)
          xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent);

        /* accumulate the sectors that have been burned in the previous track */
        if (progress.track > 0 && track_sectors[progress.track-1] > 0) {
          burned_sectors += track_sectors[progress.track-1];
          track_sectors[progress.track-1] = 0;
        }

	percent = 1.0 + ((gdouble) progress.sector + burned_sectors + 1.0) / ((gdouble) total_sectors) * 98.0;

        /*
        if ((dbg_no % 16) == 0) {
          DBG ("%.0f ; track = %d\tsector %d/%d", percent, progress.track, progress.sector, progress.sectors);
        }
        */
	xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent);

        cur_speed = ((gdouble) ((gdouble)(glong)progress.sector * 2048L) / (gdouble) (time_now - time_start)) / ((gdouble) (factor * 1000.0));
        //DBG ("(%f / %f) / %f = %f\n", (gdouble) ((gdouble)(glong)progress.sector * 2048L), (gdouble) (time_now - time_start), ((gdouble) (factor * 1000.0)), cur_speed);
	xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						  cur_speed);

        if (fifos != NULL) {
          fifo_status = burn_fifo_inquire_status (fifos[progress.track], &fifo_size, &fifo_free, &fifo_text);
          switch (fifo_status) {
            case 0:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("standby"));
              break;
            case 1:
              /* active */
              percent = (gdouble) (fifo_size - fifo_free) / (gdouble) fifo_size * 100.0;
              xfburn_progress_dialog_set_fifo_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent);
              break;
            case 2:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("ending"));
              break;
            case 3:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("failing"));
              break;
            case 4:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("unused"));
              break;
            case 5:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("abandoned"));
              break;
            case 6:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("ended"));
              break;
            case 7:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("aborted"));
              break;
            default:
              xfburn_progress_dialog_set_fifo_bar_text (XFBURN_PROGRESS_DIALOG (dialog_progress), _("no info"));
              break;
          }
        }
      } else {
        xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Burning composition..."));
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

  /* no error message by default */
#ifdef DEBUG_LIBBURN
  strcpy (msg_text, _("see console"));
#else
  msg_text[0] = '\0';
#endif

  /* check the libburn message queue for errors */
  while ((ret = burn_msgs_obtain ("FAILURE", &error_code, msg_text, &os_errno, severity)) == 1) {
    g_warning ("[%s] %d: %s (%d)", severity, error_code, msg_text, os_errno);
  }
#ifdef DEBUG
  while ((ret = burn_msgs_obtain ("ALL", &error_code, msg_text, &os_errno, severity)) == 1) {
    g_warning ("[%s] %d: %s (%d)", severity, error_code, msg_text, os_errno);
  }
#endif

  if (ret < 0)
    g_warning ("Fatal error while trying to retrieve libburn message!");

  xfburn_console_libburn_messages ();

  percent = (gdouble) progress.buffer_min_fill / (gdouble) progress.buffer_capacity * 100.0;
  xfburn_progress_dialog_set_buffer_bar_min_fill (XFBURN_PROGRESS_DIALOG (dialog_progress), percent);

  if (G_LIKELY (burn_drive_wrote_well (drive))) {
    final_message = g_strdup (_("Done"));
    final_status = XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED;
  } else {
    if (stopping) {
      final_status_text  = _("User Aborted");
      final_status = XFBURN_PROGRESS_DIALOG_STATUS_CANCELLED;
    } else {
      final_status_text  = _("Failure");
      final_status = XFBURN_PROGRESS_DIALOG_STATUS_FAILED;
    }
    if (msg_text[0] != '\0')
      final_message = g_strdup_printf ("%s: %s", final_status_text, msg_text);
    else
      final_message = g_strdup (final_status_text);
  }

  /* output it to console in case the program crashes */
  DBG ("Final burning status: %s", final_message);

  /* restore default signal handlers */
  burn_set_signal_handling (NULL, NULL, 1);

  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), final_status, final_message);
  g_free (final_message);
}

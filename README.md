xfburn
======

[xfburn's website](https://docs.xfce.org/apps/xfburn/start)

An easy to use burning software for the Xfce desktop environment (but of
course will work with others). It uses libburn and libisofs as a backend, in
difference to most other GUI programs at the moment. Visit
https://dev.lovelyhq.com/libburnia/web/wiki for more information about these libraries.

Functionality
=============

xfburn is still a new program, and does not yet do all common burning related
tasks.

Currently implemented is:
 * Create data compositions
  - Burn to CD, DVD, or BluRay (BD)
  - Create ISO images
 * Burn ISO images
 * Create and burn audio CDs
 * Blank discs
 * Format and deformat DVD-RW discs

At this time there is no multi-session support.

Note: Stream Recording disables error management for BD only, which increases
the burning speed. Using it seems to be the best option since discs with
errors tend to fail even when stream recording is disabled.

------------------------
Audio CD support details
------------------------

Included are two transcoders: basic, which just passes through uncompressed
wav data, and gst (gstreamer), which can decode any audio file for which a
gstreamer plugin is present.

The basic transcoder:
- - - - - - - - - - -
Only CD-quality, uncompressed (PCM) Wave files can be added to an audio
compilation. Use i.e. your favorite audio player with a disc writer output
mode / plugin to decompress your existing audio files. If .wav files are added
to the compilation, their headers get checked to make sure they are of the
right format. Note that this check is not very well tested (in particular it's
not likely to work on big-endian machines like PowerPC). It does not require
any external libraries

The gst transcoder:
- - - - - - - - - -
Based on the gstreamer library, it can decode pretty much any audio content,
as long as you have the appropriate plugins installed. Note that by default
most distributions do _not_ install these plugins. But a simple search for
gstreamer plugins in your package manager should quickly allow you to install
them.

You can at startup switch between the transcoders, see the command line help
for more information.


Requirements
============
 * libisofs version 0.6.2 or newer
 * libburn version 0.4.2 or newer
    * WARNING: libburn 0.4.2 - 0.5.4 are API compatible, but might trigger
            an error in libburn's fifo code.
            RECOMMENDED is version 0.5.6 or newer, where the bug was fixed.

Optional, but highly recommended
--------------------------------
 * gstreamer  (1.0+, required for burning audio CDs from compressed music
               files)
    * gstreamer pbutils (they usually come with gstreamer as far as I know)
    * gstreamer plugins (look for the good, the bad and the ugly plugin pack,
                         most likely you want all of these)

Let the **highly recommended** part be highlighted once more. Of course
gstreamer will not matter if you do not plan on burning audio CDs, but the
other two libraries will come in handy in almost all situations.

Optional, for maintenance 
-------------------------
 * libxslt (for creating docs)


The author works with both hal and thunar-vfs enabled, so there might be the
occasional bug that breaks compilation without these components. Xfburn should
work without these optional components, but it is not well tested at all.
Should something not compile or work as expected, please report a bug, and it
will get fixed.

Future Plans
============

Missing functionality that would be nice to have:
 * Save and load compositions
 * Verification
 * Copy discs (needs backend support)
 * Automatic checksum creation
 * Plugin support

There is no, and never will be, a timeline for these. Let us know if you think
something is missing on this list. If you really need something implemented,
feel free to help us out :-).

At present development is in maintenance mode, as the author does not have a
lot of time.


Bugs & Feedback
===============

Any comments are welcome! We aim to make xfburn a very easily usable program.
So if you think something could be implemented differently, feel free to speak
up and it will be considered. Please send any feedback to xfburn@xfce.org, or
use the Xfce mailing lists. Bugs are best placed in the [Xfce bugtracker](https://gitlab.xfce.org/apps/xfburn/).

Debugging
---------

If xfburn crashes, freezes, or somehow makes no progress for you, then you can
help greatly in debugging the problem. It does require you to build xfburn from
the sources, and to install gdb. If you can handle that, here are the exact
instructions:

1) Rebuild xfburn with debugging support - grab the sources, and run:
   ```
   $ meson setup build
   $ meson compile -C build
   ```
2) Enable core dumps, then run xfburn until it hangs, or crashes:
   ```
   $ ulimit -c unlimited; xfburn/xfburn 2>&1 | tee xfburn.output
   ```
3) From another terminal, kill xfburn (only if it hasn't crashed already, of course)
   ```
   $ kill -SEGV `pidof xfburn` 
   ```
4) Get a backtrace from the coredump:
   ```
   $ gdb xfburn/xfburn core.32270 --batch -ex 'thread apply all bt' > xfburn.backtrace
   ```
   (Note that sometimes the core.xxxxxx file is just called core, and that 32270 is
    just an arbitrary number which was the process id - yours will be different)
5) Open up a bug report at our [issue tracker](https://gitlab.xfce.org/apps/xfburn/) for xfburn, and add
   both the xfburn.output and the xfburn.backtrace files to it. Done!
   There is no need to attach the core dump, as it is heavily system dependent.


License
=======

This program is released under the GNU GPL version 2 or newer. See COPYING for
the full text of the license.

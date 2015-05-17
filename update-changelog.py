#!/usr/bin/env python3
# Copyright 2008 Marcus D. Hanwell <marcus@cryos.org>
#           2010,2015 David Mohr <squisher@xfce.org>
# Distributed under the terms of the GNU General Public License v2 or later
#
# Renamed from gitlog2changelog.py, from avogardro commit 8be9957e5b3b5675701ef2ed002aa9e718d4146e
# Then adjusted to my needs:
#   * Keep formatting of commit messages, but skip empty lines
#   * Newline after the files
#   * Fix linebreak at 78 chars

import re, os

# Execute git log with the desired command line options.
cmd = 'git log --summary --stat --no-merges --date=short --pretty=format:"commit %H%nAuthor: %an <%ae>%nDate: %cd%n%n%s%n"'
fin = os.popen(cmd, 'r')
# Create a ChangeLog file in the current directory.
fout = open('ChangeLog', 'w')

# Set up the loop variables in order to locate the blocks we want
authorFound = False
dateFound = False
messageFound = False
filesFound = False
message = ""
messageNL = False
files = ""
prevAuthorLine = ""

# The main part of the loop
for line in fin:
    # The commit line marks the start of a new commit object.
    if re.search('^commit', line) is not None:
        # Start all over again...
        authorFound = False
        dateFound = False
        messageFound = False
        messageNL = False
        message = ""
        filesFound = False
        files = ""
        continue
    # Match the author line and extract the part we want
    elif re.match('Author:', line) is not None:
        authorList = re.split(': ', line, 1)
        author = authorList[1]
        author = author[0:len(author)-1]
        authorFound = True
    # Match the date line
    elif re.match('Date:', line) is not None:
        dateList = re.split(': ', line, 1)
        date = dateList[1]
        date = date[0:len(date)-1]
        dateFound = True
    # The svn-id lines are ignored
    elif re.match('    git-svn-id:', line) is not None:
        continue
    # The sign off line is ignored too
    elif re.search('Signed-off-by', line) is not None:
        continue
    # Extract the actual commit message for this commit
    elif authorFound & dateFound & messageFound is False:
        # Find the commit message if we can
        if len(line) == 1:
            if messageNL:
                messageFound = True
            else:
                messageNL = True
        elif len(line) == 4:
            messageFound = True
        else:
            # strip space and tab but keep newlines
            line = line.strip('\t ')
            # swallow empty lines
            if len(line) > 1:
                message = message + '    ' + line
    # If this line is hit all of the files have been stored for this commit
    elif re.search('files? changed', line) is not None:
        filesFound = True
        continue
    # Collect the files for this commit. FIXME: Still need to add +/- to files
    elif authorFound & dateFound & messageFound:
        fileList = re.split(' \| ', line, 2)
        if len(fileList) > 1:
            if len(files) > 0:
                files = files + ", " + fileList[0].strip()
            else:
                files = fileList[0].strip()
    # All of the parts of the commit have been found - write out the entry
    if authorFound & dateFound & messageFound & filesFound:
        # First the author line, only outputted if it is the first for that
        # author on this day
        authorLine = date + "  " + author
        if len(prevAuthorLine) == 0:
            fout.write(authorLine + "\n")
        elif authorLine == prevAuthorLine:
            pass
        else:
            fout.write("\n" + authorLine + "\n")

        # Assemble the actual commit message line(s) and limit the line length
        # to 80 characters.
        commitLine = "* " + files + ":"
        i = 0
        commit = ""
        while i < len(commitLine):
            if i == 0:
                indent = '  '
                line_len = 76
            else:
                indent = '    '
                line_len = 74

            if len(commitLine) < i + line_len:
                commit = commit + "\n" + indent + commitLine[i:len(commitLine)]
                break
            index = commitLine.rfind(' ', i, i+line_len)

            if index > i:
                commit = commit + "\n" + indent + commitLine[i:index]
                i = index+1
            else:
                commit = commit + "\n" + indent + commitLine[i:line_len]
                i = i+line_len + 1

        # Write out the commit line
        fout.write(commit + "\n")

        fout.write(message)

        # Now reset all the variables ready for a new commit block.
        authorFound = False
        dateFound = False
        messageFound = False
        messageNL = False
        message = ""
        filesFound = False
        files = ""
        prevAuthorLine = authorLine

# Close the input and output lines now that we are finished.
fin.close()
fout.close()

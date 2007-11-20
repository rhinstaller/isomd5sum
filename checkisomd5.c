/*
 * checkosmd5 - simple program to check implanted md5sum
 * Copyright (C) 2001-2007 Red Hat, Inc.
 * Michael Fulbright <msf@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"
#include "libcheckisomd5.h"

int main(int argc, char **argv) {
    int i;
    int rc;
    int flags;
    int verbose;
    int gauge;
    int md5only;
    int filearg;

    if (argc < 2) {
	printf("Usage: checkisomd5 [--md5sumonly] [--verbose] [--gauge] <isofilename>|<blockdevice>\n\n");
	exit(1);
    }

    md5only = 0;
    flags = 1; /* mediaCheckFile defaults to verbose, not quiet, so prepopulate the "quiet" bit */
    verbose = 0;
    gauge = 1;
    filearg = 1;
    for (i=1; i < argc; i++) {
	if (strcmp(argv[i], "--md5sumonly") == 0) {
	    md5only = 1;
	    filearg++;
	} else if (strcmp(argv[i], "--verbose") == 0) {
	    filearg++;
	    flags ^= 1;
	    verbose = 1;
	} else if (strcmp(argv[i], "--gauge") == 0) {
	    filearg++;
	    flags ^= 2;
	    gauge = 1;
	} else 
	    break;
    }

    if (md5only|verbose)
	printMD5SUM(argv[filearg]);

    if (md5only)
	exit(0);

    rc = mediaCheckFile(argv[filearg], flags);

    /* 1 means it passed, 0 means it failed, -1 means we couldnt find chksum */
    if (rc == 1)
	exit(0);
    else
	exit(1);
}
 

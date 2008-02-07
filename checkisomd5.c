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

struct progressCBData {
    int verbose;
    int gauge;
    int gaugeat;
};

static void outputCB(void *co, long long offset, long long total) {
    struct progressCBData *data = co;
    int gaugeval = -1;

    if (data->verbose) {
        printf("\rChecking: %05.1f%%", (100.0*offset)/(total));
        fflush(stdout);
    }
    if (data->gauge) {
        gaugeval = (100.0*offset)/(total);
        if (gaugeval != data->gaugeat) {
            printf("%d\n", gaugeval);
            fflush(stdout);
            data->gaugeat = gaugeval;
        }
    }
}

int main(int argc, char **argv) {
    int i;
    int rc;
    struct progressCBData data;
    int md5only;
    int filearg;
    char * result;

    if (argc < 2) {
	printf("Usage: checkisomd5 [--md5sumonly] [--verbose] [--gauge] <isofilename>|<blockdevice>\n\n");
	exit(1);
    }

    md5only = 0;
    filearg = 1;
    memset(&data, 0, sizeof(struct progressCBData));
    for (i=1; i < argc; i++) {
	if (strcmp(argv[i], "--md5sumonly") == 0) {
	    md5only = 1;
	    filearg++;
	} else if (strcmp(argv[i], "--verbose") == 0) {
	    filearg++;
            data.verbose = 1;
	} else if (strcmp(argv[i], "--gauge") == 0) {
	    filearg++;
            data.gauge = 1;
	} else 
	    break;
    }

    if (md5only|data.verbose)
	printMD5SUM(argv[filearg]);

    if (md5only)
	exit(0);

    rc = mediaCheckFile(argv[filearg], outputCB, &data);

    if (data.verbose)
	printf("\n");

    if (rc == 0)
	result = "FAIL.\n\nIt is not recommended to use this media.";
    else if (rc > 0)
	result = "PASS.\n\nIt is OK to use this media.";
    else
	result = "NA.\n\nNo checksum information available, unable to verify media.";

    fprintf(stderr, "\nThe media check is complete, the result is: %s\n", result);

    exit (rc == 0);
}
 

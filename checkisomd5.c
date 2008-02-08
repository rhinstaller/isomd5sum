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
#include <popt.h>

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

static void usage(void) {
    fprintf(stderr, "Usage: checkisomd5 [--md5sumonly] [--verbose] [--gauge] <isofilename>|<blockdevice>\n\n");
    exit(1);
}

int main(int argc, char **argv) {
    int rc;
    const char **args;
    int md5only;
    int help;
    struct progressCBData data;
    char * result;
    poptContext optCon;

    memset(&data, 0, sizeof(struct progressCBData));

    md5only = 0;
    help = 0;
    data.verbose = 0;
    data.gauge = 0;

    struct poptOption options[] = {
	{ "md5sumonly", 'o', POPT_ARG_NONE, &md5only, 0 },
	{ "verbose", 'v', POPT_ARG_NONE, &data.verbose, 0 },
	{ "gauge", 'g', POPT_ARG_NONE, &data.gauge, 0},
	{ "help", 'h', POPT_ARG_NONE, &help, 0},
	{ 0, 0, 0, 0, 0}
    };

    optCon = poptGetContext("checkisomd5", argc, (const char **)argv, options, 0);

    if ((rc = poptGetNextOpt(optCon)) < -1) {
	fprintf(stderr, "bad option %s: %s\n",
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
		poptStrerror(rc));
	exit(1);
    }

    if (help)
	usage();

    args = poptGetArgs(optCon);
    if (!args || !args[0] || !args[0][0])
	usage();

    if (md5only|data.verbose)
	printMD5SUM((char *)args[0]);

    if (md5only)
	exit(0);

    rc = mediaCheckFile((char *)args[0], outputCB, &data);

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
 

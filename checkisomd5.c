/*
 * checkosmd5 - simple program to check implanted md5sum
 * Copyright (C) 2001-2013 Red Hat, Inc.
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
#include <termios.h>

#include "md5.h"
#include "libcheckisomd5.h"

struct progressCBData {
    int verbose;
    int gauge;
    int gaugeat;
};

int user_bailing_out(void) {
    struct timeval timev;
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    timev.tv_sec = 0;
    timev.tv_usec = 0;

    if (select(1, &rfds, NULL, NULL, &timev) && getchar() == 27)
        return 1;

    return 0;
}

static int outputCB(void *const co, const long long offset, const long long total) {
    struct progressCBData *const data = co;
    int gaugeval = -1;

    if (data->verbose) {
        printf("\rChecking: %05.1f%%", (100.0 * (double) offset) / (double) total);
        fflush(stdout);
    }
    if (data->gauge) {
        gaugeval = (int) ((100.0 * (double) offset) / (double) total);
        if (gaugeval != data->gaugeat) {
            printf("%d\n", gaugeval);
            fflush(stdout);
            data->gaugeat = gaugeval;
        }
    }
    return user_bailing_out();
}

static int usage(void) {
    fprintf(stderr, "Usage: checkisomd5 [--md5sumonly] [--verbose] [--gauge] <isofilename>|<blockdevice>\n\n");
    return 1;
}

/* Process the result code and return the proper exit status value
 *
 * return 1 for failures, 0 for good checksum and 2 if aborted.
 */
int processExitStatus(const int rc) {
    char *result;
    int exit_rc;

    switch (rc) {
        case ISOMD5SUM_CHECK_FAILED:
            result = "FAIL.\n\nIt is not recommended to use this media.";
            exit_rc = 1;
            break;
        case ISOMD5SUM_CHECK_ABORTED:
            result = "UNKNOWN.\n\nThe media check was aborted.";
            exit_rc = 2;
            break;
        case ISOMD5SUM_CHECK_NOT_FOUND:
            result = "NA.\n\nNo checksum information available, unable to verify media.";
            exit_rc = 1;
            break;
        case ISOMD5SUM_FILE_NOT_FOUND:
            result = "NA.\n\nFile not found.";
            exit_rc = 1;
            break;
        case ISOMD5SUM_CHECK_PASSED:
            result = "PASS.\n\nIt is OK to use this media.";
            exit_rc = 0;
            break;
        default:
            result = "checkisomd5 ERROR - bad return value";
            exit_rc = 1;
            break;
    }

    fprintf(stderr, "\nThe media check is complete, the result is: %s\n", result);

    return exit_rc;
}

int main(int argc, char **argv) {
    struct progressCBData data;
    memset(&data, 0, sizeof(data));
    data.verbose = 0;
    data.gauge = 0;

    int md5only = 0;
    int help = 0;

    struct poptOption options[] = {
        { "md5sumonly", 'o', POPT_ARG_NONE, &md5only, 0 },
        { "verbose", 'v', POPT_ARG_NONE, &data.verbose, 0 },
        { "gauge", 'g', POPT_ARG_NONE, &data.gauge, 0 },
        { "help", 'h', POPT_ARG_NONE, &help, 0 },
        { 0, 0, 0, 0, 0 }
    };

    poptContext optCon = poptGetContext("checkisomd5", argc, (const char **) argv, options, 0);

    int rc = poptGetNextOpt(optCon);
    if (rc < -1) {
        fprintf(stderr, "bad option %s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        return 1;
    }

    if (help) {
        poptFreeContext(optCon);
        return usage();
    }

    const char **args = poptGetArgs(optCon);
    if (!args || !args[0] || !args[0][0]) {
        poptFreeContext(optCon);
        return usage();
    }

    if (md5only | data.verbose) {
        rc = printMD5SUM((char *) args[0]);
        if (rc < 0) {
            poptFreeContext(optCon);
            return processExitStatus(rc);
        }
    }

    if (md5only) {
        poptFreeContext(optCon);
        return 0;
    }

    printf("Press [Esc] to abort check.\n");

    static struct termios oldt;
    struct termios newt;
    tcgetattr(0, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO | ECHONL | ISIG | IEXTEN);
    tcsetattr(0, TCSANOW, &newt);
    rc = mediaCheckFile((char *) args[0], outputCB, &data);
    tcsetattr(0, TCSANOW, &oldt);

    if (data.verbose)
        printf("\n");

    poptFreeContext(optCon);
    return processExitStatus(rc);
}

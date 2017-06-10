/*
 * simple program to insert an md5sum into application data area of iso99660
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
#include <popt.h>

#include "md5.h"
#include "libimplantisomd5.h"

static int usage(void) {
    fprintf(stderr, "implantisomd5:         implantisomd5 [--force] [--supported-iso] <isofilename>\n");
    return 1;
}

int main(int argc, const char **argv) {
    char *errstr;

    int forceit = 0;
    int supported = 0;
    int help = 0;

    struct poptOption options[] = {
        { "force", 'f', POPT_ARG_NONE, &forceit, 0 },
        { "supported-iso", 'S', POPT_ARG_NONE, &supported, 0 },
        { "help", 'h', POPT_ARG_NONE, &help, 0 },
        { 0, 0, 0, 0, 0 }
    };

    poptContext optCon = poptGetContext("implantisomd5", argc, argv, options, 0);

    int rc;
    if ((rc = poptGetNextOpt(optCon)) < -1) {
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

    rc = implantISOFile(args[0], supported, forceit, 0, &errstr);
    if (rc) {
        fprintf(stderr, "ERROR: ");
        fprintf(stderr, errstr, args[0]);
        rc = 1;
    }
    poptFreeContext(optCon);
    return rc;
}

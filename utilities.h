/*
 * Copyright (C) 2001-2017 Red Hat, Inc.
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
#ifndef ISOMD5_UTILITIES_H
#define ISOMD5_UTILITIES_H

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#include "md5.h"

#ifdef _WIN32
int getpagesize() { return 2048; }
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define BLOCK_SIZE 512
#define HASH_SIZE 32
/* Length in characters of string used for fragment md5sum checking */
#define FRAGMENT_SUM_SIZE 60UL
/* FRAGMENT_COUNT must be an integral divisor or FRAGMENT_SUM_SIZE */
/* 60 => 2, 3, 4, 5, 6, 10, 12, 15, 20, or 30 */
#define FRAGMENT_COUNT 20UL
/* Size offset according to ECMA-119 8.4.8 volume space size in big endian
 * format. */
#define SIZE_OFFSET 84
/* Number of sectors to ignore at end of iso when computing sum. These are
 * ignored due to problems reading last few sectors on burned CDs. */
#define SKIPSECTORS 15LL
#define SECTOR_SIZE 2048LL
#define NUM_SYSTEM_SECTORS 16LL
#define SYSTEM_AREA_SIZE (NUM_SYSTEM_SECTORS * SECTOR_SIZE)
/* According to ECMA-119 8.4.32 */
#define APPDATA_OFFSET 883LL
#define APPDATA_SIZE 512

struct volume_info {
    char hashsum[HASH_SIZE + 1];
    char fragmentsums[FRAGMENT_SUM_SIZE + 1];
    size_t supported;
    size_t fragmentcount;
    off_t offset;
    off_t isosize;
    off_t skipsectors;
};

off_t primary_volume_size(const int isofd, off_t *const offset);

struct volume_info *const parsepvd(const int isofd);

bool validate_fragment(const MD5_CTX *const hashctx, const size_t fragment,
                       const size_t fragmentsize, const char *const fragmentsums, char *const hashsums);

void md5sum(char *const hashsum, MD5_CTX *const hashctx);

#endif /* ISOMD5_UTILITIES_H */

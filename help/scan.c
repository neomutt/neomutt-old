/**
 * @file
 * Scan a directory with nftw
 *
 * @authors
 * Copyright (C) 2019 Tran Manh Tu <xxlaguna93@gmail.com>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page help_scan Scan a directory with nftw
 *
 * Scan a directory with nftw
 */

#include <ftw.h>
#include <stdio.h>
#include <string.h>
#include "private.h"
#include "mutt/lib.h"

struct stat;

static struct StringArray *AllDocs; ///< All valid help documents within $help_doc_dir folder

/**
 * add_file - Callback for nftw whenever a file is read
 * @param fpath  Filename
 * @param sb     Timestamp for the file
 * @param tflag  File type
 * @param ftwbuf Private nftw data
 *
 * @sa https://linux.die.net/man/3/nftw
 *
 * @note Only save files ending '.md'
 */
static int add_file(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
  if (tflag == FTW_F)
  {
    char *ext = strrchr(fpath, '.');
    if (ext && mutt_str_equal(ext, ".md"))
    {
      ARRAY_ADD(AllDocs, mutt_str_dup(fpath));
    }
  }
  return 0; // Tell nftw() to continue
}

/**
 * scan_dir - Scan a directory for markdown files
 * @param path     Absolute path of a directory
 * @param all_docs Array for the results
 *
 * Scan a directory recursively using nftw to find all files ending in '.md'
 */
void scan_dir(const char *path, struct StringArray *all_docs)
{
  if (!path || !all_docs)
    return;

  AllDocs = all_docs;

  // Max of 20 open file handles, 0 flags
  if (nftw(path, add_file, 20, 0) == -1)
    mutt_perror("Error scanning Help Doc directory");

  AllDocs = NULL;
}

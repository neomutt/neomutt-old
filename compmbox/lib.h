/**
 * @file
 * Compressed mbox local mailbox type
 *
 * @authors
 * Copyright (C) 1997 Alain Penders <Alain@Finale-Dev.com>
 * Copyright (C) 2016 Richard Russon <rich@flatcap.org>
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
 * @page lib_compmbox Compressed Mailbox
 *
 * Compressed mbox local mailbox type
 *
 * | File                | Description                |
 * | :------------------ | :------------------------- |
 * | compmbox/compress.c | @subpage compmbox_compress |
 */

#ifndef MUTT_COMPMBOX_LIB_H
#define MUTT_COMPMBOX_LIB_H

#include <stdbool.h>
#include <stdio.h>
#include "core/lib.h"

/**
 * struct CompressInfo - Private data for compress
 *
 * This object gets attached to the Mailbox.
 */
struct CompressInfo
{
  struct ExpandoRecord *cmd_append;        ///< append-hook command
  struct ExpandoRecord *cmd_close;         ///< close-hook  command
  struct ExpandoRecord *cmd_open;          ///< open-hook   command
  long size;                               ///< size of the compressed file
  const struct MxOps *child_ops;           ///< callbacks of de-compressed file
  bool locked;                             ///< if realpath is locked
  FILE *fp_lock;                           ///< fp used for locking
};

void mutt_comp_init(void);
bool mutt_comp_can_append(struct Mailbox *m);
bool mutt_comp_can_read(const char *path);
int  mutt_comp_valid_command(const char *cmd);

extern const struct MxOps MxCompOps;

#endif /* MUTT_COMPMBOX_LIB_H */

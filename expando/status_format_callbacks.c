/**
 * @file
 * XXX
 *
 * @authors
 * Copyright (C) 2023 Tóth János <gomba007@gmail.com>
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
 * @page expando_index_format XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include "config/lib.h"
#include "core/neomutt.h"
#include "globals.h"
#include "index/shared_data.h"
#include "node.h"
#include "status.h"
#include "status_format_callbacks.h"

void status_r(const struct ExpandoNode *self, char **buffer, int *buffer_len,
              int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  size_t i = 0;

  if (mailbox)
  {
    i = OptAttachMsg ? 3 :
                       ((mailbox->readonly || mailbox->dontwrite) ? 2 :
                        (mailbox->changed ||
                         /* deleted doesn't necessarily mean changed in IMAP */
                         (mailbox->type != MUTT_IMAP && mailbox->msg_deleted)) ?
                                                                    1 :
                                                                    0);
  }

  const struct MbTable *c_status_chars = cs_subset_mbtable(NeoMutt->sub, "status_chars");

  int printed = 0;
  if (!c_status_chars || !c_status_chars->len)
    *buffer[0] = '\0';
  else if (i >= c_status_chars->len)
    printed = snprintf(*buffer, *buffer_len, "%s", c_status_chars->chars[0]);
  else
    printed = snprintf(*buffer, *buffer_len, "%s", c_status_chars->chars[i]);

  *start_col += mb_strwidth_range(*buffer, *buffer + printed);
  *buffer_len -= printed;
  *buffer += printed;
}
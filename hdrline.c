/**
 * @file
 * String processing routines to generate the mail index
 *
 * @authors
 * Copyright (C) 1996-2000,2002,2007 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2016 Richard Russon <rich@flatcap.org>
 * Copyright (C) 2016 Ian Zimmerman <itz@primate.net>
 * Copyright (C) 2019 Pietro Cerutti <gahr@gahr.ch>
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
 * @page neo_hdrline String processing routines to generate the mail index
 *
 * String processing routines to generate the mail index
 */

#include "config.h"
#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mutt/lib.h"
#include "address/lib.h"
#include "config/lib.h"
#include "email/lib.h"
#include "core/lib.h"
#include "alias/lib.h"
#include "gui/lib.h"
#include "attach/lib.h"
#include "color/lib.h"
#include "expando/lib.h"
#include "ncrypt/lib.h"
#include "format_flags.h"
#include "hdrline.h"
#include "hook.h"
#include "maillist.h"
#include "mutt_thread.h"
#include "muttlib.h"
#include "mx.h"
#include "sort.h"
#include "subjectrx.h"
#ifdef USE_NOTMUCH
#include "notmuch/lib.h"
#endif

/**
 * mutt_make_string - Create formatted strings using mailbox expandos
 * @param buf      Buffer for the result
 * @param buflen   Buffer length
 * @param cols     Number of screen columns (OPTIONAL)
 * @param s        printf-line format string
 * @param m        Mailbox
 * @param inpgr    Message shown in the pager
 * @param e        Email
 * @param flags    Flags, see #MuttFormatFlags
 * @param progress Pager progress string
 *
 * @sa index_format_str()
 */
void mutt_make_string(char *buf, size_t buflen, int cols, const char *s,
                      struct Mailbox *m, int inpgr, struct Email *e,
                      MuttFormatFlags flags, const char *progress)
{
  assert(0 && "Remove mutt_make_string");
}

/**
 * mutt_make_string - Create formatted strings using mailbox expandos
 * @param buf      Buffer for the result
 * @param buflen   Buffer length
 * @param cols     Number of screen columns (OPTIONAL)
 * @param record   ExpandoRecord containing expando tree
 * @param m        Mailbox
 * @param inpgr    Message shown in the pager
 * @param e        Email
 * @param flags    Flags, see #MuttFormatFlags
 * @param progress Pager progress string
 *
 * @sa index_format_str()
 */
void mutt_make_string_2gmb(char *buf, size_t buflen, int cols,
                           const struct ExpandoRecord *record,
                           struct Mailbox *m, int inpgr, struct Email *e,
                           MuttFormatFlags flags, const char *progress)
{
  struct HdrFormatInfo hfi = { 0 };

  hfi.email = e;
  hfi.mailbox = m;
  hfi.msg_in_pager = inpgr;
  hfi.pager_progress = progress;

  mutt_expando_format_2gmb(buf, buflen, 0, cols, &record->tree, (intptr_t) &hfi, flags);
}

/**
 * @file
 * Maildir local mailbox type
 *
 * @authors
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
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
 * @page lib_maildir Maildir Mailbox
 *
 * Maildir local mailbox type
 *
 * | File               | Description               |
 * | :----------------- | :------------------------ |
 * | maildir/config.c   | @subpage maildir_config   |
 * | maildir/edata.c    | @subpage maildir_edata    |
 * | maildir/maildir.c  | @subpage maildir_maildir  |
 * | maildir/mdata.c    | @subpage maildir_mdata    |
 * | maildir/mdemail.c  | @subpage maildir_mdemail  |
 * | maildir/mh.c       | @subpage maildir_mh       |
 * | maildir/sequence.c | @subpage maildir_sequence |
 * | maildir/shared.c   | @subpage maildir_shared   |
 */

#ifndef MUTT_MAILDIR_LIB_H
#define MUTT_MAILDIR_LIB_H

#include <stdbool.h>
#include <stdio.h>
#include "core/lib.h"

struct Buffer;
struct Email;
struct HeaderCache;

extern const struct MxOps MxMaildirOps;
extern const struct MxOps MxMhOps;

int           maildir_check_empty      (struct Buffer *path);
struct Email *maildir_email_new        (void);
void          maildir_gen_flags        (char *dest, size_t destlen, struct Email *e);
bool          maildir_msg_open_new     (struct Mailbox *m, struct Message *msg, const struct Email *e);
FILE *        maildir_open_find_message(const char *folder, const char *msg, char **newname);
void          maildir_parse_flags      (struct Email *e, const char *path);
bool          maildir_parse_message    (enum MailboxType type, const char *fname, bool is_old, struct Email *e);
bool          maildir_parse_stream     (enum MailboxType type, FILE *fp, const char *fname, long size, bool is_old, struct Email *e);
bool          maildir_sync_mailbox_message(struct Mailbox *m, struct Email *e, struct HeaderCache *hc);
bool          maildir_update_flags     (struct Mailbox *m, struct Email *e_old, struct Email *e_new);
int           mh_check_empty           (struct Buffer *path);
int           mh_sync_mailbox_message  (struct Mailbox *m, struct Email *e, struct HeaderCache *hc);

#endif /* MUTT_MAILDIR_LIB_H */

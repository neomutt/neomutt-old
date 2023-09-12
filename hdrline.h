/**
 * @file
 * String processing routines to generate the mail index
 *
 * @authors
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
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

#ifndef MUTT_HDRLINE_H
#define MUTT_HDRLINE_H

#include <stdio.h>
#include "expando/lib.h"

struct Email;
struct Mailbox;

/**
 * struct HdrFormatInfo - Data passed to index_format_str()
 */
struct HdrFormatInfo
{
  struct Mailbox *mailbox;    ///< Current Mailbox
  int msg_in_pager;           ///< Index of Email displayed in the Pager
  struct Email *email;        ///< Current Email
  const char *pager_progress; ///< String representing Pager position through Email
};

/**
 * enum FlagChars - Index into the `$flag_chars` config variable
 */
enum FlagChars
{
  FLAG_CHAR_TAGGED,           ///< Character denoting a tagged email
  FLAG_CHAR_IMPORTANT,        ///< Character denoting a important (flagged) email
  FLAG_CHAR_DELETED,          ///< Character denoting a deleted email
  FLAG_CHAR_DELETED_ATTACH,   ///< Character denoting a deleted attachment
  FLAG_CHAR_REPLIED,          ///< Character denoting an email that has been replied to
  FLAG_CHAR_OLD,              ///< Character denoting an email that has been read
  FLAG_CHAR_NEW,              ///< Character denoting an unread email
  FLAG_CHAR_OLD_THREAD,       ///< Character denoting a thread of emails that has been read
  FLAG_CHAR_NEW_THREAD,       ///< Character denoting a thread containing at least one new email
  FLAG_CHAR_SEMPTY,           ///< Character denoting a read email, $index_format %S expando
  FLAG_CHAR_ZEMPTY,           ///< Character denoting a read email, $index_format %Z expando
};

/**
 * enum CryptChars - Index into the `$crypt_chars` config variable
 */
enum CryptChars
{
  FLAG_CHAR_CRYPT_GOOD_SIGN,      ///< Character denoting a message signed with a verified key
  FLAG_CHAR_CRYPT_ENCRYPTED,      ///< Character denoting a message is PGP-encrypted
  FLAG_CHAR_CRYPT_SIGNED,         ///< Character denoting a message is signed
  FLAG_CHAR_CRYPT_CONTAINS_KEY,   ///< Character denoting a message contains a PGP key
  FLAG_CHAR_CRYPT_NO_CRYPTO,      ///< Character denoting a message has no cryptography information
};

/**
 * enum ToChars - Index into the `$to_chars` config variable
 */
enum ToChars
{
  FLAG_CHAR_TO_NOT_IN_THE_LIST,   ///< Character denoting that the user is not in list
  FLAG_CHAR_TO_UNIQUE,            ///< Character denoting that the user is unique recipient
  FLAG_CHAR_TO_TO,                ///< Character denoting that the user is in the TO list
  FLAG_CHAR_TO_CC,                ///< Character denoting that the user is in the CC list
  FLAG_CHAR_TO_ORIGINATOR,        ///< Character denoting that the user is originator
  FLAG_CHAR_TO_SUBSCRIBED_LIST,   ///< Character denoting that the message is sent to a subscribed mailing list
  FLAG_CHAR_TO_REPLY_TO,          ///< Character denoting that the user is in the Reply-To list
};

/**
 * enum FieldType - Header types
 *
 * Strings for printing headers
 */
enum FieldType
{
  DISP_TO,    ///< To: string
  DISP_CC,    ///< Cc: string
  DISP_BCC,   ///< Bcc: string
  DISP_FROM,  ///< From: string
  DISP_PLAIN, ///< Empty string
  DISP_MAX,
};

void mutt_make_string(char *buf, size_t buflen, size_t cols, const struct ExpandoRecord *r,
                      struct Mailbox *m, int inpgr, struct Email *e,
                      MuttFormatFlags flags, const char *progress);

#endif /* MUTT_HDRLINE_H */

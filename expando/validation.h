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

#ifndef MUTT_EXPANDO_VALIDATION_H
#define MUTT_EXPANDO_VALIDATION_H

#include <stdbool.h>
#include "format_callbacks.h"

struct Buffer;
struct ConfigSet;
struct ConfigDef;

struct ExpandoFormatCallback
{
  const char *name;
  expando_format_callback callback;
};

enum ExpandoFormatIndex
{
  EFMTI_ALIAS_FORMAT = 0,
  EFMTI_ATTACH_FORMAT,
  EFMTI_ATTRIBUTION_INTRO,
  EFMTI_ATTRIBUTION_TRAILER,
  EFMTI_AUTOCRYPT_ACCT_FORMAT,
  EFMTI_COMPOSE_FORMAT,
  EFMTI_COMPRESS_FORMAT, // hook-only
  EFMTI_FOLDER_FORMAT,
  EFMTI_FORWARD_ATTRIBUTION_INTRO,
  EFMTI_FORWARD_ATTRIBUTION_TRAILER,
  EFMTI_FORWARD_FORMAT,
  EFMTI_GREETING,
  EFMTI_GROUP_INDEX_FORMAT,
  EFMTI_HISTORY_FORMAT,
  EFMTI_INDENT_STRING,
  EFMTI_INDEX_FORMAT,
  EFMTI_INEWS,
  EFMTI_MAILBOX_FOLDER_FORMAT,
  EFMTI_MESSAGE_FORMAT,
  EFMTI_MIX_ENTRY_FORMAT,
  EFMTI_NEWSRC,
  EFMTI_NEW_MAIL_COMMAND,
  EFMTI_PAGER_FORMAT,
  EFMTI_PATTERN_FORMAT,
  EFMTI_PGP_CLEAR_SIGN_COMMAND,
  EFMTI_PGP_DECODE_COMMAND,
  EFMTI_PGP_DECRYPT_COMMAND,
  EFMTI_PGP_ENCRYPT_ONLY_COMMAND,
  EFMTI_PGP_ENCRYPT_SIGN_COMMAND,
  EFMTI_PGP_ENTRY_FORMAT_DLG_GPGME,
  EFMTI_PGP_ENTRY_FORMAT_DLG_PGP,
  EFMTI_PGP_EXPORT_COMMAND,
  EFMTI_PGP_GET_KEYS_COMMAND,
  EFMTI_PGP_IMPORT_COMMAND,
  EFMTI_PGP_LIST_PUBRING_COMMAND,
  EFMTI_PGP_LIST_SECRING_COMMAND,
  EFMTI_PGP_SIGN_COMMAND,
  EFMTI_PGP_VERIFY_COMMAND,
  EFMTI_PGP_VERIFY_KEY_COMMAND,
  EFMTI_QUERY_FORMAT,
  EFMTI_SIDEBAR_FORMAT,
  EFMTI_SMIME_DECRYPT_COMMAND,
  EFMTI_SMIME_ENCRYPT_COMMAND,
  EFMTI_SMIME_GET_CERT_COMMAND,
  EFMTI_SMIME_GET_CERT_EMAIL_COMMAND,
  EFMTI_SMIME_GET_SIGNER_CERT_COMMAND,
  EFMTI_SMIME_IMPORT_CERT_COMMAND,
  EFMTI_SMIME_PK7OUT_COMMAND,
  EFMTI_SMIME_SIGN_COMMAND,
  EFMTI_SMIME_VERIFY_COMMAND,
  EFMTI_SMIME_VERIFY_OPAQUE_COMMAND,
  EFMTI_STATUS_FORMAT,
  EFMTI_TS_ICON_FORMAT,
  EFMTI_TS_STATUS_FORMAT,
  EFMTI_FORMAT_COUNT_OR_DEBUG,
};

struct ExpandoValidation
{
  const char *name;
  const struct ExpandoFormatCallback *valid_short_expandos;
  const struct ExpandoFormatCallback *valid_two_char_expandos;
  const struct ExpandoFormatCallback *valid_long_expandos;
};

struct ExpandoRecord;

int expando_validator(const struct ConfigSet *cs, const struct ConfigDef *cdef, intptr_t value, struct Buffer *err);
bool expando_revalidate(struct ExpandoRecord *record, int index);

#endif /* MUTT_EXPANDO_VALIDATION_H */

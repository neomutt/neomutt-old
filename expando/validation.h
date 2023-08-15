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
  EFMT_ALIAS_FORMAT = 0,
  EFMT_ATTACH_FORMAT,
  EFMT_ATTRIBUTION_INTRO,
  EFMT_ATTRIBUTION_TRAILER,
  EFMT_AUTOCRYPT_ACCT_FORMAT,
  EFMT_COMPOSE_FORMAT,
  EFMT_COMPRESS_FORMAT, // no config
  EFMT_FOLDER_FORMAT,
  EFMT_FORWARD_ATTRIBUTION_INTRO,
  EFMT_FORWARD_ATTRIBUTION_TRAILER,
  EFMT_FORWARD_FORMAT,
  EFMT_GREETING,
  EFMT_GROUP_INDEX_FORMAT,
  EFMT_HISTORY_FORMAT, // no config
  EFMT_INDENT_STRING,
  EFMT_INDEX_FORMAT,
  EFMT_INEWS,
  EFMT_MAILBOX_FOLDER_FORMAT,
  EFMT_MESSAGE_FORMAT,
  EFMT_MIX_ENTRY_FORMAT,
  EFMT_NEWSRC,
  EFMT_NEW_MAIL_COMMAND,
  EFMT_PAGER_FORMAT,
  EFMT_PATTERN_FORMAT,
  EFMT_PGP_CLEAR_SIGN_COMMAND,
  EFMT_PGP_DECODE_COMMAND,
  EFMT_PGP_DECRYPT_COMMAND,
  EFMT_PGP_ENCRYPT_ONLY_COMMAND,
  EFMT_PGP_ENCRYPT_SIGN_COMMAND,
  EFMT_PGP_ENTRY_FORMAT,
  EFMT_PGP_EXPORT_COMMAND,
  EFMT_PGP_GET_KEYS_COMMAND,
  EFMT_PGP_IMPORT_COMMAND,
  EFMT_PGP_LIST_PUBRING_COMMAND,
  EFMT_PGP_LIST_SECRING_COMMAND,
  EFMT_PGP_SIGN_COMMAND,
  EFMT_PGP_VERIFY_COMMAND,
  EFMT_PGP_VERIFY_KEY_COMMAND,
  EFMT_QUERY_FORMAT,
  EFMT_SIDEBAR_FORMAT,
  EFMT_SMIME_DECRYPT_COMMAND,
  EFMT_SMIME_ENCRYPT_COMMAND,
  EFMT_SMIME_ENCRYPT_WITH,
  EFMT_SMIME_GET_CERT_COMMAND,
  EFMT_SMIME_GET_CERT_EMAIL_COMMAND,
  EFMT_SMIME_GET_SIGNER_CERT_COMMAND,
  EFMT_SMIME_IMPORT_CERT_COMMAND,
  EFMT_SMIME_PK7OUT_COMMAND,
  EFMT_SMIME_SIGN_COMMAND,
  EFMT_SMIME_SIGN_DIGEST_ALG,
  EFMT_SMIME_VERIFY_COMMAND,
  EFMT_SMIME_VERIFY_OPAQUE_COMMAND,
  EFMT_STATUS_FORMAT,
  EFMT_TS_ICON_FORMAT,
  EFMT_TS_STATUS_FORMAT,
  EFMT_FORMAT_COUNT_OR_DEBUG,
};

struct ExpandoValidation
{
  const char *name;
  const struct ExpandoFormatCallback *valid_short_expandos;
  const struct ExpandoFormatCallback *valid_two_char_expandos;
  const struct ExpandoFormatCallback *valid_long_expandos;
};

int expando_validator(const struct ConfigSet *cs, const struct ConfigDef *cdef, intptr_t value, struct Buffer *err);

#endif /* MUTT_EXPANDO_VALIDATION_H */

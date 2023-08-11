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
 * @page expando_validation XXX
 *
 * XXX
 */

#include "config.h"
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include "mutt/lib.h"
#include "core/neomutt.h"
#include "global_table.h"
#include "index_format_callbacks.h"
#include "parser.h"
#include "status_format_callbacks.h"
#include "validation.h"

static const struct ExpandoFormatCallback alias_1[] = {
  { "a", NULL }, { "c", NULL }, { "f", NULL },  { "n", NULL },
  { "r", NULL }, { "t", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback attach_1[] = {
  { "c", NULL }, { "C", NULL }, { "d", NULL },  { "D", NULL }, { "e", NULL },
  { "f", NULL }, { "F", NULL }, { "I", NULL },  { "m", NULL }, { "M", NULL },
  { "n", NULL }, { "Q", NULL }, { "s", NULL },  { "t", NULL }, { "T", NULL },
  { "u", NULL }, { "X", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback autocrypt_acct_1[] = {
  { "a", NULL }, { "k", NULL }, { "n", NULL },
  { "p", NULL }, { "s", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback compose_1[] = {
  { "a", NULL }, { "h", NULL }, { "l", NULL }, { "v", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback pgp_entry_1[] = {
  { "n", NULL }, { "p", NULL },  { "t", NULL }, { "u", NULL }, { "a", NULL },
  { "A", NULL }, { "c", NULL },  { "C", NULL }, { "f", NULL }, { "F", NULL },
  { "i", NULL }, { "I", NULL },  { "k", NULL }, { "K", NULL }, { "l", NULL },
  { "L", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback folder_1[] = {
  { "C", NULL }, { "d", NULL }, { "D", NULL }, { "f", NULL }, { "F", NULL },
  { "g", NULL }, { "i", NULL }, { "l", NULL }, { "m", NULL }, { "n", NULL },
  { "N", NULL }, { "s", NULL }, { "t", NULL }, { "u", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback greeting_1[] = {
  { "n", NULL },
  { "u", NULL },
  { "v", NULL },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback group_index_1[] = {
  { "C", NULL }, { "d", NULL }, { "f", NULL }, { "M", NULL },
  { "N", NULL }, { "n", NULL }, { "s", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback index_1[] = {
  { "a", NULL },    { "A", NULL },    { "b", NULL },    { "B", NULL },
  { "c", index_c }, { "C", index_C }, { "d", NULL },    { "D", NULL },
  { "e", NULL },    { "E", NULL },    { "f", NULL },    { "F", NULL },
  { "g", NULL },    { "H", NULL },    { "i", NULL },    { "I", NULL },
  { "J", NULL },    { "K", NULL },    { "l", index_l }, { "L", index_L },
  { "m", NULL },    { "M", NULL },    { "n", NULL },    { "N", NULL },
  { "O", NULL },    { "P", NULL },    { "q", NULL },    { "r", NULL },
  { "R", NULL },    { "s", index_s }, { "S", NULL },    { "t", NULL },
  { "T", NULL },    { "u", NULL },    { "v", NULL },    { "W", NULL },
  { "x", NULL },    { "X", NULL },    { "y", NULL },    { "Y", NULL },
  { "Z", index_Z }, { NULL, NULL },
};

static const struct ExpandoFormatCallback index_2[] = {
  { "cr", index_cr }, { "Fp", NULL }, { "Gx", NULL }, { "zc", NULL },
  { "zs", NULL },     { "zt", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback mix_entry_1[] = {
  { "a", NULL }, { "c", NULL }, { "n", NULL }, { "s", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback inews_1[] = {
  { "a", NULL }, { "p", NULL }, { "P", NULL },  { "s", NULL },
  { "S", NULL }, { "u", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback pattern_1[] = {
  { "d", NULL },
  { "e", NULL },
  { "n", NULL },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback pgp_command_1[] = {
  { "a", NULL }, { "f", NULL }, { "p", NULL },
  { "r", NULL }, { "s", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback query_1[] = {
  { "a", NULL }, { "c", NULL }, { "e", NULL },
  { "n", NULL }, { "t", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback sidebar_1[] = {
  { "!", NULL }, { "B", NULL }, { "d", NULL }, { "D", NULL },  { "F", NULL },
  { "L", NULL }, { "n", NULL }, { "N", NULL }, { "o", NULL },  { "r", NULL },
  { "S", NULL }, { "t", NULL }, { "Z", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback smime_command_1[] = {
  { "a", NULL }, { "c", NULL }, { "C", NULL }, { "d", NULL },  { "f", NULL },
  { "i", NULL }, { "k", NULL }, { "s", NULL }, { NULL, NULL },
};

static const struct ExpandoFormatCallback status_1[] = {
  { "b", NULL }, { "d", NULL }, { "D", NULL },     { "f", NULL },
  { "F", NULL }, { "h", NULL }, { "l", NULL },     { "L", NULL },
  { "m", NULL }, { "M", NULL }, { "n", NULL },     { "o", NULL },
  { "p", NULL }, { "P", NULL }, { "r", status_r }, { "R", NULL },
  { "s", NULL }, { "S", NULL }, { "t", NULL },     { "T", NULL },
  { "u", NULL }, { "v", NULL }, { "V", NULL },     { NULL, NULL },
};

const struct ExpandoValidation expando_validation[EFMT_FORMAT_COUNT_OR_DEBUG] = {
  // clang-format off
  [EFMT_ALIAS_FORMAT]                  = { "alias_format",                  alias_1,          NULL },
  [EFMT_ATTACH_FORMAT]                 = { "attach_format",                 attach_1,         NULL },
  [EFMT_ATTRIBUTION_INTRO]             = { "attribution_intro",             index_1,          index_2 },
  [EFMT_ATTRIBUTION_TRAILER]           = { "attribution_trailer",           index_1,          index_2 },
  [EFMT_AUTOCRYPT_ACCT_FORMAT]         = { "autocrypt_acct_format",         autocrypt_acct_1, NULL },
  [EFMT_COMPOSE_FORMAT]                = { "compose_format",                compose_1,        NULL },
  [EFMT_FOLDER_FORMAT]                 = { "folder_format",                 folder_1,         NULL },
  [EFMT_FORWARD_ATTRIBUTION_INTRO]     = { "forward_attribution_intro",     index_1,          index_2 },
  [EFMT_FORWARD_ATTRIBUTION_TRAILER]   = { "forward_attribution_trailer",   index_1,          index_2 },
  [EFMT_FORWARD_FORMAT]                = { "forward_format",                index_1,          index_2 },
  [EFMT_GREETING]                      = { "greeting",                      greeting_1,       NULL },
  [EFMT_GROUP_INDEX_FORMAT]            = { "group_index_format",            group_index_1,    NULL },
  [EFMT_INDENT_STRING]                 = { "indent_string",                 index_1,          index_2 },
  [EFMT_INDEX_FORMAT]                  = { "index_format",                  index_1,          index_2 },
  [EFMT_INEWS]                         = { "inews",                         inews_1,          NULL },
  [EFMT_MAILBOX_FOLDER_FORMAT]         = { "mailbox_folder_format",         folder_1,         NULL },
  [EFMT_MESSAGE_FORMAT]                = { "message_format",                index_1,          index_2 },
  [EFMT_MIX_ENTRY_FORMAT]              = { "mix_entry_format",              mix_entry_1,      NULL },
  [EFMT_NEWSRC]                        = { "newsrc",                        inews_1,          NULL },
  [EFMT_NEW_MAIL_COMMAND]              = { "new_mail_command",              status_1,         NULL },
  [EFMT_PAGER_FORMAT]                  = { "pager_format",                  index_1,          index_2 },
  [EFMT_PATTERN_FORMAT]                = { "pattern_format",                pattern_1,        NULL },
  [EFMT_PGP_CLEAR_SIGN_COMMAND]        = { "pgp_clear_sign_command",        pgp_command_1,    NULL },
  [EFMT_PGP_DECODE_COMMAND]            = { "pgp_decode_command",            pgp_command_1,    NULL },
  [EFMT_PGP_DECRYPT_COMMAND]           = { "pgp_decrypt_command",           pgp_command_1,    NULL },
  [EFMT_PGP_ENCRYPT_ONLY_COMMAND]      = { "pgp_encrypt_only_command",      pgp_command_1,    NULL },
  [EFMT_PGP_ENCRYPT_SIGN_COMMAND]      = { "pgp_encrypt_sign_command",      pgp_command_1,    NULL },
  [EFMT_PGP_ENTRY_FORMAT]              = { "pgp_entry_format",              pgp_entry_1,      NULL },
  [EFMT_PGP_EXPORT_COMMAND]            = { "pgp_export_command",            pgp_command_1,    NULL },
  [EFMT_PGP_GET_KEYS_COMMAND]          = { "pgp_get_keys_command",          pgp_command_1,    NULL },
  [EFMT_PGP_IMPORT_COMMAND]            = { "pgp_import_command",            pgp_command_1,    NULL },
  [EFMT_PGP_LIST_PUBRING_COMMAND]      = { "pgp_list_pubring_command",      pgp_command_1,    NULL },
  [EFMT_PGP_LIST_SECRING_COMMAND]      = { "pgp_list_secring_command",      pgp_command_1,    NULL },
  [EFMT_PGP_SIGN_COMMAND]              = { "pgp_sign_command",              pgp_command_1,    NULL },
  [EFMT_PGP_VERIFY_COMMAND]            = { "pgp_verify_command",            pgp_command_1,    NULL },
  [EFMT_PGP_VERIFY_KEY_COMMAND]        = { "pgp_verify_key_command",        pgp_command_1,    NULL },
  [EFMT_QUERY_FORMAT]                  = { "query_format",                  query_1,          NULL },
  [EFMT_SIDEBAR_FORMAT]                = { "sidebar_format",                sidebar_1,        NULL },
  [EFMT_SMIME_DECRYPT_COMMAND]         = { "smime_decrypt_command",         smime_command_1,  NULL },
  [EFMT_SMIME_ENCRYPT_COMMAND]         = { "smime_encrypt_command",         smime_command_1,  NULL },
  [EFMT_SMIME_ENCRYPT_WITH]            = { "smime_encrypt_with",            smime_command_1,  NULL },
  [EFMT_SMIME_GET_CERT_COMMAND]        = { "smime_get_cert_command",        smime_command_1,  NULL },
  [EFMT_SMIME_GET_CERT_EMAIL_COMMAND]  = { "smime_get_cert_email_command",  smime_command_1,  NULL },
  [EFMT_SMIME_GET_SIGNER_CERT_COMMAND] = { "smime_get_signer_cert_command", smime_command_1,  NULL },
  [EFMT_SMIME_IMPORT_CERT_COMMAND]     = { "smime_import_cert_command",     smime_command_1,  NULL },
  [EFMT_SMIME_PK7OUT_COMMAND]          = { "smime_pk7out_command",          smime_command_1,  NULL },
  [EFMT_SMIME_SIGN_COMMAND]            = { "smime_sign_command",            smime_command_1,  NULL },
  [EFMT_SMIME_SIGN_DIGEST_ALG]         = { "smime_sign_digest_alg",         smime_command_1,  NULL },
  [EFMT_SMIME_VERIFY_COMMAND]          = { "smime_verify_command",          smime_command_1,  NULL },
  [EFMT_SMIME_VERIFY_OPAQUE_COMMAND]   = { "smime_verify_opaque_command",   smime_command_1,  NULL },
  [EFMT_STATUS_FORMAT]                 = { "status_format",                 status_1,         NULL },
  [EFMT_TS_ICON_FORMAT]                = { "ts_icon_format",                status_1,         NULL },
  [EFMT_TS_STATUS_FORMAT]              = { "ts_status_format",              status_1,         NULL },
  // clang-format on
};

bool expando_validate_string(struct Buffer *name, struct Buffer *value, struct Buffer *err)
{
  for (enum ExpandoFormatIndex index = 0; index < EFMT_FORMAT_COUNT_OR_DEBUG; ++index)
  {
    if (mutt_str_equal(name->data, expando_validation[index].name))
    {
      const char *input = NULL;
      if (*value->data)
      {
        input = buf_strdup(value);
        assert(input);
      }

      struct ExpandoParseError error = { 0 };
      struct ExpandoNode *root = NULL;

      expando_tree_parse(&root, &input, index, &error);

      if (error.position != NULL)
      {
        buf_printf(err, _("$%s: %s\nDefault value will be used."), name->data,
                   error.message);
        expando_tree_free(&root);

        if (*value->data)
        {
          FREE(&input);
        }

        return false;
      }

      NeoMutt->expando_table[index].string = input;
      NeoMutt->expando_table[index].tree = root;
      return true;
    }
  }

  return true;
}

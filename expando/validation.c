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
#include "config/lib.h"
#include "core/neomutt.h"
#include "expando/lib.h"
#include "validation.h"

static const struct ExpandoFormatCallback alias_1[] = {
  { "a", alias_a }, { "c", alias_c }, { "f", alias_f }, { "n", alias_n },
  { "r", alias_r }, { "t", alias_t }, { NULL, NULL },
};

static const struct ExpandoFormatCallback attach_1[] = {
  { "c", attach_c }, { "C", attach_C }, { "d", attach_d }, { "D", attach_D },
  { "e", attach_e }, { "f", attach_f }, { "F", attach_F }, { "I", attach_I },
  { "m", attach_m }, { "M", attach_M }, { "n", attach_n }, { "Q", attach_Q },
  { "s", attach_s }, { "t", attach_t }, { "T", attach_T }, { "u", attach_u },
  { "X", attach_X }, { NULL, NULL },
};

static const struct ExpandoFormatCallback autocrypt_acct_1[] = {
  { "a", autocrypt_a }, { "k", autocrypt_k }, { "n", autocrypt_n },
  { "p", autocrypt_p }, { "s", autocrypt_s }, { NULL, NULL },
};

static const struct ExpandoFormatCallback compose_1[] = {
  { "a", compose_a }, { "h", compose_h }, { "l", compose_l },
  { "v", compose_v }, { NULL, NULL },
};

static const struct ExpandoFormatCallback compress_1[] = {
  { "f", compress_f },
  { "t", compress_t },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback pgp_entry_1[] = {
  { "n", pgp_entry_n }, { "t", pgp_entry_t }, { "u", pgp_entry_u },
  { "a", pgp_entry_a }, { "A", pgp_entry_A }, { "c", pgp_entry_c },
  { "C", pgp_entry_C }, { "f", pgp_entry_f }, { "F", pgp_entry_F },
  { "k", pgp_entry_k }, { "K", pgp_entry_K }, { "l", pgp_entry_l },
  { "L", pgp_entry_L }, { NULL, NULL },
};

static const struct ExpandoFormatCallback folder_1[] = {
  { "a", folder_a }, { "C", folder_C }, { "d", folder_d }, { "D", folder_D },
  { "f", folder_f }, { "F", folder_F }, { "g", folder_g }, { "i", folder_i },
  { "l", folder_l }, { "m", folder_m }, { "n", folder_n }, { "N", folder_N },
  { "p", folder_p }, { "s", folder_s }, { "t", folder_t }, { "u", folder_u },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback greeting_1[] = {
  { "n", greeting_n },
  { "u", greeting_u },
  { "v", greeting_v },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback group_index_1[] = {
  { "a", group_index_a }, { "C", group_index_C }, { "d", group_index_d },
  { "f", group_index_f }, { "M", group_index_M }, { "n", group_index_n },
  { "N", group_index_N }, { "p", group_index_p }, { "s", group_index_s },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback history_1[] = {
  { "s", history_s },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback index_1[] = {
  { "a", index_a }, { "A", index_A }, { "b", index_b }, { "B", index_B },
  { "c", index_c }, { "C", index_C }, { "d", index_d }, { "D", index_D },
  { "e", index_e }, { "E", index_E }, { "f", index_f }, { "F", index_F },
  { "g", index_g }, { "H", index_H }, { "i", index_i }, { "I", index_I },
  { "J", index_J }, { "K", index_K }, { "l", index_l }, { "L", index_L },
  { "m", index_m }, { "M", index_M }, { "n", index_n }, { "N", index_N },
  { "O", index_O }, { "P", index_P }, { "q", index_q }, { "r", index_r },
  { "R", index_R }, { "s", index_s }, { "S", index_S }, { "t", index_t },
  { "T", index_T }, { "u", index_u }, { "v", index_v }, { "W", index_W },
  { "x", index_x }, { "X", index_X }, { "y", index_y }, { "Y", index_Y },
  { "Z", index_Z }, { NULL, NULL },
};

static const struct ExpandoFormatCallback index_2[] = {
  { "cr", index_cr }, { "Fp", index_Fp }, { "G0", index_G0 },
  { "G1", index_G1 }, { "G2", index_G2 }, { "G3", index_G3 },
  { "G4", index_G4 }, { "G5", index_G5 }, { "G6", index_G6 },
  { "G7", index_G7 }, { "G8", index_G8 }, { "G9", index_G9 },
  { "zc", index_zc }, { "zs", index_zs }, { "zt", index_zt },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback mix_entry_1[] = {
  { "a", mix_a }, { "c", mix_c }, { "n", mix_n },
  { "s", mix_s }, { NULL, NULL },
};

static const struct ExpandoFormatCallback nntp_1[] = {
  { "a", nntp_a }, { "p", nntp_p }, { "P", nntp_P }, { "s", nntp_s },
  { "S", nntp_S }, { "u", nntp_u }, { NULL, NULL },
};

static const struct ExpandoFormatCallback pattern_1[] = {
  { "d", pattern_d },
  { "e", pattern_e },
  { "n", pattern_n },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback pgp_command_1[] = {
  { "a", pgp_command_a }, { "f", pgp_command_f }, { "p", pgp_command_p },
  { "r", pgp_command_r }, { "s", pgp_command_s }, { NULL, NULL },
};

static const struct ExpandoFormatCallback query_1[] = {
  { "a", query_a }, { "c", query_c }, { "e", query_e },
  { "n", query_n }, { "t", query_t }, { NULL, NULL },
};

static const struct ExpandoFormatCallback sidebar_1[] = {
  { "!", sidebar_bang }, { "a", sidebar_a }, { "B", sidebar_B },
  { "d", sidebar_d },    { "D", sidebar_D }, { "F", sidebar_F },
  { "L", sidebar_L },    { "n", sidebar_n }, { "N", sidebar_N },
  { "o", sidebar_o },    { "p", sidebar_p }, { "r", sidebar_r },
  { "S", sidebar_S },    { "t", sidebar_t }, { "Z", sidebar_Z },
  { NULL, NULL },
};

static const struct ExpandoFormatCallback smime_command_1[] = {
  { "a", smime_command_a }, { "c", smime_command_c }, { "C", smime_command_C },
  { "d", smime_command_d }, { "f", smime_command_f }, { "i", smime_command_i },
  { "k", smime_command_k }, { "s", smime_command_s }, { NULL, NULL },
};

static const struct ExpandoFormatCallback status_1[] = {
  { "b", status_b }, { "d", status_d }, { "D", status_D }, { "f", status_f },
  { "F", status_F }, { "h", status_h }, { "l", status_l }, { "L", status_L },
  { "m", status_m }, { "M", status_M }, { "n", status_n }, { "o", status_o },
  { "p", status_p }, { "P", status_P }, { "r", status_r }, { "R", status_R },
  { "s", status_s }, { "S", status_S }, { "t", status_t }, { "T", status_T },
  { "u", status_u }, { "v", status_v }, { "V", status_V }, { NULL, NULL },
};

const struct ExpandoValidation expando_validation[EFMTI_FORMAT_COUNT_OR_DEBUG] = {
  // clang-format off
  [EFMTI_ALIAS_FORMAT]                  = { "alias_format",                  alias_1,          NULL,    NULL },
  [EFMTI_ATTACH_FORMAT]                 = { "attach_format",                 attach_1,         NULL,    NULL },
  [EFMTI_ATTRIBUTION_INTRO]             = { "attribution_intro",             index_1,          index_2, NULL },
  [EFMTI_ATTRIBUTION_TRAILER]           = { "attribution_trailer",           index_1,          index_2, NULL },
  [EFMTI_AUTOCRYPT_ACCT_FORMAT]         = { "autocrypt_acct_format",         autocrypt_acct_1, NULL,    NULL },
  [EFMTI_COMPOSE_FORMAT]                = { "compose_format",                compose_1,        NULL,    NULL },
  [EFMTI_COMPRESS_FORMAT]               = { "",                              compress_1,       NULL,    NULL },
  [EFMTI_FOLDER_FORMAT]                 = { "folder_format",                 folder_1,         NULL,    NULL },
  [EFMTI_FORWARD_ATTRIBUTION_INTRO]     = { "forward_attribution_intro",     index_1,          index_2, NULL },
  [EFMTI_FORWARD_ATTRIBUTION_TRAILER]   = { "forward_attribution_trailer",   index_1,          index_2, NULL },
  [EFMTI_FORWARD_FORMAT]                = { "forward_format",                index_1,          index_2, NULL },
  [EFMTI_GREETING]                      = { "greeting",                      greeting_1,       NULL,    NULL },
  [EFMTI_GROUP_INDEX_FORMAT]            = { "group_index_format",            group_index_1,    NULL,    NULL },
  [EFMTI_HISTORY_FORMAT]                = { "history_format",                history_1,        NULL,    NULL },
  [EFMTI_INDENT_STRING]                 = { "indent_string",                 index_1,          index_2, NULL },
  [EFMTI_INDEX_FORMAT]                  = { "index_format",                  index_1,          index_2, NULL },
  [EFMTI_INEWS]                         = { "inews",                         nntp_1,           NULL,    NULL },
  [EFMTI_MAILBOX_FOLDER_FORMAT]         = { "mailbox_folder_format",         folder_1,         NULL,    NULL },
  [EFMTI_MESSAGE_FORMAT]                = { "message_format",                index_1,          index_2, NULL },
  [EFMTI_MIX_ENTRY_FORMAT]              = { "mix_entry_format",              mix_entry_1,      NULL,    NULL },
  [EFMTI_NEWSRC]                        = { "newsrc",                        nntp_1,           NULL,    NULL },
  [EFMTI_NEW_MAIL_COMMAND]              = { "new_mail_command",              status_1,         NULL,    NULL },
  [EFMTI_PAGER_FORMAT]                  = { "pager_format",                  index_1,          index_2, NULL },
  [EFMTI_PATTERN_FORMAT]                = { "pattern_format",                pattern_1,        NULL,    NULL },
  [EFMTI_PGP_CLEAR_SIGN_COMMAND]        = { "pgp_clear_sign_command",        pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_DECODE_COMMAND]            = { "pgp_decode_command",            pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_DECRYPT_COMMAND]           = { "pgp_decrypt_command",           pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_ENCRYPT_ONLY_COMMAND]      = { "pgp_encrypt_only_command",      pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_ENCRYPT_SIGN_COMMAND]      = { "pgp_encrypt_sign_command",      pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_ENTRY_FORMAT]              = { "pgp_entry_format",              pgp_entry_1,      NULL,    NULL },
  [EFMTI_PGP_EXPORT_COMMAND]            = { "pgp_export_command",            pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_GET_KEYS_COMMAND]          = { "pgp_get_keys_command",          pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_IMPORT_COMMAND]            = { "pgp_import_command",            pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_LIST_PUBRING_COMMAND]      = { "pgp_list_pubring_command",      pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_LIST_SECRING_COMMAND]      = { "pgp_list_secring_command",      pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_SIGN_COMMAND]              = { "pgp_sign_command",              pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_VERIFY_COMMAND]            = { "pgp_verify_command",            pgp_command_1,    NULL,    NULL },
  [EFMTI_PGP_VERIFY_KEY_COMMAND]        = { "pgp_verify_key_command",        pgp_command_1,    NULL,    NULL },
  [EFMTI_QUERY_FORMAT]                  = { "query_format",                  query_1,          NULL,    NULL },
  [EFMTI_SIDEBAR_FORMAT]                = { "sidebar_format",                sidebar_1,        NULL,    NULL },
  [EFMTI_SMIME_DECRYPT_COMMAND]         = { "smime_decrypt_command",         smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_ENCRYPT_COMMAND]         = { "smime_encrypt_command",         smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_GET_CERT_COMMAND]        = { "smime_get_cert_command",        smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_GET_CERT_EMAIL_COMMAND]  = { "smime_get_cert_email_command",  smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_GET_SIGNER_CERT_COMMAND] = { "smime_get_signer_cert_command", smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_IMPORT_CERT_COMMAND]     = { "smime_import_cert_command",     smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_PK7OUT_COMMAND]          = { "smime_pk7out_command",          smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_SIGN_COMMAND]            = { "smime_sign_command",            smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_VERIFY_COMMAND]          = { "smime_verify_command",          smime_command_1,  NULL,    NULL },
  [EFMTI_SMIME_VERIFY_OPAQUE_COMMAND]   = { "smime_verify_opaque_command",   smime_command_1,  NULL,    NULL },
  [EFMTI_STATUS_FORMAT]                 = { "status_format",                 status_1,         NULL,    NULL },
  [EFMTI_TS_ICON_FORMAT]                = { "ts_icon_format",                status_1,         NULL,    NULL },
  [EFMTI_TS_STATUS_FORMAT]              = { "ts_status_format",              status_1,         NULL,    NULL },
  // clang-format on
};

int expando_validator(const struct ConfigSet *cs, const struct ConfigDef *cdef,
                      intptr_t value, struct Buffer *err)
{
  assert(value != 0);

  struct ExpandoRecord *r = (struct ExpandoRecord *) value;
  for (enum ExpandoFormatIndex index = 0; index < EFMTI_FORMAT_COUNT_OR_DEBUG; ++index)
  {
    if (mutt_str_equal(cdef->name, expando_validation[index].name))
    {
      struct ExpandoParseError error = { 0 };
      struct ExpandoNode *root = NULL;

      expando_tree_parse(&root, &r->string, index, &error);

      if (error.position != NULL)
      {
        buf_printf(err, _("$%s: %s\nDefault value will be used."), cdef->name,
                   error.message);
        expando_tree_free(&root);
        return CSR_ERR_INVALID;
      }

      r->tree = root;
      return CSR_SUCCESS;
    }
  }

  return CSR_ERR_INVALID;
}

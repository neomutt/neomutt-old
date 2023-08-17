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
#include "mutt/lib.h"
#include "address/address.h"
#include "autocrypt/lib.h"
#include "expando/lib.h"
#include "autocrypt/private.h"
#include "format_flags.h"

int autocrypt_a(const struct ExpandoNode *self, char *buf, int buf_len,
                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AccountEntry *entry = (const struct AccountEntry *) data;

  char fmt[128];

  const char *s = buf_string(entry->addr->mailbox);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int autocrypt_k(const struct ExpandoNode *self, char *buf, int buf_len,
                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AccountEntry *entry = (const struct AccountEntry *) data;

  char fmt[128];

  const char *s = entry->account->keyid;
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int autocrypt_n(const struct ExpandoNode *self, char *buf, int buf_len,
                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AccountEntry *entry = (const struct AccountEntry *) data;

  char fmt[128];

  const int num = entry->num;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int autocrypt_p(const struct ExpandoNode *self, char *buf, int buf_len,
                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AccountEntry *entry = (const struct AccountEntry *) data;

  char tmp[128], fmt[128];

  if (entry->account->prefer_encrypt)
  {
    /* L10N: Autocrypt Account menu.
           flag that an account has prefer-encrypt set */
    mutt_str_copy(tmp, _("prefer encrypt"), sizeof(tmp));
  }
  else
  {
    /* L10N: Autocrypt Account menu.
           flag that an account has prefer-encrypt unset;
           thus encryption will need to be manually enabled.  */
    mutt_str_copy(tmp, _("manual encrypt"), sizeof(tmp));
  }

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int autocrypt_s(const struct ExpandoNode *self, char *buf, int buf_len,
                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AccountEntry *entry = (const struct AccountEntry *) data;

  char tmp[128], fmt[128];

  if (entry->account->enabled)
  {
    /* L10N: Autocrypt Account menu.
           flag that an account is enabled/active */
    mutt_str_copy(tmp, _("active"), sizeof(tmp));
  }
  else
  {
    /* L10N: Autocrypt Account menu.
           flag that an account is disabled/inactive */
    mutt_str_copy(tmp, _("inactive"), sizeof(tmp));
  }

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "address/lib.h"
#include "alias/alias.h"
#include "alias/gui.h"
#include "query_format_callbacks.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "node.h"

int query_a(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AliasView *av = (const struct AliasView *) data;
  const struct Alias *alias = av->alias;

  char fmt[256], tmp[256] = { 0 };

  struct Buffer *tmpbuf = buf_pool_get();
  tmp[0] = '<';
  mutt_addrlist_write(&alias->addr, tmpbuf, true);
  mutt_str_copy(tmp + 1, buf_string(tmpbuf), sizeof(tmp) - 1);
  buf_pool_release(&tmpbuf);
  const size_t len = mutt_str_len(tmp);
  if (len < (sizeof(tmp) - 1))
  {
    tmp[len] = '>';
    tmp[len + 1] = '\0';
  }
  format_string_flags(fmt, sizeof(fmt), tmp, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int query_c(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AliasView *av = (const struct AliasView *) data;

  char fmt[128];

  const int num = av->num + 1;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int query_e(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AliasView *av = (const struct AliasView *) data;
  const struct Alias *alias = av->alias;

  char fmt[128];

  const char *s = NONULL(alias->comment);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int query_n(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AliasView *av = (const struct AliasView *) data;
  const struct Alias *alias = av->alias;

  char fmt[128];

  const char *s = NONULL(alias->name);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int query_t(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AliasView *av = (const struct AliasView *) data;

  char fmt[128];

  // NOTE(g0mb4): use $flag_chars?
  const char *s = av->is_tagged ? "*" : " ";
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

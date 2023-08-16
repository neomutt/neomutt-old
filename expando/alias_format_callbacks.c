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
#include "config/lib.h"
#include "core/neomutt.h"
#include "alias/alias.h"
#include "alias/gui.h"
#include "expando/lib.h"
#include "postpone/lib.h"
#include "globals.h"
#include "index/shared_data.h"
#include "mutt_mailbox.h"
#include "mutt_thread.h"
#include "muttlib.h"
#include "mview.h"

int alias_a(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct AliasView *av = (struct AliasView *) data;
  struct Alias *alias = av->alias;

  char fmt[128];

  const char *s = NONULL(alias->name);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int alias_c(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct AliasView *av = (struct AliasView *) data;
  struct Alias *alias = av->alias;

  char fmt[128];

  const char *s = NONULL(alias->comment);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int alias_f(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct AliasView *av = (struct AliasView *) data;

  char fmt[128];

  // NOTE(g0mb4): use $flag_chars?
  // NOTE(g0mb4): use "" if not applicable, so it can be used in a conditional?
  const char *s = av->is_deleted ? "D" : " ";
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int alias_n(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct AliasView *av = (struct AliasView *) data;

  char fmt[128];

  const int num = av->num + 1;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int alias_r(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct AliasView *av = (struct AliasView *) data;
  struct Alias *alias = av->alias;

  char tmp[128], fmt[128];

  struct Buffer *tmpbuf = buf_pool_get();
  mutt_addrlist_write(&alias->addr, tmpbuf, true);
  mutt_str_copy(tmp, buf_string(tmpbuf), sizeof(tmp));
  buf_pool_release(&tmpbuf);

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int alias_t(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct AliasView *av = (struct AliasView *) data;

  char fmt[128];

  // NOTE(g0mb4): use $flag_chars?
  // NOTE(g0mb4): use "" if not applicable, so it can be used in a conditional?
  const char *s = av->is_tagged ? "*" : " ";
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

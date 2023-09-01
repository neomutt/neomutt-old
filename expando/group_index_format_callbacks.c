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
#include "browser/lib.h"
#include "expando/lib.h"
#include "format_flags.h"
#include "nntp/mdata.h"

int group_index_a(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const int num = folder->ff->notify_user;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_C(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const int num = folder->num + 1;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_d(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  if (folder->ff->nd->desc)
  {
    char *desc = mutt_str_dup(folder->ff->nd->desc);
    const char *const c_newsgroups_charset = cs_subset_string(NeoMutt->sub, "newsgroups_charset");
    if (c_newsgroups_charset)
    {
      mutt_ch_convert_string(&desc, c_newsgroups_charset, cc_charset(), MUTT_ICONV_HOOK_FROM);
    }
    mutt_mb_filter_unprintable(&desc);
    mutt_str_copy(tmp, desc, sizeof(tmp));
    FREE(&desc);
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string_flags(fmt, sizeof(fmt), tmp, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_f(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const char *s = folder->ff->name;
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_M(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const char *s = NULL;
  // NOTE(g0mb4): use $flag_chars?
  if (folder->ff->nd->deleted)
  {
    s = "D";
  }
  else
  {
    s = folder->ff->nd->allowed ? " " : "-";
  }

  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_n(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const bool c_mark_old = cs_subset_bool(NeoMutt->sub, "mark_old");
  int num = 0;

  if (c_mark_old && (folder->ff->nd->last_cached >= folder->ff->nd->first_message) &&
      (folder->ff->nd->last_cached <= folder->ff->nd->last_message))
  {
    num = folder->ff->nd->last_message - folder->ff->nd->last_cached;
  }
  else
  {
    num = folder->ff->nd->unread;
  }

  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_N(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const char *s = NULL;
  // NOTE(g0mb4): use $flag_chars?
  if (folder->ff->nd->subscribed)
  {
    s = " ";
  }
  else
  {
    s = folder->ff->has_new_mail ? "N" : "u";
  }

  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_p(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const int num = folder->ff->poll_new_mail;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int group_index_s(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  // NOTE(g0mb4): is long required for unread?
  const int num = (int) folder->ff->nd->unread;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

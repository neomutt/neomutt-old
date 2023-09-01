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
#include "expando/lib.h"
#include "format_flags.h"
#include "mixmaster/remailer.h"

/**
 * mix_format_caps - Turn flags into a MixMaster capability string
 * @param r Remailer to use
 * @retval ptr Capability string
 *
 * @note The string is a static buffer
 */
static const char *mix_format_caps(const struct Remailer *r)
{
  // NOTE(g0mb4): use $to_chars?
  static char capbuf[10];
  char *t = capbuf;

  if (r->caps & MIX_CAP_COMPRESS)
    *t++ = 'C';
  else
    *t++ = ' ';

  if (r->caps & MIX_CAP_MIDDLEMAN)
    *t++ = 'M';
  else
    *t++ = ' ';

  if (r->caps & MIX_CAP_NEWSPOST)
  {
    *t++ = 'N';
    *t++ = 'p';
  }
  else
  {
    *t++ = ' ';
    *t++ = ' ';
  }

  if (r->caps & MIX_CAP_NEWSMAIL)
  {
    *t++ = 'N';
    *t++ = 'm';
  }
  else
  {
    *t++ = ' ';
    *t++ = ' ';
  }

  *t = '\0';

  return capbuf;
}

int mix_a(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
          intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Remailer *remailer = (const struct Remailer *) data;

  char fmt[128];

  const char *s = NONULL(remailer->addr);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int mix_c(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
          intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Remailer *remailer = (const struct Remailer *) data;

  char fmt[128];

  const char *s = mix_format_caps(remailer);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int mix_n(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
          intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Remailer *remailer = (const struct Remailer *) data;

  char fmt[128];

  const int num = remailer->num;
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int mix_s(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
          intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Remailer *remailer = (const struct Remailer *) data;

  char fmt[128];

  const char *s = NONULL(remailer->shortname);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

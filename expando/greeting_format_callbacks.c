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
#include "email/lib.h"
#include "expando/lib.h"
#include "format_flags.h"
#include "sort.h"

int greeting_n(const struct ExpandoNode *self, char *buf, int buf_len,
               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Email *email = (const struct Email *) data;
  const struct Address *to = TAILQ_FIRST(&email->env->to);

  char fmt[128];

  const char *s = mutt_get_name(to);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int greeting_u(const struct ExpandoNode *self, char *buf, int buf_len,
               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Email *email = (const struct Email *) data;
  const struct Address *to = TAILQ_FIRST(&email->env->to);

  char tmp[128], fmt[128];
  char *p = NULL;

  if (to)
  {
    mutt_str_copy(tmp, mutt_addr_for_display(to), sizeof(tmp));
    if ((p = strpbrk(tmp, "%@")))
    {
      *p = '\0';
    }
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int greeting_v(const struct ExpandoNode *self, char *buf, int buf_len,
               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Email *email = (const struct Email *) data;
  const struct Address *to = TAILQ_FIRST(&email->env->to);
  const struct Address *cc = TAILQ_FIRST(&email->env->cc);

  char tmp[128], fmt[128];
  char *p = NULL;

  if (to)
  {
    const char *s = mutt_get_name(to);
    format_string(tmp, sizeof(tmp), s, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
  }
  else if (cc)
  {
    const char *s = mutt_get_name(cc);
    format_string(tmp, sizeof(tmp), s, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
  }
  else
  {
    tmp[0] = '\0';
  }

  if ((p = strpbrk(tmp, " %@")))
    *p = '\0';

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

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
#include "core/lib.h"
#include "expando/lib.h"

int compress_f(const struct ExpandoNode *self, char *buf, int buf_len,
               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

  const struct Mailbox *m = (const struct Mailbox *) data;

  struct Buffer *quoted = buf_pool_get();
  buf_quote_filename(quoted, m->realpath, false);
  const int n = snprintf(buf, buf_len, "%s", buf_string(quoted));
  buf_pool_release(&quoted);

  return n;
}

int compress_t(const struct ExpandoNode *self, char *buf, int buf_len,
               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

  const struct Mailbox *m = (const struct Mailbox *) data;

  struct Buffer *quoted = buf_pool_get();
  buf_quote_filename(quoted, mailbox_path(m), false);
  const int n = snprintf(buf, buf_len, "%s", buf_string(quoted));
  buf_pool_release(&quoted);

  return n;
}

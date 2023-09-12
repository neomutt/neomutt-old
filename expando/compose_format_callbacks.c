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
#include <stdint.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "compose_format_callbacks.h"
#include "menu/lib.h"
#include "compose/attach_data.h"
#include "compose/private.h"
#include "compose/shared_data.h"
#include "format_callbacks.h"
#include "globals.h"
#include "helpers.h"
#include "muttlib.h"
#include "node.h"

/**
 * num_attachments - Count the number of attachments
 * @param adata Attachment data
 * @retval num Number of attachments
 */
static int num_attachments(const struct ComposeAttachData *adata)
{
  if (!adata || !adata->menu)
    return 0;
  return adata->menu->max;
}

int compose_a(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct ComposeSharedData *shared = (const struct ComposeSharedData *) data;

  char fmt[128];

  const int num = num_attachments(shared->adata);
  format_int_flags(fmt, sizeof(fmt), num, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int compose_h(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  char fmt[128];

  const char *s = NONULL(ShortHostname);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int compose_l(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct ComposeSharedData *shared = (const struct ComposeSharedData *) data;

  char tmp[128], fmt[128];

  mutt_str_pretty_size(tmp, sizeof(tmp), cum_attachs_size(shared->sub, shared->adata));
  format_string_flags(fmt, sizeof(fmt), tmp, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int compose_v(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  char fmt[128];

  const char *s = mutt_make_version();
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

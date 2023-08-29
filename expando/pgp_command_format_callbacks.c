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
#include "ncrypt/pgpinvoke.h"
#include "pattern/private.h"

int pgp_command_a(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct PgpCommandContext *cctx = (const struct PgpCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->signas);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int pgp_command_f(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct PgpCommandContext *cctx = (const struct PgpCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->fname);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int pgp_command_p(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct PgpCommandContext *cctx = (const struct PgpCommandContext *) data;

  char fmt[128];

  const char *s = cctx->need_passphrase ? "PGPPASSFD=0" : "";
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int pgp_command_r(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct PgpCommandContext *cctx = (const struct PgpCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->ids);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int pgp_command_s(const struct ExpandoNode *self, char *buf, int buf_len,
                  int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct PgpCommandContext *cctx = (const struct PgpCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->sig_fname);
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

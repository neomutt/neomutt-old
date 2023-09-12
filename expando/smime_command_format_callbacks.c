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
#include <sys/stat.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "expando/lib.h"

#include "muttlib.h"
#include "ncrypt/smime.h"

int smime_command_a(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SmimeCommandContext *cctx = (const struct SmimeCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->cryptalg);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

int smime_command_c(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SmimeCommandContext *cctx = (const struct SmimeCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->certificates);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

int smime_command_C(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const char *const c_smime_ca_location = cs_subset_path(NeoMutt->sub, "smime_ca_location");

  char fmt[128];

  struct Buffer *path = buf_pool_get();
  struct Buffer *buf1 = buf_pool_get();
  struct Buffer *buf2 = buf_pool_get();
  struct stat st = { 0 };

  buf_strcpy(path, c_smime_ca_location);
  buf_expand_path(path);
  buf_quote_filename(buf1, buf_string(path), true);

  if ((stat(buf_string(path), &st) != 0) || !S_ISDIR(st.st_mode))
  {
    buf_printf(buf2, "-CAfile %s", buf_string(buf1));
  }
  else
  {
    buf_printf(buf2, "-CApath %s", buf_string(buf1));
  }

  const char *s = buf_string(buf2);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  int n = snprintf(buf, buf_len, "%s", fmt);

  buf_pool_release(&path);
  buf_pool_release(&buf1);
  buf_pool_release(&buf2);

  return n;
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

int smime_command_d(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SmimeCommandContext *cctx = (const struct SmimeCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->digestalg);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

int smime_command_f(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SmimeCommandContext *cctx = (const struct SmimeCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->fname);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

int smime_command_i(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SmimeCommandContext *cctx = (const struct SmimeCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->intermediates);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

int smime_command_k(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SmimeCommandContext *cctx = (const struct SmimeCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->key);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

int smime_command_s(const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef HAVE_SMIME
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SmimeCommandContext *cctx = (const struct SmimeCommandContext *) data;

  char fmt[128];

  const char *s = NONULL(cctx->sig_fname);
  format_string_flags(fmt, sizeof(fmt), s, flags, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* HAVE_SMIME */
  return 0;
#endif /* HAVE_SMIME */
}

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
#include "conn/lib.h"
#include "expando/lib.h"
#include "format_flags.h"
#include "mutt_account.h"
#include "nntp/adata.h"

int nntp_a(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
           intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  struct NntpAccountData *adata = (struct NntpAccountData *) data;
  struct ConnAccount *cac = &adata->conn->account;

  char tmp[128], fmt[128];

  struct Url url = { 0 };
  mutt_account_tourl(cac, &url);
  url_tostring(&url, tmp, sizeof(tmp), U_PATH);
  char *p = strchr(tmp, '/');
  if (p)
  {
    *p = '\0';
  }

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int nntp_p(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
           intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct NntpAccountData *adata = (const struct NntpAccountData *) data;
  const struct ConnAccount *cac = &adata->conn->account;

  char fmt[128];

  const int num = (int) cac->port;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int nntp_P(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
           intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct NntpAccountData *adata = (const struct NntpAccountData *) data;
  const struct ConnAccount *cac = &adata->conn->account;

  char fmt[128];

  if (cac->flags & MUTT_ACCT_PORT)
  {
    const int num = (int) cac->port;
    format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else
  {
    return 0;
  }
}

int nntp_s(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
           intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct NntpAccountData *adata = (const struct NntpAccountData *) data;
  const struct ConnAccount *cac = &adata->conn->account;

  char tmp[128], fmt[128];

  mutt_str_copy(tmp, cac->host, sizeof(tmp));
  mutt_str_lower(tmp);

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int nntp_S(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
           intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  struct NntpAccountData *adata = (struct NntpAccountData *) data;
  struct ConnAccount *cac = &adata->conn->account;

  char tmp[128], fmt[128];

  struct Url url = { 0 };
  mutt_account_tourl(cac, &url);
  url_tostring(&url, tmp, sizeof(tmp), U_PATH);
  char *p = strchr(tmp, ':');
  if (p)
  {
    *p = '\0';
  }

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int nntp_u(const struct ExpandoNode *self, char *buf, int buf_len, int cols_len,
           intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct NntpAccountData *adata = (const struct NntpAccountData *) data;
  const struct ConnAccount *cac = &adata->conn->account;

  char fmt[128];

  const char *s = cac->user;
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

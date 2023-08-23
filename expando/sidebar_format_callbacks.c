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
#include "index/lib.h"
#include "sidebar/lib.h"
#include "format_flags.h"
#include "sidebar/private.h"

/**
 * add_indent - Generate the needed indentation
 * @param buf    Output buffer
 * @param buflen Size of output buffer
 * @param sbe    Sidebar entry
 * @retval num Bytes written
 */
static size_t add_indent(char *buf, size_t buflen, const struct SbEntry *sbe)
{
  size_t res = 0;
  const char *const c_sidebar_indent_string = cs_subset_string(NeoMutt->sub, "sidebar_indent_string");
  for (int i = 0; i < sbe->depth; i++)
  {
    res += mutt_str_copy(buf + res, c_sidebar_indent_string, buflen - res);
  }
  return res;
}

int sidebar_bang(const struct ExpandoNode *self, char *buf, int buf_len,
                 int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char tmp[128], fmt[128];

  if (mailbox->msg_flagged == 0)
  {
    mutt_str_copy(tmp, "", sizeof(tmp));
  }
  else if (mailbox->msg_flagged == 1)
  {
    mutt_str_copy(tmp, "!", sizeof(tmp));
  }
  else if (mailbox->msg_flagged == 2)
  {
    mutt_str_copy(tmp, "!!", sizeof(tmp));
  }
  else
  {
    snprintf(tmp, sizeof(tmp), "%d!", mailbox->msg_flagged);
  }

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_a(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->notify_user;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_B(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;

  char tmp[256] = { 0 }, fmt[256];

  const size_t ilen = sizeof(tmp);
  const size_t off = add_indent(tmp, ilen, sbe);
  snprintf(tmp + off, ilen - off, "%s", sbe->box);

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_d(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct IndexSharedData *shared = sfdata->shared;
  const struct Mailbox *mailbox = sbe->mailbox;
  const struct Mailbox *m_cur = shared->mailbox;

  const bool c = m_cur && mutt_str_equal(m_cur->realpath, mailbox->realpath);

  char fmt[128];

  const int num = c ? m_cur->msg_deleted : 0;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_D(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;

  char tmp[256] = { 0 }, fmt[256];

  const size_t ilen = sizeof(tmp);
  const size_t off = add_indent(tmp, ilen, sbe);

  if (sbe->mailbox->name)
  {
    snprintf(tmp + off, ilen - off, "%s", sbe->mailbox->name);
  }
  else
  {
    snprintf(tmp + off, ilen - off, "%s", sbe->box);
  }

  format_string(fmt, sizeof(fmt), tmp, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_F(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->msg_flagged;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_L(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct IndexSharedData *shared = sfdata->shared;
  const struct Mailbox *mailbox = sbe->mailbox;
  const struct Mailbox *m_cur = shared->mailbox;

  const bool c = m_cur && mutt_str_equal(m_cur->realpath, mailbox->realpath);

  char fmt[128];

  const int num = c ? m_cur->vcount : mailbox->msg_count;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_n(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  // NOTE(g0mb4): use $flag_chars?
  const char *s = mailbox->has_new ? "N" : " ";
  format_string(fmt, sizeof(fmt), s, flags, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_N(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->msg_unread;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_o(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->msg_unread - mailbox->msg_new;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_p(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->poll_new_mail;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_r(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->msg_count - mailbox->msg_unread;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_S(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->msg_count;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_t(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct IndexSharedData *shared = sfdata->shared;
  const struct Mailbox *mailbox = sbe->mailbox;
  const struct Mailbox *m_cur = shared->mailbox;

  const bool c = m_cur && mutt_str_equal(m_cur->realpath, mailbox->realpath);

  char fmt[128];

  const int num = c ? m_cur->msg_tagged : 0;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int sidebar_Z(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct SidebarFormatData *sfdata = (const struct SidebarFormatData *) data;
  const struct SbEntry *sbe = sfdata->entry;
  const struct Mailbox *mailbox = sbe->mailbox;

  char fmt[128];

  const int num = mailbox->msg_new;
  format_int(fmt, sizeof(fmt), num, flags, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

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
#include "config/lib.h"
#include "core/neomutt.h"
#include "postpone/lib.h"
#include "globals.h"
#include "index/shared_data.h"
#include "mutt_mailbox.h"
#include "mutt_thread.h"
#include "muttlib.h"
#include "node.h"
#include "status.h"
#include "status_format_callbacks.h"

int status_r(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  size_t i = 0;

  if (mailbox)
  {
    i = OptAttachMsg ? 3 :
                       ((mailbox->readonly || mailbox->dontwrite) ? 2 :
                        (mailbox->changed ||
                         /* deleted doesn't necessarily mean changed in IMAP */
                         (mailbox->type != MUTT_IMAP && mailbox->msg_deleted)) ?
                                                                    1 :
                                                                    0);
  }

  const struct MbTable *c_status_chars = cs_subset_mbtable(NeoMutt->sub, "status_chars");

  char fmt[128], tmp[128];

  if (!c_status_chars || !c_status_chars->len)
    tmp[0] = '\0';
  else if (i >= c_status_chars->len)
    snprintf(tmp, sizeof(tmp), "%s", c_status_chars->chars[0]);
  else
    snprintf(tmp, sizeof(tmp), "%s", c_status_chars->chars[i]);

  format_string(fmt, sizeof(fmt), tmp, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_D(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128], tmp[128];

  // If there's a descriptive name, use it. Otherwise, use %f
  if (mailbox && mailbox->name)
  {
    mutt_str_copy(tmp, mailbox->name, sizeof(tmp));
    format_string(fmt, sizeof(fmt), tmp, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  return status_f(self, buf, buf_len, cols_len, data, flags);
}

int status_f(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;
  char fmt[128], tmp[128];

#ifdef USE_COMP_MBOX
  if (mailbox && mailbox->compress_info && (mailbox->realpath[0] != '\0'))
  {
    mutt_str_copy(tmp, mailbox->realpath, sizeof(tmp));
    mutt_pretty_mailbox(tmp, sizeof(tmp));
  }
  else
#endif
      if (mailbox && (mailbox->type == MUTT_NOTMUCH) && mailbox->name)
  {
    mutt_str_copy(tmp, mailbox->name, sizeof(tmp));
  }
  else if (mailbox && !buf_is_empty(&mailbox->pathbuf))
  {
    mutt_str_copy(tmp, mailbox_path(mailbox), sizeof(tmp));
    mutt_pretty_mailbox(tmp, sizeof(tmp));
  }
  else
  {
    mutt_str_copy(tmp, _("(no mailbox)"), sizeof(tmp));
  }

  format_string(fmt, sizeof(fmt), tmp, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_M(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mailbox ? mailbox->vcount : 0;
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_m(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mailbox ? mailbox->msg_count : 0;
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_n(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mailbox ? mailbox->msg_new : 0;
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_o(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mailbox ? (mailbox->msg_unread - mailbox->msg_new) : 0;
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_d(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mailbox ? mailbox->msg_deleted : 0;
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_F(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mailbox ? mailbox->msg_flagged : 0;
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_t(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mailbox ? mailbox->msg_tagged : 0;
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_p(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mutt_num_postponed(mailbox, false);
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_b(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char fmt[128];

  const int num = mutt_mailbox_check(mailbox, MUTT_MAILBOX_CHECK_NO_FLAGS);
  format_int(fmt, sizeof(fmt), num, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int status_l(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct IndexSharedData *shared = msld->shared;
  struct Mailbox *mailbox = shared->mailbox;

  char tmp[128], fmt[128];

  const off_t num = mailbox ? mailbox->size : 0;
  mutt_str_pretty_size(tmp, sizeof(tmp), num);
  format_string(fmt, sizeof(fmt), tmp, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);

  return snprintf(buf, buf_len, "%s", fmt);
}

int status_T(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  char fmt[128];

  const enum UseThreads c_use_threads = mutt_thread_style();
  format_string(fmt, sizeof(fmt), get_use_threads_str(c_use_threads),
                MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);

  return snprintf(buf, buf_len, "%s", fmt);
}

/**
 * get_sort_str - Get the sort method as a string
 * @param buf    Buffer for the sort string
 * @param buflen Length of the buffer
 * @param method Sort method, see #SortType
 * @retval ptr Buffer pointer
 */
static char *get_sort_str_2gmb(char *buf, size_t buflen, enum SortType method)
{
  snprintf(buf, buflen, "%s%s%s", (method & SORT_REVERSE) ? "reverse-" : "",
           (method & SORT_LAST) ? "last-" : "",
           mutt_map_get_name(method & SORT_MASK, SortMethods));
  return buf;
}

int status_s(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  char fmt[128], tmp[128];

  const enum SortType c_sort = cs_subset_sort(NeoMutt->sub, "sort");
  format_string(fmt, sizeof(fmt), get_sort_str_2gmb(tmp, sizeof(tmp), c_sort),
                MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);

  return snprintf(buf, buf_len, "%s", fmt);
}

int status_S(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  char fmt[128], tmp[128];

  const enum SortType c_sort_aux = cs_subset_sort(NeoMutt->sub, "sort_aux");
  format_string(fmt, sizeof(fmt), get_sort_str_2gmb(tmp, sizeof(tmp), c_sort_aux),
                MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);

  return snprintf(buf, buf_len, "%s", fmt);
}

int status_P(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  struct ExpandoFormatPrivate *format = (struct ExpandoFormatPrivate *) self->ndata;

  struct MenuStatusLineData *msld = (struct MenuStatusLineData *) data;
  struct Menu *menu = msld->menu;

  char tmp[128], fmt[128];

  if (!menu)
  {
    return 0;
  }

  char *cp = NULL;
  if (menu->top + menu->page_len >= menu->max)
  {
    cp = menu->top ?
             /* L10N: Status bar message: the end of the list emails is visible in the index */
             _("end") :
             /* L10N: Status bar message: all the emails are visible in the index */
             _("all");
  }
  else
  {
    int count = (100 * (menu->top + menu->page_len)) / menu->max;
    /* L10N: Status bar, percentage of way through index.
           `%d` is the number, `%%` is the percent symbol.
           They may be reordered, or space inserted, if you wish. */
    snprintf(tmp, sizeof(tmp), _("%d%%"), count);
    cp = tmp;
  }

  format_string(fmt, sizeof(fmt), cp, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);

  return snprintf(buf, buf_len, "%s", fmt);
}

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
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mutt/lib.h"
#include "address/address.h"
#include "config/lib.h"
#include "email/lib.h"
#include "core/neomutt.h"
#include "alias/lib.h"
#include "gui/curs_lib.h"
#include "expando/lib.h"
#include "ncrypt/lib.h"
#include "hdrline.h"
#include "helpers.h"
#include "hook.h"
#include "maillist.h"
#include "mutt_thread.h"
#include "muttlib.h"
#include "mx.h"
#include "sort.h"
#include "subjectrx.h"
#ifdef USE_NOTMUCH
#include "notmuch/lib.h"
#endif

static bool thread_is_new(struct Email *e)
{
  return e->collapsed && (e->num_hidden > 1) && (mutt_thread_contains_unread(e) == 1);
}

/**
 * thread_is_old - Does the email thread contain any unread emails?
 * @param e Email
 * @retval true Thread contains unread mail
 */
static bool thread_is_old(struct Email *e)
{
  return e->collapsed && (e->num_hidden > 1) && (mutt_thread_contains_unread(e) == 2);
}

static bool user_in_addr(struct AddressList *al)
{
  struct Address *a = NULL;
  TAILQ_FOREACH(a, al, entries)
  if (mutt_addr_is_user(a))
    return true;
  return false;
}

static enum ToChars user_is_recipient(struct Email *e)
{
  if (!e || !e->env)
    return FLAG_CHAR_TO_NOT_IN_THE_LIST;

  struct Envelope *env = e->env;

  if (!e->recip_valid)
  {
    e->recip_valid = true;

    if (mutt_addr_is_user(TAILQ_FIRST(&env->from)))
    {
      e->recipient = FLAG_CHAR_TO_ORIGINATOR;
    }
    else if (user_in_addr(&env->to))
    {
      if (TAILQ_NEXT(TAILQ_FIRST(&env->to), entries) || !TAILQ_EMPTY(&env->cc))
        e->recipient = FLAG_CHAR_TO_TO; /* non-unique recipient */
      else
        e->recipient = FLAG_CHAR_TO_UNIQUE; /* unique recipient */
    }
    else if (user_in_addr(&env->cc))
    {
      e->recipient = FLAG_CHAR_TO_CC;
    }
    else if (check_for_mailing_list(&env->to, NULL, NULL, 0))
    {
      e->recipient = FLAG_CHAR_TO_SUBSCRIBED_LIST;
    }
    else if (check_for_mailing_list(&env->cc, NULL, NULL, 0))
    {
      e->recipient = FLAG_CHAR_TO_SUBSCRIBED_LIST;
    }
    else if (user_in_addr(&env->reply_to))
    {
      e->recipient = FLAG_CHAR_TO_REPLY_TO;
    }
    else
    {
      e->recipient = FLAG_CHAR_TO_NOT_IN_THE_LIST;
    }
  }

  return e->recipient;
}

/**
 * make_from_prefix - Create a prefix for an author field
 * @param disp   Type of field
 * @retval ptr Prefix string (do not free it)
 *
 * If $from_chars is set, pick an appropriate character from it.
 * If not, use the default prefix: "To", "Cc", etc
 */
static const char *make_from_prefix(enum FieldType disp)
{
  /* need 2 bytes at the end, one for the space, another for NUL */
  static char padded[8];
  static const char *long_prefixes[DISP_MAX] = {
    [DISP_TO] = "To ", [DISP_CC] = "Cc ", [DISP_BCC] = "Bcc ",
    [DISP_FROM] = "",  [DISP_PLAIN] = "",
  };

  const struct MbTable *c_from_chars = cs_subset_mbtable(NeoMutt->sub, "from_chars");

  if (!c_from_chars || !c_from_chars->chars || (c_from_chars->len == 0))
    return long_prefixes[disp];

  const char *pchar = mbtable_get_nth_wchar(c_from_chars, disp);
  if (mutt_str_len(pchar) == 0)
    return "";

  snprintf(padded, sizeof(padded), "%s ", pchar);
  return padded;
}

/**
 * make_from - Generate a From: field (with optional prefix)
 * @param env      Envelope of the email
 * @param buf      Buffer to store the result
 * @param buflen   Size of the buffer
 * @param do_lists Should we check for mailing lists?
 * @param flags    Format flags, see #MuttFormatFlags
 *
 * Generate the %F or %L field in $index_format.
 * This is the author, or recipient of the email.
 *
 * The field can optionally be prefixed by a character from $from_chars.
 * If $from_chars is not set, the prefix will be, "To", "Cc", etc
 */
static void make_from(struct Envelope *env, char *buf, size_t buflen,
                      bool do_lists, MuttFormatFlags flags)
{
  if (!env || !buf)
    return;

  bool me;
  enum FieldType disp;
  struct AddressList *name = NULL;

  me = mutt_addr_is_user(TAILQ_FIRST(&env->from));

  if (do_lists || me)
  {
    if (check_for_mailing_list(&env->to, make_from_prefix(DISP_TO), buf, buflen))
      return;
    if (check_for_mailing_list(&env->cc, make_from_prefix(DISP_CC), buf, buflen))
      return;
  }

  if (me && !TAILQ_EMPTY(&env->to))
  {
    disp = (flags & MUTT_FORMAT_PLAIN) ? DISP_PLAIN : DISP_TO;
    name = &env->to;
  }
  else if (me && !TAILQ_EMPTY(&env->cc))
  {
    disp = DISP_CC;
    name = &env->cc;
  }
  else if (me && !TAILQ_EMPTY(&env->bcc))
  {
    disp = DISP_BCC;
    name = &env->bcc;
  }
  else if (!TAILQ_EMPTY(&env->from))
  {
    disp = DISP_FROM;
    name = &env->from;
  }
  else
  {
    *buf = '\0';
    return;
  }

  snprintf(buf, buflen, "%s%s", make_from_prefix(disp), mutt_get_name(TAILQ_FIRST(name)));
}

/**
 * make_from_addr - Create a 'from' address for a reply email
 * @param env      Envelope of current email
 * @param buf      Buffer for the result
 * @param buflen   Length of buffer
 * @param do_lists If true, check for mailing lists
 */
static void make_from_addr(struct Envelope *env, char *buf, size_t buflen, bool do_lists)
{
  if (!env || !buf)
    return;

  bool me = mutt_addr_is_user(TAILQ_FIRST(&env->from));

  if (do_lists || me)
  {
    if (check_for_mailing_list_addr(&env->to, buf, buflen))
      return;
    if (check_for_mailing_list_addr(&env->cc, buf, buflen))
      return;
  }

  if (me && !TAILQ_EMPTY(&env->to))
    snprintf(buf, buflen, "%s", buf_string(TAILQ_FIRST(&env->to)->mailbox));
  else if (me && !TAILQ_EMPTY(&env->cc))
    snprintf(buf, buflen, "%s", buf_string(TAILQ_FIRST(&env->cc)->mailbox));
  else if (!TAILQ_EMPTY(&env->from))
    mutt_str_copy(buf, buf_string(TAILQ_FIRST(&env->from)->mailbox), buflen);
  else
    *buf = '\0';
}

int index_date(const struct ExpandoNode *self, char *buf, int buf_len,
               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_DATE);
  assert(self->ndata != NULL);

  struct ExpandoDatePrivate *dp = (struct ExpandoDatePrivate *) self->ndata;
  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  struct tm tm = { 0 };
  char fmt[128], tmp[128] = { 0 }, tmp2[128] = { 0 };
  const int len = self->end - self->start;

  switch (dp->date_type)
  {
    case EDT_LOCAL_SEND_TIME:
      tm = mutt_date_localtime(email->date_sent);
      break;

    case EDT_LOCAL_RECIEVE_TIME:
      tm = mutt_date_localtime(email->received);
      break;

    case EDT_SENDER_SEND_TIME:
    {
      /* restore sender's time zone */
      time_t now = email->date_sent;
      if (email->zoccident)
        now -= (email->zhours * 3600 + email->zminutes * 60);
      else
        now += (email->zhours * 3600 + email->zminutes * 60);
      tm = mutt_date_gmtime(now);
    }
    break;

    default:
      assert(0 && "Unknown date type.");
  }

  memcpy(tmp2, self->start, len);
  if (dp->use_c_locale)
  {
    strftime_l(tmp, sizeof(tmp), tmp2, &tm, NeoMutt->time_c_locale);
  }
  else
  {
    strftime(tmp, sizeof(tmp), tmp2, &tm);
  }

  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_DATE,
                MT_COLOR_INDEX, NULL, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_format_hook_callback(const struct ExpandoNode *self, char *buf, int buf_len,
                               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_INDEX_FORMAT_HOOK);

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;
  struct Mailbox *mailbox = hfi->mailbox;

  char tmp[128] = { 0 }, tmp2[128] = { 0 }, fmt[128];
  const int len = self->end - self->start;
  assert(len < sizeof(tmp));
  memcpy(tmp2, self->start, len);

  // FIXME(g0mb4): save parsed expando records somewhere
  const char *fmt_str = NONULL(mutt_idxfmt_hook(tmp2, mailbox, email));
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;
  expando_tree_parse(&root, &fmt_str, EFMTI_INDEX_FORMAT, &error);

  int n = 0;
  if (error.position == NULL)
  {
    mutt_expando_format_2gmb(tmp, sizeof(tmp), 0, sizeof(tmp), &root, data, flags);
    format_string_simple(fmt, sizeof(fmt), tmp, MUTT_FORMAT_NO_FLAGS);
    n = snprintf(buf, buf_len, "%s", fmt);
  }

  expando_tree_free(&root);
  return n;
}

/**
 * make_from_prefix - Create a prefix for an author field
 * @param disp   Type of field
 * @retval ptr Prefix string (do not free it)
 *
 * If $from_chars is set, pick an appropriate character from it.
 * If not, use the default prefix: "To", "Cc", etc
 */
static const char *make_from_prefix_2gmb(enum FieldType disp)
{
  /* need 2 bytes at the end, one for the space, another for NUL */
  static char padded[8];
  static const char *long_prefixes[DISP_MAX] = {
    [DISP_TO] = "To ", [DISP_CC] = "Cc ", [DISP_BCC] = "Bcc ",
    [DISP_FROM] = "",  [DISP_PLAIN] = "",
  };

  const struct MbTable *c_from_chars = cs_subset_mbtable(NeoMutt->sub, "from_chars");

  if (!c_from_chars || !c_from_chars->chars || (c_from_chars->len == 0))
    return long_prefixes[disp];

  const char *pchar = mbtable_get_nth_wchar(c_from_chars, disp);
  if (mutt_str_len(pchar) == 0)
    return "";

  snprintf(padded, sizeof(padded), "%s ", pchar);
  return padded;
}

/**
 * make_from - Generate a From: field (with optional prefix)
 * @param env      Envelope of the email
 * @param buf      Buffer to store the result
 * @param buflen   Size of the buffer
 * @param do_lists Should we check for mailing lists?
 * @param flags    Format flags, see #MuttFormatFlags
 *
 * Generate the %F or %L field in $index_format.
 * This is the author, or recipient of the email.
 *
 * The field can optionally be prefixed by a character from $from_chars.
 * If $from_chars is not set, the prefix will be, "To", "Cc", etc
 */
static void make_from_2gmb(struct Envelope *env, char *buf, size_t buflen,
                           bool do_lists, MuttFormatFlags flags)
{
  if (!env || !buf)
    return;

  bool me;
  enum FieldType disp;
  struct AddressList *name = NULL;

  me = mutt_addr_is_user(TAILQ_FIRST(&env->from));

  if (do_lists || me)
  {
    if (check_for_mailing_list(&env->to, make_from_prefix_2gmb(DISP_TO), buf, buflen))
      return;
    if (check_for_mailing_list(&env->cc, make_from_prefix_2gmb(DISP_CC), buf, buflen))
      return;
  }

  if (me && !TAILQ_EMPTY(&env->to))
  {
    disp = (flags & MUTT_FORMAT_PLAIN) ? DISP_PLAIN : DISP_TO;
    name = &env->to;
  }
  else if (me && !TAILQ_EMPTY(&env->cc))
  {
    disp = DISP_CC;
    name = &env->cc;
  }
  else if (me && !TAILQ_EMPTY(&env->bcc))
  {
    disp = DISP_BCC;
    name = &env->bcc;
  }
  else if (!TAILQ_EMPTY(&env->from))
  {
    disp = DISP_FROM;
    name = &env->from;
  }
  else
  {
    *buf = '\0';
    return;
  }

  snprintf(buf, buflen, "%s%s", make_from_prefix_2gmb(disp),
           mutt_get_name(TAILQ_FIRST(name)));
}

int index_a(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct Address *from = TAILQ_FIRST(&email->env->from);

  char fmt[128];
  const char *s = "";
  if (from && from->mailbox)
  {
    s = mutt_addr_for_display(from);
  }

  format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_AUTHOR,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_A(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct Address *reply_to = TAILQ_FIRST(&email->env->reply_to);

  char fmt[128];

  if (reply_to && reply_to->mailbox)
  {
    const char *s = mutt_addr_for_display(reply_to);
    format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_AUTHOR,
                  MT_COLOR_INDEX, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  return index_a(self, buf, buf_len, cols_len, data, flags);
}

int index_b(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Mailbox *mailbox = hfi->mailbox;

  char tmp[128], fmt[128];
  char *p = NULL;

  if (mailbox)
  {
    p = strrchr(mailbox_path(mailbox), '/');

#ifdef USE_NOTMUCH
    struct Email *email = hfi->email;
    if (mailbox->type == MUTT_NOTMUCH)
    {
      char *rel_path = nm_email_get_folder_rel_db(mailbox, email);
      if (rel_path)
      {
        p = rel_path;
      }
    }
#endif /* USE_NOTMUCH */

    if (p)
    {
      mutt_str_copy(tmp, p + 1, sizeof(tmp));
    }
    else
    {
      mutt_str_copy(tmp, mailbox_path(mailbox), sizeof(tmp));
    }
  }
  else
  {
    mutt_str_copy(tmp, "(null)", sizeof(tmp));
  }

  format_string(fmt, sizeof(fmt), tmp, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_B(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char tmp1[128], tmp2[128], fmt[128];

  if (first_mailing_list(tmp1, sizeof(tmp1), &email->env->to) ||
      first_mailing_list(tmp1, sizeof(tmp1), &email->env->cc))
  {
    mutt_str_copy(tmp2, tmp1, sizeof(tmp2));
    format_string(fmt, sizeof(fmt), tmp2, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  return index_b(self, buf, buf_len, cols_len, data, flags);
}

int index_c(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128], tmp[128];

  mutt_str_pretty_size(tmp, sizeof(tmp), email->body->length);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_SIZE,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_cr(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128], tmp[128];

  mutt_str_pretty_size(tmp, sizeof(tmp), email_size(email));
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_SIZE,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_C(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  const int num = email->msgno + 1;
  format_int(fmt, sizeof(fmt), num, flags, MT_COLOR_INDEX_NUMBER, MT_COLOR_INDEX, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_d(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  // NOTE(g0mb4): $date_format as expando?
  const char *c_date_format = cs_subset_string(NeoMutt->sub, "date_format");
  const char *cp = NONULL(c_date_format);
  bool use_c_locale = false;
  if (*cp == '!')
  {
    use_c_locale = true;
    cp++;
  }

  /* restore sender's time zone */
  time_t now = email->date_sent;
  if (email->zoccident)
    now -= (email->zhours * 3600 + email->zminutes * 60);
  else
    now += (email->zhours * 3600 + email->zminutes * 60);

  struct tm tm = mutt_date_gmtime(now);
  char fmt[128], tmp[128];

  if (use_c_locale)
  {
    strftime_l(tmp, sizeof(tmp), cp, &tm, NeoMutt->time_c_locale);
  }
  else
  {
    strftime(tmp, sizeof(tmp), cp, &tm);
  }

  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_DATE,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_D(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  // NOTE(g0mb4): $date_format as expando?
  const char *c_date_format = cs_subset_string(NeoMutt->sub, "date_format");
  const char *cp = NONULL(c_date_format);
  bool use_c_locale = false;
  if (*cp == '!')
  {
    use_c_locale = true;
    cp++;
  }

  struct tm tm = mutt_date_localtime(email->date_sent);
  char fmt[128], tmp[128];

  if (use_c_locale)
  {
    strftime_l(tmp, sizeof(tmp), cp, &tm, NeoMutt->time_c_locale);
  }
  else
  {
    strftime(tmp, sizeof(tmp), cp, &tm);
  }

  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_DATE,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_e(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;
  struct Mailbox *mailbox = hfi->mailbox;

  char fmt[128];

  const int num = mutt_messages_in_thread(mailbox, email, MIT_POSITION);
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_E(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;
  struct Mailbox *mailbox = hfi->mailbox;

  char fmt[128];

  const int num = mutt_messages_in_thread(mailbox, email, MIT_NUM_MESSAGES);
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_f(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char tmp[128], fmt[128];

  struct Buffer *tmpbuf = buf_pool_get();
  mutt_addrlist_write(&email->env->from, tmpbuf, true);
  mutt_str_copy(tmp, buf_string(tmpbuf), sizeof(tmp));
  buf_pool_release(&tmpbuf);

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_F(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char tmp[128], fmt[128];

  make_from(email->env, tmp, sizeof(tmp), false, MUTT_FORMAT_NO_FLAGS);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_AUTHOR,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_Fp(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char tmp[128], fmt[128];

  make_from(email->env, tmp, sizeof(tmp), false, MUTT_FORMAT_PLAIN);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_AUTHOR,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_g(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char fmt[128];
  char *tags = driver_tags_get_transformed(&email->tags);

  const char *s = NONULL(tags);
  format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_TAGS, MT_COLOR_INDEX,
                format, NO_TREE);
  const int n = snprintf(buf, buf_len, "%s", fmt);
  FREE(&tags);
  return n;
}

static int index_Gx(char x, const struct ExpandoNode *self, char *buf, int buf_len,
                    int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char fmt[128];
  char tag_format[3];

  tag_format[0] = 'G';
  tag_format[1] = x;
  tag_format[2] = '\0';

  char *tag = mutt_hash_find(TagFormats, tag_format);
  if (tag)
  {
    char *tags = driver_tags_get_transformed_for(&email->tags, tag);
    const char *s = NONULL(tags);
    format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_TAG,
                  MT_COLOR_INDEX, format, NO_TREE);
    const int n = snprintf(buf, buf_len, "%s", fmt);
    FREE(&tags);
    return n;
  }
  else
  {
    return 0;
  }
}

int index_G0(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G1(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G2(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G3(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G4(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G5(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G6(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G7(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G8(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_G9(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  return index_Gx('0', self, buf, buf_len, cols_len, data, flags);
}

int index_H(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char fmt[128];

  const char *s = buf_string(&email->env->spam);
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_i(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char fmt[128];

  const char *s = email->env->message_id ? email->env->message_id : "<no.id>";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_I(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct Address *from = TAILQ_FIRST(&email->env->from);

  char tmp[128], fmt[128];

  if (mutt_mb_get_initials(mutt_get_name(from), tmp, sizeof(tmp)))
  {
    format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_AUTHOR,
                  MT_COLOR_INDEX, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  return index_a(self, buf, buf_len, cols_len, data, flags);
}

int index_J(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  char fmt[128];

  bool have_tags = true;
  char *tags = driver_tags_get_transformed(&email->tags);
  if (tags)
  {
    if (flags & MUTT_FORMAT_TREE)
    {
      char *parent_tags = NULL;
      if (email->thread->prev && email->thread->prev->message)
      {
        parent_tags = driver_tags_get_transformed(&email->thread->prev->message->tags);
      }
      if (!parent_tags && email->thread->parent && email->thread->parent->message)
      {
        parent_tags = driver_tags_get_transformed(
            &email->thread->parent->message->tags);
      }
      if (parent_tags && mutt_istr_equal(tags, parent_tags))
        have_tags = false;
      FREE(&parent_tags);
    }
  }
  else
  {
    have_tags = false;
  }

  const char *s = have_tags ? tags : "";
  format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_TAGS, MT_COLOR_INDEX,
                format, NO_TREE);
  const int n = snprintf(buf, buf_len, "%s", fmt);
  FREE(&tags);

  return n;
}

int index_K(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char tmp1[128], tmp2[128], fmt[128];

  if (first_mailing_list(tmp1, sizeof(tmp1), &email->env->to) ||
      first_mailing_list(tmp1, sizeof(tmp1), &email->env->cc))
  {
    mutt_str_copy(tmp2, tmp1, sizeof(tmp2));
    format_string(fmt, sizeof(fmt), tmp2, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  const char *s = "";
  format_string(fmt, sizeof(fmt), s, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_l(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  const int num = email->lines;
  format_int(fmt, sizeof(fmt), num, flags, MT_COLOR_INDEX_NUMBER, MT_COLOR_INDEX, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_L(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128], tmp[128];

  make_from_2gmb(email->env, tmp, sizeof(tmp), true, flags);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_AUTHOR,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_m(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Mailbox *mailbox = hfi->mailbox;

  char fmt[128];

  if (mailbox)
  {
    const int num = mailbox->msg_count;
    format_int_simple(fmt, sizeof(fmt), num, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else
  {
    const char *s = "(null)";
    format_string_simple(fmt, sizeof(fmt), s, NULL);
    return snprintf(buf, buf_len, "%s", fmt);
  }
}

int index_M(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;
  const bool threads = mutt_using_threads();
  const bool is_index = (flags & MUTT_FORMAT_INDEX) != 0;

  char fmt[128];

  if (threads && is_index && email->collapsed && (email->num_hidden > 1))
  {
    const int num = email->num_hidden;
    format_int(fmt, sizeof(fmt), num, flags, MT_COLOR_INDEX_COLLAPSED, MT_COLOR_INDEX, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else if (is_index && threads)
  {
    const char *s = " ";
    format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_COLLAPSED,
                  MT_COLOR_INDEX, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else
  {
    return 0;
  }
}

int index_n(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;
  const struct Address *from = TAILQ_FIRST(&email->env->from);

  char fmt[128];

  const char *s = mutt_get_name(from);
  format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_AUTHOR,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_N(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  const int num = email->score;
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_O(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char tmp[128], fmt[128];
  char *p = NULL;

  make_from_addr(email->env, tmp, sizeof(tmp), true);
  const bool c_save_address = cs_subset_bool(NeoMutt->sub, "save_address");
  if (!c_save_address && (p = strpbrk(tmp, "%@")))
  {
    *p = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_P(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;

  char fmt[128];

  const char *s = hfi->pager_progress;
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_q(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);

#ifdef USE_NNTP
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  const char *s = NONULL(email->env->newsgroups);
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* USE_NNTP */
  return 0;
#endif /* USE_NNTP */
}

int index_r(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char tmp[128], fmt[128];

  struct Buffer *tmpbuf = buf_pool_get();
  mutt_addrlist_write(&email->env->to, tmpbuf, true);
  mutt_str_copy(tmp, buf_string(tmpbuf), sizeof(tmp));
  buf_pool_release(&tmpbuf);

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_R(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char tmp[128], fmt[128];

  struct Buffer *tmpbuf = buf_pool_get();
  mutt_addrlist_write(&email->env->cc, tmpbuf, true);
  mutt_str_copy(tmp, buf_string(tmpbuf), sizeof(tmp));
  buf_pool_release(&tmpbuf);

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_s(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128], tmp[128];

  subjrx_apply_mods(email->env);
  char *subj = NULL;

  if (email->env->disp_subj)
    subj = email->env->disp_subj;
  else
    subj = email->env->subject;

  if (flags & MUTT_FORMAT_TREE && !email->collapsed)
  {
    if (flags & MUTT_FORMAT_FORCESUBJ)
    {
      snprintf(tmp, sizeof(tmp), "%s%s", email->tree, NONULL(subj));
      format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_SUBJECT,
                    MT_COLOR_INDEX, format, HAS_TREE);
    }
    else
    {
      format_string(fmt, sizeof(fmt), email->tree, flags,
                    MT_COLOR_INDEX_SUBJECT, MT_COLOR_INDEX, format, HAS_TREE);
    }
  }
  else
  {
    format_string(fmt, sizeof(fmt), NONULL(subj), flags, MT_COLOR_INDEX_SUBJECT,
                  MT_COLOR_INDEX, format, NO_TREE);
  }

  return snprintf(buf, buf_len, "%s", fmt);
}

int index_S(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const int msg_in_pager = hfi->msg_in_pager;

  char fmt[128], tmp[128];

  const char *wch = NULL;
  if (email->deleted)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED);
  else if (email->attach_del)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED_ATTACH);
  else if (email->tagged)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_TAGGED);
  else if (email->flagged)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_IMPORTANT);
  else if (email->replied)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_REPLIED);
  else if (email->read && (msg_in_pager != email->msgno))
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_SEMPTY);
  else if (email->old)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD);
  else
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW);

  snprintf(tmp, sizeof(tmp), "%s", wch);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_FLAGS,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_t(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct Address *to = TAILQ_FIRST(&email->env->to);
  const struct Address *cc = TAILQ_FIRST(&email->env->cc);

  char tmp[128], fmt[128];

  if (!check_for_mailing_list(&email->env->to, "To ", tmp, sizeof(tmp)) &&
      !check_for_mailing_list(&email->env->cc, "Cc ", tmp, sizeof(tmp)))
  {
    if (to)
      snprintf(tmp, sizeof(tmp), "To %s", mutt_get_name(to));
    else if (cc)
      snprintf(tmp, sizeof(tmp), "Cc %s", mutt_get_name(cc));
    else
    {
      tmp[0] = '\0';
    }
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_T(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");

  char fmt[128];
  int i;
  const char *s = (c_to_chars && ((i = user_is_recipient(email))) < c_to_chars->len) ?
                      c_to_chars->chars[i] :
                      " ";

  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_u(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct Address *from = TAILQ_FIRST(&email->env->from);

  char tmp[128], fmt[128];
  char *p = NULL;

  if (from && from->mailbox)
  {
    mutt_str_copy(tmp, mutt_addr_for_display(from), sizeof(tmp));
    p = strpbrk(tmp, "%@");
    if (p)
    {
      *p = '\0';
    }
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_v(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct Address *from = TAILQ_FIRST(&email->env->from);
  const struct Address *to = TAILQ_FIRST(&email->env->to);
  const struct Address *cc = TAILQ_FIRST(&email->env->cc);

  char tmp[128], fmt[128];
  char *p = NULL;

  if (mutt_addr_is_user(from))
  {
    if (to)
    {
      const char *s = mutt_get_name(to);
      format_string_simple(tmp, sizeof(tmp), s, format);
    }
    else if (cc)
    {
      const char *s = mutt_get_name(cc);
      format_string_simple(tmp, sizeof(tmp), s, format);
    }
    else
    {
      tmp[0] = '\0';
    }
  }
  else
  {
    const char *s = mutt_get_name(from);
    format_string_simple(tmp, sizeof(tmp), s, format);
  }
  p = strpbrk(tmp, " %@");
  if (p)
  {
    *p = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_W(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  const char *s = NONULL(email->env->organization);
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_x(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
#ifdef USE_NNTP

  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  const char *s = NONULL(email->env->x_comment_to);
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
#else  /* USE_NNTP */
  return 0;
#endif /* USE_NNTP */
}

int index_X(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;
  struct Mailbox *mailbox = hfi->mailbox;

  char fmt[128];

  struct Message *msg = mx_msg_open(mailbox, email);
  if (msg)
  {
    const int num = mutt_count_body_parts(mailbox, email, msg->fp);
    mx_msg_close(mailbox, &msg);

    format_int_simple(fmt, sizeof(fmt), num, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else
  {
    return 0;
  }
}

int index_y(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  const char *s = NONULL(email->env->x_label);
  format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_LABEL,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_Y(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  char fmt[128];

  bool label = true;
  if (email->env->x_label)
  {
    struct Email *email_tmp = NULL;
    if (flags & MUTT_FORMAT_TREE && (email->thread->prev && email->thread->prev->message &&
                                     email->thread->prev->message->env->x_label))
    {
      email_tmp = email->thread->prev->message;
    }
    else if (flags & MUTT_FORMAT_TREE &&
             (email->thread->parent && email->thread->parent->message &&
              email->thread->parent->message->env->x_label))
    {
      email_tmp = email->thread->parent->message;
    }

    if (email_tmp && mutt_istr_equal(email->env->x_label, email_tmp->env->x_label))
    {
      label = false;
    }
  }
  else
  {
    label = false;
  }

  if (label)
  {
    const char *s = NONULL(email->env->x_label);
    format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_LABEL,
                  MT_COLOR_INDEX, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else
  {
    const char *s = "";
    format_string(fmt, sizeof(fmt), s, flags, MT_COLOR_INDEX_LABEL,
                  MT_COLOR_INDEX, format, NO_TREE);
    return snprintf(buf, buf_len, "%s", fmt);
  }
}

int index_zc(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *email = hfi->email;

  const struct MbTable *c_crypt_chars = cs_subset_mbtable(NeoMutt->sub, "crypt_chars");

  char tmp[128], fmt[128];

  const char *ch = NULL;
  if ((WithCrypto != 0) && (email->security & SEC_GOODSIGN))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_GOOD_SIGN);
  }
  else if ((WithCrypto != 0) && (email->security & SEC_ENCRYPT))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_ENCRYPTED);
  }
  else if ((WithCrypto != 0) && (email->security & SEC_SIGN))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_SIGNED);
  }
  else if (((WithCrypto & APPLICATION_PGP) != 0) && ((email->security & PGP_KEY) == PGP_KEY))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_CONTAINS_KEY);
  }
  else
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_NO_CRYPTO);
  }

  snprintf(tmp, sizeof(tmp), "%s", ch);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_FLAGS,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_zs(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  const bool threads = mutt_using_threads();
  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const int msg_in_pager = hfi->msg_in_pager;

  char tmp[128], fmt[128];

  const char *ch = NULL;
  if (email->deleted)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED);
  }
  else if (email->attach_del)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED_ATTACH);
  }
  else if (threads && thread_is_new(email))
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW_THREAD);
  }
  else if (threads && thread_is_old(email))
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD_THREAD);
  }
  else if (email->read && (msg_in_pager != email->msgno))
  {
    if (email->replied)
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_REPLIED);
    }
    else
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_ZEMPTY);
    }
  }
  else
  {
    if (email->old)
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD);
    }
    else
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW);
    }
  }

  snprintf(tmp, sizeof(tmp), "%s", ch);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_FLAGS,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_zt(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *email = hfi->email;

  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");

  char tmp[128], fmt[128];

  const char *ch = NULL;
  if (email->tagged)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_TAGGED);
  }
  else if (email->flagged)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_IMPORTANT);
  }
  else
  {
    ch = mbtable_get_nth_wchar(c_to_chars, user_is_recipient(email));
  }

  snprintf(tmp, sizeof(tmp), "%s", ch);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_FLAGS,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

int index_Z(const struct ExpandoNode *self, char *buf, int buf_len,
            int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;
  const int msg_in_pager = hfi->msg_in_pager;

  const struct MbTable *c_crypt_chars = cs_subset_mbtable(NeoMutt->sub, "crypt_chars");
  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");
  const bool threads = mutt_using_threads();

  char fmt[128], tmp[128];

  const char *first = NULL;
  if (threads && thread_is_new(e))
  {
    first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW_THREAD);
  }
  else if (threads && thread_is_old(e))
  {
    first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD_THREAD);
  }
  else if (e->read && (msg_in_pager != e->msgno))
  {
    if (e->replied)
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_REPLIED);
    }
    else
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_ZEMPTY);
    }
  }
  else
  {
    if (e->old)
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD);
    }
    else
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW);
    }
  }

  /* Marked for deletion; deleted attachments; crypto */
  const char *second = NULL;
  if (e->deleted)
    second = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED);
  else if (e->attach_del)
    second = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED_ATTACH);
  else if ((WithCrypto != 0) && (e->security & SEC_GOODSIGN))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_GOOD_SIGN);
  else if ((WithCrypto != 0) && (e->security & SEC_ENCRYPT))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_ENCRYPTED);
  else if ((WithCrypto != 0) && (e->security & SEC_SIGN))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_SIGNED);
  else if (((WithCrypto & APPLICATION_PGP) != 0) && (e->security & PGP_KEY))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_CONTAINS_KEY);
  else
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_NO_CRYPTO);

  /* Tagged, flagged and recipient flag */
  const char *third = NULL;
  if (e->tagged)
    third = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_TAGGED);
  else if (e->flagged)
    third = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_IMPORTANT);
  else
    third = mbtable_get_nth_wchar(c_to_chars, user_is_recipient(e));

  snprintf(tmp, sizeof(tmp), "%s%s%s", first, second, third);
  format_string(fmt, sizeof(fmt), tmp, flags, MT_COLOR_INDEX_FLAGS,
                MT_COLOR_INDEX, format, NO_TREE);
  return snprintf(buf, buf_len, "%s", fmt);
}

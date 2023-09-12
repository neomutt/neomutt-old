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
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "folder_format_callbacks.h"
#include "browser/lib.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "muttlib.h"
#include "node.h"

int folder_date(const struct ExpandoNode *self, char *buf, int buf_len,
                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_DATE);
  assert(self->ndata != NULL);

  struct ExpandoDatePrivate *dp = (struct ExpandoDatePrivate *) self->ndata;
  const struct Folder *folder = (const struct Folder *) data;

  if (folder->ff->local)
  {
    struct tm tm = { 0 };
    char fmt[128], tmp[128] = { 0 }, tmp2[128] = { 0 };
    const int len = self->end - self->start;

    tm = mutt_date_localtime(folder->ff->mtime);
    memcpy(tmp2, self->start, len);
    if (dp->use_c_locale)
    {
      strftime_l(tmp, sizeof(tmp), tmp2, &tm, NeoMutt->time_c_locale);
    }
    else
    {
      strftime(tmp, sizeof(tmp), tmp2, &tm);
    }

    format_string_simple(fmt, sizeof(fmt), tmp, MUTT_FORMAT_NO_FLAGS);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else
  {
    return 0;
  }
}

int folder_a(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const int num = folder->ff->notify_user;
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_C(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const int num = folder->num + 1;
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_d(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];
  bool use_c_locale = false;

  static const time_t one_year = 31536000;
  const char *t_fmt = ((mutt_date_now() - folder->ff->mtime) < one_year) ?
                          "%b %d %H:%M" :
                          "%b %d  %Y";

  if (use_c_locale)
  {
    mutt_date_localtime_format_locale(tmp, sizeof(tmp), t_fmt,
                                      folder->ff->mtime, NeoMutt->time_c_locale);
  }
  else
  {
    mutt_date_localtime_format(tmp, sizeof(tmp), t_fmt, folder->ff->mtime);
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_D(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];
  bool use_c_locale = false;
  const char *const c_date_format = cs_subset_string(NeoMutt->sub, "date_format");
  const char *t_fmt = NONULL(c_date_format);
  if (*t_fmt == '!')
  {
    t_fmt++;
    use_c_locale = true;
  }

  if (use_c_locale)
  {
    mutt_date_localtime_format_locale(tmp, sizeof(tmp), t_fmt,
                                      folder->ff->mtime, NeoMutt->time_c_locale);
  }
  else
  {
    mutt_date_localtime_format(tmp, sizeof(tmp), t_fmt, folder->ff->mtime);
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_f(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  const char *s = NONULL(folder->ff->name);

  snprintf(tmp, sizeof(tmp), "%s%s", s,
           folder->ff->local ?
               (S_ISLNK(folder->ff->mode) ?
                    "@" :
                    (S_ISDIR(folder->ff->mode) ?
                         "/" :
                         (((folder->ff->mode & S_IXUSR) != 0) ? "*" : ""))) :
               "");

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_F(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  if (folder->ff->local)
  {
    snprintf(tmp, sizeof(tmp), "%c%c%c%c%c%c%c%c%c%c",
             S_ISDIR(folder->ff->mode) ? 'd' : (S_ISLNK(folder->ff->mode) ? 'l' : '-'),
             ((folder->ff->mode & S_IRUSR) != 0) ? 'r' : '-',
             ((folder->ff->mode & S_IWUSR) != 0) ? 'w' : '-',
             ((folder->ff->mode & S_ISUID) != 0) ? 's' :
             ((folder->ff->mode & S_IXUSR) != 0) ? 'x' :
                                                   '-',
             ((folder->ff->mode & S_IRGRP) != 0) ? 'r' : '-',
             ((folder->ff->mode & S_IWGRP) != 0) ? 'w' : '-',
             ((folder->ff->mode & S_ISGID) != 0) ? 's' :
             ((folder->ff->mode & S_IXGRP) != 0) ? 'x' :
                                                   '-',
             ((folder->ff->mode & S_IROTH) != 0) ? 'r' : '-',
             ((folder->ff->mode & S_IWOTH) != 0) ? 'w' : '-',
             ((folder->ff->mode & S_ISVTX) != 0) ? 't' :
             ((folder->ff->mode & S_IXOTH) != 0) ? 'x' :
                                                   '-');
  }
#ifdef USE_IMAP
  else if (folder->ff->imap)
  {
    /* mark folders with subfolders AND mail */
    snprintf(tmp, sizeof(tmp), "IMAP %c",
             (folder->ff->inferiors && folder->ff->selectable) ? '+' : ' ');
  }
#endif
  else
  {
    tmp[0] = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_g(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  if (folder->ff->local)
  {
    struct group *gr = getgrgid(folder->ff->gid);
    if (gr)
    {
      mutt_str_copy(tmp, gr->gr_name, sizeof(tmp));
    }
    else
    {
      snprintf(tmp, sizeof(tmp), "%u", folder->ff->gid);
    }
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_i(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  const char *s = NULL;
  if (folder->ff->desc)
    s = folder->ff->desc;
  else
    s = folder->ff->name;

  snprintf(tmp, sizeof(tmp), "%s%s", s,
           folder->ff->local ?
               (S_ISLNK(folder->ff->mode) ?
                    "@" :
                    (S_ISDIR(folder->ff->mode) ?
                         "/" :
                         (((folder->ff->mode & S_IXUSR) != 0) ? "*" : ""))) :
               "");

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_l(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  if (folder->ff->local)
  {
    snprintf(tmp, sizeof(tmp), "%lu", folder->ff->nlink);
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_m(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  if (folder->ff->has_mailbox)
  {
    snprintf(tmp, sizeof(tmp), "%d", folder->ff->msg_count);
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_n(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  if (folder->ff->has_mailbox)
  {
    snprintf(tmp, sizeof(tmp), "%d", folder->ff->msg_unread);
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_N(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  // NOTE(g0mb4): use $to_chars?
  const char *s = folder->ff->has_new_mail ? "N" : " ";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_p(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  const int num = folder->ff->poll_new_mail;
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_s(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  mutt_str_pretty_size(tmp, sizeof(tmp), folder->ff->size);
  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_t(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char fmt[128];

  // NOTE(g0mb4): use $to_chars?
  const char *s = folder->ff->tagged ? "*" : " ";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int folder_u(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct Folder *folder = (const struct Folder *) data;

  char tmp[128], fmt[128];

  if (folder->ff->local)
  {
    struct passwd *pw = getpwuid(folder->ff->uid);
    if (pw)
    {
      mutt_str_copy(tmp, pw->pw_name, sizeof(tmp));
    }
    else
    {
      snprintf(tmp, sizeof(tmp), "%u", folder->ff->uid);
    }
  }
  else
  {
    tmp[0] = '\0';
  }

  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

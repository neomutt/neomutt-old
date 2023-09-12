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
#include "config/lib.h"
#include "email/lib.h"
#include "attach/lib.h"
#include "expando/lib.h"

#include "hdrline.h"
#include "muttlib.h"

int attach_c(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  // NOTE(g0mb4): use $to_chars?
  const char *s = ((aptr->body->type != TYPE_TEXT) || aptr->body->noconv) ? "n" : "c";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_C(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char tmp[128] = { 0 }, fmt[128];

  if (mutt_is_text_part(aptr->body) && mutt_body_get_charset(aptr->body, tmp, sizeof(tmp)))
  {
    format_string_simple(fmt, sizeof(fmt), tmp, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }
  else
  {
    const char *s = "";
    format_string_simple(fmt, sizeof(fmt), s, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }
}

int attach_d(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  const struct ExpandoRecord *c_message_format = cs_subset_expando(NeoMutt->sub, "message_format");
  if (aptr->body->description)
  {
    const char *s = aptr->body->description;
    format_string_simple(fmt, sizeof(fmt), s, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  if (mutt_is_message_type(aptr->body->type, aptr->body->subtype) &&
      c_message_format && aptr->body->email)
  {
    char tmp[128] = { 0 };
    mutt_make_string(tmp, sizeof(tmp), cols_len, c_message_format, NULL, -1,
                     aptr->body->email,
                     MUTT_FORMAT_FORCESUBJ | MUTT_FORMAT_ARROWCURSOR, NULL);
    if (*tmp)
    {
      format_string_simple(fmt, sizeof(fmt), tmp, format);
      return snprintf(buf, buf_len, "%s", fmt);
    }
  }

  if (!aptr->body->d_filename && !aptr->body->filename)
  {
    const char *s = "<no description>";
    format_string_simple(fmt, sizeof(fmt), s, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  return attach_F(self, buf, buf_len, cols_len, data, flags);
}

int attach_D(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  // NOTE(g0mb4): use $to_chars?
  const char *s = aptr->body->deleted ? "D" : " ";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_e(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  const char *s = ENCODING(aptr->body->encoding);
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_f(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  if (aptr->body->filename && (*aptr->body->filename == '/'))
  {
    struct Buffer *path = buf_pool_get();

    buf_strcpy(path, aptr->body->filename);
    buf_pretty_mailbox(path);
    const char *s = buf_string(path);
    format_string_simple(fmt, sizeof(fmt), s, format);
    const int n = snprintf(buf, buf_len, "%s", fmt);
    buf_pool_release(&path);
    return n;
  }
  else
  {
    const char *s = NONULL(aptr->body->filename);
    format_string_simple(fmt, sizeof(fmt), s, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }
}

int attach_F(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  if (aptr->body->d_filename)
  {
    const char *s = aptr->body->d_filename;
    format_string_simple(fmt, sizeof(fmt), s, format);
    return snprintf(buf, buf_len, "%s", fmt);
  }

  return attach_f(self, buf, buf_len, cols_len, data, flags);
}

int attach_I(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char tmp[128], fmt[128];

  // NOTE(g0mb4): use $to_chars?
  static const char dispchar[] = { 'I', 'A', 'F', '-' };
  char ch;

  if (aptr->body->disposition < sizeof(dispchar))
  {
    ch = dispchar[aptr->body->disposition];
  }
  else
  {
    mutt_debug(LL_DEBUG1, "ERROR: invalid content-disposition %d\n", aptr->body->disposition);
    ch = '!';
  }

  snprintf(tmp, sizeof(tmp), "%c", ch);
  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_m(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  const char *s = TYPE(aptr->body);
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_M(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  const char *s = aptr->body->subtype;
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_n(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  const int num = aptr->num + 1;
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_Q(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  // NOTE(g0mb4): use $to_chars?
  const char *s = aptr->body->attach_qualifies ? "Q" : " ";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_s(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char tmp[128] = { 0 }, fmt[128];

  size_t l = 0;
  if (aptr->body->filename && (flags & MUTT_FORMAT_STAT_FILE))
  {
    l = mutt_file_get_size(aptr->body->filename);
  }
  else
  {
    l = aptr->body->length;
  }

  mutt_str_pretty_size(tmp, sizeof(tmp), l);
  format_string_simple(fmt, sizeof(fmt), tmp, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_t(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  // NOTE(g0mb4): use $to_chars?
  const char *s = aptr->body->tagged ? "*" : " ";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_T(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  const char *s = NONULL(aptr->tree);
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_u(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  // NOTE(g0mb4): use $to_chars?
  const char *s = aptr->body->unlink ? "-" : " ";
  format_string_simple(fmt, sizeof(fmt), s, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

int attach_X(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_EXPANDO);
  const struct ExpandoFormatPrivate *format =
      (const struct ExpandoFormatPrivate *) self->ndata;

  const struct AttachPtr *aptr = (const struct AttachPtr *) data;

  char fmt[128];

  const int num = aptr->body->attach_count + aptr->body->attach_qualifies;
  format_int_simple(fmt, sizeof(fmt), num, format);
  return snprintf(buf, buf_len, "%s", fmt);
}

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
 * @page expando_helpers XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "mutt/lib.h"
#include "gui/lib.h"
#include "helpers.h"
#include "format_callbacks.h"
#include "mutt_thread.h"
#include "node.h"

/**
 * mb_strlen_nonnull - Measure a non null-terminated string's length (number of characters)
 * @param start      Start of the string
 * @param end        End of the string
 * @return int       Number of characters
 * 
 */
int mb_strlen_nonnull(const char *start, const char *end)
{
  assert(end >= start);

  int len = 0;
  int total_len = 0;
  const char *s = start;
  while (*s && s < end)
  {
    len = mblen(s, MB_CUR_MAX);
    if (len == 0 || len < 0)
    {
      break;
    }

    s += len;
    ++total_len;
  }
  return total_len;
}

/**
 * mb_strwidth_nonnull - Measure a non null-terminated string's display width (in screen columns)
 * @param start     Start of the string
 * @param end       End of the string
 * @return int      Number of columns the string requires
 */
int mb_strwidth_nonnull(const char *start, const char *end)
{
  assert(end >= start);

  wchar_t wc = 0;
  int len = 0;
  int width = 0;
  const char *s = start;
  while (*s && s < end)
  {
    if (mbtowc(&wc, s, MB_CUR_MAX) >= 0)
    {
      len = wcwidth(wc);
    }
    else
    {
      len = 0;
    }

    width += len;
    s++;
  }
  return width;
}

static size_t add_index_color(char *buf, int buflen, MuttFormatFlags flags, enum ColorId color)
{
  /* only add color markers if we are operating on main index entries. */
  if (!(flags & MUTT_FORMAT_INDEX))
    return 0;

  /* handle negative buflen */
  if (buflen <= 2)
    return 0;

  if (color == MT_COLOR_INDEX)
  {
    /* buf might be uninitialized other cases */
    const size_t len = mutt_str_len(buf);
    buf += len;
    buflen -= len;
  }

  if (buflen <= 2)
    return 0;

  buf[0] = MUTT_SPECIAL_INDEX;
  buf[1] = color;
  buf[2] = '\0';

  return 2;
}

void format_string(char *buf, int buf_len, const char *s, MuttFormatFlags flags,
                   enum ColorId pre, enum ColorId post,
                   const struct ExpandoFormatPrivate *format, enum HasTreeChars has_tree)
{
  if (format)
  {
    const size_t colorlen1 = add_index_color(buf, buf_len, flags, pre);
    mutt_simple_format(buf + colorlen1, buf_len - colorlen1, format->min,
                       format->max, format->justification, format->leader, s,
                       strlen(s), has_tree == HAS_TREE);
    const int n = strlen(buf);
    add_index_color(buf + n, buf_len - n, flags, post);
  }
  else
  {
    const size_t colorlen1 = add_index_color(buf, buf_len, flags, pre);
    const int n = snprintf(buf + colorlen1, buf_len - colorlen1, "%s", s);
    add_index_color(buf + colorlen1 + n, buf_len - colorlen1 - n, flags, post);
  }
}

void format_string_flags(char *buf, int buf_len, const char *s, MuttFormatFlags flags,
                         const struct ExpandoFormatPrivate *format)
{
  assert((flags & MUTT_FORMAT_INDEX) == 0);
  format_string(buf, buf_len, s, flags, 0, 0, format, NO_TREE);
}

void format_string_simple(char *buf, int buf_len, const char *s,
                          const struct ExpandoFormatPrivate *format)
{
  format_string(buf, buf_len, s, MUTT_FORMAT_NO_FLAGS, 0, 0, format, NO_TREE);
}

void format_int(char *buf, int buf_len, int number, MuttFormatFlags flags, enum ColorId pre,
                enum ColorId post, const struct ExpandoFormatPrivate *format)
{
  char tmp[32]; // 64 bit INT_MAX has 20 digits
  snprintf(tmp, sizeof(tmp), "%d", number);
  format_string(buf, buf_len, tmp, flags, pre, post, format, NO_TREE);
}

void format_int_flags(char *buf, int buf_len, int number, MuttFormatFlags flags,
                      const struct ExpandoFormatPrivate *format)
{
  assert((flags & MUTT_FORMAT_INDEX) == 0);
  format_int(buf, buf_len, number, flags, 0, 0, format);
}

void format_int_simple(char *buf, int buf_len, int number,
                       const struct ExpandoFormatPrivate *format)
{
  format_int(buf, buf_len, number, MUTT_FORMAT_NO_FLAGS, 0, 0, format);
}

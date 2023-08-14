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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include "helpers.h"

/**
 * mb_strlen_nonnull - Measure a non null-terminated string's length (number of characers)
 * @param start      Start of the string
 * @param end        End of the string
 * @return int       Number of characters
 */
int mb_strlen_nonnull(const char *start, const char *end)
{
  int len = 0;
  const char *s = start;
  while (*s && s < end)
  {
    if ((*s & 0xC0) != 0x80)
    {
      len++;
    }

    s++;
  }
  return len;
}

/**
 * mb_strwidth_nonnull - Measure a non null-terminated string's display width (in screen columns)
 * @param start     Start of the string
 * @param end       End of the string
 * @return int      Number of columns the string requires
 */
int mb_strwidth_nonnull(const char *start, const char *end)
{
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

void strftime_range(char *s, size_t max, const char *format_start,
                    const char *format_end, const struct tm *tm)
{
  char tmp[256] = { 0 };
  const int len = format_end - format_start;

  assert(len < sizeof(tmp) - 1);
  assert(max < sizeof(tmp) - 1);

  memcpy(tmp, format_start, len);
  strftime(s, max, tmp, tm);
}

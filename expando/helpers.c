#include "helpers.h"

#include <stdlib.h>
#include <wchar.h>

/**
 * mb_strlen_range - Measure a non null-terminated string's length (number of characers)
 * @param start      Start of the string
 * @param end        End of the string
 * @return int       Number of characters
 */
int mb_strlen_range(const char *start, const char *end)
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
 * mb_strwidth_range - Measure a non null-terminated string's display width (in screen columns)
 * @param start     Start of the string
 * @param end       End of the string
 * @return int      Number of columns the string requires
 */
int mb_strwidth_range(const char *start, const char *end)
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

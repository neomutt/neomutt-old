#include "helpers.h"

#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

int mb_strlen_range(const char *start, const char *end)
{
  int len = 0;
  const char *s = start;
  while (s < end)
  {
    if ((*s & 0xC0) != 0x80)
    {
      len++;
    }

    s++;
  }
  return len;
}

int mb_strwidth_range(const char *start, const char *end)
{
  wchar_t wc = 0;
  int len = 0;
  int width = 0;
  const char *s = start;
  while (s < end)
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

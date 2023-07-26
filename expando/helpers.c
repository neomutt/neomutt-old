#include "helpers.h"

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
  // TODO(g0mb4): implement me
  return mb_strlen_range(start, end);
}
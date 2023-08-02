#include <assert.h>
#include <string.h>
#include "format_callbacks.h"
#include "parser.h"

int text_format_callback(const struct ExpandoNode *self, char *buf, int buflen,
                         int col, int cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_TEXT);
  const struct ExpandoTextNode *n = (struct ExpandoTextNode *) self;

  int copylen = n->end - n->start;
  const int freelen = buflen - col;
  if (copylen > freelen)
  {
    copylen = freelen;
  }

  memcpy(buf + col, n->start, copylen);

  return copylen;
}
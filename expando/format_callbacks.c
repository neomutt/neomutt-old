#include <assert.h>
#include <string.h>
#include "format_callbacks.h"
#include "helpers.h"
#include "parser.h"

/**
 * text_format_callback - Every text node uses this. 
 * 
 * TODO(g0mb4): Fill these:
 * @param self 
 * @param buffer 
 * @param buffer_len 
 * @param start_col 
 * @param max_cols 
 * @param data 
 * @param flags 
 */
void text_format_callback(const struct ExpandoNode *self, char **buffer,
                          int *buffer_len, int *start_col, int max_cols,
                          intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_TEXT);

  // TODO(g0mb4): Handle *start_col != 0
  int copylen = self->end - self->start;
  if (copylen > *buffer_len)
  {
    copylen = *buffer_len;
  }

  memcpy(*buffer, self->start, copylen);

  *buffer += copylen;
  *buffer_len -= copylen;
  *start_col += mb_strwidth_range(self->start, self->end);
}
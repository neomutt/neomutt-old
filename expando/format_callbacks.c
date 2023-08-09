#include <assert.h>
#include <string.h>
#include "format_callbacks.h"
#include "helpers.h"
#include "parser.h"

/**
 * text_format_callback - Callback for every text node. 
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

/**
 * conditional_format_callback - Callback for every conditional node.
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
void conditional_format_callback(const struct ExpandoNode *self, char **buffer,
                                 int *buffer_len, int *start_col, int max_cols,
                                 intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_CONDITION);
  assert(self->ndata);
  struct ExpandoConditionPrivate *cp = (struct ExpandoConditionPrivate *) self->ndata;

  assert(cp->condition);
  assert(cp->if_true);

  char tmp[128] = { 0 };
  int len = sizeof(tmp) - 1;
  char *ptmp = &tmp[0];

  MuttFormatFlags temp_flags = flags;
  temp_flags &= ~MUTT_FORMAT_INDEX; // disable formatting

  int scol = 0;
  cp->condition->format_cb(cp->condition, &ptmp, &len, &scol, len, data, temp_flags);

  if (!mutt_str_equal(tmp, "0"))
  {
    cp->if_true->format_cb(cp->if_true, buffer, buffer_len, start_col, max_cols, data, flags);
  }
  else
  {
    if (cp->if_false)
    {
      cp->if_false->format_cb(cp->if_false, buffer, buffer_len, start_col,
                              max_cols, data, flags);
    }
  }
}
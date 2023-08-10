#include "config.h"
#include <assert.h>
#include <string.h>
#include "mutt/lib.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "node.h"

void format_tree(struct ExpandoNode **tree, char *buf, size_t buflen,
                 size_t col, int cols, intptr_t data, MuttFormatFlags flags)
{
  const struct ExpandoNode *n = *tree;
  int start_col = col;
  int buffer_len = buflen;

  // TODO(g0mb4): Calculate buffer's start position from `col`,
  //              so the callback doesn't need to know about it.
  while (n && start_col < cols && buffer_len > 0)
  {
    if (n->format_cb)
    {
      n->format_cb(n, &buf, &buffer_len, &start_col, cols, data, flags);
    }
    n = n->next;
  }
}

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
  assert(cp->if_true_tree);

  // TODO(g0mb4): Activate assert and remove if(), as soon as the global table is filled.
  //assert(cp->condition->format_cb);
  if (!cp->condition->format_cb)
  {
    return;
  }

  char tmp[128] = { 0 };
  int len = sizeof(tmp) - 1;
  char *ptmp = &tmp[0];

  const MuttFormatFlags temp_flags = MUTT_FORMAT_NO_FLAGS;

  int scol = 0;
  cp->condition->format_cb(cp->condition, &ptmp, &len, &scol, len, data, temp_flags);

  if (!mutt_str_equal(tmp, "0"))
  {
    memset(tmp, 0, sizeof(tmp));
    format_tree(&cp->if_true_tree, tmp, sizeof(tmp), 0, sizeof(tmp), data, flags);

    const int copylen = strlen(tmp);
    memcpy(*buffer, tmp, copylen);
    *start_col += mb_strwidth_range(*buffer, *buffer + copylen);
    *buffer += copylen;
    *buffer_len -= copylen;
  }
  else
  {
    if (cp->if_false_tree)
    {
      memset(tmp, 0, sizeof(tmp));
      format_tree(&cp->if_false_tree, tmp, sizeof(tmp), 0, sizeof(tmp), data, flags);

      const int copylen = strlen(tmp);
      memcpy(*buffer, tmp, copylen);
      *start_col += mb_strwidth_range(*buffer, *buffer + copylen);
      *buffer += copylen;
      *buffer_len -= copylen;
    }
  }
}

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
 * @page expando_format XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <string.h>
#include "mutt/lib.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "mutt_thread.h"
#include "node.h"

// TODO(g0mb4): see if it can be used for all formats
// NOTE(g0mb4): copy of hdrline.c's add_index_color()
static size_t add_index_color_2gmb(char *buf, int buflen, MuttFormatFlags flags,
                                   enum ColorId color)
{
  /* only add color markers if we are operating on main index entries. */
  if (!(flags & MUTT_FORMAT_INDEX))
    return 0;

  /* this item is going to be passed to an external filter */
  if (flags & MUTT_FORMAT_NOFILTER)
    return 0;

  if (buflen <= 2)
    return 0;

  if (color == MT_COLOR_INDEX)
  { /* buf might be uninitialized other cases */
    const size_t len = mutt_str_len(buf);
    buf += len;
    buflen -= len;
  }

  buf[0] = MUTT_SPECIAL_INDEX;
  buf[1] = color;
  buf[2] = '\0';

  return 2;
}

// NOTE(g0mb4): It cuts a long string, e.g. subject (%s) in $index_format
void format_string(char *buf, int buf_len, const char *s, MuttFormatFlags flags,
                   enum ColorId pre, enum ColorId post,
                   struct ExpandoFormatPrivate *format, enum HasTreeChars has_tree)
{
  if (format)
  {
    const size_t colorlen1 = add_index_color_2gmb(buf, buf_len, flags, pre);
    mutt_simple_format(buf + colorlen1, buf_len - colorlen1, format->min,
                       format->max, format->justification, format->leader, s,
                       strlen(s), has_tree == HAS_TREE);
    const int n = strlen(buf);
    add_index_color_2gmb(buf + n, buf_len - n, flags, post);
  }
  else
  {
    const size_t colorlen1 = add_index_color_2gmb(buf, buf_len, flags, pre);
    const int n = snprintf(buf + colorlen1, buf_len - colorlen1, "%s", s);
    add_index_color_2gmb(buf + colorlen1 + n, buf_len - colorlen1 - n, flags, post);
  }
}

void format_int(char *buf, int buf_len, int number, MuttFormatFlags flags,
                enum ColorId pre, enum ColorId post, struct ExpandoFormatPrivate *format)
{
  char tmp[32]; // 64 bit INT_MAX has 20 digits
  snprintf(tmp, sizeof(tmp), "%d", number);
  format_string(buf, buf_len, tmp, flags, pre, post, format, NO_TREE);
}

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

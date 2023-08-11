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

// TODO(g0mb4): Implement this properly
char *got_to_column(const char *s, int col)
{
  assert(col == 0);
  return (char *) s;
}

void format_tree(struct ExpandoNode **tree, char *buf, size_t buf_len, int start_col,
                 int max_col, intptr_t data, MuttFormatFlags flags, bool *optional)
{
  const struct ExpandoNode *n = *tree;
  char *buffer = got_to_column(buf, start_col);
  int buffer_len = buf_len;

  assert(max_col >= start_col);
  int col_len = max_col - start_col;

  int printed = 0;
  while (n && buffer_len > 0 && col_len > 0)
  {
    if (n->format_cb)
    {
      printed = n->format_cb(n, buffer, buffer_len, col_len, data, flags, optional);

      col_len -= mb_strwidth_range(buffer, buffer + printed);
      buffer_len -= printed;
      buffer += printed;
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
int text_format_callback(const struct ExpandoNode *self, char *buf, int buf_len,
                         int cols_len, intptr_t data, MuttFormatFlags flags, bool *optional)
{
  assert(self->type == NT_TEXT);

  int copylen = self->end - self->start;
  if (copylen > buf_len)
  {
    copylen = buf_len;
  }

  // FIXME(g0mb4): handle cols_len
  memcpy(buf, self->start, copylen);

  return copylen;
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
int conditional_format_callback(const struct ExpandoNode *self, char *buf,
                                int buf_len, int cols_len, intptr_t data,
                                MuttFormatFlags flags, bool *optional)
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
    return 0;
  }

  char tmp[128] = { 0 };
  int printed = cp->condition->format_cb(cp->condition, tmp, sizeof(tmp), sizeof(tmp),
                                         data, MUTT_FORMAT_NO_FLAGS, optional);

  if (printed > 0 && !mutt_str_equal(tmp, "0"))
  {
    memset(tmp, 0, sizeof(tmp));
    format_tree(&cp->if_true_tree, tmp, sizeof(tmp), 0, sizeof(tmp), data, flags, optional);

    int copylen = strlen(tmp);
    if (copylen > buf_len)
    {
      copylen = buf_len;
    }
    // FIXME(g0mb4): handle cols_len
    memcpy(buf, tmp, copylen);
    return copylen;
  }
  else
  {
    if (cp->if_false_tree)
    {
      memset(tmp, 0, sizeof(tmp));
      format_tree(&cp->if_false_tree, tmp, sizeof(tmp), 0, sizeof(tmp), data, flags, optional);

      // FIXME(g0mb4): handle cols_len
      int copylen = strlen(tmp);
      if (copylen > buf_len)
      {
        copylen = buf_len;
      }
      memcpy(buf, tmp, copylen);
      return copylen;
    }
    else
    {
      return 0;
    }
  }
}

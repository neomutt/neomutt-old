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
#include "expando/lib.h"
#include "mutt_thread.h"

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
    // TODO(g0mb4): Investigate this
    const size_t len = mutt_str_len(buf);
    buf += len;
    buflen -= len;
  }

  // TODO(g0mb4): Investigate this
  buf[0] = MUTT_SPECIAL_INDEX;
  buf[1] = color;
  buf[2] = '\0';

  return 2;
}

// NOTE(g0mb4): It cuts a long string, e.g. subject (%s) in $index_format
// TODO(g0mb4): implement MuttFormatFlags
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
char *got_to_column(char **start, int col)
{
  char *s = *start;
  assert(col == 0);
  return (char *) s;
}

void format_tree(const struct ExpandoNode *const *tree, char *buf, size_t buf_len,
                 int start_col, int max_col, intptr_t data, MuttFormatFlags flags)
{
  const struct ExpandoNode *n = *tree;
  char *buffer = got_to_column(&buf, start_col);
  int buffer_len = buf_len - (buffer - buf);

  assert(max_col >= start_col);
  int col_len = max_col - start_col + 1;

  int printed = 0;
  while (n && buffer_len > 0 && col_len > 0)
  {
    if (n->format_cb)
    {
      printed = n->format_cb(n, buffer, buffer_len, col_len, data, flags);

      col_len -= mb_strwidth_nonnull(buffer, buffer + printed);
      buffer_len -= printed;
      buffer += printed;
    }

    n = n->next;
  }

  // give softpad nodes a chance to act
  while (n)
  {
    if (n->type == ENT_PAD && n->format_cb)
    {
      n->format_cb(n, buffer, buffer_len, col_len, data, flags);
    }
    n = n->next;
  }
}

/**
 * text_format_callback - Callback for every text node.
 *
 */
int text_format_callback(const struct ExpandoNode *self, char *buf, int buf_len,
                         int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_TEXT);

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
 */
int conditional_format_callback(const struct ExpandoNode *self, char *buf, int buf_len,
                                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_CONDITION);
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
  int printed = cp->condition->format_cb(cp->condition, tmp, sizeof(tmp),
                                         sizeof(tmp), data, MUTT_FORMAT_NO_FLAGS);

  if (printed > 0 && !mutt_str_equal(tmp, "0"))
  {
    memset(tmp, 0, sizeof(tmp));
    format_tree(&cp->if_true_tree, tmp, sizeof(tmp), 0, sizeof(tmp), data, flags);

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
      format_tree(&cp->if_false_tree, tmp, sizeof(tmp), 0, sizeof(tmp), data, flags);

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

static int pad_format_fill_eol(const struct ExpandoNode *self, char *buf, int buf_len,
                               int cols_len, intptr_t data, MuttFormatFlags flags)
{
  const int pad_len = self->end - self->start;
  const int pad_width = mb_strwidth_nonnull(self->start, self->end);

  int len = buf_len;
  int cols = cols_len;
  bool is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) > 0);

  while (is_space_to_write)
  {
    memcpy(buf, self->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols -= pad_width;

    is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) > 0);
  }

  // formatting of the whole buffer is done
  return buf_len;
}

static int pad_format_hard_fill(const struct ExpandoNode *self, char *buf, int buf_len,
                                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  const int pad_len = self->end - self->start;
  const int pad_width = mb_strwidth_nonnull(self->start, self->end);

  char tmp[128] = { 0 };
  format_tree((struct ExpandoNode **) &self->next, tmp, sizeof(tmp), 0,
              sizeof(tmp), data, flags);
  const int right_len = mutt_str_len(tmp);
  const int right_width = mutt_strwidth(tmp);

  int len = buf_len;
  int cols = cols_len;
  bool is_space_to_write = ((len - pad_len - right_len) > 0) &&
                           ((cols - pad_width - right_width) > 0);

  while (is_space_to_write)
  {
    memcpy(buf, self->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols -= pad_width;

    is_space_to_write = ((len - pad_len - right_len) > 0) &&
                        ((cols - pad_width - right_width) > 0);
  }

  memcpy(buf, tmp, right_len);

  // formatting of the whole buffer is done
  return buf_len;
}

static int pad_format_soft_fill(const struct ExpandoNode *self, char *buf, int buf_len,
                                int cols_len, intptr_t data, MuttFormatFlags flags)
{
  const int pad_len = self->end - self->start;
  const int pad_width = mb_strwidth_nonnull(self->start, self->end);

  char tmp[128] = { 0 };
  format_tree((struct ExpandoNode **) &self->next, tmp, sizeof(tmp), 0,
              sizeof(tmp), data, flags);

  const int right_len = mutt_str_len(tmp);

  int len = buf_len;
  int cols = cols_len;

  bool is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) > 0);

  while (is_space_to_write)
  {
    memcpy(buf, self->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols -= pad_width;

    is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) > 0);
  }

  if (cols_len > 0)
  {
    // consume remaining space
    while (cols > 0)
    {
      cols--;
      buf++;
    }
  }
  else
  {
    // rewind buffer
    while (cols < 0)
    {
      cols++;
      buf--;
    }
  }

  // TODO(g0mb4): Check boundaries
  memcpy(buf - right_len - 1, tmp, right_len);

  // formatting of the whole buffer is done
  return buf_len;
}

/**
 * pad_format_callback - Callback for every pad node.
 *
 */
int pad_format_callback(const struct ExpandoNode *self, char *buf, int buf_len,
                        int cols_len, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == ENT_PAD);
  assert(self->ndata);

  struct ExpandoPadPrivate *pp = (struct ExpandoPadPrivate *) self->ndata;

  switch (pp->pad_type)
  {
    case EPT_FILL_EOL:
      return pad_format_fill_eol(self, buf, buf_len, cols_len, data, flags);
    case EPT_HARD_FILL:
      return pad_format_hard_fill(self, buf, buf_len, cols_len, data, flags);
    case EPT_SOFT_FILL:
      return pad_format_soft_fill(self, buf, buf_len, cols_len, data, flags);
    default:
      assert(0 && "Unknown pad type.");
  };

  assert(0 && "Unreachable");
  return 0;
}

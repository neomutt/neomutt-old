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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "mutt/lib.h"
#include "gui/lib.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "node.h"

void format_tree(struct ExpandoNode **tree, char *buf, size_t buf_len,
                 size_t col_len, intptr_t data, MuttFormatFlags flags)
{
  const struct ExpandoNode *n = *tree;
  char *buffer = buf;
  int buffer_len = (int) buf_len;
  int columns_len = (int) col_len;

  int printed = 0;
  while (n && buffer_len > 0 && columns_len > 0)
  {
    if (n->format_cb)
    {
      printed = n->format_cb(n, buffer, buffer_len, columns_len, data, flags);

      columns_len -= mb_strwidth_nonnull(buffer, buffer + printed);
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
      n->format_cb(n, buffer, buffer_len, columns_len, data, flags);
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

  assert(cp->condition->format_cb);
  if (!cp->condition->format_cb)
  {
    return 0;
  }

  char tmp[128] = { 0 };
  int printed = cp->condition->format_cb(cp->condition, tmp, sizeof(tmp),
                                         sizeof(tmp), data, MUTT_FORMAT_NO_FLAGS);

  // expression is true if it is not 0 (for numbers) and it is not " "(for flags)
  // NOTE(g0mb4): if $to_chars is used for flags this condition must contain the natrual
  //              char for that flag
  if (printed > 0 && !mutt_str_equal(tmp, "0") && !mutt_str_equal(tmp, " "))
  {
    memset(tmp, 0, sizeof(tmp));
    format_tree(&cp->if_true_tree, tmp, sizeof(tmp), sizeof(tmp), data, flags);

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
    if (cp->if_false_tree)
    {
      memset(tmp, 0, sizeof(tmp));
      format_tree(&cp->if_false_tree, tmp, sizeof(tmp), sizeof(tmp), data, flags);

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
  format_tree((struct ExpandoNode **) &self->next, tmp, sizeof(tmp), sizeof(tmp), data, flags);
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
  format_tree((struct ExpandoNode **) &self->next, tmp, sizeof(tmp), sizeof(tmp), data, flags);

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

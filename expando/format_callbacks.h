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

#ifndef MUTT_EXPANDO_FORMAT_CALLBACKS_H
#define MUTT_EXPANDO_FORMAT_CALLBACKS_H

#include <stddef.h>
#include <stdint.h>
#include "format_flags.h"
#include "color/color.h"

struct ExpandoNode;
struct ExpandoFormatPrivate;

typedef int (*expando_format_callback)(const struct ExpandoNode *self, char *buf,
                                        int buf_len, int cols_len,
                                        intptr_t data, MuttFormatFlags flags);


/**
 * enum HasTreeChars - Signals if the string constains tree characters.
 *
 * Characters like: '┌', '┴'.
 * More readale than a simple true / false.
 */
enum HasTreeChars
{
  NO_TREE = 0,
  HAS_TREE
};

char *got_to_column(char **start, int col);

void format_string(char *buf, int buf_len, const char *s,
                   MuttFormatFlags flags, enum ColorId pre, enum ColorId post,
                   struct ExpandoFormatPrivate *format, enum HasTreeChars has_tree);

void format_int(char *buf, int buf_len, int number,
                MuttFormatFlags flags, enum ColorId pre,
                enum ColorId post, struct ExpandoFormatPrivate *format);


void format_tree(struct ExpandoNode **tree, char *buf, size_t buf_len, int start_col, int max_col,
                 intptr_t data, MuttFormatFlags flags);

int text_format_callback(const struct ExpandoNode *self, char *buf,
                         int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int conditional_format_callback(const struct ExpandoNode *self, char *buf,
                                int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int pad_format_callback(const struct ExpandoNode *self, char *buf,
                         int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_FORMAT_CALLBACKS_H */

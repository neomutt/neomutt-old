/**
 * @file
 * Common test code for parsing
 *
 * @authors
 * Copyright (C) 2023 Richard Russon <rich@flatcap.org>
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

#include "config.h"
#include "core/lib.h"
#include "parse/lib.h"

struct ExpandoNode;
struct ExpandoParseError;

struct Command mutt_commands[] = {
  // clang-format off
  { "reset",  parse_set, MUTT_SET_RESET },
  { "set",    parse_set, MUTT_SET_SET },
  { "toggle", parse_set, MUTT_SET_INV },
  { "unset",  parse_set, MUTT_SET_UNSET },
  // clang-format on
};

void buf_expand_path(struct Buffer *buf)
{
  buf_insert(buf, 0, "expanded");
}

void myvar_append(const char *var, const char *val)
{
}

void myvar_del(const char *var)
{
}

void myvar_set(const char *var, const char *val)
{
}

void expando_tree_parse(struct ExpandoNode **root, const char *s,
                        const char *valid_short_expandos[],
                        const char *valid_two_char_expandos[],
                        const char *valid_long_expandos[], struct ExpandoParseError *error)
{
}

void expando_tree_free(struct ExpandoNode **root)
{
}

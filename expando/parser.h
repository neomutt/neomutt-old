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

#ifndef MUTT_EXPANDO_PARSER_H
#define MUTT_EXPANDO_PARSER_H

#include <stdio.h>
#include "global_table.h"

struct ExpandoNode;

struct ExpandoParseError
{
  char message[128];
  const char *position;
};

void expando_tree_parse(struct ExpandoNode **root, const char **string,
                        enum ExpandoFormatIndex index, struct ExpandoParseError *error);
void expando_tree_free(struct ExpandoNode **root);
void expando_tree_print(FILE *fp, struct ExpandoNode **root, int indent);

#endif /* MUTT_EXPANDO_PARSER_H */

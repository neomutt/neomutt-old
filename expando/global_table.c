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
 * @page expando_global XXX
 *
 * XXX
 */

#include "config.h"
#include "mutt/memory.h"
#include "global_table.h"

void expando_tree_free(struct ExpandoNode **root);

struct ExpandoRecord *expando_global_table_new(void)
{
  struct ExpandoRecord *t = mutt_mem_calloc(EFMT_FORMAT_COUNT_OR_DEBUG,
                                            sizeof(struct ExpandoRecord));
  return t;
}

void expando_global_table_free(struct ExpandoRecord **ptr)
{
  if (!ptr || !*ptr)
    return;

  struct ExpandoRecord *table = *ptr;
  for (int i = 0; i < EFMT_FORMAT_COUNT_OR_DEBUG; ++i)
  {
    struct ExpandoRecord *r = &table[i];
    if (r->tree)
    {
      expando_tree_free(&r->tree);
    }

    if (r->string)
    {
      FREE(&r->string);
    }
  }

  FREE(ptr);
}

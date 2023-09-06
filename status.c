/**
 * @file
 * GUI display a user-configurable status line
 *
 * @authors
 * Copyright (C) 1996-2000,2007 Michael R. Elkins <me@mutt.org>
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
 * @page neo_status GUI display a user-configurable status line
 *
 * GUI display a user-configurable status line
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "index/lib.h"
#include "menu/lib.h"
#include "postpone/lib.h"
#include "format_flags.h"
#include "muttlib.h"
#include "status.h"

/**
 * menu_status_line - Create the status line
 * @param[out] buf      Buffer in which to save string
 * @param[in]  buflen   Buffer length
 * @param[in]  shared   Shared Index data
 * @param[in]  menu     Current menu
 * @param[in]  cols     Maximum number of columns to use
 * @param[in]  record   ExpandoRecord
 *
 */
void menu_status_line(char *buf, size_t buflen, struct IndexSharedData *shared,
                      struct Menu *menu, int cols, const struct ExpandoRecord *record)
{
  assert(record);
  struct MenuStatusLineData data = { shared, menu };

  mutt_expando_format_2gmb(buf, buflen, cols, record, (intptr_t) &data, MUTT_FORMAT_NO_FLAGS);
}

/**
 * @file
 * GUI display a user-configurable status line
 *
 * @authors
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
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

#ifndef MUTT_STATUS_H
#define MUTT_STATUS_H

#include <stdio.h>
#include "expando/lib.h"

struct IndexSharedData;
struct Menu;

void menu_status_line(char *buf, size_t buflen, struct IndexSharedData *shared, struct Menu *menu, int cols, const char *fmt);
void menu_status_line_2gmb(char *buf, size_t buflen, struct IndexSharedData *shared, struct Menu *menu, int cols, enum ExpandoFormatIndex format_index);

#endif /* MUTT_STATUS_H */

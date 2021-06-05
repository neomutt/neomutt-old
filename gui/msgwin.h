/**
 * @file
 * Message Window
 *
 * @authors
 * Copyright (C) 2021 Richard Russon <rich@flatcap.org>
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

#ifndef MUTT_MSGWIN_H
#define MUTT_MSGWIN_H

#include <stdio.h>
#include "color.h"

void               msgwin_clear_text(void);
struct MuttWindow *msgwin_create    (void);
size_t             msgwin_get_width (void);
struct MuttWindow *msgwin_get_window(void);
void               msgwin_set_height(short height);
void               msgwin_set_text  (enum ColorId color, const char *text);

#endif /* MUTT_MSGWIN_H */
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

#ifndef MUTT_EXPANDO_VALIDATION_H
#define MUTT_EXPANDO_VALIDATION_H

#include <stdbool.h>
#include "format_callbacks.h"

struct Buffer;

struct ExpandoFormatCallback
{
  const char *name;
  expando_format_callback callback;
};

struct ExpandoValidation
{
  const char *name;
  const struct ExpandoFormatCallback *valid_short_expandos;
  const struct ExpandoFormatCallback *valid_two_char_expandos;
  // TODO(g0mb4): add valid_long_expandos
};

bool expando_validate_string(struct Buffer *name, struct Buffer *value, struct Buffer *err);

#endif /* MUTT_EXPANDO_VALIDATION_H */

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

#ifndef MUTT_EXPANDO_NNTP_FORMAT_CALLBACKS_H
#define MUTT_EXPANDO_NNTP_FORMAT_CALLBACKS_H

#include <stdint.h>
#include "format_flags.h"

struct ExpandoNode;

int nntp_a(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags);

int nntp_p(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int nntp_P(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int nntp_s(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int nntp_S(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int nntp_u(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_NNTP_FORMAT_CALLBACKS_H */
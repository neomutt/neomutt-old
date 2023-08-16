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

#ifndef MUTT_EXPANDO_STATUS_FORMAT_CALLBACKS_H
#define MUTT_EXPANDO_STATUS_FORMAT_CALLBACKS_H

#include <stdint.h>
#include "format_flags.h"

struct ExpandoNode;

int status_r(const struct ExpandoNode *self, char *buf, int buf_len,
             int cols_len, intptr_t data, MuttFormatFlags flags);

int status_D(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_f(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_M(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_m(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_n(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_o(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_d(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_F(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_t(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_p(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_b(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_l(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_T(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_s(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_S(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_P(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_h(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_L(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_R(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_u(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_v(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

int status_V(const struct ExpandoNode *self, char *buf, int buf_len,
              int cols_len, intptr_t data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_STATUS_FORMAT_CALLBACKS_H */

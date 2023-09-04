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

#ifndef MUTT_EXPANDO_INDEX_FORMAT_CALLBACKS_H
#define MUTT_EXPANDO_INDEX_FORMAT_CALLBACKS_H

#include <stdint.h>
#include "format_flags.h"

struct ExpandoNode;

int index_date(const struct ExpandoNode *self, char *buf,
                int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_format_hook_callback(const struct ExpandoNode *self, char *buf,
                int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_a(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_A(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_b(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_B(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_c(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_cr(const struct ExpandoNode *self, char *buf,
              int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_C(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_d(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_D(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_e(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_E(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_f(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_F(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_Fp(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_g(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G0(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G1(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G2(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G3(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G4(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G5(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G6(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G7(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G8(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_G9(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_H(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_i(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_I(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_J(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_K(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_l(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_L(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_m(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_M(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_n(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_N(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_O(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_P(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_q(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_r(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_R(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_s(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_S(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_t(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_T(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_u(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_v(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_W(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_x(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_X(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_y(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_Y(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_zc(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_zs(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_zt(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int index_Z(const struct ExpandoNode *self, char *buf,
             int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_INDEX_FORMAT_CALLBACKS_H */

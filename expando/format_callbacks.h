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

#ifndef MUTT_EXPANDO_FORMAT_CALLBACKS_H
#define MUTT_EXPANDO_FORMAT_CALLBACKS_H

#include <stddef.h>
#include <stdint.h>

struct ExpandoNode;
struct ExpandoFormatPrivate;

typedef uint8_t MuttFormatFlags;         ///< Flags for mutt_expando_format(), e.g. #MUTT_FORMAT_FORCESUBJ
#define MUTT_FORMAT_NO_FLAGS          0  ///< No flags are set
#define MUTT_FORMAT_FORCESUBJ   (1 << 0) ///< Print the subject even if unchanged
#define MUTT_FORMAT_TREE        (1 << 1) ///< Draw the thread tree
//#define MUTT_FORMAT_OPTIONAL    (1 << 2) ///< Allow optional field processing (obsolete)
#define MUTT_FORMAT_STAT_FILE   (1 << 3) ///< Used by attach_format_str
#define MUTT_FORMAT_ARROWCURSOR (1 << 4) ///< Reserve space for arrow_cursor
#define MUTT_FORMAT_INDEX       (1 << 5) ///< This is a main index entry
//#define MUTT_FORMAT_NOFILTER    (1 << 6) ///< Do not allow filtering on this pass (never used)
#define MUTT_FORMAT_PLAIN       (1 << 7) ///< Do not prepend DISP_TO, DISP_CC ...

/**
 * @defgroup expando_api Expando API
 *
 * Prototype for a mutt_expando_format() Callback Function
 *
 * @param[in]  self     ExpandoNode containing the callback
 * @param[out] buf      Buffer in which to save string
 * @param[in]  buf_len  Buffer length
 * @param[in]  cols_len Number of screen columnss
 * @param[in]  data     Private data
 * @param[in]  flags    Flags, see #MuttFormatFlags
 * @retval ptr src (unchanged)
 *
 * Each callback function implements some expandos, e.g.
 *
 * | Expando | Description
 * | :------ | :----------
 * | \%t     | Title
 */
typedef int (*expando_format_callback)(const struct ExpandoNode *self, char *buf,
                                        int buf_len, int cols_len,
                                        intptr_t data, MuttFormatFlags flags);

void format_tree(struct ExpandoNode **tree, char *buf, size_t buf_len,
                 size_t col_len, intptr_t data, MuttFormatFlags flags);

int text_format_callback(const struct ExpandoNode *self, char *buf,
                         int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int conditional_format_callback(const struct ExpandoNode *self, char *buf,
                                int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

int pad_format_callback(const struct ExpandoNode *self, char *buf,
                         int buf_len, int cols_len, intptr_t data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_FORMAT_CALLBACKS_H */

/**
 * @file
 * Parse expando string
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
 * @page lib_expando Parse expando string
 *
 * Parse expando string
 *
 * | File                              | Description                    |
 * | :-------------------------------- | :----------------------------- |
 * | expando/alias_format_callbacks.c  | @subpage expando_alias_format  |
 * | expando/format_callbacks.c        | @subpage expando_format        |
 * | expando/global_table.c            | @subpage expando_global        |
 * | expando/helpers.c                 | @subpage expando_helpers       |
 * | expando/index_format_callbacks.c  | @subpage expando_index_format  |
 * | expando/node.c                    | @subpage expando_node          |
 * | expando/parser.c                  | @subpage expando_parser        |
 * | expando/status_format_callbacks.c | @subpage expando_status_format |
 * | expando/type.c                    | @subpage expando_type          |
 * | expando/validation.c              | @subpage expando_validation    |
 */

#ifndef MUTT_EXPANDO_LIB_H
#define MUTT_EXPANDO_LIB_H

// IWYU pragma: begin_keep
#include "alias_format_callbacks.h"
#include "attach_format_callbacks.h"
#include "autocrypt_format_callbacks.h"
#include "compose_format_callbacks.h"
#include "compress_format_callbacks.h"
#include "folder_format_callbacks.h"
#include "format_callbacks.h"
#include "greeting_format_callbacks.h"
#include "group_index_format_callbacks.h"
#include "helpers.h"
#include "history_format_callbacks.h"
#include "index_format_callbacks.h"
#include "mix_format_callbacks.h"
#include "nntp_format_callbacks.h"
#include "node.h"
#include "parser.h"
#include "pattern_format_callbacks.h"
#include "pgp_command_format_callbacks.h"
#include "pgp_entry_format_callbacks.h"
#include "query_format_callbacks.h"
#include "sidebar_format_callbacks.h"
#include "smime_command_format_callbacks.h"
#include "status_format_callbacks.h"
#include "type.h"
#include "validation.h"
// IWYU pragma: end_keep

#endif /* MUTT_EXPANDO_LIB_H */
